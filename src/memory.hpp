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
#include <type_traits>
#include <cstring>

#include <stdexcept>

/*  class: ArrayIterator
    Author: Philip Haynes
    Because const correctness can't work without it...      */
template<typename T>
class ArrayIterator {
    T* data;
public:
    ArrayIterator() : data(nullptr) {}
    ArrayIterator(T* d) : data(d) {}
    bool operator!=(const ArrayIterator<T> &other) const {
        return data != other.data;
    }
    const ArrayIterator<T>& operator++() {
        data++;
        return *this;
    }
    const T& operator*() const {
        return *data;
    }
};

/*  struct: Array
    Author: Philip Haynes
    A replacement for std::vector which is hopefully less obnoxious to debug.   */
template<typename T, i32 allocTail=0> // allocTail will allocate extra space at the end
struct Array {
    T *data = nullptr;
    i32 allocated = 0;
    i32 size = 0;

    Array() : data(nullptr) , allocated(0) , size(0) {
        if (allocTail != 0) {
            data = new T[allocTail];
            for (i32 i = size; i < allocated+allocTail; i++) {
                data[i] = T();
            }
        }
    }
    Array(const i32 newSize, const T& value=T()) : data(new T[newSize + allocTail]) , allocated(newSize) , size(newSize) {
        for (i32 i = 0; i < size; i++) {
            data[i] = value;
        }
        for (i32 i = size; i < allocated+allocTail; i++) {
            data[i] = T();
        }
    }
    Array(const Array<T, allocTail>& other) : allocated(other.size) , size(other.size) {
        data = new T[allocated + allocTail];
        if (std::is_trivially_copyable<T>::value) {
            memcpy((void*)data, (void*)other.data, sizeof(T) * allocated);
        } else {
            for (i32 i = 0; i < size; i++) {
                data[i] = other.data[i];
            }
        }
        for (i32 i = size; i < allocated+allocTail; i++) {
            data[i] = T();
        }
    }
    Array(const std::initializer_list<T>& init) : allocated(init.size()) , size(allocated) {
        if (size != 0 || allocTail != 0) {
            data = new T[allocated + allocTail];
            i32 i = 0;
            for (const T& val : init) {
                data[i++] = val;
            }
            for (i32 i = size; i < allocated+allocTail; i++) {
                data[i] = T();
            }
        } else {
            data = nullptr;
        }
    }
    Array(const Array<T, allocTail>&& other) noexcept : data(other.data) , allocated(other.allocated) , size(other.size) {}

    ~Array() {
        if (data != nullptr) {
            delete[] data;
        }
        data = nullptr;
    }

    Array<T, allocTail>& operator=(const Array<T, allocTail>& other) {
        size = other.size;
        if (size == 0 && allocTail == 0) {
            return *this;
        }
        if (allocated >= size) {
            if (std::is_trivially_copyable<T>::value) {
                memcpy((void*)data, (void*)other.data, sizeof(T) * size);
            } else {
                for (i32 i = 0; i < size; i++) {
                    data[i] = other.data[i];
                }
            }
            for (i32 i = size; i < allocated+allocTail; i++) {
                data[i] = T();
            }
            return *this;
        }
        // We definitely have to allocate
        allocated = size;
        if (data != nullptr) {
            delete[] data;
        }
        data = new T[allocated + allocTail];
        if (std::is_trivially_copyable<T>::value) {
            memcpy((void*)data, (void*)other.data, sizeof(T) * allocated);
        } else {
            for (i32 i = 0; i < size; i++) {
                data[i] = other.data[i];
            }
        }
        for (i32 i = size; i < allocated+allocTail; i++) {
            data[i] = T();
        }
        return *this;
    }

    Array<T, allocTail>& operator=(const Array<T, allocTail>&& other) {
        if (data != nullptr) {
            delete[] data;
        }
        data = other.data;
        size = other.size;
        allocated = other.allocated;
        return *this;
    }

