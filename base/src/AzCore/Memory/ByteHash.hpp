/*
	File: ByteHash.hpp
	Author: Philip Haynes
	Provides a progressive byte-by-byte hash.
*/

#ifndef AZCORE_BYTE_HASH_HPP
#define AZCORE_BYTE_HASH_HPP

#include "../basictypes.hpp"
#include <type_traits>

namespace AzCore {

// A progressive byte-by-byte hash. UInt can be u32, u64, or u128 (if available).
template<typename UInt>
constexpr UInt ByteHash(u8 *data, i32 size, UInt hash=0) {
#if AZCORE_COMPILER_SUPPORTS_128BIT_TYPES
	static_assert(
		std::is_same<UInt, u32 >::value ||
		std::is_same<UInt, u64 >::value ||
		std::is_same<UInt, u128>::value
	);
#else
	static_assert(
		std::is_same<UInt, u32>::value ||
		std::is_same<UInt, u64>::value
	);
#endif
	for (i32 i = 0; i < size; i++) {
		hash ^= (UInt)data[i];
		hash *= 1234567891;
		hash ^= hash >> 17;
	}
	return hash;
}

} // namespace AzCore

#endif // AZCORE_BYTE_HASH_HPP