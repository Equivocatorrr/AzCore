/*
	File: definitions.hpp
	Author: Philip Haynes
	definitions for working with DWARF debugging information
*/

#ifndef AZCORE_DWARF_DEFINITIONS_HPP
#define AZCORE_DWARF_DEFINITIONS_HPP

#include "../BasicTypes.hpp"
#include "../Memory/String.hpp"
#include "../Memory/StaticArray.hpp"
#include "../Memory/Result.hpp"

namespace AzCore::dwarf {
//
// Each byte effectively holds 7 bits of data, so to store at least 64 bits of data, we need 10 bytes.
constexpr i32 LEB128_MAX_BYTES_COUNT = 10;

struct ULEB {
	u64 value;
	StaticArray<u8, LEB128_MAX_BYTES_COUNT> binary;
};
struct SLEB {
	i64 value;
	StaticArray<u8, LEB128_MAX_BYTES_COUNT> binary;
};

inline bool operator==(const az::dwarf::ULEB &lhs, const az::dwarf::ULEB &rhs) {
	return lhs.value == rhs.value && lhs.binary == rhs.binary;
}

inline bool operator==(const az::dwarf::SLEB &lhs, const az::dwarf::SLEB &rhs) {
	return lhs.value == rhs.value && lhs.binary == rhs.binary;
}

// binary range can be to the end of the actual binary and the result will only read as many bytes as the encoding requires.
// ULEB::binary.size will hold the total byte length of the value.
// If cur is given, it will be progressed by the number of bytes consumed.
Result<ULEB, String> DecodeULEB(Range<u8> binary, i64 *cur=nullptr);
ULEB EncodeULEB(u64 value);
// binary range can be to the end of the actual binary and the result will only read as many bytes as the encoding requires.
// SLEB::binary.size will hold the total byte length of the value.
// If cur is given, it will be progressed by the number of bytes consumed.
Result<SLEB, String> DecodeSLEB(Range<u8> binary, i64 *cur=nullptr);
SLEB EncodeSLEB(i64 value);

enum class TAG : u64 {
	ARRAY_TYPE               = 0x01,
	CLASS_TYPE               = 0x02,
	ENTRY_POINT              = 0x03,
	ENUMERATION_TYPE         = 0x04,
	FORMAL_PARAMETER         = 0x05,
	// 0x06-0x07 reserved
	IMPORTED_DECLARATION     = 0x08,
	// 0x09 reserved
	LABEL                    = 0x0A,
	LEXICAL_BLOCK            = 0x0B,
	// 0x0C reserved
	MEMBER                   = 0x0D,
	// 0x0E reserved
	POINTER_TYPE             = 0x0F,
	REFERENCE_TYPE           = 0x10,
	COMPILE_UNIT             = 0x11,
	STRING_TYPE              = 0x12,
	STRUCTURE_TYPE           = 0x13,
	// 0x14 reserved
	SUBROUTINE_TYPE          = 0x15,
	TYPEDEF                  = 0x16,
	UNION_TYPE               = 0x17,
	UNSPECIFIED_PARAMETERS   = 0x18,
	VARIANT                  = 0x19,
	COMMON_BLOCK             = 0x1A,
	COMMON_INCLUSION         = 0x1B,
	INHERITANCE              = 0x1C,
	INLINED_SUBROUTINE       = 0x1D,
	MODULE                   = 0x1E,
	PTR_TO_MEMBER_TYPE       = 0x1F,
	SET_TYPE                 = 0x20,
	SUBRANGE_TYPE            = 0x21,
	WITH_STMT                = 0x22,
	ACCESS_DECLARATION       = 0x23,
	BASE_TYPE                = 0x24,
	CATCH_BLOCK              = 0x25,
	CONST_TYPE               = 0x26,
	CONSTANT                 = 0x27,
	ENUMERATOR               = 0x28,
	FILE_TYPE                = 0x29,
	FRIEND                   = 0x2A,
	NAMELIST                 = 0x2B,
	NAMELIST_ITEM            = 0x2C,
	PACKED_TYPE              = 0x2D,
	SUBPROGRAM               = 0x2E,
	TEMPLATE_TYPE_PARAMETER  = 0x2F,
	TEMPLATE_VALUE_PARAMETER = 0x30,
	THROWN_TYPE              = 0x31,
	TRY_BLOCK                = 0x32,
	VARIANT_PART             = 0x33,
	VARIABLE                 = 0x34,
	VOLATILE_TYPE            = 0x35,
	DWARF_PROCEDURE          = 0x36,
	RESTRICT_TYPE            = 0x37,
	INTERFACE_TYPE           = 0x38,
	NAMESPACE                = 0x39,
	IMPORTED_MODULE          = 0x3A,
	UNSPECIFIED_TYPE         = 0x3B,
	PARTIAL_UNIT             = 0x3C,
	IMPORTED_UNIT            = 0x3D,
	// Included for backwards compatibility
	DWARF3_MUTABLE_TYPE      = 0x3E,
	CONDITION                = 0x3F,
	SHARED_TYPE              = 0x40,
	TYPE_UNIT                = 0x41,
	RVALUE_REFERENCE_TYPE    = 0x42,
	TEMPLATE_ALIAS           = 0x43,

