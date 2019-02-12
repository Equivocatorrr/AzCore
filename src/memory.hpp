/*
    File: memory.hpp
    Author: Philip Haynes
    Description: Provides shorthand aliasing for common memory types
        as well as implementing some custom memory primitives.
*/
#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "math.hpp"

#include <initializer_list>

#include <string>

using String = std::string;
using WString = std::wstring;

inline bool operator==(const String& a, const String& b) {
    return a.compare(b) == 0;
}

inline bool operator==(const String& a, const char *b) {
    return a.compare(b) == 0;
}

inline bool operator==(const char *b, const String& a) {
    return a.compare(b) == 0;
}

bool equals(const char *a, const char *b);

// Converts a UTF-8 string to Unicode string
WString ToWString(const char *string);
WString ToWString(String string);

#include <vector>

template<typename T>
using Array = std::vector<T>;

/*  class: ArrayPtr
    Author: Philip Haynes
    Uses an index of an Array rather than an actual pointer.
    Note that this can only be used with nested Arrays given that the array
    it points to isn't moved. This means that the Arrays must be allocated
    in ascending order, starting with the base array.     */
template<typename T>
class ArrayPtr {
    Array<T> *array = nullptr;
    u32 index = 0;
public:
    ArrayPtr() {}
    ArrayPtr(Array<T>& a, u32 i) {
        array = &a;
        index = i;
    }
    void SetPtr(Array<T>& a, u32 i) {
        array = &a;
        index = i;
    }
    T& operator*() {
        return (*array)[index];
    }
    T* operator->() {
        return &(*array)[index];
    }
};

#include <map>

template<typename T, typename B>
using Map = std::map<T, B>;

#include <mutex>
// #ifdef __MINGW32__
// #include <mingw/mutex.h>
// #endif

using Mutex = std::mutex;

#include <memory>

template<typename T, typename Deleter=std::default_delete<T>>
using UniquePtr = std::unique_ptr<T, Deleter>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

/*  class: List
    Author: Philip Haynes
    Just a linked list that can clean itself up.      */
template<typename T>
class List {
    List<T> *next=nullptr;
    u32 _size=0;
    T value{};
public:
    List() {}
    List(const T& a) {
        value = a;
    }
    List(const List<T>& other) {
        *this = other;
    }
    List(std::initializer_list<T> init) {
        *this = init;
    }
    List(const Array<T>& array) {
        *this = array;
    }
    ~List() {
        if (next != nullptr) {
            delete next;
        }
    }
    List<T>& operator=(const List<T>& other) {
        _size = 0;
        if (next != nullptr) {
            delete next;
        }
        next = nullptr;
        if (other._size != 0)
            push_back(other.value);
        List<T> *it = other.next;
        while (it != nullptr) {
            push_back(it->value);
            it = it->next;
        }
        return *this;
    }
    List<T>& operator=(const std::initializer_list<T> init) {
        if (next != nullptr) {
            delete next;
        }
        next = nullptr;
        u32 prevSize = init.size()+1;
        List<T> *it = this;
        for (u32 i = 0; i < init.size(); i++) {
            it->_size = --prevSize;
            it->value = *(init.begin()+i);
            if (i < init.size()-1) {
                it->next = new List<T>();
                it = it->next;
            }
        }
        return *this;
    }
    List<T>& operator=(const Array<T>& array) {
        if (next != nullptr) {
            delete next;
        }
        next = nullptr;
        u32 prevSize = array.size()+1;
        List<T> *it = this;
        for (u32 i = 0; i < array.size(); i++) {
            it->_size = --prevSize;
            it->value = array[i];
            if (i < array.size()-1) {
                it->next = new List<T>();
                it = it->next;
            }
        }
        return *this;
    }
    T& operator[](const u32 index) {
        List<T> *it = this;
        for (u32 i = 0; i < index; i++) {
            it = it->next;
        }
        return it->value;
    }
    const T& operator[](const u32 index) const {
        const List<T> *it = this;
        for (u32 i = 0; i < index; i++) {
            it = it->next;
        }
        return it->value;
    }
    void resize(u32 s) {
        if (s > _size) {
            for (u32 i = 0; i < s-_size; i++) {
                push_back(T());
            }
        } else if (_size > s) {
            List<T> *it = this;
            for (u32 i = 0; i < s-1; i++) {
                it->_size = s-i;
                it = it->next;
            }
            delete it->next;
            it->next = nullptr;
        }
        _size = s;
    }
    void push_back(const T& a) {
        if (_size == 0) {
            _size++;
            value = a;
            return;
        }
        List<T> *it = this;
        while (it->next != nullptr) {
            it->_size++;
            it = it->next;
        }
        it->_size++;
        it->next = new List<T>(a);
    }
    void insert(const u32 index, const T& a) {
        if (index == 0) {
            _size++;
            List<T> *n = next;
            next = new List<T>(value);
            next->next = n;
            next->_size = _size-1;
            value = a;
            return;
        }
        List<T> *it = this;
        for (u32 i = 1; i < index; i++) {
            it->_size++;
            if (it->next == nullptr) {
                it->next = new List<T>();
            }
            it = it->next;
        }
        List<T> *n = it->next;
        it->next = new List<T>(a);
        it->next->next = n;
        it->next->_size = it->_size-1;
    }
    void erase(const u32 index) {
        if (index == 0) {
            _size--;
            List<T> *n = next;
            value = next->value;
            next = next->next;
            n->next = nullptr;
            delete n;
            return;
        }
        List<T> *it = this;
        for (u32 i = 1; i < index; i++) {
            it->_size--;
            it = it->next;
        }
        List<T> *n = it->next;
        it->next = it->next->next;
        n->next = nullptr;
        delete n;
    }
    const u32& size() const {
        return _size;
    }
};

