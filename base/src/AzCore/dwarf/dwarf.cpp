/*
	File: dwarf.cpp
	Author: Philip Haynes
*/

#include "dwarf.hpp"
#include "definitions.hpp"

#define ENSURE_BUFFER(binary, bytes, offset) if ((i64)(binary).size < (((i64)bytes) + ((i64)offset))) {\
	return Stringify("Expected ", FormatInt(bytes, 16, true), " bytes to be available at offset ", FormatInt(offset, 16, true), " (binary size = ", FormatInt((binary).size, 16, true), ")");\
}

#define GET_DATA(dst, src)\
	ENSURE_BUFFER(src, sizeof(dst), cur);\
	memcpy(&(dst), &(src)[cur], sizeof(dst));\
	cur += sizeof(dst);

#define GET_DATA_PROXY(dst, src, proxyType)\
	ENSURE_BUFFER(src, sizeof(proxyType), cur);\
	{\
		proxyType _proxy;\
		memcpy(&_proxy, &(src)[cur], sizeof(proxyType));\
		(dst) = static_cast<decltype(dst)>(_proxy);\
		cur += sizeof(proxyType);\
	}

namespace AzCore::dwarf {

[[nodiscard]] Result<None_t, String> ComputeUnitHeader::Parse(Range<u8> _binary) {
	binary = _binary;
	bool is64bit = false;
	i64 cur = 0;
	GET_DATA(initial_length, binary);
	if (initial_length < 0xfffffff0) {
		unit_length = initial_length;
	} else if (initial_length == 0xffffffff) {
		is64bit = true;
		GET_DATA(unit_length, binary);
	} else {
		return Stringify("Unknown special initial_length ", FormatInt(initial_length, 16, true));
	}
	GET_DATA(version, binary);
	if (version != 5) { // TODO: Support older versions maybe?
		return Stringify("Unsupported DWARF version ", version);
	}
	GET_DATA(unit_type, binary);
	GET_DATA(address_size, binary);
	if (address_size != 4 && address_size != 8) {
		return Stringify("Invalid address_size ", address_size);
	}
	if (is64bit) {
		GET_DATA(debug_abbrev_offset, binary);
	} else {
		GET_DATA_PROXY(debug_abbrev_offset, binary, u32);
	}
	switch (unit_type) {
		case ComputeUnitType::COMPILE:
		case ComputeUnitType::PARTIAL:
			break; // No additional fields
		case ComputeUnitType::SKELETON:
		case ComputeUnitType::SPLIT_COMPILE: {
			GET_DATA(dwo_id, binary);
		} break;
		case ComputeUnitType::TYPE:
		case ComputeUnitType::SPLIT_TYPE: {
			GET_DATA(type_signature, binary);
			if (is64bit) {
				GET_DATA(type_offset, binary);
			} else {
				GET_DATA_PROXY(type_offset, binary, u32);
			}
		} break;
	}
	i64 totalSize = GetTotalBinarySize();
	if (totalSize > binary.size) {
		return Stringify("ComputeUnitHeader expected at least ", FormatInt(totalSize, 16, true), " bytes to be available in the binary (had ", FormatInt(binary.size, 16, true), " bytes)");
	}
	binary.size = cur;
	return None;
}

} // namespace AzCore::dwarf
