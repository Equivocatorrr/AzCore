/*
	File: IndexHash.hpp
	Author: Philip Haynes
	IndexHash functions for HashSets and HashMaps
*/

#ifndef AZCORE_INDEX_HASH_HPP
#define AZCORE_INDEX_HASH_HPP

#include "../basictypes.hpp"

namespace AzCore {

template<u16 bounds>
constexpr i32 IndexHash(u8 in) {
	if constexpr (bounds < 256)
		return in % bounds;
	else
		return in;
}
template<u16 bounds>
constexpr i32 IndexHash(i8 in) {
	if constexpr (bounds < 256)
		return u8(in) % bounds;
	else
		return in;
}
template<u16 bounds>
constexpr i32 IndexHash(u16 in) {
	return in % bounds;
}
template<u16 bounds>
constexpr i32 IndexHash(i16 in) {
	return u16(in) % bounds;
}
template<u16 bounds>
constexpr i32 IndexHash(u32 in) {
	return in % bounds;
}
template<u16 bounds>
constexpr i32 IndexHash(i32 in) {
	return u32(in) % bounds;
}
template<u16 bounds>
constexpr i32 IndexHash(u64 in) {
	return in % bounds;
}
template<u16 bounds>
constexpr i32 IndexHash(i64 in) {
	return u64(in) % bounds;
}
template<u16 bounds>
constexpr i32 IndexHash(u128 in) {
	return in % bounds;
}
template<u16 bounds>
constexpr i32 IndexHash(i128 in) {
	return u128(in) % bounds;
}

template<u16 bounds>
constexpr i32 IndexHash(void *in) {
	return u64(in) % bounds;
}

} // namespace AzCore

#endif // AZCORE_INDEX_HASH_HPP
