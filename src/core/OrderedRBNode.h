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

#ifndef __CORE_ORDEREDRBNode_H
#define __CORE_ORDEREDRBNode_H

#include <QtDebug>
#include <QtGlobal>

template <template<class NT> class Node, class T>
class OrderedRBTree;

/**
 * Base node type lacking the logic to access parent or child nodes.
 */
template <class T> class OrderedRBNodeBase
{
public:
	typedef enum { BLACK, RED } Color;
protected:
	quint32 _leftSize;
	Color _color;
	T _value;

public:
	OrderedRBNodeBase(const T &va) : _leftSize(0), _color(RED), _value(va)
	{
	}

	virtual ~OrderedRBNodeBase()
	{
	}

	Color color() const { return _color; }
	void setColor(Color col) { _color = col; }
	quint32 leftSize() const { return _leftSize; }

	virtual OrderedRBNodeBase<T> *left() const = 0;
	virtual OrderedRBNodeBase<T> *right() const = 0;
	virtual OrderedRBNodeBase<T> *parent() const = 0;

	const T &value() const { return _value; }
	virtual void setValue(const T &nv)
	{
		_value = nv;
	}

	OrderedRBNodeBase<T> *grandParent()
	{
		OrderedRBNodeBase<T> *p = parent();
		if (p != 0) return p->parent();
		else return 0;
	}

	OrderedRBNodeBase<T> *uncle()
	{
		OrderedRBNodeBase<T> *gp = grandParent();
		if (!gp) return 0;
		if (parent() == gp->left()) return gp->right();
		else return gp->left();
	}

	OrderedRBNodeBase<T> *sibling()
	{
		OrderedRBNodeBase<T> *p = parent();
		if (!p) return 0;
		if (p->left() == this) return p->right();
		else return p->left();
	}

	int size() const
	{
		int ret = 0;
		const OrderedRBNodeBase<T> *current = this;
		while (current) {
			ret += current->leftSize() + 1;
			current = current->right();
		}
		return ret;
	}

	void calculateLeftSize()
	{
		if (!left()) _leftSize = 0;
		else _leftSize = left()->size();
	}

	int position() const
	{
		const OrderedRBNodeBase<T> *current = this;
		const OrderedRBNodeBase<T> *curParent = this->parent();
		int ret = this->_leftSize;
		while (curParent) {
			if (current = curParent->right()) ret += curParent->_leftSize + 1;
			current = curParent;
			curParent = curParent->parent();
		}
		return ret;
	}
};

/**
 * A node type that implements hierarchy through simple pointers. This class is final and
 * do not need any virtual functions.
 */
template <class T> class OrderedRBNode : public OrderedRBNodeBase<T>
{
private:
	OrderedRBNode<T> *_left, *_right, *_parent;

friend class OrderedRBTree<OrderedRBNode, T>;
friend class OrderedRBTreeTests;

public:
	OrderedRBNode(const T &va) : OrderedRBNodeBase<T>(va), _left(0), _right(0), _parent(0)
	{
	}

	~OrderedRBNode()
	{
	}

	OrderedRBNode<T> *left() const { return _left; }
	OrderedRBNode<T> *right() const { return _right; }
	OrderedRBNode<T> *parent() const { return _parent; }
	void setLeft(OrderedRBNode<T> *nl)
	{
		_left = nl;
		if (nl) nl->_parent = this;
	}

	void setRight(OrderedRBNode<T> *nr)
	{
		_right = nr;
		if (nr) nr->_parent = this;
	}

	/// Detach the node from the current parent
	void detach()
	{
		if (_parent) {
			if (_parent->left() == this) _parent->_left = 0;
			else if (_parent->right() == this) _parent->_right = 0;
			_parent = 0;
		}
	}
};

