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


/*  struct: StringTerminators
    Author: Philip Haynes
    If you want to use value-terminated strings with Arrays or StringLength, the correct
    string terminator must be set. Most types will work with just the default constructor.  */
template<typename T>
struct StringTerminators {
    static T value;
};
// Macro to easily set a terminator. Must be called from one and only one .cpp file.
// Definitions for char and wchar_t are done in memory.cpp
#define STRING_TERMINATOR(TYPE, VAL) template<> TYPE StringTerminators<TYPE>::value = VAL

template<typename T>
i32 StringLength(const T* string) {
    i32 length = 0;
    for (; string[length] != StringTerminators<T>::value; length++) {}
    return length;
}

template<typename T, i32 allocTail=0> struct Array;

/*  struct: Ptr
    Author: Philip Haynes
    May refer to either an index in an Array or just a raw pointer.     */
template<typename T>
struct Ptr {
    void *ptr = nullptr;
    i32 index = 0;
    Ptr() {}
    Ptr(T *a) {
        ptr = a;
        index = -1;
    }
    Ptr(Array<T> *a, i32 i) {
        ptr = a;
        index = i;
    }
    void Set(Array<T> *a, i32 i) {
        ptr = a;
        index = i;
    }
    void Set(T *a) {
        ptr = a;
        index = -1;
    }
    bool Valid() const {
        return ptr != nullptr;
    }
    bool operator==(T *other) const {
        if (index < 0) {
            return other == ptr;
        } else {
            return other == &(*reinterpret_cast<Array<T>*>(ptr))[index];
        }
    }
    bool operator!=(T *other) const {
        if (index < 0) {
            return !(other == ptr);
        } else {
            return !(other == &(*reinterpret_cast<Array<T>*>(ptr))[index]);
        }
    }
    bool operator==(Ptr<T> other) const {
        return ptr == other.ptr;
    }
    bool operator!=(Ptr<T> other) const {
        return ptr != other.ptr;
    }
    T& operator*() {
        if (index < 0) {
            return *reinterpret_cast<T*>(ptr);
        } else {
            return (*reinterpret_cast<Array<T>*>(ptr))[index];
        }
    }
    T* operator->() {
        if (index < 0) {
            return reinterpret_cast<T*>(ptr);
        } else {
            return &(*reinterpret_cast<Array<T>*>(ptr))[index];
        }
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
    void Set(Array<T> *a, i32 i, i32 s) {
        array = &a;
        index = i;
        size = s;
    }
    Ptr<T> ToPtr(const i32& i) {
        return Ptr<T>(array, index+i);
    }
    T& operator[](const i32& i) {
        if (i >= size) {
            throw std::out_of_range("ArrayRange index is out of bounds");
        }
        return (*array)[i+index];
    }
    const T& operator[](const i32& i) const {
        return (*array)[i+index];
    }
};

/*  struct: Array
    Author: Philip Haynes
    A replacement for std::vector which is nicer to work with, and can be used to
    make strings of any data type so long as there is an allocTail of at least 1
    (default is 0). Accessing data and size directly is the proper way to use this
    type, so long as you don't modify size (unless you know what you're doing).   */
template<typename T, i32 allocTail>
struct Array {
    T *data;
    i32 allocated;
    i32 size;

    void SetTerminator() {
        if constexpr (allocTail != 0) {
            for (i32 i = size; i < size+allocTail; i++) {
                data[i] = StringTerminators<T>::value;
            }
        }
    }

