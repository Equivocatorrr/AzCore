/*
	File: IndexHash.hpp
	Author: Philip Haynes
	IndexHash functions for HashSets and HashMaps
*/

#ifndef AZCORE_INDEX_HASH_HPP
#define AZCORE_INDEX_HASH_HPP

#include "../basictypes.hpp"
#include "ByteHash.hpp"
#include <type_traits>

namespace AzCore {

template<u16 bounds, typename T>
constexpr i32 IndexHash(const T &in) {
	static_assert(std::is_trivially_copyable<T>::value);
	return ByteHash<u64>((u8*)&in, sizeof(in)) % bounds;
}

} // namespace AzCore

#endif // AZCORE_INDEX_HASH_HPP
