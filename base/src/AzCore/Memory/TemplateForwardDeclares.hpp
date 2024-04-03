/*
	File: TemplateForwardDeclares.hpp
	Author: Philip Haynes
	Because C++ is a good language with only good decisions behind it, handling circular dependencies between templates is a real pain in the ass. This is a meager attempt to cope with template header dependency hell. This MUST be included first because the first declaration (whether forward or actual) is the only place any default template arguments can be declared.
	This header can be used in any place that doesn't instantiate the dependency, such as methods with these as parameters. If the template is a member, you need to include the full definition.
*/

#ifndef AZCORE_MEMORY_TEMPLATE_FORWARD_DECLARES_HPP
#define AZCORE_MEMORY_TEMPLATE_FORWARD_DECLARES_HPP

namespace AzCore {

template <typename T, i32 allocTail=0>
struct Array;

template <typename T, i32 noAllocCount, i32 allocTail=0>
struct ArrayWithBucket;

template <typename T, i32 count>
struct StaticArray;

template <typename T>
struct List;
template <typename T>
struct ListIndex;
template <typename T>
class ListIterator;

template <typename T>
struct Ptr;

template <typename T>
struct Range;
template <typename T>
struct RangeIterator;

template <typename T>
struct SimpleRange;

template <typename Key_t, typename Value_t>
struct BinaryMap;
template <typename Node_t>
class BinaryMapIterator;

template <typename Key_t>
struct BinarySet;
template <typename Key_t>
class BinarySetIterator;

template <typename Key_t, typename Value_t, u16 arraySize=256>
struct HashMap;
template <typename Node_t, u16 arraySize>
class HashMapIterator;

template <typename Key_t, u16 arraySize=256>
struct HashSet;
template <typename Node_t, u16 arraySize>
class HashSetIterator;

} // namespace AzCore

#endif // AZCORE_MEMORY_TEMPLATE_FORWARD_DECLARES_HPP