    Array<T, allocTail>& operator=(const std::initializer_list<T>& init) {
        size = init.size();
        if (size == 0 && allocTail == 0) {
            return *this;
        }
        if (allocated >= size) {
            i32 i = 0;
            for (const T& val : init) {
                data[i++] = val;
            }
        }
        // We definitely have to allocate
        allocated = size;
        if (data != nullptr) {
            delete[] data;
        }
        data = new T[allocated + allocTail];
        i32 i = 0;
        for (const T& val : init) {
            data[i++] = val;
        }
        for (i32 i = size; i < allocated+allocTail; i++) {
            data[i] = T();
        }
        return *this;
    }

    bool operator==(const Array<T, allocTail>& other) const {
        if (size != other.size) {
            return false;
        }
        for (i32 i = 0; i < size; i++) {
            if (data[i] != other.data[i]) {
                return false;
            }
        }
        return true;
    }

    const T& operator[](const i32 index) const {
        if (index > size) {
            throw std::domain_error("Array index is out of bounds");
        }
        return data[index];
    }

    T& operator[](const i32 index) {
        if (index > size) {
            throw std::domain_error("Array index is out of bounds");
        }
        return data[index];
    }

    inline T& operator+=(const T& value) {
        return Append(value);
    }

    Array<T, allocTail> operator+(const T& other) const {
        Array<T, allocTail> result(*this);
        result += other;
        return result;
    }

    Array<T, allocTail> operator+(const Array<T, allocTail>& other) const {
        Array<T, allocTail> result(*this);
        result += other;
        return result;
    }

    inline Array<T, allocTail> operator+=(const Array<T, allocTail>& other) {
        return Append(other);
    }

    void Reserve(const i32 newSize) {
        if (newSize <= allocated) {
            return;
        }
        allocated = newSize;
        if (size > 0) {
            T *temp = new T[newSize + allocTail];
            if (std::is_trivially_copyable<T>::value) {
                memcpy((void*)temp, (void*)data, sizeof(T) * size);
            } else {
                for (i32 i = 0; i < size; i++) {
                    temp[i] = data[i];
                }
            }
            delete[] data;
            data = temp;
            for (i32 i = size; i < allocated+allocTail; i++) {
                data[i] = T();
            }
            return;
        }
        if (data != nullptr) {
            delete[] data;
        }
        data = new T[newSize + allocTail];
        for (i32 i = size; i < allocated+allocTail; i++) {
            data[i] = T();
        }
    }

    void Resize(const i32 newSize, const T& value=T()) {
        if (newSize > allocated) {
            Reserve(max(newSize, (allocated >> 1) + 2));
        } else if (newSize < (allocated>>1)) {
            size = newSize;
            allocated = size;
            T *temp = new T[newSize + allocTail];
            if (std::is_trivially_copyable<T>::value) {
                memcpy((void*)temp, (void*)data, sizeof(T) * size);
            } else {
                for (i32 i = 0; i < size; i++) {
                    temp[i] = data[i];
                }
            }
            delete[] data;
            data = temp;
            for (i32 i = size; i < allocated+allocTail; i++) {
                data[i] = T();
            }
            return;
        } else if (newSize == 0 && allocTail == 0) {
            delete[] data;
            allocated = 0;
            size = 0;
            return;
        }
        for (i32 i = size; i < newSize; i++) {
            data[i] = value;
        }
        size = newSize;
    }

    T& Append(const T& value) {
        if (size >= allocated) {
            allocated += (allocated >> 1) + 2;
            T *temp = new T[allocated + allocTail];
            if (size > 0) {
                if (std::is_trivially_copyable<T>::value) {
                    memcpy((void*)temp, (void*)data, sizeof(T) * size);
                } else {
                    for (i32 i = 0; i < size; i++) {
                        temp[i] = data[i];
                    }
                }
            }
            if (data != nullptr) {
                delete[] data;
            }
            data = temp;
            for (i32 i = size; i < allocated+allocTail; i++) {
                data[i] = T();
            }
        }
        return data[size++] = value;
    }

