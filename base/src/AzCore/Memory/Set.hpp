/*
    File: Set.hpp
    Author: Philip Haynes
    A set implemented as a binary tree.
    Requires operator< to be defined for T.
*/

#ifndef AZCORE_SET_HPP
#define AZCORE_SET_HPP

#include "../basictypes.hpp"
#include "Array.hpp"
#include <utility>
#include <initializer_list>

namespace AzCore {

template <typename Key_t>
struct SetIterator;

template <typename Key_t>
struct Set {
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

    Set() = default;
    // TODO: Maybe allow this?
    Set(const Set &other) = delete;
    Set(Set &&other) : base(other.base) {other.base = nullptr;};
    Set(const std::initializer_list<Node> &init) {
        // TODO: Maybe sort and recursively bisect before emplacing?
        for (const Node &node : init) {
            Node newNode = node;
            Emplace(std::move(newNode));
        }
    }
    ~Set() {
        if (base) delete base;
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

    SetIterator<Key_t> begin() {
        return SetIterator<Key_t>(base);
    }
    SetIterator<Key_t> end() {
        return SetIterator<Key_t>(nullptr);
    }
    SetIterator<const Key_t> begin() const {
        return SetIterator<const Key_t>(base);
    }
    SetIterator<const Key_t> end() const {
        return SetIterator<const Key_t>(nullptr);
    }
};

template <typename Key_t>
class SetIterator {
    Array<typename Set<Key_t>::Node*> stack;

public:
    SetIterator() : stack({nullptr}) {}
    SetIterator(typename Set<Key_t>::Node *startNode) : stack({startNode}) {
        while (stack.Back()->left) {
            stack.Append(stack.Back()->left);
        }
    }
    bool operator!=(const SetIterator &other) const {
        return stack.Back() != other.stack.Back();
    }
    void operator++() {
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
