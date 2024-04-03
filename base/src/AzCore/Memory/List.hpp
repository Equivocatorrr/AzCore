/*
	File: List.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_LIST_HPP
#define AZCORE_LIST_HPP

#include "../basictypes.hpp"
#include "TemplateForwardDeclares.hpp"
#include <initializer_list>
#include <utility> // std::move

namespace AzCore {

/*  struct: ListIndex
	Author: Philip Haynes
	A single index in a linked list     */
template <typename T>
struct ListIndex {
	ListIndex *next;
	T value;
	ListIndex() : next(nullptr), value() {}
	ListIndex(const T &a) : next(nullptr), value(a) {}
	ListIndex(T &&a) : next(nullptr), value(std::move(a)) {}
	ListIndex(const ListIndex &other) = delete;
	ListIndex(ListIndex &&other) = delete;
	~ListIndex() {
		if (next) delete next;
	}
	// Remove all entries after this one
	void Cut() {
		if (next) delete next;
		next = nullptr;
	}
	// Places an index after this one.
	T& InsertNext(const T &in) {
		ListIndex *nextNext = next;
		next = new ListIndex(in);
		next->next = nextNext;
		return next->value;
	}
	// Places an index after this one.
	T& InsertNext(T &&in) {
		ListIndex *nextNext = next;
		next = new ListIndex(std::move(in));
		next->next = nextNext;
		return next->value;
	}
	// Erases the next index, keeping the nodes after that
	void EraseNext() {
		if (!next) return;
		ListIndex *nextNext = next->next;
		next->next = nullptr;
		delete next;
		next = nextNext;
	}
};

/*  class: ListIterator
	Author: Philip Haynes
	Iterating over our Linked List      */
template <typename T>
class ListIterator {
	ListIndex<T> *me = nullptr;

public:
	ListIterator() {}
	ListIterator(ListIndex<T> *a) {
		me = a;
	}
	bool operator!=(const ListIterator<T> &other) {
		return me != other.me;
	}
	ListIterator<T> &operator++() {
		me = me->next;
		return *this;
	}
	const ListIterator<T> operator+(i32 count) const {
		ListIterator<T> it = *this;
		while (count && it.me) {
			it.me = it.me->next;
			count--;
		}
		return it;
	}
	ListIndex<T>& operator*() {
		return *me;
	}
	const ListIndex<T>& operator*() const {
		return *me;
	}
};

/*  struct: List
	Author: Philip Haynes
	Just a linked list that can clean itself up.      */
