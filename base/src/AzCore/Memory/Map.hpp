/*
    File: Map.hpp
    Author: Philip Haynes
    A map implemented as a binary tree.
    Requires operator< to be defined for T.
*/

#ifndef AZCORE_MAP_HPP
#define AZCORE_MAP_HPP

#include "../basictypes.hpp"
#include <utility>
#include <initializer_list>

namespace AzCore {

template <typename Key_t, typename Value_t>
struct Map {
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

        void Emplace(Node && node) {
            if (key == node.key) {
                value = std::move(node.value);
            } else if (node.key < key) {
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

        Value_t& ValueOf(Key_t testKey) {
            if (testKey == key) return value;
            if (testKey < key) {
                if (left) {
                    return left->ValueOf(testKey);
                } else {
                    left = new Node();
                    left->key = testKey;
                    return left->value;
                }
            } else {
                if (right) {
                    return right->ValueOf(testKey);
                } else {
                    right = new Node();
                    right->key = testKey;
                    return right->value;
                }
            }
        }
    };
    Node *base = nullptr;

    Map() = default;
    // TODO: Maybe allow this?
    Map(const Map &other) = delete;
    Map(Map &&other) : base(other.base) {other.base = nullptr;};
    Map(const std::initializer_list<Node> &init) {
        // TODO: Maybe sort and recursively bisect before emplacing?
        for (const Node &node : init) {
            Node newNode = node;
            Emplace(std::move(newNode));
        }
    }
    ~Map() {
        if (base) delete base;
    }

    inline force_inline
    void Emplace(Key_t key, Value_t value) {
        Emplace(Node(key, value));
    }

    void Emplace(Node &&node) {
        // The branch predictor should predict this correctly almost every time.
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

    Value_t& ValueOf(Key_t key) {
        if (base) {
            return base->ValueOf(key);
        } else {
            base = new Node();
            base->key = key;
            return base->value;
        }
    }

    inline force_inline
    Value_t& operator[](Key_t key) {
        return ValueOf(key);
    }
};

} // namespace AzCore

#endif // AZCORE_MAP_HPP
