/*
    File: memory.hpp
    Author: Philip Haynes
    Description: Provides shorthand aliasing for common memory types
        as well as implementing some custom memory primitives.
*/
#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "math.hpp"

#include <string>

using String = std::string;
using WString = std::wstring;

// Converts a UTF-8 string to Unicode string
WString ToWString(const char *string);
WString ToWString(String string);

#include <vector>

template<typename T>
using Array = std::vector<T>;

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

/*  struct: List
    Author: Philip Haynes
    Data structure useful for sparse chunks of data at a very wide range of indices.
    Good for mapping values from Unicode characters, for example.
    Negative indices are also valid, if that's your thing.     */
template<typename T>
struct List {
    List<T> *prev=nullptr, *next=nullptr;
    i32 first=0, last=0;
    T outOfBoundsValue=0;
    Array<T> indices{Array<T>(1)};
    List<T>() {}
    List<T>(const List<T>& other) {
        *this = other;
    }
    ~List<T>() {
        if (prev != nullptr) {
            prev->next = nullptr;
            delete prev;
        }
        if (next != nullptr) {
            next->prev = nullptr;
            delete next;
        }
    }
    const List<T>& operator=(const List<T>& other) {
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
        List<T> *it = other.prev;
        List<T> *me = this;
        while (it != nullptr) {
            List<T> *created = new List<T>;
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
            List<T> *created = new List<T>;
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
    // have to allocate another List or expand an Array
    void Set(const i32 index, T value) {
        if (index < first-1) {
            if (prev == nullptr) {
                prev = new List<T>;
                prev->next = this;
                prev->first = index;
                prev->last = index;
                prev->outOfBoundsValue = outOfBoundsValue;
            }
            if (index > prev->last+1) {
                List<T> *between = new List<T>;
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
                next = new List<T>;
                next->prev = this;
                next->first = index;
                next->last = index;
                next->outOfBoundsValue = outOfBoundsValue;
            }
            if (index < next->first-1) {
                List<T> *between = new List<T>;
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
    void Shift(const i32 amount) {
        last++;
        if (next != nullptr) {
            next->Shift(1);
        }
    }
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
        List<T> *it = prev;
        while (it != nullptr) {
            actualFirst = it->first;
            it = it->prev;
        }
        return actualFirst;
    }
    i32 Last() {
        i32 actualLast = last;
        List<T> *it = next;
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
