/*
	File: BinarySet.hpp
	Author: Philip Haynes
	A set implemented as a binary tree.
	Requires operator< to be defined for T.
*/

#ifndef AZCORE_BINARY_SET_HPP
#define AZCORE_BINARY_SET_HPP

#include "../basictypes.hpp"
#include "Array.hpp"
#include <utility>
#include <initializer_list>

namespace AzCore {

template <typename Key_t>
struct BinarySetIterator;

template <typename Key_t>
struct BinarySet {
	struct Node {
		Node *left = nullptr;
		Node *right = nullptr;
		Key_t key;

		Node() = default;
		Node(const Node &other) : key(other.key) {
			if (other.left) {
				left = new Node(*other.left);
			}
			if (other.right) {
				right = new Node(*other.right);
			}
		}
		Node(Node &&other) :
			left(other.left), right(other.right),
			key(std::move(other.key))
		{
			other.left = nullptr;
			other.right = nullptr;
		}
		Node(Key_t newKey) : key(newKey) {}
		~Node() {
			if (left) delete left;
			if (right) delete right;
		}
		Node& operator=(const Node &other) {
			if (left) {
				if (other.left) {
					*left = *other.left;
				} else {
					delete left;
					left = nullptr;
				}
			} else {
				if (other.left) {
					left = new Node(*other.left);
				}
			}
			if (right) {
				if (other.right) {
					*right = *other.right;
				} else {
					delete right;
					right = nullptr;
				}
			} else {
				if (other.right) {
					right = new Node(*other.right);
				}
			}
			return *this;
		}

		Node& operator=(Node &&other) {
			if (left) delete left;
			left = other.left;
			other.left = nullptr;
			if (right) delete right;
			right = other.right;
			other.right = nullptr;
			return *this;
		}

		void Emplace(Node && node) {
			if (key == node.key) {
				return;
			}
			if (node.key < key) {
				if (left) {
					left->Emplace(std::move(node));
				} else {
					left = new Node(std::move(node));
				}
			} else {
				if (right) {
					right->Emplace(std::move(node));
				} else {
					right = new Node(std::move(node));
				}
			}
		}

		bool Exists(Key_t testKey) const {
			if (testKey == key) return true;
			if (testKey < key) {
				if (!left) return false;
				return left->Exists(testKey);
			} else {
				if (!right) return false;
				return right->Exists(testKey);
			}
		}
	};
	Node *base = nullptr;

	BinarySet() = default;
	BinarySet(const BinarySet &other) {
		if (other.base) {
			base = new Node(*other.base);
		}
	}
	BinarySet(BinarySet &&other) : base(other.base) {other.base = nullptr;};
	BinarySet(const std::initializer_list<Node> &init) {
		// TODO: Maybe sort and recursively bisect before emplacing?
		for (const Node &node : init) {
			Node newNode = node;
			Emplace(std::move(newNode));
		}
	}
	~BinarySet() {
		if (base) delete base;
	}
	BinarySet& operator=(const BinarySet &other) {
		if (this == &other) return *this;
		if (base) {
			if (other.base) {
				*base = *other.base;
			} else {
				delete base;
				base = nullptr;
			}
		} else {
			if (other.base) {
				base = new Node(*other.base);
			}
		}
		return *this;
	}
	BinarySet& operator=(BinarySet &&other) {
		if (this == &other) return *this;
		if (base) delete base;
		base = other.base;
		other.base = nullptr;
		return *this;
	}

	inline force_inline
	void Emplace(Key_t key) {
		Emplace(Node(key));
	}

	void Emplace(Node &&node) {
		if (base) {
			base->Emplace(std::move(node));
		} else {
			base = new Node(std::move(node));
		}
	}

	bool Exists(Key_t key) const {
		if (!base) return false;
		return base->Exists(key);
	}

	BinarySetIterator<Key_t> begin() {
		return BinarySetIterator<Key_t>(base);
	}
	BinarySetIterator<Key_t> end() {
		return BinarySetIterator<Key_t>(nullptr);
	}
	BinarySetIterator<const Key_t> begin() const {
		return BinarySetIterator<const Key_t>(base);
	}
	BinarySetIterator<const Key_t> end() const {
		return BinarySetIterator<const Key_t>(nullptr);
	}
};

template <typename Key_t>
class BinarySetIterator {
	Array<typename BinarySet<Key_t>::Node*> stack;

public:
	BinarySetIterator() : stack({nullptr}) {}
	BinarySetIterator(typename BinarySet<Key_t>::Node *startNode) : stack({startNode}) {
		if (stack.Back()) {
			while (stack.Back()->left) {
				stack.Append(stack.Back()->left);
			}
		}
	}
	bool operator!=(const BinarySetIterator &other) const {
		return stack.Back() != other.stack.Back();
	}
	void operator++() {
		if (!stack.Back()) return;
		if (stack.Back()->right) {
			stack.Back() = stack.Back()->right;
			while (stack.Back()->left) {
				stack.Append(stack.Back()->left);
			}
		} else if (stack.size > 1) {
			stack.size--;
		} else {
			stack.Back() = nullptr;
		}
	}
	Key_t& operator*() {
		return stack.Back()->key;
	}
};

} // namespace AzCore

#endif // AZCORE_SET_HPP