template <template<class NT> class Node, class T> class OrderedRBTree
{
private:
	Node<T> *root;

	void insertCase1(Node<T> *inserted)
	{
		// Added a root Node<T>?
		if (!inserted->parent())
			inserted->_color = Node<T>::BLACK;
		else insertCase2(inserted);
	}

	void insertCase2(Node<T> *inserted)
	{
		// Parent black? Tree still valid
		if (inserted->parent()->color() == Node<T>::BLACK) return;
		else insertCase3(inserted);
	}

	void insertCase3(Node<T> *inserted)
	{
		Node<T> *uncle = static_cast<Node<T> *>(inserted->uncle());
		// Parent and uncle red? Recolor them.
		if (uncle != 0 && uncle->color() == Node<T>::RED) {
			inserted->parent()->setColor(Node<T>::BLACK);
			uncle->setColor(Node<T>::BLACK);
			Node<T> *grandParent = static_cast<Node<T> *>(inserted->grandParent());
			grandParent->setColor(Node<T>::RED);
			insertCase1(grandParent);
		} else insertCase4(inserted);
	}

	void insertCase4(Node<T> *inserted)
	{
		Node<T> *grandParent = static_cast<Node<T> *>(inserted->grandParent());
		// Parent red, uncle black, inserted node and parent on
		// opposite sides from their parent
		if (inserted == inserted->parent()->right() && inserted->parent() == grandParent->left()) {
			rotateLeft(inserted->parent());
			inserted = inserted->left();
		} else if (inserted == inserted->parent()->left() && inserted->parent() == grandParent->right()) {
			rotateRight(inserted->parent());
			inserted = inserted->right();
		}
		insertCase5(inserted);
	}

	void insertCase5(Node<T> *inserted)
	{
		// Parent red, uncle black, inserted Node<T> and parent on
		// same side from their parent
		Node<T> *grandParent = static_cast<Node<T> *>(inserted->grandParent());
		inserted->parent()->setColor(Node<T>::BLACK);
		grandParent->setColor(Node<T>::RED);
		if (inserted == inserted->parent()->left() && inserted->parent() == grandParent->left()) {
			rotateRight(grandParent);
		} else {
			rotateLeft(grandParent);
		}
	}

	void rotateLeft(Node<T> *pivot)
	{
		enum { ROOT, LEFT, RIGHT } parentSide = pivot->parent() ?
			pivot == pivot->parent()->left() ? LEFT : RIGHT : ROOT;
		Node<T> *newParent = pivot->right();
		// Move right child to node's place
		switch (parentSide) {
		case ROOT:
			newParent->detach();
			root = newParent;
			break;
		case LEFT:
			pivot->parent()->setLeft(newParent);
			break;
		case RIGHT:
			pivot->parent()->setRight(newParent);
			break;
		}
		// Move node as the left child of its right child
		pivot->setRight(newParent->left());
		// Move node to new parent's left
		newParent->setLeft(pivot);
		// Update left weight of rotated node
		newParent->calculateLeftSize();
	}

	void rotateRight(Node<T> *pivot)
	{
		enum { ROOT, LEFT, RIGHT } parentSide = pivot->parent() ?
			pivot == pivot->parent()->left() ? LEFT : RIGHT : ROOT;
		Node<T> *newParent = pivot->left();
		// Move left child to node's place
		switch (parentSide) {
		case ROOT:
			newParent->detach();
			root = newParent;
			break;
		case LEFT:
			pivot->parent()->setLeft(newParent);
			break;
		case RIGHT:
			pivot->parent()->setRight(newParent);
			break;
		}
		// Move node as the right child of its left child
		pivot->setLeft(newParent->right());
		// Move node to new parent's right
		newParent->setRight(pivot);
		// Update left weight of rotated node
		pivot->calculateLeftSize();
	}

public:
	OrderedRBTree() : root(0)
	{
	}

	~OrderedRBTree()
	{
		clear();
	}

	/**
	 * Complexity: O(log n)
	 */
	int size() const
	{
		if (!root) return 0;
		else return root->size();
	}

	/**
	 * Returns the value at index. Will crash if access is made out of bounds.
	 * Complexity: O(log n)
	 */
	const T& operator[](int index) const
	{
		const Node<T> *current = root;
		unsigned int baseIdx = 0;

		while (current) {
			int curPos = baseIdx + current->leftSize();
			if (curPos == index) break;
			else if (index < curPos) current = current->left();
			else {
				baseIdx += current->leftSize() + 1;
				current = current->right();
			}
		}
		// Will crash if access is out of bounds because current would then be 0
		if (!current) qFatal("Error: accessing RBTree out of bounds");
		return current->value();
	}

	/**
	 * Insert the value val at index.
	 * Complexity: O(log n)
	 * TODO No test to check whether we are inserting out of bounds! (both inferior and superior)
	 */
	void insert(const T &val, int index)
	{
		Node<T> *current = root;
		unsigned int baseIdx = 0;
		Node<T> *newNode = new Node<T>(val);

		// Insert into root
		if (!current) {
			root = newNode;
		}
		// Otherwise find the leaf where to add our node
		else while (true) {
			int curPos = baseIdx + current->leftSize();
			// We add on the left, so leftSize must be updated
			if (index <= curPos) {
				++current->_leftSize;
				if (!current->left()) {
					current->setLeft(newNode);
					break;
				}
				else current = current->left();
			}
			// We add on the right, update the base position index
			else {
				baseIdx += current->leftSize() + 1;
				if (!current->right()) {
					current->setRight(newNode);
					break;
				}
				else current = current->right();
			}
		}
		// Perform balancing
		insertCase1(newNode);
	}

	/**
	 * Remove the value at position index. Returns true on success, false otherwise.
	 * Complexity: O(log n)
	 */
	bool remove(int index)
	{
		if (index < 0) return false;

		Node<T> *current = root;
		unsigned int baseIdx = 0;

		// First find the node to remove
		while (current) {
			int curPos = baseIdx + current->leftSize();
			if (curPos == index) break;
			else if (index < curPos) current = current->left();
			else {
				baseIdx += current->leftSize() + 1;
				current = current->right();
			}
		}

		// Index to remove not found
		if (!current) return false;

		// Node has two childs, replace its value with the leftmost value at its
		// right side and replace that latter node with its right child before deleting it.
		if (current->left() && current->right()) {
			Node<T> *successor = current->right();
			while (successor->left()) successor = successor->left();
			current->setValue(successor->value());
			Node<T> *successorParent = successor->parent();
			if (successorParent->left() == successor) successorParent->setLeft(successor->right);
			else successorParent->setRight(successor->right());
			delete successor;
			// TODO update leftSizes!
			return true;

		}
		// Node has only one child, replace it by the child before deleting
		else if (current->left() || current->right()) {
			Node<T> *parent = current->parent();
			Node<T> *child = current->left() ? current->left() : current->right();
			// Is it root?
			if (!parent) root = child;
			else if (parent->left() == current) parent->setLeft(child);
			else parent->setRight(child);
			delete current;
			// TODO update leftSizes!
			return true;
		}
		// We are removing a leaf
		else {
			// Is it root?
			if (current == root) root = 0;
			else current->detach();
			delete current;
			// TODO update leftSizes!
			return true;
		}
		return false;
	}

	void clear()
	{
		Node<T> *current = root;
		while (current) {
			if (current->left()) current = current->left();
			else if (current->right()) current = current->right();
			else {
				Node<T> *parent = current->parent();
				enum { ROOT, LEFT, RIGHT } sideToClear = parent ?
					current == parent->left() ? LEFT : RIGHT : ROOT;

				switch (sideToClear) {
				case ROOT:
					delete root;
					root = 0;
					break;
				case LEFT:
					delete parent->left();
					parent->setLeft(0);
					break;
				case RIGHT:
					delete parent->right();
					parent->setRight(0);
					break;
				}
				current = parent;
			}
		}
	}

friend class OrderedRBTreeTests;
};

#endif
