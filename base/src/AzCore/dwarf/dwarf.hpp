/*
	File: dwarf.hpp
	Author: Philip Haynes
	Utilities and structures for working with DWARF debugging information
*/

#ifndef AZCORE_DWARF_HPP
#define AZCORE_DWARF_HPP

#include "definitions.hpp"
#include "../Memory/Result.hpp"

namespace AzCore::dwarf {

// Debugging Information Entry
struct DIE {
	Range<u8> binary;
	ULEB tag;
};

// Header for CU found in .debug_info
struct ComputeUnitHeader {
	Range<u8> binary;
	// For the 32-bit DWARF format this will be < 0xfffffff0, and will represent unit_length
	// For the 64-bit DWARF format this will be = 0xffffffff, and unit_length will come immediately after (unpadded and therefore unaligned)
	u32 initial_length;
	u64 unit_length;
	u16 version; // 5 for DWARF version 5
	ComputeUnitType unit_type; // New in DWARF 5
	u8 address_size; // byte size of an address on the target architecture
	u64 debug_abbrev_offset; // offset into the .debug_abbrev section, relating this compilation unit with a set of DIE abbreviations. binary is a u32 on 32-bit DWARF, u64 on 64-bit DWARF.

	// Specific fields for COMPILE and PARTIAL

	// NONE

	// Specific fields for SKELETON and SPLIT_COMPILE

	u64 dwo_id; // Compilation Unit ID

	// Specific fields for TYPE and SPLIT_TYPE

	u64 type_signature; // Unique 8-byte signature of the type described.
	u64 type_offset; // Offset (relative to start of this header) to the DIE that describes the type. binary is a u32 on 32-bit DWARF, u64 on 64-bit DWARF.

	[[nodiscard]] Result<None_t, String> Parse(Range<u8> _binary);

	// How many bytes in the source binary does this entire entry take up, including the header?
	inline u64 GetTotalBinarySize() const {
		u64 result = unit_length;
		if (initial_length == 0xffffffff) {
			result += 12;
		} else {
			result += 4;
		}
		return result;
	}
};

// CU found in .debug_info
struct ComputeUnit {
	Range<u8> binary;
	ComputeUnitHeader header;
	Array<DIE> dies;
};

// Header for CU found in .debug_abbrev
struct AbbrevHeader {
	Range<u8> binary;
	ULEB tag;
	bool has_children;
};

// Attribute specification
struct AbbrevSpec {
	ULEB name; // Encodes an Attribute
	ULEB form;
};

} // namespace AzCore::dwarf

#endif // AZCORE_DWARF_HPP