/*
	File: HashSet.hpp
	Author: Philip Haynes
	A map implemented as a hash table.
	Requires u8 HashSetHash(T) to be defined for T.
*/

#ifndef AZCORE_HASH_SET_HPP
#define AZCORE_HASH_SET_HPP

#include "../basictypes.hpp"
#include "Array.hpp"
#include "IndexHash.hpp"
#include <utility>
#include <initializer_list>
#include <stdexcept>

namespace AzCore {

template <typename Node_t, u16 arraySize>
class HashSetIterator;

template <typename Key_t, u16 arraySize=256>
struct HashSet {
	struct Node {
		typedef Key_t KeyType;
		Key_t key;
		Node *next = nullptr;

		Node() = default;
		Node(const Node &other) : key(other.key) {
			if (other.next) {
				next = new Node(*other.next);
			}
		}
		Node(Node &&other) :
			key(std::move(other.key)),
			next(other.next)
		{
			other.next = nullptr;
		}
		Node(Key_t newKey) :
			key(newKey) {}
		~Node() {
			if (next) delete next;
		}

		Node& operator=(const Node &other) {
			key = other.key;
			if (next) {
				if (other.next) {
					*next = *other.next;
				} else {
					delete next;
					next = nullptr;
				}
			} else {
				if (other.next) {
					next = new Node(*other.next);
				}
			}
			return *this;
		}

		Node& operator=(Node &&other) {
			if (next) delete next;
			key = std::move(other.key);
			next = other.next;
			other.next = nullptr;
			return *this;
		}

		// Returns whether the key already exists.
		bool Emplace(Node && node) {
			if (key != node.key) {
				if (next) {
					return next->Emplace(std::move(node));
				} else {
					next = new Node(std::move(node));
					return false;
				}
			}
			return true;
		}

		bool Exists(const Key_t& testKey) const {
			if (key == testKey) return true;
			if (next) {
				return next->Exists(testKey);
			} else {
				return false;
			}
		}

		Node* Find(const Key_t& testKey) {
			if (key == testKey) return this;
			if (next) {
				return next->Find(testKey);
			} else {
				return nullptr;
			}
		}
	};
	Array<Node*> nodes;

	force_inline() HashSet() : nodes(arraySize, nullptr) {}

	force_inline() HashSet(const HashSet& other) : nodes(arraySize) {
		for (i32 i = 0; i < arraySize; i++) {
			if (other.nodes[i]) {
				nodes[i] = new Node(*other.nodes[i]);
			} else {
				nodes[i] = nullptr;
			}
		}
	}
	force_inline() HashSet(HashSet&& other) : nodes(std::move(other.nodes)) {
		other.nodes.Resize(arraySize, nullptr);
	}

	HashSet(const std::initializer_list<Node> &init) :
	nodes(arraySize, nullptr) {
		for (const Node &node : init) {
			Emplace(node.key);
		}
	}

	~HashSet() {
		Clear();
	}

	void Clear() {
		for (i32 i = 0; i < nodes.size; i++) {
			if (nodes[i]) {
				delete nodes[i];
				nodes[i] = nullptr;
			}
		}
	}

	HashSet& operator=(const HashSet &other) {
		if (this == &other) return *this;
		Clear();
		for (i32 i = 0; i < arraySize; i++) {
			if (other.nodes[i]) {
				nodes[i] = new Node(*other.nodes[i]);
			} else {
				nodes[i] = nullptr;
			}
		}
		return *this;
	}
	HashSet& operator=(HashSet &&other) {
		if (this == &other) return *this;
		Clear();
		nodes = std::move(other.nodes);
		other.nodes.Resize(arraySize, nullptr);
		return *this;
	}

	force_inline(bool)
	Emplace(Node &&node) {
		return Emplace(node.key);
	}

	// Returns whether the key already exists.
	bool Emplace(Key_t key) {
		i32 index = IndexHash<arraySize>(key);
		if (nodes[index]) {
			return nodes[index]->Emplace(Node(key));
		} else {
			nodes[index] = new Node(key);
			return false;
		}
	}

	bool Exists(Key_t key) const {
		i32 index = IndexHash<arraySize>(key);
		if (nodes[index] == nullptr) return false;
		return nodes[index]->Exists(key);
	}

	Node* Find(Key_t key) {
		i32 index = IndexHash(key);
		if (nodes[index] == nullptr) return nullptr;
		return nodes[index]->Find(key);
	}

	HashSetIterator<Node, arraySize> begin() {
		i32 firstIndex = 0;
		while (nodes[firstIndex] == nullptr && firstIndex < arraySize) {
			firstIndex++;
		}
		if (firstIndex == arraySize) {
			return HashSetIterator<Node, arraySize>(&nodes, firstIndex, nullptr);
		} else {
			return HashSetIterator<Node, arraySize>(&nodes, firstIndex, nodes[firstIndex]);
		}
	}
	HashSetIterator<Node, arraySize> end() {
		return HashSetIterator<Node, arraySize>(nullptr, arraySize, nullptr);
	}
	HashSetIterator<const Node, arraySize> begin() const {
		i32 firstIndex = 0;
		while (nodes[firstIndex] == nullptr && firstIndex < arraySize) {
			firstIndex++;
		}
		if (firstIndex == arraySize) {
			return HashSetIterator<const Node, arraySize>((const Array<const Node*>*)&nodes, firstIndex, nullptr);
		} else {
			return HashSetIterator<const Node, arraySize>((const Array<const Node*>*)&nodes, firstIndex, nodes[firstIndex]);
		}
	}
	HashSetIterator<const Node, arraySize> end() const {
		return HashSetIterator<const Node, arraySize>(nullptr, arraySize, nullptr);
	}
};

template <typename Node_t, u16 arraySize>
class HashSetIterator {
	const Array<Node_t*> *nodes;
	i32 index;
	Node_t *node;

public:
	HashSetIterator(const Array<Node_t*> *nodesArray, i32 i, Node_t *n) :
	nodes(nodesArray), index(i), node(n) {}
	void Rebase(const Array<Node_t*> *nodesArray) {
		nodes = nodesArray;
	}
	force_inline(bool)
	operator!=(const HashSetIterator &other) const {
		return node != other.node;
	}
	inline void
	operator++() {
		node = node->next;
		while (node == nullptr) {
			index++;
			if (index < arraySize) {
				node = (*nodes)[index];
			} else {
				break;
			}
		}
	}
	force_inline(typename Node_t::KeyType&)
	operator*() {
		return node->key;
	}
};

} // namespace AzCore

#endif // AZCORE_HASH_MAP_HPP
