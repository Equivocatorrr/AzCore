/*
	File: TypeHash.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_TYPE_HASH_HPP
#define AZCORE_TYPE_HASH_HPP

#include "../basictypes.hpp"
#include "StringCommon.hpp"
#include "ByteHash.hpp"

namespace AzCore {

// Provides a 32-bit hash for types that doesn't rely on RTTI.
// NOTE: Hashes will probably not be equal for the same types if linking objects compiled by different compilers.
// TODO: We could probably remedy this with some more processing by making a unified function signature format.
template <typename T>
constexpr u32 TypeHash() {
	const char *string = AZCORE_PRETTY_FUNCTION;
	return ByteHash<u32>((u8*)string, StringLength(string));
}

} // namespace AzCore

#endif // AZCORE_TYPE_HASH_HPP