/*
	File: definitions.cpp
	Author: Philip Haynes
*/

#include "definitions.hpp"
#include "../Math/Basic.hpp"

namespace AzCore::dwarf {

Result<ULEB, String> DecodeULEB(Range<u8> binary, i64 *cur) {
	i64 _myCur = 0;
	if (!cur) cur = &_myCur;
	if (*cur >= binary.size) {
		return Stringify("No bytes to decode ULEB in binary (size ", binary.size, ") at offset ", *cur);
	}
	binary = binary.SubRange(*cur);
	i64 minSize = min(binary.size, (i64)LEB128_MAX_BYTES_COUNT);
	ULEB result;
	result.value = 0;
	u32 shift = 0;
	for (i32 i = 0;; i++) {
		u8 byte = binary[i];
		result.binary.Append(byte);
		result.value |= (u64)(byte & 0x7f) << shift;
		if (0 == (byte & 0x80)) {
			break;
		}
		if (i+1 == minSize) {
			return Stringify("ULEB (", result, ") was not terminated, but we ran out of bytes (processed ", i, " bytes in a binary of size ", binary.size, ")");
		}
		shift += 7;
	}
	*cur += result.binary.size;
	return result;
}

ULEB EncodeULEB(u64 value) {
	ULEB result;
	result.value = value;
	do {
		u8 byte = value & 0x7f;
		value >>= 7;
		if (value) {
			byte |= 0x80;
		}
		result.binary.Append(byte);
	} while (value != 0);
	return result;
}


Result<SLEB, String> DecodeSLEB(Range<u8> binary, i64 *cur) {
	i64 _myCur = 0;
	if (!cur) cur = &_myCur;
	if (*cur >= binary.size) {
		return Stringify("No bytes to decode SLEB in binary (size ", binary.size, ") at offset ", *cur);
	}
	binary = binary.SubRange(*cur);
	i64 minSize = min(binary.size, (i64)LEB128_MAX_BYTES_COUNT);
	SLEB result;
	result.value = 0;
	u32 shift = 0;
	for (i32 i = 0;; i++) {
		u8 byte = binary[i];
		result.binary.Append(byte);
		result.value |= (u64)(byte & 0x7f) << shift;
		shift += 7;
		if (0 == (byte & 0x80)) {
			if (shift < 64 && 0 != (byte & 0x40)) {
				// sign extend result
				result.value |= -((i64)1 << shift);
			}
			break;
		}
		if (i+1 == minSize) {
			return Stringify("SLEB (", result, ") was not terminated, but we ran out of bytes (processed ", i, " bytes in a binary of size ", binary.size, ")");
		}
	}
	*cur += result.binary.size;
	return result;
}

SLEB EncodeSLEB(i64 value) {
	SLEB result;
	result.value = value;
	bool more = true;
	while (more) {
		u8 byte = value & 0x7f;
		value >>= 7;
		// The example explicity sign-extends, but this should be an arithmetic shift so we don't have to do that.
		bool byteNegative = 0 != (byte & 0x40);
		if (((value == -1) && byteNegative) || ((value == 0) && !byteNegative)) {
			more = false;
		} else {
			byte |= 0x80;
		}
		result.binary.Append(byte);
	}
	return result;
}

StaticArray<Class, 4> AttribNameClasses[0x8D] = {
	{}, // 0x00
	{ Class::REFERENCE }, // 0x01 SIBLING
	{ Class::EXPRLOC, Class::LOCLIST }, // 0x02 LOCATION
	{ Class::STRING }, // 0x03 NAME
	{}, // 0x04 reserved
	{}, // 0x05 reserved
	{}, // 0x06 reserved
	{}, // 0x07 reserved
	{}, // 0x08 reserved
	{ Class::CONSTANT }, // 0x09 ORDERING
	{}, // 0x0A reserved
	{ Class::CONSTANT, Class::EXPRLOC, Class::REFERENCE }, // 0x0B BYTE_SIZE
	{ Class::CONSTANT, Class::EXPRLOC, Class::REFERENCE }, // 0x0C DWARF3_BIT_OFFSET
	{ Class::CONSTANT, Class::EXPRLOC, Class::REFERENCE }, // 0x0D BIT_SIZE
	{}, // 0x0E reserved
	{}, // 0x0F reserved
	{ Class::LINEPTR }, // 0x10 STMT_LIST
	{ Class::ADDRESS }, // 0x11 LOW_PC
	{ Class::ADDRESS, Class::CONSTANT }, // 0x12 HIGH_PC
	{ Class::CONSTANT }, // 0x13 LANGUAGE
	{}, // 0x14 reserved
	{ Class::REFERENCE }, // 0x15 DISCR
	{ Class::CONSTANT }, // 0x16 DISCR_VALUE
	{ Class::CONSTANT }, // 0x17 VISIBILITY
	{ Class::REFERENCE }, // 0x18 IMPORT
	{ Class::EXPRLOC, Class::LOCLIST, Class::REFERENCE }, // 0x19 STRING_LENGTH
	{ Class::REFERENCE }, // 0x1A COMMON_REFERENCE
	{ Class::STRING }, // 0x1B COMP_DIR
	{ Class::BLOCK, Class::CONSTANT, Class::FLAG }, // 0x1C CONST_VALUE
	{ Class::REFERENCE }, // 0x1D CONTAINING_TYPE
	{ Class::CONSTANT, Class::REFERENCE, Class::FLAG }, // 0x1E DEFAULT_VALUE
	{}, // 0x1F reserved
	{ Class::CONSTANT }, // 0x20 INLINE
	{ Class::FLAG }, // 0x21 IS_OPTIONAL
	{ Class::CONSTANT, Class::EXPRLOC, Class::REFERENCE }, // 0x22 LOWER_BOUND
	{}, // 0x23 reserved
	{}, // 0x24 reserved
	{ Class::STRING }, // 0x25 PRODUCER
	{}, // 0x26 reserved
	{ Class::FLAG }, // 0x27 PROTOTYPED
	{}, // 0x28 reserved
	{}, // 0x29 reserved
	{ Class::EXPRLOC, Class::LOCLIST }, // 0x2A RETURN_ADDR
	{}, // 0x2B reserved
	{ Class::CONSTANT, Class::RNGLIST }, // 0x2C START_SCOPE
	{}, // 0x2D reserved
	{ Class::CONSTANT, Class::EXPRLOC, Class::REFERENCE }, // 0x2E BIT_STRIDE
	{ Class::CONSTANT, Class::EXPRLOC, Class::REFERENCE }, // 0x2F UPPER_BOUND
	{}, // 0x30 reserved
	{ Class::REFERENCE }, // 0x31 ABSTRACT_ORIGIN
	{ Class::CONSTANT }, // 0x32 ACCESSIBILITY
	{ Class::CONSTANT }, // 0x33 ADDRESS_CLASS
	{ Class::FLAG }, // 0x34 ARTIFICIAL
	{ Class::REFERENCE }, // 0x35 BASE_TYPES
	{ Class::CONSTANT }, // 0x36 CALLING_CONVENTION
	{ Class::CONSTANT, Class::EXPRLOC, Class::REFERENCE }, // 0x37 COUNT
	{ Class::CONSTANT, Class::EXPRLOC, Class::LOCLIST }, // 0x38 DATA_MEMBER_LOCATION
	{ Class::CONSTANT }, // 0x39 DECL_COLUMN
	{ Class::CONSTANT }, // 0x3A DECL_FILE
	{ Class::CONSTANT }, // 0x3B DECL_LINE
	{ Class::FLAG }, // 0x3C DECLARATION
	{ Class::BLOCK }, // 0x3D DISCR_LIST
	{ Class::CONSTANT }, // 0x3E ENCODING
	{ Class::FLAG }, // 0x3F EXTERNAL
	{ Class::EXPRLOC, Class::LOCLIST }, // 0x40 FRAME_BASE
	{ Class::REFERENCE }, // 0x41 FRIEND
	{ Class::CONSTANT }, // 0x42 IDENTIFIER_CASE
	{ Class::MACPTR }, // 0x43 DWARF4_MACRO_INFO
	{ Class::REFERENCE }, // 0x44 NAMELIST_ITEM
	{ Class::REFERENCE }, // 0x45 PRIORITY
	{ Class::EXPRLOC, Class::LOCLIST }, // 0x46 SEGMENT
	{ Class::REFERENCE }, // 0x47 SPECIFICATION
	{ Class::EXPRLOC, Class::LOCLIST }, // 0x48 STATIC_LINK
	{ Class::REFERENCE }, // 0x49 TYPE
	{ Class::EXPRLOC, Class::LOCLIST }, // 0x4A USE_LOCATION
	{ Class::FLAG }, // 0x4B VARIABLE_PARAMETER
	{ Class::CONSTANT }, // 0x4C VIRTUALITY
	{ Class::EXPRLOC, Class::LOCLIST }, // 0x4D VTABLE_ELEM_LOCATION
	{ Class::CONSTANT, Class::EXPRLOC, Class::REFERENCE }, // 0x4E ALLOCATED
	{ Class::CONSTANT, Class::EXPRLOC, Class::REFERENCE }, // 0x4F ASSOCIATED
	{ Class::EXPRLOC }, // 0x50 DATA_LOCATION
	{ Class::CONSTANT, Class::EXPRLOC, Class::REFERENCE }, // 0x51 BYTE_STRIDE
	{ Class::ADDRESS, Class::CONSTANT }, // 0x52 ENTRY_PC
	{ Class::FLAG }, // 0x53 USE_UTF8
	{ Class::REFERENCE }, // 0x54 EXTENSION
	{ Class::RNGLIST }, // 0x55 RANGES
	{ Class::ADDRESS, Class::FLAG, Class::REFERENCE, Class::STRING }, // 0x56 TRAMPOLINE
	{ Class::CONSTANT }, // 0x57 CALL_COLUMN
	{ Class::CONSTANT }, // 0x58 CALL_FILE
	{ Class::CONSTANT }, // 0x59 CALL_LINE
	{ Class::STRING }, // 0x5A DESCRIPTION
	{ Class::CONSTANT }, // 0x5B BINARY_SCALE
	{ Class::CONSTANT }, // 0x5C DECIMAL_SCALE
	{ Class::REFERENCE }, // 0x5D SMALL
	{ Class::CONSTANT }, // 0x5E DECIMAL_SIGN
	{ Class::CONSTANT }, // 0x5F DIGIT_COUNT
	{ Class::STRING }, // 0x60 PICTURE_STRING
	{ Class::FLAG }, // 0x61 MUTABLE
	{ Class::FLAG }, // 0x62 THREADS_SCALED
	{ Class::FLAG }, // 0x63 EXPLICIT
	{ Class::REFERENCE }, // 0x64 OBJECT_POINTER
	{ Class::CONSTANT }, // 0x65 ENDIANITY
	{ Class::FLAG }, // 0x66 ELEMENTAL
	{ Class::FLAG }, // 0x67 PURE
	{ Class::FLAG }, // 0x68 RECURSIVE
	{ Class::REFERENCE }, // 0x69 SIGNATURE
	{ Class::FLAG }, // 0x6A MAIN_SUBPROGRAM
	{ Class::CONSTANT }, // 0x6B DATA_BIT_OFFSET
	{ Class::FLAG }, // 0x6C CONST_EXPR
	{ Class::FLAG }, // 0x6D ENUM_CLASS
	{ Class::STRING }, // 0x6E LINKAGE_NAME
	{ Class::CONSTANT }, // 0x6F STRING_LENGTH_BIT_SIZE
	{ Class::CONSTANT }, // 0x70 STRING_LENGTH_BYTE_SIZE
	{ Class::CONSTANT, Class::EXPRLOC }, // 0x71 RANK
	{ Class::STROFFSETSPTR }, // 0x72 STR_OFFSETS_BASE
	{ Class::ADDRPTR }, // 0x73 ADDR_BASE
	{ Class::RNGLISTPTR }, // 0x74 RNGLISTS_BASE
	{}, // 0x75 reserved
	{ Class::STRING }, // 0x76 DWO_NAME
	{ Class::FLAG }, // 0x77 REFERENCE
	{ Class::FLAG }, // 0x78 RVALUE_REFERENCE
	{ Class::MACPTR }, // 0x79 MACROS
	{ Class::FLAG }, // 0x7A CALL_ALL_CALLS
	{ Class::FLAG }, // 0x7B CALL_ALL_SOURCE_CALLS
	{ Class::FLAG }, // 0x7C CALL_ALL_TAIL_CALLS
	{ Class::ADDRESS }, // 0x7D CALL_RETURN_PC
	{ Class::EXPRLOC }, // 0x7E CALL_VALUE
	{ Class::EXPRLOC }, // 0x7F CALL_ORIGIN
	{ Class::REFERENCE }, // 0x80 CALL_PARAMETER
	{ Class::ADDRESS }, // 0x81 CALL_PC
	{ Class::FLAG }, // 0x82 CALL_TAIL_CALL
	{ Class::EXPRLOC }, // 0x83 CALL_TARGET
	{ Class::EXPRLOC }, // 0x84 CALL_TARGET_CLOBBERED
	{ Class::EXPRLOC }, // 0x85 CALL_DATA_LOCATION
	{ Class::EXPRLOC }, // 0x86 CALL_DATA_VALUE
	{ Class::FLAG }, // 0x87 NORETURN
	{ Class::CONSTANT }, // 0x88 ALIGNMENT
	{ Class::FLAG }, // 0x89 EXPORT_SYMBOLS
	{ Class::FLAG }, // 0x8A DELETED
	{ Class::CONSTANT }, // 0x8B DEFAULTED
	{ Class::LOCLISTPTR }, // 0x8C LOCLISTS_BASE
};

StaticArray<Class, 8> FormClasses[0x2D] = {
	{}, // 0x00
	{ Class::ADDRESS }, // 0x01 ADDR
	{}, // 0x02 reserved
	{ Class::BLOCK }, // 0x03 BLOCK2
	{ Class::BLOCK }, // 0x04 BLOCK4
	{ Class::CONSTANT }, // 0x05 DATA2
	{ Class::CONSTANT }, // 0x06 DATA4
	{ Class::CONSTANT }, // 0x07 DATA8
	{ Class::STRING }, // 0x08 STRING
	{ Class::BLOCK }, // 0x09 BLOCK
	{ Class::BLOCK }, // 0x0A BLOCK1
	{ Class::CONSTANT }, // 0x0B DATA1
	{ Class::FLAG }, // 0x0C FLAG
	{ Class::CONSTANT }, // 0x0D SDATA
	{ Class::STRING }, // 0x0E STRP
	{ Class::CONSTANT }, // 0x0F UDATA
	{ Class::REFERENCE }, // 0x10 REF_ADDR
	{ Class::REFERENCE }, // 0x11 REF1
	{ Class::REFERENCE }, // 0x12 REF2
	{ Class::REFERENCE }, // 0x13 REF4
	{ Class::REFERENCE }, // 0x14 REF8
	{ Class::REFERENCE }, // 0x15 REF_UDATA
	{ /* Just the specialest little boy */ }, // 0x16 INDIRECT
	{ Class::ADDRPTR, Class::LINEPTR, Class::LOCLIST, Class::LOCLISTPTR, Class::MACPTR, Class::RNGLIST, Class::RNGLISTPTR, Class::STROFFSETSPTR}, // 0x17 SEC_OFFSET
	{ Class::EXPRLOC }, // 0x18 EXPRLOC
	{ Class::FLAG }, // 0x19 FLAG_PRESENT
	{ Class::STRING }, // 0x1A STRX
	{ Class::ADDRESS }, // 0x1B ADDRX
	{ Class::REFERENCE }, // 0x1C REF_SUP4
	{ Class::STRING }, // 0x1D STRP_SUP
	{ Class::CONSTANT }, // 0x1E DATA16
	{ Class::STRING }, // 0x1F LINE_STRP
	{ Class::REFERENCE }, // 0x20 REF_SIG8
	{ Class::CONSTANT }, // 0x21 IMPLICIT_CONST
	{ Class::LOCLIST }, // 0x22 LOCLISTX
	{ Class::RNGLIST }, // 0x23 RNGLISTX
	{ Class::REFERENCE}, // 0x24 REF_SUP8
	{ Class::STRING }, // 0x25 STRX1
	{ Class::STRING }, // 0x26 STRX2
	{ Class::STRING }, // 0x27 STRX3
	{ Class::STRING }, // 0x28 STRX4
	{ Class::ADDRESS }, // 0x29 ADDRX1
	{ Class::ADDRESS }, // 0x2A ADDRX2
	{ Class::ADDRESS }, // 0x2B ADDRX3
	{ Class::ADDRESS }, // 0x2C ADDRX4
};

} // namespace AzCore::dwarf

