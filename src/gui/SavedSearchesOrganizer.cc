/*
 *  Copyright (C) 2009  Alexandre Courbot
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

#include "gui/SavedSearchesOrganizer.h"
#include "sqlite/Query.h"
#include "core/Database.h"

#include <QtDebug>
#include <QMenu>
#include <QPushButton>
#include <QMessageBox>

SavedSearchTreeItem::SavedSearchTreeItem(int setId, int position, bool isFolder, const QString &label) : QTreeWidgetItem(isFolder ? FolderType : SavedSearchType), _setId(setId), _position(position), _parentCopy(0)
{
	setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
	if (type() == FolderType) {
		setFlags(flags() | Qt::ItemIsDropEnabled);
		setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	}
	else setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
	setText(0, label);
}

SavedSearchTreeItem::SavedSearchTreeItem(const SavedSearchTreeItem &other) : QTreeWidgetItem(other.type())
{
	*this = other;
	_parentCopy = other.parent();
}

SavedSearchTreeItem::~SavedSearchTreeItem()
{
}

void SavedSearchTreeItem::setData(int column, int role, const QVariant & value)
{
	if (column == 0 && role == Qt::EditRole) {
		SQLite::Query query(Database::connection());
		query.prepare("UPDATE sets SET label = ? WHERE rowid = ?");
		query.bindValue(value.toString());
		query.bindValue(setId());
		if (!query.exec()) {
			qDebug() << "Error executing query:" << query.lastError().message();
			return;
		}
	}
	QTreeWidgetItem::setData(column, role, value);
}

SavedSearchesTreeWidget::SavedSearchesTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
	_mimeTypes << "tagaini/settreeitem";
}

void SavedSearchesTreeWidget::dropEvent(QDropEvent *event)
{
	QAbstractItemView::dropEvent(event);
}

QMimeData *SavedSearchesTreeWidget::mimeData(const QList<QTreeWidgetItem *> items) const
{
	QByteArray ba;
	QDataStream ds(&ba, QIODevice::WriteOnly);
	foreach (QTreeWidgetItem *item, items) {
		SavedSearchTreeItem *clone = new SavedSearchTreeItem(*static_cast<SavedSearchTreeItem *>(item));
		populateFolder(clone);
		// This is sooo bad... >_<;
		ds << (quint64)clone;
	}
	QMimeData *md = new QMimeData();
	md->setData("tagaini/settreeitem", ba);
	return md;
}

bool SavedSearchesTreeWidget::dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action)
{
	SavedSearchTreeItem *movedItem;
	SavedSearchTreeItem *prevParent, *newParent(static_cast<SavedSearchTreeItem *>(parent));
	int prevParentId, newParentId(newParent ? newParent->setId() : 0);
	QByteArray ba = data->data("tagaini/settreeitem");
	QDataStream ds(&ba, QIODevice::ReadOnly);
	while (!ds.atEnd()) {
		int prevPosition, newPosition(index);
		// This is bad... >_<;
		quint64 tmp;
		ds >> tmp; movedItem = (SavedSearchTreeItem *)tmp;
		prevPosition = movedItem->position();
		prevParent = static_cast<SavedSearchTreeItem *>(movedItem->parentCopy());
		prevParentId = prevParent ? prevParent->setId() : 0;
		// Fix the new position with respect to the real database
		if (index > 0 && (prevParent == newParent && index > prevPosition)) newPosition--;

		// Update the position of the item
		movedItem->setPosition(newPosition);

		// Only update the layout if there is any change
		if (!(prevParent == newParent && prevPosition == newPosition)) {
			SQLite::Query query(Database::connection());
#define QUERY(q) { if (!query.exec(q)) qDebug() << "Query failed" << query.lastError().message(); }
			// Update the positions of the items after the one we removed
			QUERY(QString("UPDATE sets SET position = position - 1 where parent %1 and position > %2").arg(!prevParentId ? "is null" : QString("= %1").arg(prevParentId)).arg(prevPosition));
			QList<SavedSearchTreeItem *> siblings(childsOf(prevParent));
			foreach (SavedSearchTreeItem *sibling, siblings) {
				int sibPos = sibling->position();
				if (sibPos > prevPosition) sibling->setPosition(sibPos - 1);
			}
			// Update the positions of the items after the one we add
			QUERY(QString("UPDATE sets SET position = position + 1 where parent %1 and position >= %2").arg(!newParentId ? "is null" : QString("= %1").arg(newParentId)).arg(newPosition));
			siblings = childsOf(newParent);
			foreach (SavedSearchTreeItem *sibling, siblings) {
				// Do not update the item
				if (sibling == movedItem) continue;
				int sibPos = sibling->position();
				if (sibPos >= newPosition) sibling->setPosition(sibPos + 1);
			}
			// Move our item
			QUERY(QString("UPDATE sets SET position = %1, parent = %2 where rowid = %3").arg(newPosition).arg(!newParentId ? "null" : QString("%1").arg(newParentId)).arg(movedItem->setId()));
		}
		// Now put our item to its new position
		if (!newParent) insertTopLevelItem(index, movedItem);
		else newParent->insertChild(index, movedItem);
#undef QUERY
	}
	return true;
}

void SavedSearchesTreeWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	menu.addAction(QIcon(":/images/icons/delete.png"), tr("Delete"), this, SLOT(deleteSelection()));
	menu.exec(event->globalPos());
}

void SavedSearchesTreeWidget::deleteSavedSearch(SavedSearchTreeItem *item)
{
	SavedSearchTreeItem *parent = static_cast<SavedSearchTreeItem *>(item->parent());
	// If the row is a folder, delete recursively
	if (item->isFolder()) {
		while (item->childCount()) {
			SavedSearchTreeItem *child(static_cast<SavedSearchTreeItem *>(item->child(0)));
			deleteSavedSearch(child);
		}
	}
	SQLite::Query query(Database::connection());
	// Delete the row
	query.prepare("DELETE FROM sets where rowid = ?");
	query.bindValue(item->setId());
	if (!query.exec()) {
		qDebug() << "Error executing query:" << query.lastError().message();
		return;
	}
	// Decrease the position of sets after the deleted one
	query.prepare(QString("UPDATE sets SET position = position - 1 where parent %1 and position > %2").arg(!parent ? "is null" : QString("= %1").arg(parent->setId())).arg(item->position()));
	if (!query.exec()) {
		qDebug() << "Error executing query:" << query.lastError().message();
		return;
	}
	// Don't forget to update our model
	QList<SavedSearchTreeItem *> siblings(childsOf(parent));
	foreach (SavedSearchTreeItem *sibling, siblings) if (sibling->position() > item->position()) sibling->setPosition(sibling->position() - 1);
	delete item;
}

void SavedSearchesTreeWidget::deleteSelection()
{
	if (QMessageBox::question(this, tr("Confirm deletion"), tr("Are you sure you want to delete the selected searches/folders?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) return;

	QList<QTreeWidgetItem *> selected = selectedItems();
	foreach (QTreeWidgetItem *rItem, selected) {
		SavedSearchTreeItem *item(static_cast<SavedSearchTreeItem *>(rItem));
		deleteSavedSearch(item);
	}
}

QList<SavedSearchTreeItem *> SavedSearchesTreeWidget::childsOf(SavedSearchTreeItem *parent)
{
		QList<SavedSearchTreeItem *> siblings;
		if (!parent) for (int i = 0; i < topLevelItemCount(); i++) siblings << static_cast<SavedSearchTreeItem *>(topLevelItem(i));
		else for (int i = 0; i < parent->childCount(); i++) siblings << static_cast<SavedSearchTreeItem *>(parent->child(i));
		return siblings;
}

void SavedSearchesTreeWidget::populateRoot()
{
	SQLite::Query query(Database::connection());

	query.exec("SELECT rowid, position, state IS NULL, label FROM sets WHERE parent IS NULL ORDER BY position");
	while (query.next()) {
		SavedSearchTreeItem *item = new SavedSearchTreeItem(query.valueInt(0), query.valueInt(1), query.valueBool(2), query.valueString(3));
		if (item->isFolder()) {
			item->setIcon(0, QIcon(":/images/icons/folder.png"));
			populateFolder(item);
		}
		addTopLevelItem(item);
	}
}

void SavedSearchesTreeWidget::populateFolder(SavedSearchTreeItem *parent) const
{
	SQLite::Query query(Database::connection());

	query.prepare("SELECT rowid, position, state IS NULL, label FROM sets WHERE parent = ? ORDER BY position");
	query.bindValue(parent->setId());
	query.exec();
	while (query.next()) {
		SavedSearchTreeItem *item = new SavedSearchTreeItem(query.valueInt(0), query.valueInt(1), query.valueBool(2), query.valueString(3));
		if (item->isFolder()) {
			item->setIcon(0, QIcon(":/images/icons/folder.png"));
			populateFolder(item);
		}
		parent->addChild(item);
	}
}

SavedSearchesOrganizer::SavedSearchesOrganizer(QWidget *parent) : QDialog(parent)
{
	setupUi(this);
	treeWidget->populateRoot();
}
