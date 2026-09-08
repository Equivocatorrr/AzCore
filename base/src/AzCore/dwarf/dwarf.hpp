/*
	File: dwarf.hpp
	Author: Philip Haynes
	Utilities and structures for working with DWARF debugging information
*/

#ifndef AZCORE_DWARF_HPP
#define AZCORE_DWARF_HPP

#include "../elf/elf.hpp"
#include "definitions.hpp"
#include "../Memory/Result.hpp"
#include "../Memory/Ptr.hpp"

namespace AzCore::dwarf {

// Attribute specification
struct AbbrevAttrib {
	AttribName name; // Encoded as a ULEB (special value 0 denotes the end of attributes for this decl)
	Form form; // Encoded as a ULEB
	i64 constant; // Encoded as a SLEB (only exists with Form::IMPLICIT_CONST)
};

// All the attributes for parsing a single TAG in a compute unit
struct AbbrevDecl {
	Range<u8> binary;
	u64 abbrev_code; // Encoded as a ULEB (special value 0 denotes end of decls for this CU)
	TAG tag; // Encoded as a ULEB
	bool has_children; // Encoded as a u8
	Array<AbbrevAttrib> attribs;
};

// One whole CU from .debug_abbrev
struct AbbrevUnit {
	Range<u8> binary;
	Array<AbbrevDecl> decls;

	[[nodiscard]] Result<None_t, String> Parse(Range<u8> debug_abbrev);
	// the return value won't be Valid() if we don't find the code.
	[[nodiscard]] Ptr<AbbrevDecl> GetDecl(u64 abbrev_code);
};

struct Attrib {
	AttribName name;
	Form form;
	Class _class;
	union {
		Str string;
		Range<u8> block;
		u64 addr; // Used for addresses, offsets, and indexes
		struct {
			u64 lo, hi;
		} constant;
		bool flag;
	};
	constexpr Attrib() : name((AttribName)0), form((Form)0), _class((Class)0), string(nullptr) {}
	[[nodiscard]] Result<None_t, String> Parse(Range<u8> binary, i64 &cur, const AbbrevAttrib &spec, u8 dwarfPtrSize, u8 targetArchPtrSize);
};

// Debugging Information Entry
struct DIE {
	Range<u8> binary;
	u64 abbrev_code; // Maps to an abbrev_code in the CU's AbbrevUnit (should match one AbbrevDecl), Encoded as a ULEB
	Ptr<AbbrevDecl> abbrev;
	Array<Attrib> attribs;
	Array<DIE> children;
};

struct InitialLength {
	// For the 32-bit DWARF format this will be < 0xfffffff0, and will represent unit_length
	// For the 64-bit DWARF format this will be = 0xffffffff, and unit_length will come immediately after (unpadded and therefore unaligned)
	u32 initial_length;
	// This will always contain the actual length
	u64 unit_length;

	[[nodiscard]] Result<None_t, String> Parse(Range<u8> binary, i64 &cur);

	inline bool Is64Bit() const {
		return initial_length == 0xffffffff;
	}
	// Since unit_length's value doesn't include itself, we need to handle that.
	inline u64 GetTotalLength() const {
		u64 result = unit_length;
		if (initial_length == 0xffffffff) {
			result += 12;
		} else {
			result += 4;
		}
		return result;
	}
};

// Header for CU found in .debug_info
struct InfoUnitHeader {
	Range<u8> binary;

	InitialLength unit_length;
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

	[[nodiscard]] Result<None_t, String> Parse(Range<u8> debug_info, i64 &cur);

	// How many bytes in the source binary does this entire entry take up, including the header?
	inline u64 GetTotalBinarySize() const {
		return unit_length.GetTotalLength();
	}
};

// CU found in .debug_info
struct InfoUnit {
	Range<u8> binary;
	InfoUnitHeader header;
	Ptr<AbbrevUnit> abbrev; // Necessary for parsing the DIEs
	Array<DIE> dies;

	// debug_info should be offset into the actual section
	// abbrev_unit should point to an already-parsed AbbrevUnit
	[[nodiscard]] Result<None_t, String> Parse(Range<u8> debug_info, i64 &cur, Ptr<AbbrevUnit> abbrev_unit, u8 targetArchPtrSize);
};

struct ARangeDescriptor {
	u64 segment_selector; // This isn't here if segment_selector_size is 0
	u64 offset;
	u64 length;
};

// CU found in .debug_aranges
struct ARangeUnit {
	Range<u8> binary;
	InitialLength unit_length;
	u16 version;
	u64 debug_info_offset; // Offset into .debug_info of the CU we're referencing
	u8 address_size;
	u8 segment_selector_size;
	Ptr<InfoUnit> debug_info_unit;
	Array<ARangeDescriptor> ranges;

	[[nodiscard]] Result<None_t, String> Parse(Range<u8> debug_aranges, i64 &cur, Ptr<InfoUnit> info_unit);
};

struct DebuggerInfo {

	// Sections

	Range<u8> debug_aranges;
	Range<u8> debug_info;
	Range<u8> debug_abbrev;
	Range<u8> debug_line;
	Range<u8> debug_str;
	Range<u8> debug_line_str;
	Range<u8> debug_rnglists;

	// Parsed info

	Array<InfoUnit> info_units;
	Array<AbbrevUnit> abbrev_units;
	Array<ARangeUnit> arange_units;

	[[nodiscard]] Result<None_t, String> ParseFromELF(elf::File &file);
};

} // namespace AzCore::dwarf

namespace AzCore {

void AppendToString(String &string, const az::dwarf::Attrib &value);

} // namespace AzCore

#endif // AZCORE_DWARF_HPP