namespace AzCore {

void AppendToString(String &string, const az::dwarf::ULEB &uleb) {
	AppendToString(string, uleb.value);
	AppendToString(string, " [ ");
	for (i32 i = 0; i < uleb.binary.size; i++) {
		AppendToString(string, FormatInt(uleb.binary[i], 16));
		string.Append(' ');
	}
	string.Append(']');
}

void AppendToString(String &string, const az::dwarf::SLEB &sleb) {
	if (sleb.value >= 0) string.Append(' ');
	AppendToString(string, sleb.value);
	AppendToString(string, " [ ");
	for (i32 i = 0; i < sleb.binary.size; i++) {
		if (sleb.binary[i] < 16) string.Append('0');
		AppendToString(string, FormatInt(sleb.binary[i], 16));
		string.Append(' ');
	}
	string.Append(']');
}

void AppendToString(String &string, dwarf::TAG tag) {
	switch (tag) {
		case dwarf::TAG::ARRAY_TYPE:
			AppendToString(string, "ARRAY_TYPE");
			break;
		case dwarf::TAG::CLASS_TYPE:
			AppendToString(string, "CLASS_TYPE");
			break;
		case dwarf::TAG::ENTRY_POINT:
			AppendToString(string, "ENTRY_POINT");
			break;
		case dwarf::TAG::ENUMERATION_TYPE:
			AppendToString(string, "ENUMERATION_TYPE");
			break;
		case dwarf::TAG::FORMAL_PARAMETER:
			AppendToString(string, "FORMAL_PARAMETER");
			break;
		case dwarf::TAG::IMPORTED_DECLARATION:
			AppendToString(string, "IMPORTED_DECLARATION");
			break;
		case dwarf::TAG::LABEL:
			AppendToString(string, "LABEL");
			break;
		case dwarf::TAG::LEXICAL_BLOCK:
			AppendToString(string, "LEXICAL_BLOCK");
			break;
		case dwarf::TAG::MEMBER:
			AppendToString(string, "MEMBER");
			break;
		case dwarf::TAG::POINTER_TYPE:
			AppendToString(string, "POINTER_TYPE");
			break;
		case dwarf::TAG::REFERENCE_TYPE:
			AppendToString(string, "REFERENCE_TYPE");
			break;
		case dwarf::TAG::COMPILE_UNIT:
			AppendToString(string, "COMPILE_UNIT");
			break;
		case dwarf::TAG::STRING_TYPE:
			AppendToString(string, "STRING_TYPE");
			break;
		case dwarf::TAG::STRUCTURE_TYPE:
			AppendToString(string, "STRUCTURE_TYPE");
			break;
		case dwarf::TAG::SUBROUTINE_TYPE:
			AppendToString(string, "SUBROUTINE_TYPE");
			break;
		case dwarf::TAG::TYPEDEF:
			AppendToString(string, "TYPEDEF");
			break;
		case dwarf::TAG::UNION_TYPE:
			AppendToString(string, "UNION_TYPE");
			break;
		case dwarf::TAG::UNSPECIFIED_PARAMETERS:
			AppendToString(string, "UNSPECIFIED_PARAMETERS");
			break;
		case dwarf::TAG::VARIANT:
			AppendToString(string, "VARIANT");
			break;
		case dwarf::TAG::COMMON_BLOCK:
			AppendToString(string, "COMMON_BLOCK");
			break;
		case dwarf::TAG::COMMON_INCLUSION:
			AppendToString(string, "COMMON_INCLUSION");
			break;
		case dwarf::TAG::INHERITANCE:
			AppendToString(string, "INHERITANCE");
			break;
		case dwarf::TAG::INLINED_SUBROUTINE:
			AppendToString(string, "INLINED_SUBROUTINE");
			break;
		case dwarf::TAG::MODULE:
			AppendToString(string, "MODULE");
			break;
		case dwarf::TAG::PTR_TO_MEMBER_TYPE:
			AppendToString(string, "PTR_TO_MEMBER_TYPE");
			break;
		case dwarf::TAG::SET_TYPE:
			AppendToString(string, "SET_TYPE");
			break;
		case dwarf::TAG::SUBRANGE_TYPE:
			AppendToString(string, "SUBRANGE_TYPE");
			break;
		case dwarf::TAG::WITH_STMT:
			AppendToString(string, "WITH_STMT");
			break;
		case dwarf::TAG::ACCESS_DECLARATION:
			AppendToString(string, "ACCESS_DECLARATION");
			break;
		case dwarf::TAG::BASE_TYPE:
			AppendToString(string, "BASE_TYPE");
			break;
		case dwarf::TAG::CATCH_BLOCK:
			AppendToString(string, "CATCH_BLOCK");
			break;
		case dwarf::TAG::CONST_TYPE:
			AppendToString(string, "CONST_TYPE");
			break;
		case dwarf::TAG::CONSTANT:
			AppendToString(string, "CONSTANT");
			break;
		case dwarf::TAG::ENUMERATOR:
			AppendToString(string, "ENUMERATOR");
			break;
		case dwarf::TAG::FILE_TYPE:
			AppendToString(string, "FILE_TYPE");
			break;
		case dwarf::TAG::FRIEND:
			AppendToString(string, "FRIEND");
			break;
		case dwarf::TAG::NAMELIST:
			AppendToString(string, "NAMELIST");
			break;
		case dwarf::TAG::NAMELIST_ITEM:
			AppendToString(string, "NAMELIST_ITEM");
			break;
		case dwarf::TAG::PACKED_TYPE:
			AppendToString(string, "PACKED_TYPE");
			break;
		case dwarf::TAG::SUBPROGRAM:
			AppendToString(string, "SUBPROGRAM");
			break;
		case dwarf::TAG::TEMPLATE_TYPE_PARAMETER:
			AppendToString(string, "TEMPLATE_TYPE_PARAMETER");
			break;
		case dwarf::TAG::TEMPLATE_VALUE_PARAMETER:
			AppendToString(string, "TEMPLATE_VALUE_PARAMETER");
			break;
		case dwarf::TAG::THROWN_TYPE:
			AppendToString(string, "THROWN_TYPE");
			break;
		case dwarf::TAG::TRY_BLOCK:
			AppendToString(string, "TRY_BLOCK");
			break;
		case dwarf::TAG::VARIANT_PART:
			AppendToString(string, "VARIANT_PART");
			break;
		case dwarf::TAG::VARIABLE:
			AppendToString(string, "VARIABLE");
			break;
		case dwarf::TAG::VOLATILE_TYPE:
			AppendToString(string, "VOLATILE_TYPE");
			break;
		case dwarf::TAG::DWARF_PROCEDURE:
			AppendToString(string, "DWARF_PROCEDURE");
			break;
		case dwarf::TAG::RESTRICT_TYPE:
			AppendToString(string, "RESTRICT_TYPE");
			break;
		case dwarf::TAG::INTERFACE_TYPE:
			AppendToString(string, "INTERFACE_TYPE");
			break;
		case dwarf::TAG::NAMESPACE:
			AppendToString(string, "NAMESPACE");
			break;
		case dwarf::TAG::IMPORTED_MODULE:
			AppendToString(string, "IMPORTED_MODULE");
			break;
		case dwarf::TAG::UNSPECIFIED_TYPE:
			AppendToString(string, "UNSPECIFIED_TYPE");
			break;
		case dwarf::TAG::PARTIAL_UNIT:
			AppendToString(string, "PARTIAL_UNIT");
			break;
		case dwarf::TAG::IMPORTED_UNIT:
			AppendToString(string, "IMPORTED_UNIT");
			break;
		case dwarf::TAG::DWARF3_MUTABLE_TYPE:
			AppendToString(string, "DWARF3_MUTABLE_TYPE");
			break;
		case dwarf::TAG::CONDITION:
			AppendToString(string, "CONDITION");
			break;
		case dwarf::TAG::SHARED_TYPE:
			AppendToString(string, "SHARED_TYPE");
			break;
		case dwarf::TAG::TYPE_UNIT:
			AppendToString(string, "TYPE_UNIT");
			break;
		case dwarf::TAG::RVALUE_REFERENCE_TYPE:
			AppendToString(string, "RVALUE_REFERENCE_TYPE");
			break;
		case dwarf::TAG::TEMPLATE_ALIAS:
			AppendToString(string, "TEMPLATE_ALIAS");
			break;
		case dwarf::TAG::COARRAY_TYPE:
			AppendToString(string, "COARRAY_TYPE");
			break;
		case dwarf::TAG::GENERIC_SUBRANGE:
			AppendToString(string, "GENERIC_SUBRANGE");
			break;
		case dwarf::TAG::DYNAMIC_TYPE:
			AppendToString(string, "DYNAMIC_TYPE");
			break;
		case dwarf::TAG::ATOMIC_TYPE:
			AppendToString(string, "ATOMIC_TYPE");
			break;
		case dwarf::TAG::CALL_SITE:
			AppendToString(string, "CALL_SITE");
			break;
		case dwarf::TAG::CALL_SITE_PARAMETER:
			AppendToString(string, "CALL_SITE_PARAMETER");
			break;
		case dwarf::TAG::SKELETON_UNIT:
			AppendToString(string, "SKELETON_UNIT");
			break;
		case dwarf::TAG::IMMUTABLE_TYPE:
			AppendToString(string, "IMMUTABLE_TYPE");
			break;
		default:
			AppendToString(string, FormatInt((u64)tag, 16, true));
			break;
	}
}

void AppendToString(String &string, dwarf::ComputeUnitType value) {
	switch (value) {
		case dwarf::ComputeUnitType::COMPILE:
			AppendToString(string, "COMPILE");
			break;
		case dwarf::ComputeUnitType::TYPE:
			AppendToString(string, "TYPE");
			break;
		case dwarf::ComputeUnitType::PARTIAL:
			AppendToString(string, "PARTIAL");
			break;
		case dwarf::ComputeUnitType::SKELETON:
			AppendToString(string, "SKELETON");
			break;
		case dwarf::ComputeUnitType::SPLIT_COMPILE:
			AppendToString(string, "SPLIT_COMPILE");
			break;
		case dwarf::ComputeUnitType::SPLIT_TYPE:
			AppendToString(string, "SPLIT_TYPE");
			break;
		default:
			AppendToString(string, FormatInt((u32)value, 16, true));
			break;
	}
}

void AppendToString(String &string, dwarf::Class value) {
	switch (value) {
		case dwarf::Class::ADDRESS:
			AppendToString(string, "ADDRESS");
			break;
		case dwarf::Class::ADDRPTR:
			AppendToString(string, "ADDRPTR");
			break;
		case dwarf::Class::BLOCK:
			AppendToString(string, "BLOCK");
			break;
		case dwarf::Class::CONSTANT:
			AppendToString(string, "CONSTANT");
			break;
		case dwarf::Class::EXPRLOC:
			AppendToString(string, "EXPRLOC");
			break;
		case dwarf::Class::FLAG:
			AppendToString(string, "FLAG");
			break;
		case dwarf::Class::LINEPTR:
			AppendToString(string, "LINEPTR");
			break;
		case dwarf::Class::LOCLIST:
			AppendToString(string, "LOCLIST");
			break;
		case dwarf::Class::LOCLISTPTR:
			AppendToString(string, "LOCLISTPTR");
			break;
		case dwarf::Class::MACPTR:
			AppendToString(string, "MACPTR");
			break;
		case dwarf::Class::RNGLIST:
			AppendToString(string, "RNGLIST");
			break;
		case dwarf::Class::RNGLISTPTR:
			AppendToString(string, "RNGLISTPTR");
			break;
		case dwarf::Class::REFERENCE:
			AppendToString(string, "REFERENCE");
			break;
		case dwarf::Class::STRING:
			AppendToString(string, "STRING");
			break;
		case dwarf::Class::STROFFSETSPTR:
			AppendToString(string, "STROFFSETSPTR");
			break;
		default:
			AppendToString(string, FormatInt((u32)value, 16, true));
			break;
	}
}

void AppendToString(String &string, dwarf::AttribName value) {
	switch (value) {
		case dwarf::AttribName::SIBLING:
			AppendToString(string, "SIBLING");
			break;
		case dwarf::AttribName::LOCATION:
			AppendToString(string, "LOCATION");
			break;
		case dwarf::AttribName::NAME:
			AppendToString(string, "NAME");
			break;
		case dwarf::AttribName::ORDERING:
			AppendToString(string, "ORDERING");
			break;
		case dwarf::AttribName::BYTE_SIZE:
			AppendToString(string, "BYTE_SIZE");
			break;
		case dwarf::AttribName::DWARF3_BIT_OFFSET:
			AppendToString(string, "DWARF3_BIT_OFFSET");
			break;
		case dwarf::AttribName::BIT_SIZE:
			AppendToString(string, "BIT_SIZE");
			break;
		case dwarf::AttribName::STMT_LIST:
			AppendToString(string, "STMT_LIST");
			break;
		case dwarf::AttribName::LOW_PC:
			AppendToString(string, "LOW_PC");
			break;
		case dwarf::AttribName::HIGH_PC:
			AppendToString(string, "HIGH_PC");
			break;
		case dwarf::AttribName::LANGUAGE:
			AppendToString(string, "LANGUAGE");
			break;
		case dwarf::AttribName::DISCR:
			AppendToString(string, "DISCR");
			break;
		case dwarf::AttribName::DISCR_VALUE:
			AppendToString(string, "DISCR_VALUE");
			break;
		case dwarf::AttribName::VISIBILITY:
			AppendToString(string, "VISIBILITY");
			break;
		case dwarf::AttribName::IMPORT:
			AppendToString(string, "IMPORT");
			break;
		case dwarf::AttribName::STRING_LENGTH:
			AppendToString(string, "STRING_LENGTH");
			break;
		case dwarf::AttribName::COMMON_REFERENCE:
			AppendToString(string, "COMMON_REFERENCE");
			break;
		case dwarf::AttribName::COMP_DIR:
			AppendToString(string, "COMP_DIR");
			break;
		case dwarf::AttribName::CONST_VALUE:
			AppendToString(string, "CONST_VALUE");
			break;
		case dwarf::AttribName::CONTAINING_TYPE:
			AppendToString(string, "CONTAINING_TYPE");
			break;
		case dwarf::AttribName::DEFAULT_VALUE:
			AppendToString(string, "DEFAULT_VALUE");
			break;
		case dwarf::AttribName::INLINE:
			AppendToString(string, "INLINE");
			break;
		case dwarf::AttribName::IS_OPTIONAL:
			AppendToString(string, "IS_OPTIONAL");
			break;
		case dwarf::AttribName::LOWER_BOUND:
			AppendToString(string, "LOWER_BOUND");
			break;
		case dwarf::AttribName::PRODUCER:
			AppendToString(string, "PRODUCER");
			break;
		case dwarf::AttribName::PROTOTYPED:
			AppendToString(string, "PROTOTYPED");
			break;
		case dwarf::AttribName::RETURN_ADDR:
			AppendToString(string, "RETURN_ADDR");
			break;
		case dwarf::AttribName::START_SCOPE:
			AppendToString(string, "START_SCOPE");
			break;
		case dwarf::AttribName::BIT_STRIDE:
			AppendToString(string, "BIT_STRIDE");
			break;
		case dwarf::AttribName::UPPER_BOUND:
			AppendToString(string, "UPPER_BOUND");
			break;
		case dwarf::AttribName::ABSTRACT_ORIGIN:
			AppendToString(string, "ABSTRACT_ORIGIN");
			break;
		case dwarf::AttribName::ACCESSIBILITY:
			AppendToString(string, "ACCESSIBILITY");
			break;
		case dwarf::AttribName::ADDRESS_CLASS:
			AppendToString(string, "ADDRESS_CLASS");
			break;
		case dwarf::AttribName::ARTIFICIAL:
			AppendToString(string, "ARTIFICIAL");
			break;
		case dwarf::AttribName::BASE_TYPES:
			AppendToString(string, "BASE_TYPES");
			break;
		case dwarf::AttribName::CALLING_CONVENTION:
			AppendToString(string, "CALLING_CONVENTION");
			break;
		case dwarf::AttribName::COUNT:
			AppendToString(string, "COUNT");
			break;
		case dwarf::AttribName::DATA_MEMBER_LOCATION:
			AppendToString(string, "DATA_MEMBER_LOCATION");
			break;
		case dwarf::AttribName::DECL_COLUMN:
			AppendToString(string, "DECL_COLUMN");
			break;
		case dwarf::AttribName::DECL_FILE:
			AppendToString(string, "DECL_FILE");
			break;
		case dwarf::AttribName::DECL_LINE:
			AppendToString(string, "DECL_LINE");
			break;
		case dwarf::AttribName::DECLARATION:
			AppendToString(string, "DECLARATION");
			break;
		case dwarf::AttribName::DISCR_LIST:
			AppendToString(string, "DISCR_LIST");
			break;
		case dwarf::AttribName::ENCODING:
			AppendToString(string, "ENCODING");
			break;
		case dwarf::AttribName::EXTERNAL:
			AppendToString(string, "EXTERNAL");
			break;
		case dwarf::AttribName::FRAME_BASE:
			AppendToString(string, "FRAME_BASE");
			break;
		case dwarf::AttribName::FRIEND:
			AppendToString(string, "FRIEND");
			break;
		case dwarf::AttribName::IDENTIFIER_CASE:
			AppendToString(string, "IDENTIFIER_CASE");
			break;
		case dwarf::AttribName::DWARF4_MACRO_INFO:
			AppendToString(string, "DWARF4_MACRO_INFO");
			break;
		case dwarf::AttribName::NAMELIST_ITEM:
			AppendToString(string, "NAMELIST_ITEM");
			break;
		case dwarf::AttribName::PRIORITY:
			AppendToString(string, "PRIORITY");
			break;
		case dwarf::AttribName::SEGMENT:
			AppendToString(string, "SEGMENT");
			break;
		case dwarf::AttribName::SPECIFICATION:
			AppendToString(string, "SPECIFICATION");
			break;
		case dwarf::AttribName::STATIC_LINK:
			AppendToString(string, "STATIC_LINK");
			break;
		case dwarf::AttribName::TYPE:
			AppendToString(string, "TYPE");
			break;
		case dwarf::AttribName::USE_LOCATION:
			AppendToString(string, "USE_LOCATION");
			break;
		case dwarf::AttribName::VARIABLE_PARAMETER:
			AppendToString(string, "VARIABLE_PARAMETER");
			break;
		case dwarf::AttribName::VIRTUALITY:
			AppendToString(string, "VIRTUALITY");
			break;
		case dwarf::AttribName::VTABLE_ELEM_LOCATION:
			AppendToString(string, "VTABLE_ELEM_LOCATION");
			break;
		case dwarf::AttribName::ALLOCATED:
			AppendToString(string, "ALLOCATED");
			break;
		case dwarf::AttribName::ASSOCIATED:
			AppendToString(string, "ASSOCIATED");
			break;
		case dwarf::AttribName::DATA_LOCATION:
			AppendToString(string, "DATA_LOCATION");
			break;
		case dwarf::AttribName::BYTE_STRIDE:
			AppendToString(string, "BYTE_STRIDE");
			break;
		case dwarf::AttribName::ENTRY_PC:
			AppendToString(string, "ENTRY_PC");
			break;
		case dwarf::AttribName::USE_UTF8:
			AppendToString(string, "USE_UTF8");
			break;
		case dwarf::AttribName::EXTENSION:
			AppendToString(string, "EXTENSION");
			break;
		case dwarf::AttribName::RANGES:
			AppendToString(string, "RANGES");
			break;
		case dwarf::AttribName::TRAMPOLINE:
			AppendToString(string, "TRAMPOLINE");
			break;
		case dwarf::AttribName::CALL_COLUMN:
			AppendToString(string, "CALL_COLUMN");
			break;
		case dwarf::AttribName::CALL_FILE:
			AppendToString(string, "CALL_FILE");
			break;
		case dwarf::AttribName::CALL_LINE:
			AppendToString(string, "CALL_LINE");
			break;
		case dwarf::AttribName::DESCRIPTION:
			AppendToString(string, "DESCRIPTION");
			break;
		case dwarf::AttribName::BINARY_SCALE:
			AppendToString(string, "BINARY_SCALE");
			break;
		case dwarf::AttribName::DECIMAL_SCALE:
			AppendToString(string, "DECIMAL_SCALE");
			break;
		case dwarf::AttribName::SMALL:
			AppendToString(string, "SMALL");
			break;
		case dwarf::AttribName::DECIMAL_SIGN:
			AppendToString(string, "DECIMAL_SIGN");
			break;
		case dwarf::AttribName::DIGIT_COUNT:
			AppendToString(string, "DIGIT_COUNT");
			break;
		case dwarf::AttribName::PICTURE_STRING:
			AppendToString(string, "PICTURE_STRING");
			break;
		case dwarf::AttribName::MUTABLE:
			AppendToString(string, "MUTABLE");
			break;
		case dwarf::AttribName::THREADS_SCALED:
			AppendToString(string, "THREADS_SCALED");
			break;
		case dwarf::AttribName::EXPLICIT:
			AppendToString(string, "EXPLICIT");
			break;
		case dwarf::AttribName::OBJECT_POINTER:
			AppendToString(string, "OBJECT_POINTER");
			break;
		case dwarf::AttribName::ENDIANITY:
			AppendToString(string, "ENDIANITY");
			break;
		case dwarf::AttribName::ELEMENTAL:
			AppendToString(string, "ELEMENTAL");
			break;
		case dwarf::AttribName::PURE:
			AppendToString(string, "PURE");
			break;
		case dwarf::AttribName::RECURSIVE:
			AppendToString(string, "RECURSIVE");
			break;
		case dwarf::AttribName::SIGNATURE:
			AppendToString(string, "SIGNATURE");
			break;
		case dwarf::AttribName::MAIN_SUBPROGRAM:
			AppendToString(string, "MAIN_SUBPROGRAM");
			break;
		case dwarf::AttribName::DATA_BIT_OFFSET:
			AppendToString(string, "DATA_BIT_OFFSET");
			break;
		case dwarf::AttribName::CONST_EXPR:
			AppendToString(string, "CONST_EXPR");
			break;
		case dwarf::AttribName::ENUM_CLASS:
			AppendToString(string, "ENUM_CLASS");
			break;
		case dwarf::AttribName::LINKAGE_NAME:
			AppendToString(string, "LINKAGE_NAME");
			break;
		case dwarf::AttribName::STRING_LENGTH_BIT_SIZE:
			AppendToString(string, "STRING_LENGTH_BIT_SIZE");
			break;
		case dwarf::AttribName::STRING_LENGTH_BYTE_SIZE:
			AppendToString(string, "STRING_LENGTH_BYTE_SIZE");
			break;
		case dwarf::AttribName::RANK:
			AppendToString(string, "RANK");
			break;
		case dwarf::AttribName::STR_OFFSETS_BASE:
			AppendToString(string, "STR_OFFSETS_BASE");
			break;
		case dwarf::AttribName::ADDR_BASE:
			AppendToString(string, "ADDR_BASE");
			break;
		case dwarf::AttribName::RNGLISTS_BASE:
			AppendToString(string, "RNGLISTS_BASE");
			break;
		case dwarf::AttribName::DWO_NAME:
			AppendToString(string, "DWO_NAME");
			break;
		case dwarf::AttribName::REFERENCE:
			AppendToString(string, "REFERENCE");
			break;
		case dwarf::AttribName::RVALUE_REFERENCE:
			AppendToString(string, "RVALUE_REFERENCE");
			break;
		case dwarf::AttribName::MACROS:
			AppendToString(string, "MACROS");
			break;
		case dwarf::AttribName::CALL_ALL_CALLS:
			AppendToString(string, "CALL_ALL_CALLS");
			break;
		case dwarf::AttribName::CALL_ALL_SOURCE_CALLS:
			AppendToString(string, "CALL_ALL_SOURCE_CALLS");
			break;
		case dwarf::AttribName::CALL_ALL_TAIL_CALLS:
			AppendToString(string, "CALL_ALL_TAIL_CALLS");
			break;
		case dwarf::AttribName::CALL_RETURN_PC:
			AppendToString(string, "CALL_RETURN_PC");
			break;
		case dwarf::AttribName::CALL_VALUE:
			AppendToString(string, "CALL_VALUE");
			break;
		case dwarf::AttribName::CALL_ORIGIN:
			AppendToString(string, "CALL_ORIGIN");
			break;
		case dwarf::AttribName::CALL_PARAMETER:
			AppendToString(string, "CALL_PARAMETER");
			break;
		case dwarf::AttribName::CALL_PC:
			AppendToString(string, "CALL_PC");
			break;
		case dwarf::AttribName::CALL_TAIL_CALL:
			AppendToString(string, "CALL_TAIL_CALL");
			break;
		case dwarf::AttribName::CALL_TARGET:
			AppendToString(string, "CALL_TARGET");
			break;
		case dwarf::AttribName::CALL_TARGET_CLOBBERED:
			AppendToString(string, "CALL_TARGET_CLOBBERED");
			break;
		case dwarf::AttribName::CALL_DATA_LOCATION:
			AppendToString(string, "CALL_DATA_LOCATION");
			break;
		case dwarf::AttribName::CALL_DATA_VALUE:
			AppendToString(string, "CALL_DATA_VALUE");
			break;
		case dwarf::AttribName::NORETURN:
			AppendToString(string, "NORETURN");
			break;
		case dwarf::AttribName::ALIGNMENT:
			AppendToString(string, "ALIGNMENT");
			break;
		case dwarf::AttribName::EXPORT_SYMBOLS:
			AppendToString(string, "EXPORT_SYMBOLS");
			break;
		case dwarf::AttribName::DELETED:
			AppendToString(string, "DELETED");
			break;
		case dwarf::AttribName::DEFAULTED:
			AppendToString(string, "DEFAULTED");
			break;
		case dwarf::AttribName::LOCLISTS_BASE:
			AppendToString(string, "LOCLISTS_BASE");
			break;
		default:
			AppendToString(string, FormatInt((u64)value, 16, true));
			break;
	}
}

void AppendToString(String &string, dwarf::Form value) {
	switch (value) {
		case dwarf::Form::ADDR:
			AppendToString(string, "ADDR");
			break;
		case dwarf::Form::BLOCK2:
			AppendToString(string, "BLOCK2");
			break;
		case dwarf::Form::BLOCK4:
			AppendToString(string, "BLOCK4");
			break;
		case dwarf::Form::DATA2:
			AppendToString(string, "DATA2");
			break;
		case dwarf::Form::DATA4:
			AppendToString(string, "DATA4");
			break;
		case dwarf::Form::DATA8:
			AppendToString(string, "DATA8");
			break;
		case dwarf::Form::STRING:
			AppendToString(string, "STRING");
			break;
		case dwarf::Form::BLOCK:
			AppendToString(string, "BLOCK");
			break;
		case dwarf::Form::BLOCK1:
			AppendToString(string, "BLOCK1");
			break;
		case dwarf::Form::DATA1:
			AppendToString(string, "DATA1");
			break;
		case dwarf::Form::FLAG:
			AppendToString(string, "FLAG");
			break;
		case dwarf::Form::SDATA:
			AppendToString(string, "SDATA");
			break;
		case dwarf::Form::STRP:
			AppendToString(string, "STRP");
			break;
		case dwarf::Form::UDATA:
			AppendToString(string, "UDATA");
			break;
		case dwarf::Form::REF_ADDR:
			AppendToString(string, "REF_ADDR");
			break;
		case dwarf::Form::REF1:
			AppendToString(string, "REF1");
			break;
		case dwarf::Form::REF2:
			AppendToString(string, "REF2");
			break;
		case dwarf::Form::REF4:
			AppendToString(string, "REF4");
			break;
		case dwarf::Form::REF8:
			AppendToString(string, "REF8");
			break;
		case dwarf::Form::REF_UDATA:
			AppendToString(string, "REF_UDATA");
			break;
		case dwarf::Form::INDIRECT:
			AppendToString(string, "INDIRECT");
			break;
		case dwarf::Form::SEC_OFFSET:
			AppendToString(string, "SEC_OFFSET");
			break;
		case dwarf::Form::EXPRLOC:
			AppendToString(string, "EXPRLOC");
			break;
		case dwarf::Form::FLAG_PRESENT:
			AppendToString(string, "FLAG_PRESENT");
			break;
		case dwarf::Form::STRX:
			AppendToString(string, "STRX");
			break;
		case dwarf::Form::ADDRX:
			AppendToString(string, "ADDRX");
			break;
		case dwarf::Form::REF_SUP4:
			AppendToString(string, "REF_SUP4");
			break;
		case dwarf::Form::STRP_SUP:
			AppendToString(string, "STRP_SUP");
			break;
		case dwarf::Form::DATA16:
			AppendToString(string, "DATA16");
			break;
		case dwarf::Form::LINE_STRP:
			AppendToString(string, "LINE_STRP");
			break;
		case dwarf::Form::REF_SIG8:
			AppendToString(string, "REF_SIG8");
			break;
		case dwarf::Form::IMPLICIT_CONST:
			AppendToString(string, "IMPLICIT_CONST");
			break;
		case dwarf::Form::LOCLISTX:
			AppendToString(string, "LOCLISTX");
			break;
		case dwarf::Form::RNGLISTX:
			AppendToString(string, "RNGLISTX");
			break;
		case dwarf::Form::REF_SUP8:
			AppendToString(string, "REF_SUP8");
			break;
		case dwarf::Form::STRX1:
			AppendToString(string, "STRX1");
			break;
		case dwarf::Form::STRX2:
			AppendToString(string, "STRX2");
			break;
		case dwarf::Form::STRX3:
			AppendToString(string, "STRX3");
			break;
		case dwarf::Form::STRX4:
			AppendToString(string, "STRX4");
			break;
		case dwarf::Form::ADDRX1:
			AppendToString(string, "ADDRX1");
			break;
		case dwarf::Form::ADDRX2:
			AppendToString(string, "ADDRX2");
			break;
		case dwarf::Form::ADDRX3:
			AppendToString(string, "ADDRX3");
			break;
		case dwarf::Form::ADDRX4:
			AppendToString(string, "ADDRX4");
			break;
		default:
			AppendToString(string, FormatInt((u64)value, 16, true));
			break;
	}
}


} // namespace AzCore