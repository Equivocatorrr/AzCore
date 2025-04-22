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

#define GET_ULEB(dst, src) {\
	ULEB _uleb;\
	if (auto _result = DecodeULEB(src, &cur); _result.isError) {\
		return _result.error;\
	} else {\
		_uleb = _result.value;\
	}\
	(dst) = static_cast<decltype(dst)>(_uleb.value);\
}

#define GET_SLEB(dst, src) {\
	SLEB _uleb;\
	if (auto _result = DecodeSLEB(src, &cur); _result.isError) {\
		return _result.error;\
	} else {\
		_uleb = _result.value;\
	}\
	(dst) = static_cast<decltype(dst)>(_uleb.value);\
}

//             deadbeef
// Big Endian:    de ad be ef
// Little Endian: ef be ad de


namespace AzCore::dwarf {

[[nodiscard]] Result<None_t, String> AbbrevUnit::Parse(Range<u8> debug_abbrev) {
	binary = debug_abbrev;
	i64 cur = 0;
	while (true) {
		i64 startCur = cur;
		AbbrevDecl decl;
		ENSURE_BUFFER(binary, 1, cur);
		GET_ULEB(decl.abbrev_code, binary);
		if (decl.abbrev_code == 0) break;
		GET_ULEB(decl.tag, binary);
		GET_DATA(decl.has_children, binary);
		while (true) {
			AbbrevAttrib attrib;
			GET_ULEB(attrib.name, binary);
			GET_ULEB(attrib.form, binary);
			if ((u64)attrib.name == 0 && (u64)attrib.form == 0) break;
			if (attrib.form == Form::IMPLICIT_CONST) {
				GET_SLEB(attrib.constant, binary);
			}
			decl.attributes.Append(attrib);
		}
		decl.binary = binary.SubRange(startCur, cur-startCur);
		decls.Append(std::move(decl));
	}
	binary.size = cur;
	return None;
}

[[nodiscard]] Result<None_t, String> InfoUnitHeader::Parse(Range<u8> _binary) {
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

[[nodiscard]] Result<None_t, String> InfoUnit::Parse(Range<u8> debug_info, Ptr<AbbrevUnit> abbrev_unit) {
	return None;
}

[[nodiscard]] Result<None_t, String> DebuggerInfo::ParseFromELF(elf::File &file) {
	debug_aranges = file.GetSectionByName(".debug_aranges");
	debug_info = file.GetSectionByName(".debug_info");
	if (debug_info.size == 0) {
		return String("There is no .debug_info section in the file.");
	}
	debug_abbrev = file.GetSectionByName(".debug_abbrev");
	if (debug_abbrev.size == 0) {
		return String("There is no .debug_abbrev section in the file.");
	}
	debug_line = file.GetSectionByName(".debug_line");
	debug_str = file.GetSectionByName(".debug_str");
	debug_line_str = file.GetSectionByName(".debug_line_str");
	debug_rnglists = file.GetSectionByName(".debug_rnglists");

	// Parse abbreviations first since they're needed for parsing debug_info CUs
	i64 cur = 0;
	while (cur < debug_abbrev.size) {
		AbbrevUnit abbrev_unit;
		if (auto result = abbrev_unit.Parse(debug_abbrev.SubRange(cur)); result.isError) {
			return result.error;
		}
		cur += abbrev_unit.binary.size;
		abbrev_units.Append(abbrev_unit);
	}
	return None;
}

} // namespace AzCore::dwarf