    Array<T, allocTail>& Append(const Array<T, allocTail>& other) {
        i32 copyStart = size;
        Resize(size+other.size);
        if (std::is_trivially_copyable<T>::value) {
            memcpy((void*)(data+copyStart), (void*)other.data, sizeof(T) * other.size);
        } else {
            for (i32 i = copyStart; i < size; i++) {
                data[i] = other.data[i-copyStart];
            }
        }
        for (i32 i = size; i < allocated+allocTail; i++) {
            data[i] = T();
        }
        return *this;
    }

    T& Insert(const i32 index, const T& value) {
        if (index > size) {
            throw std::domain_error("Array::Insert index is out of bounds");
        }
        if (size >= allocated) {
            allocated += (allocated >> 1) + 2;
            T *temp = new T[allocated + allocTail];
            if (std::is_trivially_copyable<T>::value) {
                memcpy((void*)temp, (void*)data, sizeof(T) * index);
                temp[index] = value;
                memcpy((void*)(temp+index+1), (void*)(data+index), sizeof(T) * (size-index));
            } else {
                for (i32 i = 0; i < index; i++) {
                    temp[i] = data[i];
                }
                temp[index] = value;
                for (i32 i = index+1; i < size+1; i++) {
                    temp[i] = data[i-1];
                }
            }
            delete[] data;
            data = temp;
            for (i32 i = 0; i < allocTail; i++) {
                data[allocated+i] = T();
            }
            size++;
            return data[index];
        }
        for (i32 i = size; i > index; i--) {
            data[i] = data[i-1];
        }
        size++;
        return data[index] = value;
    }

    void Erase(const i32 index) {
        if (index >= size) {
            throw std::domain_error("Array index is out of bounds");
        }
        size--;
        if (std::is_trivially_copyable<T>::value) {
            if (size > index) {
                memcpy((void*)(data+index), (void*)(data+index+1), sizeof(T) * (size-index));
            }
        } else {
            for (i32 i = index; i < size-1; i++) {
                data[i] = data[i+1];
            }
        }
    }

    Array<T, allocTail>& Reverse() {
        for (i32 i = 0; i < size/2; i++) {
            T buf = data[i];
            data[i] = data[size-i-1];
            data[size-i-1] = buf;
        }
        return *this;
    }

    ArrayIterator<T> begin() const {
        return ArrayIterator<T>(data);
    }
    ArrayIterator<T> end() const {
        return ArrayIterator<T>(data + size);
    }
    T* begin() {
        return data;
    }
    T* end() {
        return data + size;
    }
    inline T& Back() {
        return data[size-1];
    }
};

// #include <string>
//
// using String = std::string;
// using WString = std::wstring;

/*  struct: BasicString
    Author: Philip Haynes
    A basic string type, meant to typedef to String and WString */
template<typename T>
struct BasicString : public Array<T, 1> {
    BasicString() : Array<T, 1>() {}
    BasicString(const i32 newSize, const T& value=T()) : Array<T, 1>(newSize, value) {}
    BasicString(const Array<T, 1>& other) : Array<T, 1>(other) {}
    BasicString(const std::initializer_list<T>& init) : Array<T, 1>(init) {}
    BasicString(const Array<T, 1>&& other) noexcept : Array<T, 1>(other) {}

    BasicString(const T* string) {
        for (this->size = 0; string[this->size] != 0; this->size++) {}
        this->allocated = this->size;
        if (this->size != 0) {
            this->data = new T[this->allocated + 1];
            for (i32 i = 0; i <= this->size; i++) {
                this->data[i] = string[i];
            }
        }
    }

    ~BasicString() {
        Array<T,1>::~Array();
    }

    BasicString<T>& operator=(const T* string) {
        i32 newSize = 0;
        for (newSize = 0; string[newSize] != 0; newSize++) {}
        this->Reserve(newSize);
        this->size = newSize;
        for (i32 i = 0; i <= this->size; i++) {
            this->data[i] = string[i];
        }
        return *this;
    }

    bool operator==(const T* string) const {
        i32 i = 0;
        for (; string[i] != 0 && i < this->size; i++) {
            if (string[i] != this->data[i]) {
                return false;
            }
        }
        if (i != this->size) {
            return false;
        }
        return true;
    }

    BasicString<T> operator+(const T& value) const {
        BasicString<T> result(*this);
        result += value;
        return result;
    }