/*  class: ListPtr
    Author: Philip Haynes
    Uses an index of a List rather than an actual pointer.
    Using a List like this comes with the benefit that newly allocated memory
    doesn't move previously allocated memory, meaning nested Lists are valid
    even with sporadic allocations.
    Note that this guarantee is invalid if the List is itself moved.     */
template<typename T>
class ListPtr {
    List<T> *list = nullptr;
    u32 index = 0;
public:
    ListPtr() {}
    ListPtr(List<T>& a, u32 i) {
        list = &a;
        index = i;
    }
    void SetPtr(List<T>& a, u32 i) {
        list = &a;
        index = i;
    }
    T& operator*() {
        return (*list)[index];
    }
    T* operator->() {
        return &(*list)[index];
    }
};

/*  struct: ArrayList
    Author: Philip Haynes
    Data structure useful for sparse chunks of data at a very wide range of indices.
    Good for mapping values from Unicode characters, for example.
    Negative indices are also valid, if that's your thing.     */
template<typename T>
struct ArrayList {
    ArrayList<T> *prev=nullptr, *next=nullptr;
    i32 first=0, last=0;
    T outOfBoundsValue{};
    Array<T> indices{Array<T>(1)};
    ArrayList<T>() {}
    ArrayList<T>(const ArrayList<T>& other) {
        *this = other;
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
    const ArrayList<T>& operator=(const ArrayList<T>& other) {
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
    const T& operator[](const i32 index) const {
        if (index < first) {
            if (prev != nullptr) {
                if (index <= prev->last)
                    return (*prev)[index];
                else
                    return outOfBoundsValue;
            } else {
                return outOfBoundsValue;
            }
        } else if (index > last) {
            if (next != nullptr) {
                if (index >= next->first)
                    return (*next)[index];
                else
                    return outOfBoundsValue;
            } else {
                return outOfBoundsValue;
            }
        } else {
            return indices[index-first];
        }
    }
    bool Exists(const i32 index) {
        if (index < first) {
            if (prev != nullptr) {
                if (index <= prev->last)
                    return prev->Exists(index);
                else
                    return false;
            } else {
                return false;
            }
        } else if (index > last) {
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
    void Set(const i32 index, T value) {
        if (index < first-1) {
            if (prev == nullptr) {
                prev = new ArrayList<T>;
                prev->next = this;
                prev->first = index;
                prev->last = index;
                prev->outOfBoundsValue = outOfBoundsValue;
            }
            if (index > prev->last+1) {
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
        } else if (index > last+1) {
            if (next == nullptr) {
                next = new ArrayList<T>;
                next->prev = this;
                next->first = index;
                next->last = index;
                next->outOfBoundsValue = outOfBoundsValue;
            }
            if (index < next->first-1) {
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
            if (index == first-1) {
                indices.insert(indices.begin(), value);
                first--;
            } else if (index == last+1) {
                indices.push_back(value);
                last++;
            } else {
                indices[index-first] = value;
            }
        }
    }
    // TODO: Can't say I remember what I was smoking when I implemented this...
    // void Shift(const i32 amount) {
    //     last++;
    //     if (next != nullptr) {
    //         next->Shift(1);
    //     }
    // }
    void Append(Array<T> &values) {
        if (next != nullptr) {
            next->Append(values);
        } else {
            indices.resize(indices.size()+values.size());
            for (i32 i = 0; i < values.size(); i++) {
                indices[i+last-first] = values[i];
            }
            last += values.size();
        }
    }
    void Append(T &value) {
        if (next != nullptr) {
            next->Append(value);
        } else {
            indices.push_back(value);
            last++;
        }
    }
    void SetRange(const i32 f, const i32 l) {
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
        indices.resize(last-first);
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
        return Last()-First();
    }
};

#endif
