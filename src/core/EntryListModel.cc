/*
 *  Copyright (C) 2010  Alexandre Courbot
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/Database.h"
#include "core/EntryListModel.h"
#include "core/EntryListCache.h"

#include <QFont>
#include <QFontMetrics>
#include <QSize>
#include <QPalette>

#define TRANSACTION Database::transaction()
#define ROLLBACK Database::rollback()
#define COMMIT Database::commit()

#define EXEC(q) if (!q.exec()) { qDebug() << __FILE__ << __LINE__ << "Cannot execute query:" << q.lastError().message(); return false; }
#define EXEC_T(q) if (!q.exec()) { qDebug() << __FILE__ << __LINE__ << "Cannot execute query:" << q.lastError().message(); goto transactionFailed; }

void EntryListModel::setRoot(int rootId)
{
	// Nothing changes?
	if (rootId == _rootId) return;

	beginResetModel();
	_rootId = rootId;
	endResetModel();
	emit rootHasChanged(rootId);
}

QModelIndex EntryListModel::index(int row, int column, const QModelIndex &parent) const
{
	if (column > 0) return QModelIndex();
	const EntryListCachedEntry &cEntry = EntryListCache::instance().get(parent.isValid() ? parent.internalId() : rootId(), row);
	if (cEntry.isRoot()) return QModelIndex();
	else return createIndex(row, column, cEntry.rowId());
}
	
QModelIndex EntryListModel::index(int rowId) const
{
	// Invalid items have no parent
	if (rowId == -1) return QModelIndex();
	const EntryListCachedEntry &cEntry = EntryListCache::instance().get(rowId);
	if (cEntry.isRoot()) return QModelIndex();
	return createIndex(cEntry.position(), 0, rowId);
}

QModelIndex EntryListModel::realParent(const QModelIndex &idx) const
{
	const EntryListCachedEntry &cEntry = EntryListCache::instance().get(idx.isValid() ? idx.internalId() : rootId());
	if (cEntry.isRoot()) return QModelIndex();
	else {
		int pIndex(cEntry.parent());
		return index(pIndex);
	}
}

QModelIndex EntryListModel::parent(const QModelIndex &idx) const
{
	const EntryListCachedEntry &cEntry = EntryListCache::instance().get(idx.isValid() ? idx.internalId() : rootId());
	if (cEntry.isRoot() || cEntry.rowId() == rootId()) return QModelIndex();
	else {
		int pIndex(cEntry.parent());
		return index(pIndex == rootId() ? -1 : pIndex);
	}
}

int EntryListModel::rowCount(const QModelIndex &parent) const
{
	const EntryListCachedEntry &cEntry = EntryListCache::instance().get(parent.isValid() ? parent.internalId() : rootId());
	// Not a list? No child!
	if (!cEntry.isList()) return 0;
	return cEntry.count();
}

QVariant EntryListModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.column() != 0) return QVariant();
	const EntryListCachedEntry &cEntry = EntryListCache::instance().get(index.isValid() ? index.internalId() : -1);
	if (cEntry.isRoot()) return QVariant();
	switch (role) {
		case Qt::DisplayRole:
		case Qt::EditRole:
		{
			if (cEntry.isList()) return cEntry.label();
			EntryPointer entry(cEntry.entryRef().get());
			if (!entry) return QVariant();
			else return entry->shortVersion(Entry::TinyVersion);
		}
		case Qt::BackgroundRole:
		{
			if (cEntry.isList()) return QPalette().button();
			EntryPointer entry(cEntry.entryRef().get());
			if (!entry || !entry->trained()) return QVariant();
			else return entry->scoreColor();
		}
		case Qt::FontRole:
		{
			if (cEntry.isList()) {
				QFont font;
				font.setPointSize(font.pointSize() + 1);
				font.setItalic(true);
				return font;
			}
			else return QVariant();
		}
		case Qt::SizeHintRole:
			if (cEntry.isList()) {
				QFont font;
				font.setPointSize(font.pointSize() + 1);
				font.setItalic(true);
				return QSize(300, QFontMetrics(font).height());
			}
			else return QVariant();
		case Entry::EntryRole:
		{
			if (cEntry.isList()) return QVariant();
			EntryPointer entry(cEntry.entryRef().get());
			if (!entry) return QVariant();
			else return QVariant::fromValue(entry);
		}
		case Entry::EntryRefRole:
		{
			if (cEntry.isList()) return QVariant();
			return QVariant::fromValue(cEntry.entryRef());
		}
		default:
			return QVariant();
	}
}

bool EntryListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::EditRole && index.isValid()) {
		if (!TRANSACTION) return false;
		{
			SQLite::Query query(Database::connection());
			query.prepare("select type from lists where rowid = ?");
			query.bindValue(index.internalId());
			query.exec();
			if (query.next() && query.valueIsNull(0)) {
				query.prepare("update listsLabels set label = ? where rowid = ?");
				query.bindValue(value.toString());
				query.bindValue(index.internalId());
				EXEC_T(query);
			}
		}
		if (!COMMIT) goto transactionFailed;
		// Invalidate the cache entry
		EntryListCache::instance().invalidate(index.internalId());
		return true;
	}
	return false;
transactionFailed:
	ROLLBACK;
	EntryListCache::instance().invalidateAll();
	return false;
}
	
bool EntryListModel::moveRows(int row, int delta, const QModelIndex &parent, SQLite::Query &query)
{
	QString queryText("update lists set position = position + ? where parent %1 and position >= ?");
	if (!parent.isValid()) query.prepare(queryText.arg("is null"));
	else query.prepare(queryText.arg("= ?"));
	query.bindValue(delta);
	if (parent.isValid()) query.bindValue(parent.internalId());
	query.bindValue(row);
	EXEC(query);
	return true;
}

// TODO How to handle the beginInsertRows if the transaction failed and endInsertRows is not called?
bool EntryListModel::insertRows(int row, int count, const QModelIndex & parent)
{
	const QModelIndex realParent(parent.isValid() ? parent : index(rootId()));
	if (!TRANSACTION) return false;
	beginInsertRows(parent, row, row + count -1);
	{
		SQLite::Query query(Database::connection());
		// First update the positions of items located after the new lists
		if (!moveRows(row, count, realParent, query)) goto transactionFailed;
		// Then insert the lists themselves with a default name
		for (int i = 0; i < count; i++) {
			query.prepare("insert into lists values(?, ?, NULL, NULL)");
			if (realParent.isValid()) query.bindValue(realParent.internalId());
			else query.bindNullValue();
			query.bindValue(row + i);
			EXEC_T(query);
			int rowId = query.lastInsertId();
			query.prepare("insert into listsLabels(docid, label) values(?, ?)");
			query.bindValue(rowId);
			query.bindValue(tr("New list"));
			EXEC_T(query);
		}
	}
	if (!COMMIT) goto transactionFailed;
	EntryListCache::instance().invalidateAll();
	endInsertRows();
	return true;
transactionFailed:
	ROLLBACK;
	EntryListCache::instance().invalidateAll();
	return false;
}

bool EntryListModel::_removeRows(int row, int count, const QModelIndex &parent)
{
	SQLite::Query query(Database::connection());
	// Get the list of items to remove
	QString queryString("select rowid, type, id from lists where parent %1 and position between ? and ?");
	if (!parent.isValid()) query.prepare(queryString.arg("is null"));
	else {
		query.prepare(queryString.arg("= ?"));
		query.bindValue(parent.internalId());
	}
	query.bindValue(row);
	query.bindValue(row + count - 1);
	EXEC(query);
	QList<int> ids;
	QList<EntryRef> entries;
	while (query.next()) {
		ids << query.valueInt(0);
		entries << EntryRef(query.valueUInt(1), query.valueUInt(2));
	}
	// Remove the list from the entries if they are loaded
	int i = 0;
	foreach (EntryRef ref, entries) {
		if (ref.isLoaded()) ref.get()->lists().remove(ids[i]);
		++i;
	}
	// Recursively remove any child the items may have
	foreach (int id, ids) {
		QModelIndex idx(index(id));
		if (!idx.isValid()) continue;
		int childsNbr(rowCount(idx));
		if (childsNbr > 0 && !_removeRows(0, childsNbr, idx)) return false;
	}
	// Remove the listsLabels entries
	QStringList strIds;
	foreach (int id, ids) strIds << QString::number(id);
	
	query.prepare(QString("delete from listsLabels where rowid in (%1)").arg(strIds.join(", ")));
	EXEC(query);
	// Remove the lists entries
	query.prepare(QString("delete from lists where rowid in (%1)").arg(strIds.join(", ")));
	EXEC(query);
	// Update the positions of items that were after the ones we removed
	if (!moveRows(row + count, -count, parent, query)) return false;
	return true;
}

bool EntryListModel::removeRows(int row, int count, const QModelIndex &parent)
{
	const QModelIndex realParent(parent.isValid() ? parent : index(rootId()));
	if (!TRANSACTION) return false;
	beginRemoveRows(parent, row, row + count - 1);
	if (!_removeRows(row, count, realParent)) goto transactionFailed;
	if (!COMMIT) goto transactionFailed;
	EntryListCache::instance().invalidateAll();
	endRemoveRows();
	return true;
transactionFailed:
	ROLLBACK;
	EntryListCache::instance().invalidateAll();
	return false;
}

Qt::ItemFlags EntryListModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags ret(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
	
	if (index.isValid()) {
		const EntryListCachedEntry &cEntry = EntryListCache::instance().get(index.internalId());
		if (cEntry.isRoot()) return Qt::ItemIsEnabled;
		if (cEntry.isList()) ret |= Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
	}
	else ret |= Qt::ItemIsDropEnabled;
	return ret;
}

QStringList EntryListModel::mimeTypes() const
{
	QStringList ret;
	ret << "tagainijisho/entry";
	ret << "tagainijisho/listitem";
	return ret;
}

QMimeData *EntryListModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData = new QMimeData();
	QByteArray entriesEncodedData;
	QDataStream entriesStream(&entriesEncodedData, QIODevice::WriteOnly);
	QByteArray itemsEncodedData;
	QDataStream itemsStream(&itemsEncodedData, QIODevice::WriteOnly);
	
	foreach (const QModelIndex &index, indexes) {
		if (index.isValid()) {
			// Add the item
			itemsStream << (int)index.internalId();
			
			// If the item is an entry, add it
			const EntryListCachedEntry &cEntry = EntryListCache::instance().get(index.internalId());
			if (!cEntry.isRoot()) entriesStream << cEntry.entryRef();
		}
	}
	if (!entriesEncodedData.isEmpty()) mimeData->setData("tagainijisho/entry", entriesEncodedData);
	if (!itemsEncodedData.isEmpty()) mimeData->setData("tagainijisho/listitem", itemsEncodedData);
	return mimeData;
}

bool EntryListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &_parent)
{
	if (action == Qt::IgnoreAction) return true;
	if (column == -1) column = 0;
	if (column > 0) return false;

	// If dropped on the root, update the parent to the real one
	// Direct affectation of model indexes does not seem to copy the internal id
	const QModelIndex parent(_parent.isValid() ? _parent : index(rootId()));

	// If we have list items, we must move the items instead of inserting them
	if (data->hasFormat("tagainijisho/listitem")) {
		// Row ids of the items we move
		QList<int> rowIds;
		// Fetch the ids of the rows that moved from
		// the stream
		QByteArray ba = data->data("tagainijisho/listitem");
		QDataStream ds(&ba, QIODevice::ReadOnly);
		while (!ds.atEnd()) {
			int rowId;
			ds >> rowId;
			rowIds << rowId;
		}
		// If dropped on a list, append the entries
		if (row == -1) row = rowCount(parent);
		int realRow(row);

		// First check if we are not dropping a parent into one of its children
		foreach (int rowid, rowIds) {
			QModelIndex idx(index(rowid));
			QModelIndex p(parent);
			while (p.isValid()) {
				if (p == idx) return false;
				p = p.parent();
			}
		}
	
		// Do the move
		int itemCpt = 0;
		if (!TRANSACTION) return false;
		emit layoutAboutToBeChanged();
		{
			SQLite::Query query(Database::connection());
			foreach (int rowid, rowIds) {
				// First get the model index
				QModelIndex idx(index(rowid));
				// and its parent
				QModelIndex idxParent(realParent(idx));
				// If the destination parent is the same as the source and the destination row superior, we must decrement
				// the latter because the source has not been moved yet.
				if (idxParent == parent && row > idx.row() && realRow > 0) --realRow;
				// Do not bother if the position did not change
				if (idxParent == parent && realRow == idx.row()) continue;
				
				// Update rows position after the one we move
				if (!moveRows(idx.row() + 1, -1, idxParent, query)) goto transactionFailed;
				// Update rows position after the one we insert
				if (!moveRows(realRow + itemCpt, 1, parent, query)) goto transactionFailed;
				// And do the move
				query.prepare("update lists set parent = ?, position = ? where rowid = ?");
				if (parent.isValid()) query.bindValue(parent.internalId());
				else query.bindNullValue();
				query.bindValue(realRow + itemCpt);
				query.bindValue(rowid);
				EXEC_T(query);
				EntryListCache::instance().invalidateAll();
				// Don't forget to change any persistent index - views use them at least to keep track
				// of the current selection
				changePersistentIndex(idx, index(rowid));
				++itemCpt;
			}
		}
		if (!COMMIT) goto transactionFailed;
		EntryListCache::instance().invalidateAll();
		emit layoutChanged();
		return true;
	}
	// No list data, we probably dropped from the results view or something -
	// add the entries to the list
	else if (data->hasFormat("tagainijisho/entry")) {
		//if (!parent.isValid()) return false;
		QByteArray ba = data->data("tagainijisho/entry");
		QDataStream ds(&ba, QIODevice::ReadOnly);
		QList<EntryRef> entries;
		QList<quint32> ids;
		while (!ds.atEnd()) {
			EntryRef entryRef;
			ds >> entryRef;
			entries << entryRef;
		}
		// If dropped on a list, append the entries
		if (row == -1) row = rowCount(parent);
		if (!TRANSACTION) return false;
		{
			SQLite::Query query(Database::connection());
			// Start by moving the rows after the destination
			if (!moveRows(row, entries.size(), parent, query)) goto transactionFailed;
			// And insert the new rows at the right position
			query.prepare("insert into lists values(?, ?, ?, ?)");
			for (int i = 0; i < entries.size(); ++i) {
				if (parent.isValid()) query.bindValue(parent.internalId());
				else query.bindNullValue();
				query.bindValue(row + i);
				query.bindValue(entries[i].type());
				query.bindValue(entries[i].id());
				EXEC_T(query);
				ids << query.lastInsertId();
			}
		}
		// Insert rows must be done on what the view thinks is the parent
		beginInsertRows(_parent, row, row + entries.size() - 1);
		if (!COMMIT) goto transactionFailed;
		EntryListCache::instance().invalidateAll();
		endInsertRows();
		// Now add the list to the entries, if necessary
		int i = 0;
		foreach (EntryRef ref, entries) {
			if (ref.isLoaded()) ref.get()->lists() << ids[i];
			++i;
		}
		return true;
	}
	else return false;
transactionFailed:
	ROLLBACK;
	EntryListCache::instance().invalidateAll();
	return false;
}