	// New in DWARF 5

	COARRAY_TYPE             = 0x44,
	GENERIC_SUBRANGE         = 0x45,
	DYNAMIC_TYPE             = 0x46,
	ATOMIC_TYPE              = 0x47,
	CALL_SITE                = 0x48,
	CALL_SITE_PARAMETER      = 0x49,
	SKELETON_UNIT            = 0x4A,
	IMMUTABLE_TYPE           = 0x4B,
};
constexpr u64 TAG_LO_USER = 0x4080;
constexpr u64 TAG_HI_USER = 0xFFFF;

enum class ComputeUnitType : u8 {
	COMPILE       = 0x01,
	TYPE          = 0x02,
	PARTIAL       = 0x03,
	SKELETON      = 0x04,
	SPLIT_COMPILE = 0x05,
	SPLIT_TYPE    = 0x06,
};
constexpr u8 ComputeUnitType_LO_USER = 0x80;
constexpr u8 ComputeUnitType_HI_USER = 0xff;

enum class Class : u8 {
	// Can mean 2 different things:
	// - An index into .debug_addr relative to the ADDR_BASE attribute of the associated CU (Form::ADDRX_)
	// - A location in the address space of the described program (value will need to be relocated) (Form::ADDR)
	ADDRESS,
	// Offset into .debug_addr encoded as a target ptr (Form::SEC_OFFSET)
	ADDRPTR,
	// A chunk of data (size may be implicit or specified by a ULEB, non-inclusive)
	BLOCK,
	// 1, 2, 4, 8, or 16 bytes or an LEB128 value (Form::DATA_ and Form::_DATA)
	CONSTANT,
	// A DWARF expression (size is specified by a ULEB, non-inclusive) (Form::EXPRLOC)
	EXPRLOC,
	// Encoded as a u8 with Form::FLAG, or implicit with Form::FLAG_PRESENT
	FLAG,
	// Offset into .debug_line encoded as a target ptr (Form::SEC_OFFSET)
	LINEPTR,
	// Can mean 2 different things:
	// - An index into .debug_loclists encoded as a ULEB, relative to the location of the first offset in the section (Form::LOCLISTX)
	// - An offset into .debug_loclists encoded as a target ptr (Form::SEC_OFFSET)
	LOCLIST,
	// Offset into .debug_loc encoded as a target ptr (Form::SEC_OFFSET)
	LOCLISTPTR,
	// Offset into .debug_macro encoded as a target ptr (Form::SEC_OFFSET)
	MACPTR,
	// Can mean 2 different things:
	// - An index into .debug_rnglists encoded as a ULEB (Form::RNGLISTX)
	// - An offset into .debug_rnglists encoded as a target ptr (Form::SEC_OFFSET)
	RNGLIST, // New in DWARF 5
	// Offset into .debug_rnglists encoded as a target ptr (Form::SEC_OFFSET)
	RNGLISTPTR, // New in DWARF 5
	// refers to one of the DIEs. Can be one of four types:
	// - Offset relative to the beginning of the current CU, encoded as 1-, 2-, 4-, or 8-byte, or ULEB (Form::REF_ and Form::REF_UDATA)
	// - Offset of a DIE in any CU encoded as a target ptr (Form::REF_ADDR)
	// - Indirect ref to a type def using an 8-byte signature (Form::REF_SIG8)
	// - Reference to an external DIE (in a separate file) (Form::REF_SUP_)
	REFERENCE,
	// Can mean 3 different things:
	// - Null-terminated string encoded directly (Form::STRING)
	// - An offset into .debug_str (Form::STRP) or .debug_line_str (Form::LINE_STRP), or an offset into a separate file .debug_str (Form::STRP_SUP), encoded as a target ptr
	// - An index into .debug_str_offsets encoded as 1-, 2-, 3-, or 4-byte, or ULEB (Form::STRX_), whose values are interpreted as an offset into .debug_str
	STRING,
	// Offset into .debug_str_offsets encoded as a target ptr (Form::SEC_OFFSET)
	STROFFSETSPTR, // New in DWARF 5
};

enum class AttribName : u64 {
	SIBLING                 = 0x01,
	LOCATION                = 0x02,
	NAME                    = 0x03,
	// 0x04-0x08 reserved
	ORDERING                = 0x09,
	// 0x0A reserved
	BYTE_SIZE               = 0x0B,
	DWARF3_BIT_OFFSET       = 0x0C,
	BIT_SIZE                = 0x0D,
	// 0x0E-0x0F reserved
	STMT_LIST               = 0x10,
	LOW_PC                  = 0x11,
	HIGH_PC                 = 0x12,
	LANGUAGE                = 0x13,
	// 0x14 reserved
	DISCR                   = 0x15,
	DISCR_VALUE             = 0x16,
	VISIBILITY              = 0x17,
	IMPORT                  = 0x18,
	STRING_LENGTH           = 0x19,
	COMMON_REFERENCE        = 0x1A,
	COMP_DIR                = 0x1B,
	CONST_VALUE             = 0x1C,
	CONTAINING_TYPE         = 0x1D,
	DEFAULT_VALUE           = 0x1E,
	// 0x1F reserved
	INLINE                  = 0x20,
	IS_OPTIONAL             = 0x21,
	LOWER_BOUND             = 0x22,
	// 0x23-0x24 reserved
	PRODUCER                = 0x25,
	// 0x26 reserved
	PROTOTYPED              = 0x27,
	// 0x28-0x29 reserved
	RETURN_ADDR             = 0x2A,
	// 0x2B reserved
	START_SCOPE             = 0x2C,
	// 0x2D reserved
	BIT_STRIDE              = 0x2E,
	UPPER_BOUND             = 0x2F,
	// 0x30 reserved
	ABSTRACT_ORIGIN         = 0x31,
	ACCESSIBILITY           = 0x32,
	ADDRESS_CLASS           = 0x33,
	ARTIFICIAL              = 0x34,
	BASE_TYPES              = 0x35,
	CALLING_CONVENTION      = 0x36,
	COUNT                   = 0x37,
	DATA_MEMBER_LOCATION    = 0x38,
	DECL_COLUMN             = 0x39,
	DECL_FILE               = 0x3A,
	DECL_LINE               = 0x3B,
	DECLARATION             = 0x3C,
	DISCR_LIST              = 0x3D,
	ENCODING                = 0x3E,
	EXTERNAL                = 0x3F,
	FRAME_BASE              = 0x40,
	FRIEND                  = 0x41,
	IDENTIFIER_CASE         = 0x42,
	DWARF4_MACRO_INFO       = 0x43,
	NAMELIST_ITEM           = 0x44,
	PRIORITY                = 0x45,
	SEGMENT                 = 0x46,
	SPECIFICATION           = 0x47,
	STATIC_LINK             = 0x48,
	TYPE                    = 0x49,
	USE_LOCATION            = 0x4A,
	VARIABLE_PARAMETER      = 0x4B,
	VIRTUALITY              = 0x4C,
	VTABLE_ELEM_LOCATION    = 0x4D,
	ALLOCATED               = 0x4E,
	ASSOCIATED              = 0x4F,
	DATA_LOCATION           = 0x50,
	BYTE_STRIDE             = 0x51,
	ENTRY_PC                = 0x52,
	USE_UTF8                = 0x53,
	EXTENSION               = 0x54,
	RANGES                  = 0x55,
	TRAMPOLINE              = 0x56,
	CALL_COLUMN             = 0x57,
	CALL_FILE               = 0x58,
	CALL_LINE               = 0x59,
	DESCRIPTION             = 0x5A,
	BINARY_SCALE            = 0x5B,
	DECIMAL_SCALE           = 0x5C,
	SMALL                   = 0x5D,
	DECIMAL_SIGN            = 0x5E,
	DIGIT_COUNT             = 0x5F,
	PICTURE_STRING          = 0x60,
	MUTABLE                 = 0x61,
	THREADS_SCALED          = 0x62,
	EXPLICIT                = 0x63,
	OBJECT_POINTER          = 0x64,
	ENDIANITY               = 0x65,
	ELEMENTAL               = 0x66,
	PURE                    = 0x67,
	RECURSIVE               = 0x68,
	SIGNATURE               = 0x69,
	MAIN_SUBPROGRAM         = 0x6A,
	DATA_BIT_OFFSET         = 0x6B,
	CONST_EXPR              = 0x6C,
	ENUM_CLASS              = 0x6D,
	LINKAGE_NAME            = 0x6E,

