/*
	File: List.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_LIST_HPP
#define AZCORE_LIST_HPP

#include "../basictypes.hpp"
#include "Array.hpp"
#include <initializer_list>

namespace AzCore {

/*  struct: ListIndex
	Author: Philip Haynes
	A single index in a linked list     */
template <typename T>
struct ListIndex {
	ListIndex<T> *next;
	T value;
	ListIndex() : next(nullptr), value() {}
	ListIndex(const T &a) : next(nullptr), value(a) {}
	ListIndex(T &&a) : next(nullptr), value(std::move(a)) {}
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
	const ListIterator<T> &operator++() {
		me = me->next;
		return *this;
	}
	T &operator*() {
		return me->value;
	}
};

/*  class: ListIteratorConst
	Author: Philip Haynes
	Iterating over our Linked List with const-ness      */
template <typename T>
class ListIteratorConst {
	ListIndex<T> *me = nullptr;

public:
	ListIteratorConst() {}
	ListIteratorConst(ListIndex<T> *a) {
		me = a;
	}
	bool operator!=(const ListIteratorConst<T> &other) {
		return me != other.me;
	}
	const ListIteratorConst<T> &operator++() {
		me = me->next;
		return *this;
	}
	const T &operator*() {
		return me->value;
	}
};

/*  struct: List
	Author: Philip Haynes
	Just a linked list that can clean itself up.      */
template <typename T>
struct List {
	ListIndex<T> *first;
	i32 size;
	List() : first(nullptr), size(0) {}
	List(const List<T> &other) : first(nullptr), size(0) {
		*this = other;
	}
	List(List<T> &&other) noexcept : first(other.first), size(other.size) {
		other.first = nullptr;
		other.size = 0;
	}
	List(std::initializer_list<T> init) : first(nullptr), size(0) {
		*this = init;
	}
	List(const Array<T> &array) : first(nullptr), size(0) {
		*this = array;
	}
	~List() {
		ListIndex<T> *next = first;
		while (next != nullptr) {
			first = next->next;
			delete next;
			next = first;
		}
	}
	List<T> &operator=(const List<T> &other) {
		Resize(other.size);
		ListIndex<T> *it = other.first;
		ListIndex<T> *me = first;
		for (i32 i = 0; i < size; i++) {
			me->value = it->value;
			me = me->next;
			it = it->next;
		}
		return *this;
	}
	List<T> &operator=(List<T> &&other) {
		if (first != nullptr) {
			delete first;
		}
		first = other.first;
		other.first = nullptr;
		size = other.size;
		other.size = 0;
		return *this;
	}
	List<T> &operator=(const std::initializer_list<T> init) {
		Resize(init.size());
		ListIndex<T> *it = first;
		for (u32 i = 0; i < init.size(); i++) {
			it->value = *(init.begin() + i);
			it = it->next;
		}
		return *this;
	}
	List<T> &operator=(const Array<T> &array) {
		Resize(array.size);
		ListIndex<T> *it = first;
		for (i32 i = 0; i < array.size; i++) {
			it->value = array[i];
			it = it->next;
		}
		return *this;
	}
	T &operator[](i32 index) {
		ListIndex<T> *it = first;
		for (i32 i = 0; i < index; i++) {
			it = it->next;
		}
		return it->value;
	}
	const T &operator[](i32 index) const {
		const ListIndex<T> *it = first;
		for (i32 i = 0; i < index; i++) {
			it = it->next;
		}
		return it->value;
	}
	ListIterator<T> begin() {
		return ListIterator<T>(first);
	}
	ListIterator<T> end() {
		return ListIterator<T>();
	}
	ListIteratorConst<T> begin() const {
		return ListIteratorConst<T>(first);
	}
	ListIteratorConst<T> end() const {
		return ListIteratorConst<T>();
	}
	void Resize(i32 s) {
		if (s > size) {
			for (i32 i = size; i < s; i++) {
				Append(T());
			}
		}
		else if (s == 0) {
			ListIndex<T> *it = first;
			while (it != nullptr) {
				ListIndex<T> *next = it->next;
				delete it;
				it = next;
			}
			first = nullptr;
		}
		else if (size > s) {
			ListIndex<T> *it = first;
			for (i32 i = 0; i < s - 1; i++) {
				it = it->next;
			}
			ListIndex<T> *middle = it;
			it = it->next;
			for (i32 i = s; i < size; i++) {
				ListIndex<T> *n = it->next;
				delete it;
				it = n;
			}
			middle->next = nullptr;
		}
		size = s;
	}
	T &Append(const T &a) {
		size++;
		if (size == 1) {
			first = new ListIndex<T>(a);
			return first->value;
		}
		ListIndex<T> *it = first;
		for (i32 i = 2; i < size; i++) {
			it = it->next;
		}
		it->next = new ListIndex<T>(a);
		return it->next->value;
	}
	T &Append(T &&a) {
		size++;
		if (size == 1) {
			first = new ListIndex<T>(std::move(a));
			return first->value;
		}
		ListIndex<T> *it = first;
		for (i32 i = 2; i < size; i++) {
			it = it->next;
		}
		it->next = new ListIndex<T>(std::move(a));
		return it->next->value;
	}
	T &Insert(i32 index, const T &a) {
		size++;
		if (index == 0) {
			ListIndex<T> *f = first;
			first = new ListIndex<T>(a);
			first->next = f;
			return first->value;
		}
		ListIndex<T> *it = first;
		for (i32 i = 1; i < index; i++) {
			it = it->next;
		}
		ListIndex<T> *n = it->next;
		it->next = new ListIndex<T>(a);
		it->next->next = n;
		return it->next->value;
	}
	T &Insert(i32 index, T &&a) {
		size++;
		if (index == 0) {
			ListIndex<T> *f = first;
			first = new ListIndex<T>(std::move(a));
			first->next = f;
			return first->value;
		}
		ListIndex<T> *it = first;
		for (i32 i = 1; i < index; i++) {
			it = it->next;
		}
		ListIndex<T> *n = it->next;
		it->next = new ListIndex<T>(std::move(a));
		it->next->next = n;
		return it->next->value;
	}
	void Erase(i32 index) {
		size--;
		if (index == 0) {
			ListIndex<T> *f = first;
			first = first->next;
			delete f;
			return;
		}
		ListIndex<T> *it = first;
		for (i32 i = 1; i < index; i++) {
			it = it->next;
		}
		ListIndex<T> *n = it->next;
		it->next = it->next->next;
		delete n;
	}
	void Clear() {
		while (first != nullptr) {
			ListIndex<T> *tmp = first->next;
			delete first;
			first = tmp;
		}
	}

	bool Contains(const T &val) const {
		ListIndex<T> *it = first;
		for (i32 i = 0; i < size; i++) {
			if (val == it->value)
				return true;
			it = it->next;
		}
		return false;
	}

	i32 Count(const T &val) const {
		ListIndex<T> *it = first;
		i32 count = 0;
		for (i32 i = 0; i < size; i++) {
			if (val == it->value)
				count++;
			it = it->next;
		}
		return count;
	}
};

} // namespace AzCore

#endif // AZCORE_LIST_HPP