template <typename T>
struct List {
	using Index = ListIndex<T>;
	Index *first;
	mutable Index *last;
	List() : first(nullptr), last(nullptr) {}
	List(const List<T> &other) : first(nullptr), last(nullptr) {
		*this = other;
	}
	List(List<T> &&other) noexcept : first(other.first), last(other.last) {
		other.first = nullptr;
		other.last = nullptr;
	}
	List(std::initializer_list<T> init) : first(nullptr), last(nullptr) {
		*this = init;
	}
	List(const Array<T,0> &array) : first(nullptr), last(nullptr) {
		*this = array;
	}
	~List() {
		if (first) delete first;
	}
	List<T>& operator=(const List<T> &other) {
		if (this == &other) return *this;
		if (other.first && !first) {
			first = new Index;
		} else if (first) {
			Clear();
			return *this;
		} else {
			// We're both empty
			return *this;
		}
		Index *it = other.first;
		Index *me = first;
		while (true) {
			me->value = it->value;
			if (!it->next) {
				me->Cut();
				last = me;
				break;
			}
			if (!me->next) {
				me->next = new Index;
			}
			me = me->next;
			it = it->next;
		}
		return *this;
	}
	List<T>& operator=(List<T> &&other) {
		if (this == &other) return *this;
		if (first != nullptr) {
			delete first;
		}
		first = other.first;
		other.first = nullptr;
		last = other.last;
		other.last = nullptr;
		return *this;
	}
	List<T>& operator=(const std::initializer_list<T> init) {
		if (init.size() == 0) {
			Clear();
			return *this;
		}
		if (!first) {
			first = new Index;
		}
		ListIndex<T> *me = first;
		auto iterator = init.begin();
		auto end = init.end();
		while (true) {
			const T& value = *iterator;
			me->value = value;
			if (++iterator == end) {
				me->Cut();
				last = me;
				break;
			}
			if (nullptr == me->next) {
				me->next = new Index;
			}
			me = me->next;
		}
		return *this;
	}
	List<T>& operator=(const Array<T,0> &array) {
		if (array.size == 0) {
			Clear();
			return *this;
		}
		if (!first) {
			first = new Index;
		}
		ListIndex<T> *me = first;
		auto iterator = array.begin();
		auto end = array.end();
		while (true) {
			const T& value = *iterator;
			me->value = value;
			if (++iterator == end) {
				me->Cut();
				last = me;
				break;
			}
			if (nullptr == me->next) {
				me->next = new Index;
			}
			me = me->next;
		}
		return *this;
	}
	ListIterator<T> begin() {
		return ListIterator<T>(first);
	}
	ListIterator<T> end() {
		return ListIterator<T>();
	}
	ListIterator<const T> begin() const {
		return ListIterator<const T>(first);
	}
	ListIterator<const T> end() const {
		return ListIterator<const T>();
	}
	inline void _UpdateLast() {
		if (!last) return;
		while (last->next) {
			// Since entries can be inserted by Index::Insert, which can be at the end of the list
			// So last isn't necessarily always correct
			last = last->next;
		}
	}
	T& Prepend(const T &a) {
		Index *next = first;
		first = new Index(a);
		first->next = next;
		return first->value;
	}
	T& Prepend(T &&a) {
		Index *next = first;
		first = new Index(std::move(a));
		first->next = next;
		return first->value;
	}
	T& Append(const T &a) {
		if (!first) {
			first = new Index(a);
			last = first;
			return last->value;
		}
		_UpdateLast();
		last->next = new Index(a);
		last = last->next;
		return last->value;
	}
	T& Append(T &&a) {
		if (!first) {
			first = new Index(std::move(a));
			last = first;
			return last->value;
		}
		_UpdateLast();
		last->next = new Index(std::move(a));
		last = last->next;
		return last->value;
	}
	void Clear() {
		if (first) delete first;
		first = nullptr;
		last = nullptr;
	}
	void EraseFirst() {
		if (!first) return;
		Index *leftover = first;
		first = first->next;
		leftover->next = nullptr;
		delete leftover;
	}
	void EraseLast() {
		_UpdateLast();
		if (!last) return;
		Index *oldLast = last;
		delete last;
		if (last != first) {
			last = first;
		} else {
			first = nullptr;
			last = nullptr;
		}
		while (last->next != oldLast) {
			last = last->next;
		}
		last->next = nullptr;
	}

	T& Back() {
		_UpdateLast();
		return last->value;
	}
	const T& Back() const {
		_UpdateLast();
		return last->value;
	}

	// Whether we're empty or not
	bool Empty() const {
		return nullptr == first;
	}

	// Iterates the entire List to find the size
	i32 size() const {
		i32 result = 0;
		Index *it = first;
		while (it) {
			result++;
			it = it->next;
		}
		return result;
	}

	bool Contains(const T &val) const {
		Index *it = first;
		while (it) {
			if (val == it->value)
				return true;
			it = it->next;
		}
		return false;
	}

	i32 Count(const T &val) const {
		Index *it = first;
		i32 count = 0;
		while (it) {
			if (val == it->value)
				count++;
			it = it->next;
		}
		return count;
	}

	i32 IndexOf(Index *index) {
		Index *it = first;
		i32 i = 0;
		while (it) {
			if (it == index) return i;
			it = it->next;
			++i;
		}
		return -1;
	}
};

} // namespace AzCore

#endif // AZCORE_LIST_HPP
