/*
	File: definitions.cpp
	Author: Philip Haynes
*/

#include "definitions.hpp"
#include "../Math/Basic.hpp"

namespace AzCore::dwarf {

ULEB DecodeULEB(Range<u8> binary) {
	binary.size = min(binary.size, (i64)LEB128_MAX_BYTES_COUNT);
	ULEB result;
	result.value = 0;
	u32 shift = 0;
	for (i32 i = 0; i < binary.size; i++) {
		u8 byte = binary[i];
		result.binary.Append(byte);
		result.value |= (u64)(byte & 0x7f) << shift;
		if (0 == (byte & 0x80)) {
			break;
		}
		shift += 7;
	}
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


SLEB DecodeSLEB(Range<u8> binary) {
	binary.size = min(binary.size, (i64)LEB128_MAX_BYTES_COUNT);
	SLEB result;
	result.value = 0;
	u32 shift = 0;
	for (i32 i = 0; i < binary.size; i++) {
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
	}
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

void AppendToString(String &string, dwarf::Attribute value) {
	switch (value) {
		case dwarf::Attribute::SIBLING:
			AppendToString(string, "SIBLING");
			break;
		case dwarf::Attribute::LOCATION:
			AppendToString(string, "LOCATION");
			break;
		case dwarf::Attribute::NAME:
			AppendToString(string, "NAME");
			break;
		case dwarf::Attribute::ORDERING:
			AppendToString(string, "ORDERING");
			break;
		case dwarf::Attribute::BYTE_SIZE:
			AppendToString(string, "BYTE_SIZE");
			break;
		case dwarf::Attribute::DWARF3_BIT_OFFSET:
			AppendToString(string, "DWARF3_BIT_OFFSET");
			break;
		case dwarf::Attribute::BIT_SIZE:
			AppendToString(string, "BIT_SIZE");
			break;
		case dwarf::Attribute::STMT_LIST:
			AppendToString(string, "STMT_LIST");
			break;
		case dwarf::Attribute::LOW_PC:
			AppendToString(string, "LOW_PC");
			break;
		case dwarf::Attribute::HIGH_PC:
			AppendToString(string, "HIGH_PC");
			break;
		case dwarf::Attribute::LANGUAGE:
			AppendToString(string, "LANGUAGE");
			break;
		case dwarf::Attribute::DISCR:
			AppendToString(string, "DISCR");
			break;
		case dwarf::Attribute::DISCR_VALUE:
			AppendToString(string, "DISCR_VALUE");
			break;
		case dwarf::Attribute::VISIBILITY:
			AppendToString(string, "VISIBILITY");
			break;
		case dwarf::Attribute::IMPORT:
			AppendToString(string, "IMPORT");
			break;
		case dwarf::Attribute::STRING_LENGTH:
			AppendToString(string, "STRING_LENGTH");
			break;
		case dwarf::Attribute::COMMON_REFERENCE:
			AppendToString(string, "COMMON_REFERENCE");
			break;
		case dwarf::Attribute::COMP_DIR:
			AppendToString(string, "COMP_DIR");
			break;
		case dwarf::Attribute::CONST_VALUE:
			AppendToString(string, "CONST_VALUE");
			break;
		case dwarf::Attribute::CONTAINING_TYPE:
			AppendToString(string, "CONTAINING_TYPE");
			break;
		case dwarf::Attribute::DEFAULT_VALUE:
			AppendToString(string, "DEFAULT_VALUE");
			break;
		case dwarf::Attribute::INLINE:
			AppendToString(string, "INLINE");
			break;
		case dwarf::Attribute::IS_OPTIONAL:
			AppendToString(string, "IS_OPTIONAL");
			break;
		case dwarf::Attribute::LOWER_BOUND:
			AppendToString(string, "LOWER_BOUND");
			break;
		case dwarf::Attribute::PRODUCER:
			AppendToString(string, "PRODUCER");
			break;
		case dwarf::Attribute::PROTOTYPED:
			AppendToString(string, "PROTOTYPED");
			break;
		case dwarf::Attribute::RETURN_ADDR:
			AppendToString(string, "RETURN_ADDR");
			break;
		case dwarf::Attribute::START_SCOPE:
			AppendToString(string, "START_SCOPE");
			break;
		case dwarf::Attribute::BIT_STRIDE:
			AppendToString(string, "BIT_STRIDE");
			break;
		case dwarf::Attribute::UPPER_BOUND:
			AppendToString(string, "UPPER_BOUND");
			break;
		case dwarf::Attribute::ABSTRACT_ORIGIN:
			AppendToString(string, "ABSTRACT_ORIGIN");
			break;
		case dwarf::Attribute::ACCESSIBILITY:
			AppendToString(string, "ACCESSIBILITY");
			break;
		case dwarf::Attribute::ADDRESS_CLASS:
			AppendToString(string, "ADDRESS_CLASS");
			break;
		case dwarf::Attribute::ARTIFICIAL:
			AppendToString(string, "ARTIFICIAL");
			break;
		case dwarf::Attribute::BASE_TYPES:
			AppendToString(string, "BASE_TYPES");
			break;
		case dwarf::Attribute::CALLING_CONVENTION:
			AppendToString(string, "CALLING_CONVENTION");
			break;
		case dwarf::Attribute::COUNT:
			AppendToString(string, "COUNT");
			break;
		case dwarf::Attribute::DATA_MEMBER_LOCATION:
			AppendToString(string, "DATA_MEMBER_LOCATION");
			break;
		case dwarf::Attribute::DECL_COLUMN:
			AppendToString(string, "DECL_COLUMN");
			break;
		case dwarf::Attribute::DECL_FILE:
			AppendToString(string, "DECL_FILE");
			break;
		case dwarf::Attribute::DECL_LINE:
			AppendToString(string, "DECL_LINE");
			break;
		case dwarf::Attribute::DECLARATION:
			AppendToString(string, "DECLARATION");
			break;
		case dwarf::Attribute::DISCR_LIST:
			AppendToString(string, "DISCR_LIST");
			break;
		case dwarf::Attribute::ENCODING:
			AppendToString(string, "ENCODING");
			break;
		case dwarf::Attribute::EXTERNAL:
			AppendToString(string, "EXTERNAL");
			break;
		case dwarf::Attribute::FRAME_BASE:
			AppendToString(string, "FRAME_BASE");
			break;
		case dwarf::Attribute::FRIEND:
			AppendToString(string, "FRIEND");
			break;
		case dwarf::Attribute::IDENTIFIER_CASE:
			AppendToString(string, "IDENTIFIER_CASE");
			break;
		case dwarf::Attribute::DWARF4_MACRO_INFO:
			AppendToString(string, "DWARF4_MACRO_INFO");
			break;
		case dwarf::Attribute::NAMELIST_ITEM:
			AppendToString(string, "NAMELIST_ITEM");
			break;
		case dwarf::Attribute::PRIORITY:
			AppendToString(string, "PRIORITY");
			break;
		case dwarf::Attribute::SEGMENT:
			AppendToString(string, "SEGMENT");
			break;
		case dwarf::Attribute::SPECIFICATION:
			AppendToString(string, "SPECIFICATION");
			break;
		case dwarf::Attribute::STATIC_LINK:
			AppendToString(string, "STATIC_LINK");
			break;
		case dwarf::Attribute::TYPE:
			AppendToString(string, "TYPE");
			break;
		case dwarf::Attribute::USE_LOCATION:
			AppendToString(string, "USE_LOCATION");
			break;
		case dwarf::Attribute::VARIABLE_PARAMETER:
			AppendToString(string, "VARIABLE_PARAMETER");
			break;
		case dwarf::Attribute::VIRTUALITY:
			AppendToString(string, "VIRTUALITY");
			break;
		case dwarf::Attribute::VTABLE_ELEM_LOCATION:
			AppendToString(string, "VTABLE_ELEM_LOCATION");
			break;
		case dwarf::Attribute::ALLOCATED:
			AppendToString(string, "ALLOCATED");
			break;
		case dwarf::Attribute::ASSOCIATED:
			AppendToString(string, "ASSOCIATED");
			break;
		case dwarf::Attribute::DATA_LOCATION:
			AppendToString(string, "DATA_LOCATION");
			break;
		case dwarf::Attribute::BYTE_STRIDE:
			AppendToString(string, "BYTE_STRIDE");
			break;
		case dwarf::Attribute::ENTRY_PC:
			AppendToString(string, "ENTRY_PC");
			break;
		case dwarf::Attribute::USE_UTF8:
			AppendToString(string, "USE_UTF8");
			break;
		case dwarf::Attribute::EXTENSION:
			AppendToString(string, "EXTENSION");
			break;
		case dwarf::Attribute::RANGES:
			AppendToString(string, "RANGES");
			break;
		case dwarf::Attribute::TRAMPOLINE:
			AppendToString(string, "TRAMPOLINE");
			break;
		case dwarf::Attribute::CALL_COLUMN:
			AppendToString(string, "CALL_COLUMN");
			break;
		case dwarf::Attribute::CALL_FILE:
			AppendToString(string, "CALL_FILE");
			break;
		case dwarf::Attribute::CALL_LINE:
			AppendToString(string, "CALL_LINE");
			break;
		case dwarf::Attribute::DESCRIPTION:
			AppendToString(string, "DESCRIPTION");
			break;
		case dwarf::Attribute::BINARY_SCALE:
			AppendToString(string, "BINARY_SCALE");
			break;
		case dwarf::Attribute::DECIMAL_SCALE:
			AppendToString(string, "DECIMAL_SCALE");
			break;
		case dwarf::Attribute::SMALL:
			AppendToString(string, "SMALL");
			break;
		case dwarf::Attribute::DECIMAL_SIGN:
			AppendToString(string, "DECIMAL_SIGN");
			break;
		case dwarf::Attribute::DIGIT_COUNT:
			AppendToString(string, "DIGIT_COUNT");
			break;
		case dwarf::Attribute::PICTURE_STRING:
			AppendToString(string, "PICTURE_STRING");
			break;
		case dwarf::Attribute::MUTABLE:
			AppendToString(string, "MUTABLE");
			break;
		case dwarf::Attribute::THREADS_SCALED:
			AppendToString(string, "THREADS_SCALED");
			break;
		case dwarf::Attribute::EXPLICIT:
			AppendToString(string, "EXPLICIT");
			break;
		case dwarf::Attribute::OBJECT_POINTER:
			AppendToString(string, "OBJECT_POINTER");
			break;
		case dwarf::Attribute::ENDIANITY:
			AppendToString(string, "ENDIANITY");
			break;
		case dwarf::Attribute::ELEMENTAL:
			AppendToString(string, "ELEMENTAL");
			break;
		case dwarf::Attribute::PURE:
			AppendToString(string, "PURE");
			break;
		case dwarf::Attribute::RECURSIVE:
			AppendToString(string, "RECURSIVE");
			break;
		case dwarf::Attribute::SIGNATURE:
			AppendToString(string, "SIGNATURE");
			break;
		case dwarf::Attribute::MAIN_SUBPROGRAM:
			AppendToString(string, "MAIN_SUBPROGRAM");
			break;
		case dwarf::Attribute::DATA_BIT_OFFSET:
			AppendToString(string, "DATA_BIT_OFFSET");
			break;
		case dwarf::Attribute::CONST_EXPR:
			AppendToString(string, "CONST_EXPR");
			break;
		case dwarf::Attribute::ENUM_CLASS:
			AppendToString(string, "ENUM_CLASS");
			break;
		case dwarf::Attribute::LINKAGE_NAME:
			AppendToString(string, "LINKAGE_NAME");
			break;
		case dwarf::Attribute::STRING_LENGTH_BIT_SIZE:
			AppendToString(string, "STRING_LENGTH_BIT_SIZE");
			break;
		case dwarf::Attribute::STRING_LENGTH_BYTE_SIZE:
			AppendToString(string, "STRING_LENGTH_BYTE_SIZE");
			break;
		case dwarf::Attribute::RANK:
			AppendToString(string, "RANK");
			break;
		case dwarf::Attribute::STR_OFFSETS_BASE:
			AppendToString(string, "STR_OFFSETS_BASE");
			break;
		case dwarf::Attribute::ADDR_BASE:
			AppendToString(string, "ADDR_BASE");
			break;
		case dwarf::Attribute::RNGLISTS_BASE:
			AppendToString(string, "RNGLISTS_BASE");
			break;
		case dwarf::Attribute::DWO_NAME:
			AppendToString(string, "DWO_NAME");
			break;
		case dwarf::Attribute::REFERENCE:
			AppendToString(string, "REFERENCE");
			break;
		case dwarf::Attribute::RVALUE_REFERENCE:
			AppendToString(string, "RVALUE_REFERENCE");
			break;
		case dwarf::Attribute::MACROS:
			AppendToString(string, "MACROS");
			break;
		case dwarf::Attribute::CALL_ALL_CALLS:
			AppendToString(string, "CALL_ALL_CALLS");
			break;
		case dwarf::Attribute::CALL_ALL_SOURCE_CALLS:
			AppendToString(string, "CALL_ALL_SOURCE_CALLS");
			break;
		case dwarf::Attribute::CALL_ALL_TAIL_CALLS:
			AppendToString(string, "CALL_ALL_TAIL_CALLS");
			break;
		case dwarf::Attribute::CALL_RETURN_PC:
			AppendToString(string, "CALL_RETURN_PC");
			break;
		case dwarf::Attribute::CALL_VALUE:
			AppendToString(string, "CALL_VALUE");
			break;
		case dwarf::Attribute::CALL_ORIGIN:
			AppendToString(string, "CALL_ORIGIN");
			break;
		case dwarf::Attribute::CALL_PARAMETER:
			AppendToString(string, "CALL_PARAMETER");
			break;
		case dwarf::Attribute::CALL_PC:
			AppendToString(string, "CALL_PC");
			break;
		case dwarf::Attribute::CALL_TAIL_CALL:
			AppendToString(string, "CALL_TAIL_CALL");
			break;
		case dwarf::Attribute::CALL_TARGET:
			AppendToString(string, "CALL_TARGET");
			break;
		case dwarf::Attribute::CALL_TARGET_CLOBBERED:
			AppendToString(string, "CALL_TARGET_CLOBBERED");
			break;
		case dwarf::Attribute::CALL_DATA_LOCATION:
			AppendToString(string, "CALL_DATA_LOCATION");
			break;
		case dwarf::Attribute::CALL_DATA_VALUE:
			AppendToString(string, "CALL_DATA_VALUE");
			break;
		case dwarf::Attribute::NORETURN:
			AppendToString(string, "NORETURN");
			break;
		case dwarf::Attribute::ALIGNMENT:
			AppendToString(string, "ALIGNMENT");
			break;
		case dwarf::Attribute::EXPORT_SYMBOLS:
			AppendToString(string, "EXPORT_SYMBOLS");
			break;
		case dwarf::Attribute::DELETED:
			AppendToString(string, "DELETED");
			break;
		case dwarf::Attribute::DEFAULTED:
			AppendToString(string, "DEFAULTED");
			break;
		case dwarf::Attribute::LOCLISTS_BASE:
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