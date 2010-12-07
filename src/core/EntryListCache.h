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

#ifndef __CORE_ENTRYLISTCACHE_H
#define __CORE_ENTRYLISTCACHE_H

#include "core/EntriesCache.h"
#include "core/EntryListDB.h"

#include <QPair>

/**
 * A cache class that is responsible for providing information about the
 * lists. Whenever an entry is changed in the DB, it has to be invalidated
 * here.
 *
 * Methods of this class are thread-safe.
 */
class EntryListCache {
private:
	static EntryListCache *_instance;
	SQLite::Connection _connection;
	SQLite::Query ownerQuery, goUpQuery, listFromRootQuery;
	EntryListDBAccess _dbAccess;
	QMap<quint64, EntryList *> _cachedLists;
	QMap<quint64, QPair<const EntryList *, quint32> > _cachedParents;
	QMutex _cacheLock;

	EntryListCache();
	~EntryListCache();

	EntryList *_get(quint64 id);
	void _clearListCache(quint64 id);
	QPair <const EntryList *, quint32> _getOwner(quint64 id);
	void _clearOwnerCache(quint64 id);

public:
	/// Returns a reference to the unique instance of this class.
	static EntryListCache &instance();
	/// Clears all resources used by the cache. instance() can be called
	/// again in order to allocate a new one.
	static void cleanup();

	static EntryList *get(quint64 id) { return instance()._get(id); }
	static void clearListCache(quint64 id) { return instance()._clearListCache(id); }
	/// Returns the list that contains the list which id is given in parameter.
	static QPair<const EntryList *, quint32> getOwner(quint64 id) { return instance()._getOwner(id); }
	static void clearOwnerCache(quint64 id) { instance()._clearOwnerCache(id); }
	/// Returns the database connection used by the entry list system
	static SQLite::Connection *connection() { return &instance()._connection; }
};

#endif
