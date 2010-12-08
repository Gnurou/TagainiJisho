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

void EntryListModel::setRoot(quint64 rootId)
{
	// Nothing changes?
	if (rootId == _rootId) return;

#if QT_VERSION >= 0x040600
	beginResetModel();
#endif
	_rootId = rootId;
#if QT_VERSION >= 0x040600
	endResetModel();
#else
	reset();
#endif
	emit rootHasChanged(rootId);
}

#define LISTFORINDEX(index) (*EntryListCache::get(index.isValid() ? index.internalId() : 0))
#define INDEXDATA(index) LISTFORINDEX(index)[index.row()]

QModelIndex EntryListModel::index(int row, int column, const QModelIndex &parent) const
{
	if (column > 0) return QModelIndex();

	quint64 listId;
	// If the parent is valid, then fetch its real list
	if (parent.isValid()) {
		const EntryListData &cEntry = INDEXDATA(parent);
		// Parent will always be a list
		Q_ASSERT(cEntry.isList());
		listId = cEntry.id;
	} else listId = 0;

	return indexFromList(listId, row);
}

QModelIndex EntryListModel::indexFromList(quint64 listId, quint64 position) const
{
	const EntryList *list = EntryListCache::get(listId);
	if (!list || position > list->size()) return QModelIndex();

	// FIXME Qt is wrong here - internalId() returns a qint64, so this function should take a qint64 too!
	return createIndex(position, 0, (quint32)list->listId());
}
	
QModelIndex EntryListModel::parent(const QModelIndex &idx) const
{
	// Invalid index has no parent
	if (!idx.isValid()) return QModelIndex();
	EntryList &list = LISTFORINDEX(idx);
	// Root list has no parent
	if (list.listId() == 0) return QModelIndex();
	// We need to construct an index by determining the parent list of idx and the position of idx within it
	else {
		// We have the id of the list - all we need to do is find
		// which list contains it.
		QPair<const EntryList *, quint32> container = EntryListCache::getOwner(list.listId());
		return createIndex(container.second, 0, container.first ? container.first->listId() : 0);
	}
}

int EntryListModel::rowCount(const QModelIndex &index) const
{
	// Asking for size of root?
	if (!index.isValid()) return EntryListCache::get(0)->size();
	else {
		const EntryListData &cEntry = INDEXDATA(index);
		// Not a list? No child!
		if (!cEntry.isList()) return 0;
		else return EntryListCache::get(cEntry.id)->size();
	}
}

QVariant EntryListModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.column() != 0) return QVariant();

	const EntryListData &cEntry = INDEXDATA(index);

	switch (role) {
		case Qt::DisplayRole:
		case Qt::EditRole:
		{
			if (cEntry.isList()) return EntryListCache::get(cEntry.id)->label();
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

Qt::ItemFlags EntryListModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags ret(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);

	if (index.isValid()) {
		const EntryListData &cEntry = INDEXDATA(index);
		if (cEntry.isList()) ret |= Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
	}
	else ret |= Qt::ItemIsDropEnabled;
	return ret;
}

bool EntryListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::EditRole && index.isValid()) {
		const EntryListData &cEntry = INDEXDATA(index);
		if (cEntry.isList()) {
			EntryList &list = *EntryListCache::get(cEntry.id);
			if (list.setLabel(value.toString())) return true;
		}
	}
	return false;
}
	
// TODO How to handle the beginInsertRows if the transaction failed and endInsertRows is not called?
bool EntryListModel::insertRows(int row, int count, const QModelIndex & parent)
{
	// First fetch the list in which we will insert
	EntryList *parentList;
	if (!parent.isValid()) parentList = EntryListCache::get(0);
	else {
		const EntryListData &cEntry = INDEXDATA(parent);
		Q_ASSERT(cEntry.isList());
		parentList = EntryListCache::get(cEntry.id);
	}
	beginInsertRows(parent, row, row + count - 1);
	if (!EntryListCache::connection()->transaction()) goto failure_1;
	for (int i = 0; i < count; i++)
	{
		EntryList *newList = EntryListCache::newList();
		EntryListData data = { 0, newList->listId() };
		parentList->insert(data, row + i);
	}
	if (!EntryListCache::connection()->commit()) goto failure_2;
	endInsertRows();
	return true;

failure_2:
	EntryListCache::connection()->rollback();
failure_1:
	return false;
	/*
	const QModelIndex realParent(parent.isValid() ? parent : index(rootId()));
	if (!TRANSACTION) return false;
	beginInsertRows(parent, row, row + count - 1);
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
	return false;*/
}

