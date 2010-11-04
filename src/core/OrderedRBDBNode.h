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
#ifndef __CORE_ORDEREDRBDBNODE_H
#define __CORE_ORDEREDRBDBNODE_H

#include "core/OrderedRBNode.h"
#include "core/EntryListDB.h"

#include <QSet>

template <class T> class OrderedRBDBTree;

/**
 * Implements database storage for RB tree nodes.
 * When a node is created, all its data is loaded and cached.
 * When the left/right members are first accessed, the corresponding
 * node is created and loaded.
 */
template <class T> class OrderedRBDBNode : public OrderedRBNodeBase<T>
{
private:
	OrderedRBDBTree<T> * _tree;
	mutable OrderedRBDBNode<T> * _left;
	mutable OrderedRBDBNode<T> * _right;
	mutable OrderedRBDBNode<T> * _parent;
	DBListEntry<T> e;

public:
	OrderedRBDBNode(OrderedRBDBTree<T> *tree, const T &va) : _tree(tree), _left(0), _right(0), _parent(0), e()
	{
		// Here a new node is to be inserted in the tree - we need to insert it right now into
		// the DB in order to get its ID.
		setValue(va);
		updateDB();
	}

	OrderedRBDBNode(OrderedRBDBTree<T> *tree, quint32 rowid) : _tree(tree), _left(0), _right(0), _parent(0), e()
	{
		// The new node is expected to exist in the DB with the given ID - just load it.
		e = _tree->dbAccess()->getEntry(rowid);
		OrderedRBNodeBase<T>::setColor(e.red ?  OrderedRBNodeBase<T>::RED : OrderedRBNodeBase<T>::BLACK);
		OrderedRBNodeBase<T>::setLeftSize(e.leftSize);
	}

	~OrderedRBDBNode()
	{
	}

	void setColor(typename OrderedRBNodeBase<T>::Color col)
	{
		OrderedRBNodeBase<T>::setColor(col);
		e.red = (col == OrderedRBNodeBase<T>::RED);
		_tree->nodeChanged(this);
	}

	void setLeftSize(quint32 lSize)
	{
		OrderedRBNodeBase<T>::setLeftSize(lSize);
		e.leftSize = lSize;
		_tree->nodeChanged(this);
	}

	const T &value() const { return e.data; }
	void setValue(const T &nv)
	{
		e.data = nv;
		_tree->nodeChanged(this);
	}

	OrderedRBDBNode<T> *left() const
	{
		// Load the node if not already done and return it
		if (!_left && e.left) {
			_left = new OrderedRBDBNode<T>(_tree, e.left);
			_left->_parent = const_cast<OrderedRBDBNode<T> *>(this);
		}
		return _left;
	}
	OrderedRBDBNode<T> *right() const
	{
		// Load the node if not already done and return it
		if (!_right && e.right) {
			_right = new OrderedRBDBNode<T>(_tree, e.right);
			_right->_parent = const_cast<OrderedRBDBNode<T> *>(this);
		}
		return _right;
	}
	OrderedRBDBNode<T> *parent() const
	{
		// Parent is always available
		return _parent;
	}
	void setLeft(OrderedRBDBNode<T> *nl)
	{
		_left = nl;
		e.left = nl ? nl->e.rowId : 0;
		_tree->nodeChanged(this);
		if (nl) nl->setParent(this);
	}
	void setRight(OrderedRBDBNode<T> *nr)
	{
		_right = nr;
		e.right = nr ? nr->e.rowId : 0;
		_tree->nodeChanged(this);
		if (nr) nr->setParent(this);
	}
	void setParent(OrderedRBDBNode<T> *np)
	{
		_parent = np;
		e.parent = np ? np->e.rowId : 0;
		_tree->nodeChanged(this);
	}

	/**
	 * Called by OrderedRBDBTree for every node that has been marked as being changed.
	 */
	bool updateDB()
	{
		e.rowId = _tree->dbAccess()->insertEntry(e);
		return e.rowId != 0;
	}
friend class OrderedRBDBTree<T>;
};

template <class T> class OrderedRBDBTree
{
public:
	typedef OrderedRBDBNode<T> Node;

private:
	mutable Node *_root;
	mutable quint32 _rootId;
	QSet<Node *> _changedNodes;
	DBList<T> *_ldb;

	void clearMemCache();

public:
	OrderedRBDBTree() : _root(0), _rootId(0), _ldb(0)
	{
	}

	/// Remove all the in-memory structures, leaving the database unchanged
	~OrderedRBDBTree()
	{
		clearMemCache();
	}

	Node *root() const
	{
		// Load the root node from the DB if necessary
		if (!_root) {
			if (_rootId == 0) {
				DBListEntry<T> e = _ldb->getRoot();
				_rootId = e.rowId;
			}
			if (_rootId != 0) _root = new Node(const_cast<OrderedRBDBTree<T> *>(this), _rootId);
		}
		return _root;
	}

	void setRoot(Node *node)
	{
		_root = node;
		if (node) _rootId = node->e.rowId;
		else _rootId = 0;
	}

        bool aboutToChange()
	{
		_changedNodes.clear();
		// TODO start transaction
		return true;
	}

        void nodeChanged(Node *n)
	{
		_changedNodes << n;
	}

	bool removeNode(Node *node)
	{
		if (!_ldb->removeEntry(node->e.rowId)) return false;
		// Make sure deleted nodes do not get updated
		_changedNodes.remove(node);
		delete node;
		return true;
	}

        bool commitChanges()
	{
		foreach (Node *n, _changedNodes) {
			if (!n->updateDB()) {
				// TODO Abort transaction
				return false;
			}
		}
		_changedNodes.clear();
		// TODO Commit transaction
		return true;
	}

	void abortChanges()
	{
		_changedNodes.clear();
		// TODO Abort transaction
	}

	void setDBAccess(DBList<T> *ldb)
	{
		_ldb = ldb;
	}

	/// Sets the rowid of the root node for this list.
	/// This allows to implement nested lists using our
	/// flat model.
	void setRootId(quint32 id)
	{
		_rootId = id;
		clearMemCache();
	}

	DBList<T> *dbAccess() { return _ldb; }

};

template <class T>
void OrderedRBDBTree<T>::clearMemCache()
{
	Node *current = _root;
	while (current) {
		if (current->_left) current = current->_left;
		else if (current->_right) current = current->_right;
		else {
			Node *parent = current->_parent;
			if (current == _root) {
				delete _root;
				_root = 0;
			}
			else if (current == parent->_left) {
				delete parent->_left;
				parent->_left = 0;
			}
			else {
				delete parent->_right;
				parent->_right = 0;
			}
			current = parent;
		}
	}
}

#endif