    Array(bool tailInitialize = true) : data(nullptr) , allocated(0) , size(0) {
        if (tailInitialize && allocTail != 0) { // To avoid double allocating when only one is necessary
            data = new T[allocTail];
            SetTerminator();
        }
    }
    Array(const i32 newSize, const T& value=T()) : data(new T[newSize + allocTail]) , allocated(newSize) , size(newSize) {
        for (i32 i = 0; i < size; i++) {
            data[i] = value;
        }
        SetTerminator();
    }
    Array(const u32 newSize, const T& value=T()) : Array((i32)newSize, value) {}
    Array(const std::initializer_list<T>& init) : allocated(init.size()) , size(allocated) {
        if (size != 0 || allocTail != 0) {
            data = new T[allocated + allocTail];
            i32 i = 0;
            for (const T& val : init) {
                data[i++] = val;
            }
            SetTerminator();
        } else {
            data = nullptr;
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
        SetTerminator();
    }
    Array(Array<T, allocTail>&& other) noexcept : data(other.data) , allocated(other.allocated) , size(other.size) {
        other.data = nullptr;
        other.size = 0;
        other.allocated = 0;
    }

    Array(const T* string) : allocated(StringLength(string)) , size(allocated) {
        data = new T[allocated+allocTail];
        for (i32 i = 0; i < size; i++) {
            data[i] = string[i];
        }
        SetTerminator();
    }

    ~Array() {
        if (data != nullptr) {
            delete[] data;
        }
    }

    Array<T, allocTail>& operator=(const Array<T, allocTail>& other) {
        Array<T, allocTail> value(other);
        return operator=(std::move(value));
    }

    Array<T, allocTail>& operator=(Array<T, allocTail>&& other) {
        if (data != nullptr) {
            delete[] data;
        }
        data = other.data;
        size = other.size;
        allocated = other.allocated;
        other.data = nullptr;
        other.size = 0;
        other.allocated = 0;
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
        SetTerminator();
        return *this;
    }

    Array<T, allocTail>& operator=(const T* string) {
        Resize(StringLength(string));
        for (i32 i = 0; i < size; i++) {
            data[i] = string[i];
        }
        SetTerminator();
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

    bool operator==(const T* string) const {
        i32 i = 0;
        for (; string[i] != StringTerminators<T>::value && i < size; i++) {
            if (string[i] != data[i]) {
                return false;
            }
        }
        if (i != size) {
            return false;
        }
        return true;
    }

    const T& operator[](const i32 index) const {
        if (index > size) {
            throw std::out_of_range("Array index is out of bounds");
        }
        return data[index];
    }

    T& operator[](const i32 index) {
        if (index > size) {
            throw std::out_of_range("Array index is out of bounds");
        }
        return data[index];
    }

    Array<T, allocTail> operator+(T&& other) const {
        Array<T, allocTail> result(*this);
        result += other;
        return result;
    }

    Array<T, allocTail> operator+(const T& other) const {
        Array<T, allocTail> result(*this);
        result += other;
        return result;
    }

    Array<T, allocTail> operator+(const T* string) const {
        Array<T, allocTail> result(*this);
        result += string;
        return result;
    }

    Array<T, allocTail> operator+(Array<T, allocTail>&& other) const {
        Array<T, allocTail> result(*this);
        result += other;
        return result;
    }

    Array<T, allocTail> operator+(const Array<T, allocTail>& other) const {
        Array<T, allocTail> result(*this);
        result += other;
        return result;
    }

    inline T& operator+=(T&& value) {
        return Append(value);
    }

    inline T& operator+=(const T& value) {
        return Append(value);
    }

    inline Array<T, allocTail>& operator+=(const Array<T, allocTail>& other) {
        return Append(other);
    }

    inline Array<T, allocTail>& operator+=(Array<T, allocTail>&& other) {
        return Append(other);
    }

    inline Array<T, allocTail>& operator+=(const T* string) {
        return Append(string);
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
                    temp[i] = std::move(data[i]);
                }
            }
            delete[] data;
            data = temp;
            SetTerminator();
            return;
        }
        if (data != nullptr) {
            delete[] data;
        }
        data = new T[newSize + allocTail];
        SetTerminator();
    }

