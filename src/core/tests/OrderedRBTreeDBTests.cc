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

#include "OrderedRBTreeDBTests.h"

static const unsigned int nbEntries = 3;
static EntryListEntry entries[nbEntries] =
{
	{ 0, 1, false, 0, 2, 3, { 1, 28 } },
	{ 0, 0, true,  1, 0, 0, { 1, 29 } },
	{ 0, 0, false, 1, 0, 0, { 2, 44 } }
};

static EntryListData listData[nbEntries] =
{
	{ 1, 10 }, { 0, 0, "SubList"}, { 1, 30 }
};

static EntryListData subListData[nbEntries] =
{
	{ 1, 20 }, { 1, 25 }, { 1, 29 }
};

void OrderedRBTreeDBTests::initTestCase()
{
	QVERIFY(dbFile.open());
	QVERIFY(connection.connect(dbFile.fileName()));
}

void OrderedRBTreeDBTests::cleanupTestCase()
{
	stringListDB.prepareForConnection(0);
	listDB.prepareForConnection(0);
	QVERIFY(connection.close());
}

void OrderedRBTreeDBTests::createTableTest()
{
	QVERIFY(listDB.createTables(&connection));
	QVERIFY(listDB.prepareForConnection(&connection));

	QVERIFY(stringListDB.createTables(&connection));
	QVERIFY(stringListDB.prepareForConnection(&connection));
}

void OrderedRBTreeDBTests::insertDataTest()
{
	SQLite::Query q;
	q.useWith(&connection);
	QVERIFY(q.prepare(QString("select count(*) from %1").arg(listDB.tableName())));
	for (unsigned int i = 0; i < nbEntries; i++) {
		QCOMPARE(listDB.insertEntry(entries[i]), i + 1);
		entries[i].rowId = i + 1;
	
		// Check the number of DB rows is consistent
		QVERIFY(q.exec());
		QVERIFY(q.next());
		QCOMPARE(q.valueUInt(0), i + 1);
		q.reset();
	}

}

void OrderedRBTreeDBTests::retrieveDataTest()
{
	for (unsigned int i = 0; i < nbEntries; i++) {
		EntryListEntry res = listDB.getEntry(i + 1);
		QCOMPARE(res.rowId, entries[i].rowId);
		QCOMPARE(res.leftSize, entries[i].leftSize);
		QCOMPARE(res.red, entries[i].red);
		QCOMPARE(res.parent, entries[i].parent);
		QCOMPARE(res.left, entries[i].left);
		QCOMPARE(res.right, entries[i].right);

		//QCOMPARE(res.data.listId, entries[i].data.listId);
		QCOMPARE(res.data.type, entries[i].data.type);
		QCOMPARE(res.data.id, entries[i].data.id);
	}
}

void OrderedRBTreeDBTests::updateDataTest()
{
	// Change values in DB
	for (unsigned int i = 0; i < nbEntries; i++) {
		entries[i].rowId = nbEntries - i;
		QVERIFY(listDB.insertEntry(entries[i]));
	}

	// Ensure the number of rows did not change
	SQLite::Query q;
	q.useWith(&connection);
	QVERIFY(q.exec(QString("select count(*) from %1").arg(listDB.tableName())));
	QVERIFY(q.next());
	QCOMPARE(q.valueUInt(0), nbEntries);
	
	// Ensure the data is consistent
	for (unsigned int i = 0; i < nbEntries; i++) {
		EntryListEntry res = listDB.getEntry(i + 1);
		unsigned int idx = nbEntries - (i + 1);
		QCOMPARE(res.rowId, entries[idx].rowId);
		QCOMPARE(res.leftSize, entries[idx].leftSize);
		QCOMPARE(res.red, entries[idx].red);
		QCOMPARE(res.parent, entries[idx].parent);
		QCOMPARE(res.left, entries[idx].left);
		QCOMPARE(res.right, entries[idx].right);

		//QCOMPARE(res.data.listId, entries[idx].data.listId);
		QCOMPARE(res.data.type, entries[idx].data.type);
		QCOMPARE(res.data.id, entries[idx].data.id);
	}
}

void OrderedRBTreeDBTests::removeDataTest()
{
	SQLite::Query q;
	q.useWith(&connection);
	QVERIFY(q.prepare(QString("select count(*), min(rowid), max(rowid) from %1").arg(listDB.tableName())));
	for (unsigned int i = 0; i < nbEntries; i++) {
		listDB.removeEntry(i + 1);
		// Check the number of DB rows is consistent
		QVERIFY(q.exec());
		QVERIFY(q.next());
		QCOMPARE(q.valueUInt(0), nbEntries - (i + 1));
		q.reset();
	}
}

template <> QString DBListEntry<QString>::tableDataMembers()
{
	return "str VARCHAR";
}

template <> void DBListEntry<QString>::bindDataValues(SQLite::Query &query) const
{
	query.bindValue(data);
}

template <> void DBListEntry<QString>::readDataValues(SQLite::Query &query, int start)
{
	data = query.valueString(start);
}