    BasicString<T> operator+(const T* string) const {
        BasicString<T> result(*this);
        result += string;
        return result;
    }

    inline BasicString<T> operator+(const BasicString<T>& other) const {
        return Array<T, 1>::operator+((const Array<T,1>&)other);
    }

    inline BasicString<T>& operator+=(const T& value) {
        return Append(value);
    }

    inline BasicString<T>& operator+=(const BasicString<T>& other) {
        return Append(other);
    }

    inline BasicString<T>& operator+=(const T* string) {
        return Append(string);
    }

    BasicString<T>& Append(const T* string) {
        i32 i = 0;
        for (; string[i] != 0; i++) {}
        i32 oldSize = this->size;
        this->Resize(oldSize+i);
        for (i32 j = 0; string[j] != 0; j++) {
            this->data[oldSize+j] = string[j];
        }
        return *this;
    }

    inline BasicString<T>& Append(const T& value) {
        return (BasicString<T>&)Array<T,1>::Append(value);
    }

    inline BasicString<T>& Append(const BasicString<T>& other) {
        return (BasicString<T>&)Array<T,1>::Append((const Array<T,1>&)other);
    }
};

template<typename T>
BasicString<T> operator+(const T* cString, const BasicString<T>& string) {
    BasicString<T> result;
    i32 size = 0;
    for (; cString[size] != 0; size++) {}
    result.Reserve(size+string.size);
    result += cString;
    result += string;
    return result;
}

using String = BasicString<char>;
using WString = BasicString<wchar_t>;

String ToString(const u32& value, i32 base=10);
String ToString(const u64& value, i32 base=10);
String ToString(const i32& value, i32 base=10);
String ToString(const i64& value, i32 base=10);
String ToString(const f32& value, i32 base=10);
// String ToString(f64 value, i32 base=10);

inline bool operator==(const char *b, const String& a) {
    return a == b;
}

bool equals(const char *a, const char *b);

// Converts a UTF-8 string to Unicode string
WString ToWString(const char *string);
WString ToWString(String string);

// #include <vector>
//
// template<typename T>
// using Array = std::vector<T>;

/*  struct: ArrayPtr
    Author: Philip Haynes
    Uses an index of an Array rather than an actual pointer.
    Note that this can only be used with nested Arrays given that the array
    it points to isn't moved. This means that the Arrays must be allocated
    in ascending order, starting with the base array.     */
template<typename T>
struct ArrayPtr {
    Array<T> *array = nullptr;
    i32 index = 0;
    ArrayPtr() {}
    ArrayPtr(Array<T> *a, i32 i) {
        array = a;
        index = i;
    }
    void SetPtr(Array<T> *a, i32 i) {
        array = a;
        index = i;
    }
    bool Valid() const {
        return array != nullptr;
    }
    T& operator*() {
        return (*array)[index];
    }
    T* operator->() {
        return &(*array)[index];
    }
};

/*  struct: ArrayRange
    Author: Philip Haynes
    Using an index and count, points to a range of values from an Array.        */
template<typename T>
struct ArrayRange {
    Array<T> *array = nullptr;
    i32 index = 0;
    i32 size = 0;
    ArrayRange() {}
    ArrayRange(Array<T> *a, i32 i, i32 s) {
        array = a;
        index = i;
        size = s;
    }
    void SetRange(Array<T> *a, i32 i, i32 s) {
        array = &a;
        index = i;
        size = s;
    }
    ArrayPtr<T> GetPtr(const i32& i) {
        return ArrayPtr<T>(array, index+i);
    }
    T& operator[](const i32& i) {
        if (i >= size) {
            throw std::out_of_range("ArrayRange dereferenced beyond size");
        }
        return (*array)[i+index];
    }
    const T& operator[](const i32& i) const {
        return (*array)[i+index];
    }
};

#include <map>

template<typename T, typename B>
using Map = std::map<T, B>;

#include <set>

template<typename T>
using Set = std::set<T>;

#include <mutex>
// #ifdef __MINGW32__
// #include <mingw/mutex.h>
// #endif

using Mutex = std::mutex;

#include <thread>

