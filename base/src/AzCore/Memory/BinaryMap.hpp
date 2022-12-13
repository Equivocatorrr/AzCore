/*
	File: BinaryMap.hpp
	Author: Philip Haynes
	A map implemented as a binary tree.
	Requires operator< to be defined for T.
*/

#ifndef AZCORE_BINARY_MAP_HPP
#define AZCORE_BINARY_MAP_HPP

#include "../basictypes.hpp"
#include "../math.hpp"
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
		i32 depthDiff = 0;
		Key_t key;
		Value_t value;

		Node() = default;
		Node(const Node &other) : depthDiff(other.depthDiff), key(other.key), value(other.value) {
			if (other.left) {
				left = new Node(*other.left);
			}
			if (other.right) {
				right = new Node(*other.right);
			}
		}
		Node(Node &&other) :
			left(other.left), right(other.right), depthDiff(other.depthDiff),
			key(std::move(other.key)), value(std::move(other.value))
		{
			other.left = nullptr;
			other.right = nullptr;
		}
		Node(const Key_t &newKey, const Value_t &newValue) :
			key(newKey), value(newValue) {}
		Node(const Key_t &newKey, Value_t &&newValue) :
			key(newKey), value(std::move(newValue)) {}
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
			depthDiff = other.depthDiff;
			return *this;
		}

		Node& operator=(Node &&other) {
			if (left) delete left;
			left = other.left;
			other.left = nullptr;
			if (right) delete right;
			right = other.right;
			other.right = nullptr;
			depthDiff = other.depthDiff;
			other.depthDiff = 0;
			return *this;
		}

		i32 Emplace(Node && node, Value_t **dstValue) {
			i32 depthDiffChange = 0;
			if (key == node.key) {
				*dstValue = &(value = std::move(node.value));
				return 0;
			} else if (node.key < key) {
				if (left) {
					depthDiffChange = left->Emplace(std::move(node), dstValue);
					depthDiffChange -= MaybeDoRotations(&left);
					depthDiff -= depthDiffChange;
				} else {
					left = new Node(std::move(node));
					depthDiffChange = i32(nullptr == right);
					depthDiff--;
					*dstValue = &left->value;
				}
			} else {
				if (right) {
					depthDiffChange = right->Emplace(std::move(node), dstValue);
					depthDiffChange -= MaybeDoRotations(&right);
					depthDiff += depthDiffChange;
				} else {
					right = new Node(std::move(node));
					depthDiffChange = i32(nullptr == left);
					depthDiff++;
					*dstValue = &right->value;
				}
			}
			return depthDiffChange;
		}

		// returns 1 if there was a change in depth (whether it did anything), or 0 otherwise
		static i32 MaybeDoRotations(Node **node) {
			if (nullptr == node) return 0;
			if ((*node)->depthDiff == 2) {
				if ((*node)->right->depthDiff == -1) {
					(*node)->right->RotateRight(&(*node)->right);
				}
				(*node)->RotateLeft(node);
				return 1;
			} else if ((*node)->depthDiff == -2) {
				if ((*node)->left->depthDiff == 1) {
					(*node)->left->RotateLeft(&(*node)->left);
				}
				(*node)->RotateRight(node);
				return 1;
			}
			return 0;
		}

		void RotateLeft(Node **parentNodePtr) {
			Node *newLeft, *newTop;
			newLeft = this;
			newTop = right;
			newLeft->right = right->left;
			newTop->left = newLeft;
			newLeft->depthDiff -= 1 + max(0, newTop->depthDiff);
			newTop->depthDiff -= 1 - min(0, newLeft->depthDiff);
			*parentNodePtr = newTop;
		}
		void RotateRight(Node **parentNodePtr) {
			Node *newRight, *newTop;
			newRight = this;
			newTop = left;
			newRight->left = left->right;
			newTop->right = newRight;
			newRight->depthDiff += 1 - min(0, newTop->depthDiff);
			newTop->depthDiff += 1 + max(0, newRight->depthDiff);
			*parentNodePtr = newTop;
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

		i32 ValueOf(const Key_t &testKey, Value_t **dstValue, const Value_t &valueDefault) {
			i32 depthDiffChange = 0;
			if (key == testKey) {
				*dstValue = &value;
				return 0;
			}
			if (testKey < key) {
				if (left) {
					depthDiffChange = left->ValueOf(testKey, dstValue, valueDefault);
					depthDiffChange -= MaybeDoRotations(&left);
					depthDiff -= depthDiffChange;
				} else {
					left = new Node(testKey, valueDefault);
					depthDiffChange = i32(nullptr == right);
					depthDiff--;
					*dstValue = &left->value;
				}
			} else {
				if (right) {
					depthDiffChange = right->ValueOf(testKey, dstValue, valueDefault);
					depthDiffChange -= MaybeDoRotations(&right);
					depthDiff += depthDiffChange;
				} else {
					right = new Node(testKey, valueDefault);
					depthDiffChange = i32(nullptr == left);
					depthDiff++;
					*dstValue = &right->value;
				}
			}
			return depthDiffChange;
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

	force_inline(Value_t&)
	Emplace(Key_t key, Value_t &&value) {
		return Emplace(Node(key, std::move(value)));
	}

	force_inline(Value_t&)
	Emplace(Key_t key, const Value_t &value) {
		return Emplace(Node(key, newValue));
	}

	Value_t& Emplace(Node &&node) {
		// The branch predictor should predict this correctly almost every time.
		if (base) {
			Value_t *result;
			base->Emplace(std::move(node), &result);
			Node::MaybeDoRotations(&base);
			return *result;
		} else {
			base = new Node(std::move(node));
			return base->value;
		}
	}

	bool Exists(const Key_t &key) const {
		if (!base) return false;
		return base->Exists(key);
	}

	Node* Find(const Key_t &key) {
		if (!base) return nullptr;
		return base->Find(key);
	}

	Value_t& ValueOf(const Key_t &key, const Value_t &valueDefault=Value_t()) {
		if (base) {
			Value_t *result;
			base->ValueOf(key, &result, valueDefault);
			Node::MaybeDoRotations(&base);
			return *result;
		} else {
			base = new Node(key, valueDefault);
			return base->value;
		}
	}

	force_inline(Value_t&)
	operator[](const Key_t &key) {
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
