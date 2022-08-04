/*
	File: BinaryMap.hpp
	Author: Philip Haynes
	A map implemented as a binary tree.
	Requires operator< to be defined for T.
*/

#ifndef AZCORE_BINARY_MAP_HPP
#define AZCORE_BINARY_MAP_HPP

#include "../basictypes.hpp"
#include "Array.hpp"
#include <utility>
#include <initializer_list>

namespace AzCore {

template <typename Node_t>
class BinaryMapIterator;

template <typename Key_t, typename Value_t>
struct BinaryMap {
	struct Node {
		Node *left = nullptr;
		Node *right = nullptr;
		Key_t key;
		Value_t value;

		Node() = default;
		Node(const Node &other) : key(other.key), value(other.value) {
			if (other.left) {
				left = new Node(*other.left);
			}
			if (other.right) {
				right = new Node(*other.right);
			}
		}
		Node(Node &&other) :
			left(other.left), right(other.right),
			key(std::move(other.key)), value(std::move(other.value))
		{
			other.left = nullptr;
			other.right = nullptr;
		}
		Node(Key_t newKey, Value_t newValue) :
			key(newKey), value(newValue) {}
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

		Value_t& Emplace(Node && node) {
			if (key == node.key) {
				return value = std::move(node.value);
			} else if (key < node.key) {
				if (right) {
					return right->Emplace(std::move(node));
				} else {
					right = new Node(std::move(node));
					return right->value;
				}
			} else {
				if (left) {
					return left->Emplace(std::move(node));
				} else {
					left = new Node(std::move(node));
					return left->value;
				}
			}
		}

		bool Exists(const Key_t& testKey) const {
			if (key == testKey) return true;
			if (key < testKey) {
				if (!right) return false;
				return right->Exists(testKey);
			} else {
				if (!left) return false;
				return left->Exists(testKey);
			}
		}

		Node* Find(const Key_t& testKey) {
			if (key == testKey) return this;
			if (key < testKey) {
				if (!right) return nullptr;
				return right->Find(testKey);
			} else {
				if (!left) return nullptr;
				return left->Find(testKey);
			}
		}

		Value_t& ValueOf(const Key_t& testKey) {
			if (key == testKey) return value;
			if (key < testKey) {
				if (right) {
					return right->ValueOf(testKey);
				} else {
					right = new Node();
					right->key = testKey;
					return right->value;
				}
			} else {
				if (left) {
					return left->ValueOf(testKey);
				} else {
					left = new Node();
					left->key = testKey;
					return left->value;
				}
			}
		}
	};
	Node *base = nullptr;

	BinaryMap() = default;
	BinaryMap(const BinaryMap &other) {
		if (other.base) {
			base = new Node(*other.base);
		}
	}
	BinaryMap(BinaryMap &&other) : base(other.base) {other.base = nullptr;};
	BinaryMap(const std::initializer_list<Node> &init) {
		// TODO: Maybe sort and recursively bisect before emplacing?
		for (const Node &node : init) {
			Node newNode = node;
			Emplace(std::move(newNode));
		}
	}
	~BinaryMap() {
		if (base) delete base;
	}

	void Clear() {
		if (base) delete base;
		base = nullptr;
	}

	inline bool Empty() const {
		return nullptr == base;
	}

	BinaryMap& operator=(const BinaryMap &other) {
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
	BinaryMap& operator=(BinaryMap &&other) {
		if (this == &other) return *this;
		if (base) delete base;
		base = other.base;
		other.base = nullptr;
		return *this;
	}

	force_inline()
	Value_t& Emplace(Key_t key, Value_t value) {
		return Emplace(Node(key, value));
	}

	Value_t& Emplace(Node &&node) {
		// The branch predictor should predict this correctly almost every time.
		if (base) {
			return base->Emplace(std::move(node));
		} else {
			base = new Node(std::move(node));
			return base->value;
		}
	}

	bool Exists(Key_t key) const {
		if (!base) return false;
		return base->Exists(key);
	}

	Node* Find(Key_t key) {
		if (!base) return nullptr;
		return base->Find(key);
	}

	Value_t& ValueOf(Key_t key) {
		if (base) {
			return base->ValueOf(key);
		} else {
			base = new Node();
			base->key = key;
			return base->value;
		}
	}

	force_inline()
	Value_t& operator[](Key_t key) {
		return ValueOf(key);
	}

	BinaryMapIterator<Node> begin() {
		return BinaryMapIterator<Node>(base);
	}
	BinaryMapIterator<Node> end() {
		return BinaryMapIterator<Node>(nullptr);
	}
	BinaryMapIterator<const Node> begin() const {
		return BinaryMapIterator<const Node>(base);
	}
	BinaryMapIterator<const Node> end() const {
		return BinaryMapIterator<const Node>(nullptr);
	}
};

template <typename Node_t>
class BinaryMapIterator {
	Array<Node_t*> stack;

public:
	BinaryMapIterator() : stack({nullptr}) {}
	BinaryMapIterator(Node_t *startNode) : stack({startNode}) {
		if (stack.Back()) {
			while (stack.Back()->left) {
				stack.Append(stack.Back()->left);
			}
		}
	}
	bool operator!=(const BinaryMapIterator &other) const {
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
	Node_t& operator*() {
		return *stack.Back();
	}
};

} // namespace AzCore

#endif // AZCORE_BINARY_MAP_HPP