	// New in DWARF 5

	STRING_LENGTH_BIT_SIZE  = 0x6F,
	STRING_LENGTH_BYTE_SIZE = 0x70,
	RANK                    = 0x71,
	STR_OFFSETS_BASE        = 0x72,
	ADDR_BASE               = 0x73,
	RNGLISTS_BASE           = 0x74,
	// 0x75 reserved
	DWO_NAME                = 0x76,
	REFERENCE               = 0x77,
	RVALUE_REFERENCE        = 0x78,
	MACROS                  = 0x79,
	CALL_ALL_CALLS          = 0x7A,
	CALL_ALL_SOURCE_CALLS   = 0x7B,
	CALL_ALL_TAIL_CALLS     = 0x7C,
	CALL_RETURN_PC          = 0x7D,
	CALL_VALUE              = 0x7E,
	CALL_ORIGIN             = 0x7F,
	CALL_PARAMETER          = 0x80,
	CALL_PC                 = 0x81,
	CALL_TAIL_CALL          = 0x82,
	CALL_TARGET             = 0x83,
	CALL_TARGET_CLOBBERED   = 0x84,
	CALL_DATA_LOCATION      = 0x85,
	CALL_DATA_VALUE         = 0x86,
	NORETURN                = 0x87,
	ALIGNMENT               = 0x88,
	EXPORT_SYMBOLS          = 0x89,
	DELETED                 = 0x8A,
	DEFAULTED               = 0x8B,
	LOCLISTS_BASE           = 0x8C,
};
constexpr u64 AttribName_LO_USER = 0x2000;
constexpr u64 AttribName_HI_USER = 0x3fff;
extern StaticArray<Class, 4> AttribNameClasses[0x8D];

enum class Form : u64 {
	ADDR           = 0x01,
	// 0x02 reserved
	BLOCK2         = 0x03,
	BLOCK4         = 0x04,
	DATA2          = 0x05,
	DATA4          = 0x06,
	DATA8          = 0x07,
	STRING         = 0x08,
	BLOCK          = 0x09,
	BLOCK1         = 0x0A,
	DATA1          = 0x0B,
	FLAG           = 0x0C,
	SDATA          = 0x0D,
	STRP           = 0x0E,
	UDATA          = 0x0F,
	REF_ADDR       = 0x10,
	REF1           = 0x11,
	REF2           = 0x12,
	REF4           = 0x13,
	REF8           = 0x14,
	REF_UDATA      = 0x15,
	INDIRECT       = 0x16,
	SEC_OFFSET     = 0x17, // An offset into a sector. Which sector is context-dependent.
	EXPRLOC        = 0x18,
	FLAG_PRESENT   = 0x19,

