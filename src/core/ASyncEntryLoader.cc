/*
 *  Copyright (C) 2008  Alexandre Courbot
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

#include "core/EntriesCache.h"
#include "core/ASyncEntryLoader.h"

ASyncEntryLoader::ASyncEntryLoader(DatabaseThread *dbConn) : ASyncQuery(dbConn)
{
	connect(this, SIGNAL(result(const QList<QVariant> &)), this, SLOT(_loadEntry(const QList<QVariant> &)));
}

void ASyncEntryLoader::_loadEntry(const QList<QVariant> &record)
{
	if (record.size() != 2) return;
	EntryPointer entry = EntryRef(record[0].toUInt(), record[1].toUInt()).get();
	emit result(entry);
}

ASyncEntryLoader::~ASyncEntryLoader()
{
}
