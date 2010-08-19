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

#include "EntryListCache.h"

EntryListCachedList::EntryListCachedList(quint64 id)
{
	QSqlQuery query;
	// TODO cache this
	query.prepare("select lists.rowid, parent, next, type, id, label from lists left join listsLabels on lists.rowid == listsLabels.rowid where parent = ?");
	query.addBindValue(id);
	query.exec();

	while (query.next()) {
		EntryListCachedEntry cachedEntry(query);
		_entries[cachedEntry.rowId()] = cachedEntry;
	}

	// Now link the entries together and build the ordered array
	EntryListCachedEntry *tail;
	int entryCpt(0);
	foreach(const EntryListCachedEntry &entry, _entries.values()) {
		++entryCpt;
		EntryListCachedEntry *curEntry = &_entries[entry.rowId()];
		if (entry._nextId != 0) {
			curEntry->_next = &_entries[entry._nextId];
			_entries[entry._nextId]._prev = curEntry;
		}
		else tail = curEntry;
	}
	_entriesArray.resize(entryCpt);
	while (tail != 0) {
		_entriesArray[--entryCpt] = tail;
		tail = tail->_prev;
	}
}

EntryListCachedEntry::EntryListCachedEntry()
{
	_rowId = -1;
	_parent = -1;
	_position = -1;
	_type = -1;
	_id = -1;
	QSqlQuery query;
	query.exec("select count(*) from lists where parent is null");
	if (query.next()) _count = query.value(0).toInt();
	else _count = 0;
}

// TODO change root id to 0 as it is the value of the DB
EntryListCachedEntry::EntryListCachedEntry(QSqlQuery &query) : _next(0), _prev(0)
{
	_rowId = query.value(0).toULongLong();
	_parent = query.value(1).isNull() ? -1 : query.value(1).toULongLong();
	_nextId = query.value(2).toULongLong();
	_type = query.value(3).isNull() ? -1 : query.value(3).toInt();
	_id = query.value(4).toInt();
	_label = query.value(5).toString();

	// If the type is a list, get its number of childs
	if (_type == -1) {
		QSqlQuery query2;
		// TODO cache this!
		query2.prepare("select count(*) from lists where parent = ?");
		query2.addBindValue(rowId());
		query2.exec();
		if (query2.next()) _count = query2.value(0).toInt();
	}
	else _count = 0;
}

EntryListCache::EntryListCache()
{
	getByIdQuery.prepare("select lists.rowid, parent, position, type, id, label from lists left join listsLabels on lists.rowid == listsLabels.rowid where lists.rowid = ?");
//	getByParentPosQuery.prepare("select lists.rowid, parent, position, type, id, label from lists left join listsLabels on lists.rowid == listsLabels.rowid where lists.parent = ? order by position limit 1 offset ?");
	getByParentPosQuery.prepare("select lists.rowid, parent, position, type, id, label from lists left join listsLabels on lists.rowid == listsLabels.rowid where lists.parent = ? and position = ?");
//	getByParentPosRootQuery.prepare("select lists.rowid, parent, position, type, id, label from lists left join listsLabels on lists.rowid == listsLabels.rowid where lists.parent is null order by position limit 1 offset ?");
	getByParentPosRootQuery.prepare("select lists.rowid, parent, position, type, id, label from lists left join listsLabels on lists.rowid == listsLabels.rowid where lists.parent is null and position = ?");
//	fixListPositionQuery.prepare("update lists set position = ? where rowid = ?");
}

EntryListCache &EntryListCache::instance()
{
	static EntryListCache _instance;
	return _instance;
}

const EntryListCachedEntry &EntryListCache::get(int rowId)
{
	QMutexLocker ml(&_cacheLock);
	if (!rowIdCache.contains(rowId)) {
		getByIdQuery.addBindValue(rowId);
		getByIdQuery.exec();
		EntryListCachedEntry nCache(getByIdQuery);
		getByIdQuery.finish();
		rowIdCache[nCache.rowId()] = nCache;
		rowParentCache[QPair<int, int>(nCache.parent(), nCache.position())] = nCache;
	}
	return rowIdCache[rowId];
}

static EntryListCachedEntry invalidEntry;

const EntryListCachedEntry &EntryListCache::get(int parent, int pos)
{
	QMutexLocker ml(&_cacheLock);
	// First look for the parent list
	if (!_cachedLists.contains(parent)) {
		_cachedLists[parent] = EntryListCachedList(parent);
	}
	const EntryListCachedList &parentList = _cachedLists[parent];
	// If we required the root list, update its count
	if (parent == 0) invalidEntry._count = parentList._entriesArray.size();
	if (pos < 0) return invalidEntry;
	if (pos >= parentList._entries.size()) return invalidEntry;
	else return *(parentList._entriesArray[pos]);
}

void EntryListCache::invalidate(uint rowId)
{
	QMutexLocker ml(&_cacheLock);
	if (!rowIdCache.contains(rowId)) return;
	const EntryListCachedEntry &cEntry = rowIdCache[rowId];
	rowParentCache.remove(QPair<int, int>(cEntry.parent(), cEntry.position()));
	rowIdCache.remove(rowId);
}

void EntryListCache::invalidateAll()
{
	QMutexLocker ml(&_cacheLock);
	rowParentCache.clear();
	rowIdCache.clear();
}
