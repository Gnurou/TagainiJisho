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

#include "OrderedRBTreeTests.h"

#include <QtDebug>

#define TEST_SIZE 3000
#define VSTRING "String at position %1"

void OrderedRBTreeTests::initTestCase()
{
}

void OrderedRBTreeTests::cleanupTestCase()
{
}

void OrderedRBTreeTests::testNode()
{
	OrderedRBNode<int> node(0, 5), left(0, -1), right(0, 10), grandChild(0, 0);
	left.setColor(OrderedRBNode<int>::BLACK);
	right.setColor(OrderedRBNode<int>::BLACK);
	QCOMPARE(node.color(), OrderedRBNode<int>::RED);
	QCOMPARE(left.color(), OrderedRBNode<int>::BLACK);
	QCOMPARE(right.color(), OrderedRBNode<int>::BLACK);
	QCOMPARE(node.value(), 5);
	QCOMPARE(left.value(), -1);
	QCOMPARE(right.value(), 10);
	QCOMPARE(grandChild.value(), 0);

	QVERIFY(!node.parent());
	QVERIFY(!node.left());
	QVERIFY(!node.right());
	node.setLeft(&left);
	left.setRight(&grandChild);
	node.setRight(&right);
	QVERIFY(node.left() == &left);
	QVERIFY(left.parent() == &node);
	QVERIFY(node.right() == &right);
	QVERIFY(right.parent() == &node);
	QVERIFY(left.right() == &grandChild);
	QVERIFY(grandChild.parent() == &left);

	typedef OrderedRBTree<OrderedRBMemTree<int> > treeType;
	QVERIFY(treeType::grandParent(&grandChild) == &node);
	QVERIFY(treeType::uncle(&grandChild) == &right);

	// Needed to access the nethods below
	OrderedRBTree<OrderedRBMemTree<int> > tree;
	tree.calculateLeftSize(&grandChild);
	tree.calculateLeftSize(&left);
	tree.calculateLeftSize(&right);
	tree.calculateLeftSize(&node);
	QCOMPARE(tree.size(&right), 1);
	QCOMPARE(tree.size(&grandChild), 1);
	QCOMPARE(tree.size(&left), 2);
	QCOMPARE(tree.size(&node), 4);

	tree.detach(&right);
	QVERIFY(!right.parent());
}

void OrderedRBTreeTests::massInsertEnd()
{
	QCOMPARE(treeEnd.size(), (unsigned int)0);
	treeEnd.checkValid();

	// Mass-insert from end
	for (int i = 0; i < TEST_SIZE; i++) {
		treeEnd.insert(QString(VSTRING).arg(i), i);
		QCOMPARE(treeEnd.size(), (unsigned int)i + 1);
		// Every 16 times, check that the tree's order is respected and that it is valid
		if ((i & 0xf) == 0) {
			treeEnd.checkValid();
			for (int j = 0; j <= i; j++)
				QCOMPARE(treeEnd[j], QString(VSTRING).arg(j));
		}
	}
	treeEnd.checkValid();
	for (int i = 0; i < TEST_SIZE; i++) {
		QCOMPARE(treeEnd[i], QString(VSTRING).arg(i));
	}
}

void OrderedRBTreeTests::massInsertBegin()
{
	QCOMPARE(treeBegin.size(), (unsigned int)0);
	treeBegin.checkValid();

	for (int i = 0; i < TEST_SIZE; i++) {
		treeBegin.insert(QString(VSTRING).arg(TEST_SIZE - (i + 1)), 0);
		QCOMPARE(treeBegin.size(), (unsigned int)i + 1);
		// Every 16 times, check that the tree's order is respected and that it is valid
		if ((i & 0xf) == 0) {
			treeBegin.checkValid();
			for (int j = 0; j <= i; j++)
				QCOMPARE(treeBegin[j], QString(VSTRING).arg(TEST_SIZE - (i + 1) + j));
		}
	}
	treeBegin.checkValid();
	for (int i = 0; i < TEST_SIZE; i++) {
		QCOMPARE(treeBegin[i], QString(VSTRING).arg(i));
	}
}