void OrderedRBTreeDBTests::createTreeTest()
{
        OrderedRBTree<OrderedRBDBTree<QString> > tree;
	tree.tree()->setDBAccess(&stringListDB);
	tree.tree()->setListId(1);
	tree.tree()->setLabel("Test list");

	QCOMPARE(tree.size(), 0);

	QVERIFY(tree.insert(QString("Test"), 0));
	QCOMPARE(tree.size(), 1);
	QVERIFY(tree.insert(QString("Test2"), 1));
	QCOMPARE(tree.size(), 2);
	QVERIFY(tree.insert(QString("Test3"), 0));
	QCOMPARE(tree.size(), 3);
	QCOMPARE(tree[0], QString("Test3"));
	QCOMPARE(tree[1], QString("Test"));
	QCOMPARE(tree[2], QString("Test2"));

	SQLite::Query q;
	q.useWith(&connection);
	QVERIFY(q.exec(QString("select * from %1 order by rowid").arg(stringListDB.tableName())));
	QVERIFY(q.next());
	QCOMPARE(q.valueInt(0), 1);
	QCOMPARE(q.valueInt(1), 1);
	QCOMPARE(q.valueBool(2), false);
	QCOMPARE(q.valueInt(3), 0);
	QCOMPARE(q.valueInt(4), 3);
	QCOMPARE(q.valueInt(5), 2);
	QCOMPARE(q.valueString(6), QString("Test"));
	QVERIFY(q.next());
	QCOMPARE(q.valueInt(0), 2);
	QCOMPARE(q.valueInt(1), 0);
	QCOMPARE(q.valueBool(2), false);
	QCOMPARE(q.valueInt(3), 1);
	QCOMPARE(q.valueInt(4), 0);
	QCOMPARE(q.valueInt(5), 0);
	QCOMPARE(q.valueString(6), QString("Test2"));
	QVERIFY(q.next());
	QCOMPARE(q.valueInt(0), 3);
	QCOMPARE(q.valueInt(1), 0);
	QCOMPARE(q.valueBool(2), false);
	QCOMPARE(q.valueInt(3), 1);
	QCOMPARE(q.valueInt(4), 0);
	QCOMPARE(q.valueInt(5), 0);
	QCOMPARE(q.valueString(6), QString("Test3"));
	QVERIFY(!q.next());

	QCOMPARE(tree.tree()->listId(), (quint32) 1);
	QVERIFY(q.exec(QString("select * from %1Roots").arg(stringListDB.tableName())));
	QVERIFY(q.next());
	QCOMPARE(q.valueInt(0), 1);
	QCOMPARE(q.valueInt(1), 1);
	QCOMPARE(q.valueString(2), QString("Test list"));
	QVERIFY(!q.next());
}

void OrderedRBTreeDBTests::retrieveTreeTest()
{
	OrderedRBTree<OrderedRBDBTree<QString> > tree;
	tree.tree()->setDBAccess(&stringListDB);
	tree.tree()->setListId(1);

	QCOMPARE(tree.tree()->listId(), (quint32)1);
	QCOMPARE(tree.tree()->label(), QString("Test list"));
	QCOMPARE(tree.size(), 3);
	QCOMPARE(tree[0], QString("Test3"));
	QCOMPARE(tree[1], QString("Test"));
	QCOMPARE(tree[2], QString("Test2"));
}

void OrderedRBTreeDBTests::removeTreeTest() {
	SQLite::Query q;
	q.useWith(&connection);
	QVERIFY(q.prepare(QString("select count(*) from %1").arg(stringListDB.tableName())));

        OrderedRBTree<OrderedRBDBTree<QString> > tree;
	tree.tree()->setDBAccess(&stringListDB);
	tree.tree()->setListId(1);

	for (int i = 0; i < 3; i++) {
		QVERIFY(tree.remove(0));
		QVERIFY(q.exec());
		QVERIFY(q.next());
		QCOMPARE(q.valueInt(0), 3 - (i + 1));
		QCOMPARE(tree.size(), 3 - (i + 1));
		q.reset();
	}
	tree.tree()->removeList();
	QVERIFY(q.exec(QString("select * from %1Roots").arg(stringListDB.tableName())));
	QVERIFY(!q.next());
}

void OrderedRBTreeDBTests::subTreeTest()
{
        EntryList tree, subTree;

	// First create the root tree
	tree.tree()->setDBAccess(&listDB);
	tree.tree()->setListId(1);
	for (unsigned int i = 0; i < nbEntries; i++) {
		QVERIFY(tree.insert(listData[i], i));
	}
	// Clear it
	tree.tree()->clearMemCache();
	// Quickly check the data is valid
	QCOMPARE(tree[1].id, listData[1].id);
	QCOMPARE(tree[2].id, listData[2].id);
	QCOMPARE(tree[0].id, listData[0].id);
	// Insert sub-tree
}

QTEST_MAIN(OrderedRBTreeDBTests)

