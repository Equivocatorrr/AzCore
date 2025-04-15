/*
	File: Endian.hpp
	Author: Philip Haynes
	Utilities for knowing endianness and swapping between big and little endian.
*/

#ifndef AZCORE_ENDIAN_HPP
#define AZCORE_ENDIAN_HPP

#include "../BasicTypes.hpp"
#include <type_traits>
#include <byteswap.h>

namespace AzCore {

struct SystemEndianness_t {
	union {
		u16 _bytes;
		struct {
			bool little, big;
		};
	};
};

extern SystemEndianness_t SysEndian;

inline void EndianSwap(u16 &data) {
	data = bswap_16(data);
}

inline void EndianSwap(u32 &data) {
	data = bswap_32(data);
}

inline void EndianSwap(u64 &data) {
	data = bswap_64(data);
}

template<
	typename T,
	typename = std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>
>
inline void EndianSwap(T &data) {
	static_assert(sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "What the hell are you doing, boy?");
	if constexpr (sizeof(T) == 2) {
		EndianSwap(*(u16*)&data);
	} else if constexpr (sizeof(T) == 4) {
		EndianSwap(*(u32*)&data);
	} else if constexpr (sizeof(T) == 8) {
		EndianSwap(*(u64*)&data);
	}
}

u16 bytesToU16(char bytes[2], bool swapEndian);
u32 bytesToU32(char bytes[4], bool swapEndian);
u64 bytesToU64(char bytes[8], bool swapEndian);

i16 bytesToI16(char bytes[2], bool swapEndian);
i32 bytesToI32(char bytes[4], bool swapEndian);
i64 bytesToI64(char bytes[8], bool swapEndian);

f32 bytesToF32(char bytes[4], bool swapEndian);
f64 bytesToF64(char bytes[8], bool swapEndian);

u16 endianSwap(u16 in, bool swapEndian = true);
u32 endianSwap(u32 in, bool swapEndian = true);
u64 endianSwap(u64 in, bool swapEndian = true);

inline i16 endianSwap(i16 in, bool swapEndian = true) {
	return (i16)endianSwap((u16)in, swapEndian);
}
inline i32 endianSwap(i32 in, bool swapEndian = true) {
	return (i32)endianSwap((u32)in, swapEndian);
}
inline i64 endianSwap(i64 in, bool swapEndian = true) {
	return (i64)endianSwap((u64)in, swapEndian);
}

template <typename T>
inline T endianFromL(T in) {
	return endianSwap(in, SysEndian.big);
}
template <typename T>
inline T endianToL(T in) {
	return endianSwap(in, SysEndian.big);
}
template <typename T>
inline T endianFromB(T in) {
	return endianSwap(in, SysEndian.little);
}
template <typename T>
inline T endianToB(T in) {
	return endianSwap(in, SysEndian.little);
}

} // namespace AzCore

#endif // AZCORE_ENDIAN_HPP
