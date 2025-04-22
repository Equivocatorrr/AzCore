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
		return Stringify("No bytes to decode in binary (size ", binary.size, ") at offset ", *cur);
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
		return Stringify("No bytes to decode in binary (size ", binary.size, ") at offset ", *cur);
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