void OrderedRBTreeTests::massInsertRandom()
{
	treeRandom.clear();
	QCOMPARE(treeRandom.size(), (unsigned int)0);
	treeRandom.checkValid();

	for (int i = 0; i < TEST_SIZE; i++) {
		int pos = qrand() % (treeRandom.size() + 1);
		treeRandom.insert(QString(VSTRING).arg(i), pos);
		treeRandomPos.insert(pos, i);
		QCOMPARE(treeRandom.size(), (unsigned int)i + 1);
		// Every 16 times, check that the tree's order is respected and that it is valid
		if ((i & 0xf) == 0) {
			treeRandom.checkValid();
			for (int j = 0; j <= i; j++)
				QCOMPARE(treeRandom[j], QString(VSTRING).arg(treeRandomPos[j]));
		}
	}
	treeRandom.checkValid();

	// Also check that out-of-bounds insert fails as expected
	QCOMPARE(treeRandom.size(), (unsigned int)TEST_SIZE);
	QVERIFY(!treeRandom.insert("Out-of-bounds string", -1));
	QCOMPARE(treeRandom.size(), (unsigned int)TEST_SIZE);
	QVERIFY(!treeRandom.insert("Out-of-bounds string", TEST_SIZE + 1));
	QCOMPARE(treeRandom.size(), (unsigned int)TEST_SIZE);
	treeRandom.checkValid();

	// Make sure the tree is still in order
	for (int i = 0; i < TEST_SIZE; i++) {
		QCOMPARE(treeRandom[i], QString(VSTRING).arg(treeRandomPos[i]));
	}

}

void OrderedRBTreeTests::massRemoveEnd()
{
	QCOMPARE(treeEnd.size(), (unsigned int)TEST_SIZE);
	treeEnd.checkValid();

	for (int i = TEST_SIZE - 1; i >= 0; i--) {
		treeEnd.remove(i);
		QCOMPARE(treeEnd.size(), (unsigned int)i);
		// Every 16 times, check that the tree's order is respected and that it is valid
		if ((i & 0xf) == 0) {
			treeEnd.checkValid();
			for (unsigned int j = 0; j < treeEnd.size(); j++)
			     QCOMPARE(treeEnd[j], QString(VSTRING).arg(j));
		}
	}
	treeEnd.checkValid();
}

void OrderedRBTreeTests::massRemoveBegin()
{
	QCOMPARE(treeBegin.size(), (unsigned int)TEST_SIZE);
	treeBegin.checkValid();

	for (int i = 0; i < TEST_SIZE; ) {
		treeBegin.remove(0);
		i++;
		QCOMPARE(treeBegin.size(), (unsigned int)TEST_SIZE - i);
		// Every 16 times, check that the tree's order is respected and that it is valid
		if ((i & 0xf) == 0) {
			treeBegin.checkValid();
			for (int j = 0; j < TEST_SIZE - i; j++)
				QCOMPARE(treeBegin[j], QString(VSTRING).arg(j + i));
		}
	}
	treeBegin.checkValid();
}

void OrderedRBTreeTests::massRemoveRandom()
{
	QCOMPARE(treeRandom.size(), (unsigned int)TEST_SIZE);
	treeRandom.checkValid();

	for (int i = 0; i < TEST_SIZE; ) {
		int pos = qrand() % treeRandom.size();
		treeRandom.remove(pos);
		treeRandomPos.removeAt(pos);
		i++;
		QCOMPARE(treeRandom.size(), (unsigned int)TEST_SIZE - i);
		// Every 16 times, check that the tree's order is respected and that it is valid
		if ((i & 0xf) == 0) {
			treeRandom.checkValid();
			for (int j = 0; j < TEST_SIZE - i; j++)
				QCOMPARE(treeRandom[j], QString(VSTRING).arg(treeRandomPos[j]));
		}
	}
	treeRandom.checkValid();
}

#define SHORT_TEST_SIZE (TEST_SIZE / 5)
void OrderedRBTreeTests::deepCheckValidity()
{
	tree.clear();
	QCOMPARE(tree.size(), (unsigned int)0);
	tree.checkValid();

	for (int i = 0; i < SHORT_TEST_SIZE; i++) {
		int pos = qrand() % (i + 1);
		tree.insert(QString(VSTRING).arg(pos), pos);
		QCOMPARE(tree.size(), (unsigned int)i + 1);
		tree.checkValid();
	}

	for (int i = 0; i < SHORT_TEST_SIZE; ) {
		int pos = qrand() % tree.size();
		tree.remove(pos);
		i++;
		QCOMPARE(tree.size(), (unsigned int)SHORT_TEST_SIZE - i);
		tree.checkValid();
	}
}

QTEST_MAIN(OrderedRBTreeTests)