bool EntryListModel::removeRows(int row, int count, const QModelIndex &parent)
{
	beginRemoveRows(parent, row, row + count - 1);
	if (!EntryListCache::connection()->transaction()) goto failure_1;
	{
		// The list from which we are actually removing
		EntryList *list = EntryListCache::get(parent.isValid() ? INDEXDATA(parent).id : 0); 
		while (count--) {
			if (!list->remove(row)) goto failure_2;
		}
	}
	if (!EntryListCache::connection()->commit()) goto failure_2;
	endRemoveRows();
	return true;

failure_2:
	EntryListCache::connection()->rollback();
failure_1:
	return false;
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
			itemsStream << (quint64)index.internalId() << (quint64)index.row();
			
			// If the item is an entry, add it
			const EntryListData &cEntry = INDEXDATA(index);
			if (!cEntry.isList()) entriesStream << cEntry.entryRef();
			// TODO in case of a list, add all the items the list contains
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

	// Destination list
	EntryList &list = *EntryListCache::get(_parent.isValid() ? INDEXDATA(_parent).id : 0);

	// If dropped on a list, append the entries
	if (row == -1) row = rowCount(_parent);

	// If we have list items, we must move the items instead of inserting them
	if (data->hasFormat("tagainijisho/listitem")) {
		// ids of the items we move
		QList<QPair<quint64, quint64> > ids;
		// Fetch the ids of the rows that moved from
		// the stream
		QByteArray ba = data->data("tagainijisho/listitem");
		QDataStream ds(&ba, QIODevice::ReadOnly);
		while (!ds.atEnd()) {
			quint64 listId, pos;
			ds >> listId;
			ds >> pos;
			ids << QPair<quint64, quint64>(listId, pos);
		}
		typedef QPair<quint64, quint64> qpp;

		emit layoutAboutToBeChanged();
		if (!EntryListCache::connection()->transaction()) goto failure_1;
		// First, record all the list/node pairs to move into a list
		typedef QPair<EntryList *, EntryList::TreeType::Node *> EntryListRef;
		QList<EntryListRef> elRefs;
		QList<QModelIndex> srcIdxs;
		unsigned int origRow = (unsigned int)row;

		foreach (const qpp &id, ids) {
			// Store the index in order to notify persistent indexes of the change
			srcIdxs << indexFromList(id.first, id.second);
			EntryList *srcList = EntryListCache::get(id.first);
			EntryList::TreeType::Node *node = srcList->getNode(id.second);
			elRefs << EntryListRef(srcList, node);
			// Clear the owner cache if the owner is going to change
			if (srcList != &list) EntryListCache::clearOwnerCache(node->rowId());
			// Decrease the destination index if we are removing an item in the destination
			// list with an index inferior to that of the drop
			if (srcList == &list && id.second < origRow) --row;
		}
		// Second, remove all nodes from their source list
		for (int i = 0; i < elRefs.size(); i++) {
			EntryListRef &elr = elRefs[i];
			if (!elr.first->removeNode(elr.second)) {
				qDebug("Error removing node from list, aborting.");
				goto failure_2;
			}
		}
		// Finally, insert all the nodes into the destination list
		for (int i = 0; i < elRefs.size(); i++) {
			const EntryListRef &elr = elRefs[i];
			if (!elr.second) continue;
			if (!list.insertNode(elr.second, row + i)) {
				qDebug("Error inserting node into list, aborting");
				goto failure_2;
			}
			// Update the persistent indexes (needed to correctly keep track of the current selection
			const QModelIndex &idx = srcIdxs[i];
			changePersistentIndex(idx, indexFromList(list.listId(), row + i));
		}
		if (!EntryListCache::connection()->commit()) goto failure_2;
		emit layoutChanged();
	}

	// No list data, we probably dropped from the results view or something -
	// add the entries to the list
	else if (data->hasFormat("tagainijisho/entry")) {
		QByteArray ba = data->data("tagainijisho/entry");
		QDataStream ds(&ba, QIODevice::ReadOnly);
		QList<EntryRef> entries;

		while (!ds.atEnd()) {
			EntryRef entryRef;
			ds >> entryRef;
			entries << entryRef;
		}

		beginInsertRows(_parent, row, row + entries.size() - 1);
		if (!EntryListCache::connection()->transaction()) goto failure_1;
		// Insert rows must be done on what the view thinks is the parent
		int cpt = 0;
		foreach (const EntryRef &entry, entries) {
			EntryListData eData;
			eData.type = entry.type();
			eData.id = entry.id();
			if (!list.insert(eData, row + cpt++)) {
				qDebug("Error inserting list item, aborting.");
				goto failure_2;
			}

			// Now add the list to the entry if it is loaded
			//if (entry.isLoaded()) entry.get()->lists() << list.listId();
		}
		if (!EntryListCache::connection()->commit()) goto failure_2;
		endInsertRows();
	}

	emit layoutChanged();
	return true;

failure_2:
	EntryListCache::connection()->rollback();
failure_1:
	emit layoutChanged();
	return false;
}