    void Resize(const i32 newSize, const T& value=T()) {
        if (newSize > allocated) {
            Reserve(max(newSize, (allocated >> 1) + 2));
        } else if (newSize == 0 && allocTail == 0) {
            if (data != nullptr) {
                delete[] data;
            }
            data = nullptr;
            allocated = 0;
            size = 0;
            return;
        }
        for (i32 i = size; i < newSize; i++) {
            data[i] = value;
        }
        size = newSize;
        SetTerminator();
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
                        temp[i] = std::move(data[i]);
                    }
                }
            }
            if (data != nullptr) {
                delete[] data;
            }
            data = temp;
        }
        size++;
        SetTerminator();
        return data[size-1] = value;
    }

    Array<T, allocTail>& Append(const T* string) {
        i32 newSize = size + StringLength(string);
        Reserve(newSize);
        for (i32 i = size; i < newSize; i++) {
            data[i] = string[i-size];
        }
        size = newSize;
        SetTerminator();
        return *this;
    }

    Array<T, allocTail>& Append(const Array<T, allocTail>& other) {
        Array<T, allocTail> value(other);
        return Append(std::move(value));
    }

    Array<T, allocTail>& Append(Array<T, allocTail>&& other) {
        i32 copyStart = size;
        Resize(size+other.size);
        if (std::is_trivially_copyable<T>::value) {
            memcpy((void*)(data+copyStart), (void*)other.data, sizeof(T) * other.size);
        } else {
            for (i32 i = copyStart; i < size; i++) {
                data[i] = std::move(other.data[i-copyStart]);
            }
        }
        SetTerminator();
        if (other.data != nullptr) {
            delete[] other.data;
        }
        other.data = nullptr;
        other.size = 0;
        other.allocated = 0;
        return *this;
    }

    T& Insert(const i32 index, const T& value) {
        T val(value);
        return Insert(index, std::move(val));
    }

    T& Insert(const i32 index, T&& value) {
        if (index > size) {
            throw std::out_of_range("Array::Insert index is out of bounds");
        }
        if (size >= allocated) {
            allocated += (allocated >> 1) + 2;
            T *temp = new T[allocated + allocTail];
            if (std::is_trivially_copyable<T>::value) {
                if (index > 0) {
                    memcpy((void*)temp, (void*)data, sizeof(T) * index);
                }
                temp[index] = std::move(value);
                if (size-index > 0) {
                    memcpy((void*)(temp+index+1), (void*)(data+index), sizeof(T) * (size-index));
                }
            } else {
                for (i32 i = 0; i < index; i++) {
                    temp[i] = std::move(data[i]);
                }
                temp[index] = value;
                for (i32 i = index+1; i < size+1; i++) {
                    temp[i] = std::move(data[i-1]);
                }
            }
            delete[] data;
            data = temp;
            size++;
            SetTerminator();
            return data[index];
        }
        size++;
        for (i32 i = size-1; i > index; i--) {
            data[i] = std::move(data[i-1]);
        }
        SetTerminator();
        return data[index] = value;
    }

    void Erase(const i32 index) {
        if (index >= size) {
            throw std::out_of_range("Array::Erase index is out of bounds");
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

    Ptr<T> GetPtr(const i32& index) {
        if (index >= size) {
            throw std::out_of_range("Array::GetPtr index is out of bounds");
        }
        return Ptr<T>(this, index);
    }

    ArrayRange<T> GetRange(const i32& index, const i32& _size) {
        if (index+_size > size) {
            throw std::out_of_range("Array::Range index + size is out of bounds");
        }
        return ArrayRange<T>(this, index, _size);
    }
};

// #include <string>
//
// using String = std::string;
// using WString = std::wstring;

using String = Array<char, 1>;
using WString = Array<wchar_t, 1>;

String operator+(const char* cString, String&& string);
String operator+(const char* cString, const String& string);
WString operator+(const wchar_t* cString, WString&& string);
WString operator+(const wchar_t* cString, const WString& string);

String ToString(const u32& value, i32 base=10);
String ToString(const u64& value, i32 base=10);
String ToString(const i32& value, i32 base=10);
String ToString(const i64& value, i32 base=10);
String ToString(const f32& value, i32 base=10);
String ToString(const f64& value, i32 base=10);

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