using Thread = std::thread;

#include <chrono>

using Milliseconds = std::chrono::milliseconds;
using Nanoseconds = std::chrono::nanoseconds;

using Clock = std::chrono::steady_clock;
using ClockTime = std::chrono::steady_clock::time_point;

#include <memory>

template<typename T, typename Deleter=std::default_delete<T>>
using UniquePtr = std::unique_ptr<T, Deleter>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

template<typename T>
class List;

template<typename T>
class ListIterator;

/*  class: ListIndex
    Author: Philip Haynes
    A single index in a linked list     */
template<typename T>
class ListIndex {
    friend class List<T>;
    friend class ListIterator<T>;
    ListIndex<T> *next=nullptr;
public:
    T value{};
};

/*  class: ListIterator
    Author: Philip Haynes
    Iterating over our Linked List      */
template<typename T>
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
    const ListIterator<T>& operator++() {
        me = me->next;
        return *this;
    }
    T& operator*() {
        return me->value;
    }
};

/*  struct: List
    Author: Philip Haynes
    Just a linked list that can clean itself up.      */
template<typename T>
struct List {
    ListIndex<T> *first;
    i32 size;
    List() : first(nullptr) , size(0) {}
    List(const List<T>& other) : first(nullptr) , size(0) {
        *this = other;
    }
    List(const List<T>&& other) noexcept : first(other.first) , size(other.size) {}
    List(std::initializer_list<T> init) : first(nullptr) , size(0) {
        *this = init;
    }
    List(const Array<T>& array) : first(nullptr) , size(0) {
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
    List<T>& operator=(const List<T>& other) {
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
    List<T>& operator=(const std::initializer_list<T> init) {
        Resize(init.size());
        ListIndex<T> *it = first;
        for (u32 i = 0; i < init.size(); i++) {
            it->value = *(init.begin()+i);
            it = it->next;
        }
        return *this;
    }
    List<T>& operator=(const Array<T>& array) {
        Resize(array.size);
        ListIndex<T> *it = first;
        for (i32 i = 0; i < array.size; i++) {
            it->value = array[i];
            it = it->next;
        }
        return *this;
    }
    T& operator[](const i32 index) {
        ListIndex<T> *it = first;
        for (i32 i = 0; i < index; i++) {
            it = it->next;
        }
        return it->value;
    }
    const T& operator[](const i32 index) const {
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
    void Resize(i32 s) {
        if (s > size) {
            for (i32 i = size; i < s; i++) {
                Append(T());
            }
        } else if (size > s) {
            ListIndex<T> *it = first;
            for (i32 i = 0; i < s-1; i++) {
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
    void Append(const T& a) {
        size++;
        if (size == 1) {
            first = new ListIndex<T>();
            first->value = a;
            return;
        }
        ListIndex<T> *it = first;
        for (i32 i = 2; i < size; i++) {
            it = it->next;
        }
        it->next = new ListIndex<T>();
        it->next->value = a;
    }
    void Insert(const i32 index, const T& a) {
        size++;
        if (index == 0) {
            ListIndex<T> *f = first;
            first = new ListIndex<T>();
            first->value = a;
            first->next = f;
            return;
        }
        ListIndex<T> *it = first;
        for (i32 i = 1; i < index; i++) {
            it = it->next;
        }
        ListIndex<T> *n = it->next;
        it->next = new ListIndex<T>();
        it->next->value = a;
        it->next->next = n;
    }
    void Erase(const i32 index) {
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
};

/*  class: ArrayList
    Author: Philip Haynes
    Data structure useful for sparse chunks of data at a very wide range of indices.
    Good for mapping values from Unicode characters, for example.
    Negative indices are also valid, if that's your thing.     */
template<typename T>
class ArrayList {
    ArrayList<T> *prev=nullptr, *next=nullptr;
    i32 first=0, last=0;
    T outOfBoundsValue{};
    Array<T> indices{Array<T>(1)};
public:
    ArrayList<T>() {}
    ArrayList<T>(const ArrayList<T>& other) {
        *this = other;
    }
    ArrayList<T>(const ArrayList<T>&& other) noexcept {
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