	// New in DWARF 5

	STRX           = 0x1A,
	ADDRX          = 0x1B,
	REF_SUP4       = 0x1C,
	STRP_SUP       = 0x1D,
	DATA16         = 0x1E,
	LINE_STRP      = 0x1F,
	REF_SIG8       = 0x20,
	IMPLICIT_CONST = 0x21,
	LOCLISTX       = 0x22,
	RNGLISTX       = 0x23,
	REF_SUP8       = 0x24,
	STRX1          = 0x25,
	STRX2          = 0x26,
	STRX3          = 0x27,
	STRX4          = 0x28,
	ADDRX1         = 0x29,
	ADDRX2         = 0x2A,
	ADDRX3         = 0x2B,
	ADDRX4         = 0x2C,
};
extern StaticArray<Class, 8> FormClasses[0x2D];

} // namespace AzCore::dwarf

namespace AzCore {

void AppendToString(String &string, const az::dwarf::ULEB &uleb);
void AppendToString(String &string, const az::dwarf::SLEB &sleb);
void AppendToString(String &string, dwarf::TAG tag);
void AppendToString(String &string, dwarf::ComputeUnitType value);
void AppendToString(String &string, dwarf::Class value);
void AppendToString(String &string, dwarf::AttribName value);
void AppendToString(String &string, dwarf::Form value);

} // namespace AzCore

#endif // AZCORE_DWARF_DEFINITIONS_HPP