/*
	File: ArrayList.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_ARRAYLIST_HPP
#define AZCORE_ARRAYLIST_HPP

#include "../basictypes.hpp"
#include "Array.hpp"

namespace AzCore {

/*  struct: ArrayList
	Author: Philip Haynes
	Data structure useful for sparse chunks of data at a very wide range of indices.
	Good for mapping values from Unicode characters, for example.
	Negative indices are also valid, if that's your thing.     */
template <typename T>
struct ArrayList {
	ArrayList<T> *prev = nullptr, *next = nullptr;
	i32 first = 0, last = 0;
	T outOfBoundsValue{};
	Array<T> indices{Array<T>(1)};

	ArrayList<T>() {}
	ArrayList<T>(const ArrayList<T> &other) {
		*this = other;
	}
	ArrayList<T>(const ArrayList<T> &&other) noexcept {
		prev = other.prev;
		next = other.next;
		first = other.first;
		last = other.last;
		outOfBoundsValue = std::move(other.outOfBoundsValue);
		indices = std::move(other.indices);
	}
	~ArrayList<T>() {
		if (prev != nullptr) {
			prev->next = nullptr;
			delete prev;
		}
		if (next != nullptr) {
			next->prev = nullptr;
			delete next;
		}
	}
	const ArrayList<T> &operator=(const ArrayList<T> &other) {
		if (this == &other) return *this;
		first = other.first;
		last = other.last;
		outOfBoundsValue = other.outOfBoundsValue;
		indices = other.indices;
		if (prev != nullptr) {
			prev->next = nullptr;
			delete prev;
		}
		if (next != nullptr) {
			next->prev = nullptr;
			delete next;
		}
		next = nullptr;
		prev = nullptr;
		ArrayList<T> *it = other.prev;
		ArrayList<T> *me = this;
		while (it != nullptr) {
			ArrayList<T> *created = new ArrayList<T>;
			created->first = it->first;
			created->last = it->last;
			created->outOfBoundsValue = it->outOfBoundsValue;
			created->indices = it->indices;
			created->next = me;
			me->prev = created;
			me = created;
			it = it->prev;
		}
		it = other.next;
		me = this;
		while (it != nullptr) {
			ArrayList<T> *created = new ArrayList<T>;
			created->first = it->first;
			created->last = it->last;
			created->outOfBoundsValue = it->outOfBoundsValue;
			created->indices = it->indices;
			created->prev = me;
			me->next = created;
			me = created;
			it = it->next;
		}
		return *this;
	}
	// This has to be const because changing values may require reallocating data
	// And I'd rather not reallocate data just for a read since reading out of bounds is valid
	const T &operator[](i32 index) const {
		if (index < first) {
			if (prev != nullptr) {
				if (index <= prev->last)
					return (*prev)[index];
				else
					return outOfBoundsValue;
			} else {
				return outOfBoundsValue;
			}
		}
		else if (index > last) {
			if (next != nullptr) {
				if (index >= next->first)
					return (*next)[index];
				else
					return outOfBoundsValue;
			} else {
				return outOfBoundsValue;
			}
		} else {
			return indices[index - first];
		}
	}
	bool Exists(i32 index) {
		if (index < first) {
			if (prev != nullptr) {
				if (index <= prev->last)
					return prev->Exists(index);
				else
					return false;
			} else {
				return false;
			}
		}
		else if (index > last) {
			if (next != nullptr) {
				if (index >= next->first)
					return next->Exists(index);
				else
					return false;
			} else {
				return false;
			}
		} else {
			return true;
		}
	}
	// Having this in a separate function is useful because it may
	// have to allocate another ArrayList or expand an Array
	void Set(i32 index, T value) {
		if (index < first - 1) {
			if (prev == nullptr) {
				prev = new ArrayList<T>;
				prev->next = this;
				prev->first = index;
				prev->last = index;
				prev->outOfBoundsValue = outOfBoundsValue;
			}
			if (index > prev->last + 1) {
				ArrayList<T> *between = new ArrayList<T>;
				between->next = this;
				between->prev = prev;
				between->first = index;
				between->last = index;
				between->outOfBoundsValue = outOfBoundsValue;
				prev->next = between;
				prev = between;
			}
			prev->Set(index, value);
		}
		else if (index > last + 1) {
			if (next == nullptr) {
				next = new ArrayList<T>;
				next->prev = this;
				next->first = index;
				next->last = index;
				next->outOfBoundsValue = outOfBoundsValue;
			}
			if (index < next->first - 1) {
				ArrayList<T> *between = new ArrayList<T>;
				between->prev = this;
				between->next = next;
				between->first = index;
				between->last = index;
				between->outOfBoundsValue = outOfBoundsValue;
				next->prev = between;
				next = between;
			}
			next->Set(index, value);
		} else {
			if (index == first - 1) {
				indices.Insert(0, value);
				first--;
			}
			else if (index == last + 1) {
				indices.Append(value);
				last++;
			} else {
				indices[index - first] = value;
			}
		}
	}
	void Append(Array<T> &values) {
		if (next != nullptr) {
			next->Append(values);
		} else {
			indices.Resize(indices.size + values.size);
			for (i32 i = 0; i < values.size; i++) {
				indices[i + last - first] = values[i];
			}
			last += values.size;
		}
	}
	void Append(T &value) {
		if (next != nullptr) {
			next->Append(value);
		} else {
			indices.Append(value);
			last++;
		}
	}
	void SetRange(i32 f, i32 l) {
		if (prev != nullptr) {
			prev->next = nullptr;
			delete prev;
		}
		if (next != nullptr) {
			next->prev = nullptr;
			delete next;
		}
		first = f;
		last = l;
		indices.Resize(last - first);
	}
	i32 First() {
		i32 actualFirst = first;
		ArrayList<T> *it = prev;
		while (it != nullptr) {
			actualFirst = it->first;
			it = it->prev;
		}
		return actualFirst;
	}
	i32 Last() {
		i32 actualLast = last;
		ArrayList<T> *it = next;
		while (it != nullptr) {
			actualLast = it->last;
			it = it->next;
		}
		return actualLast;
	}
	i32 Size() {
		return Last() - First();
	}
};

} // namespace AzCore

#endif // AZCORE_ARRAYLIST_HPP
