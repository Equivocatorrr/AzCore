/*
	File: dwarf.cpp
	Author: Philip Haynes
*/

#include "dwarf.hpp"
#include "definitions.hpp"
#include "../Utility/TypeName.hpp"

#include <type_traits>

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

// TODO: This only works on little-endian systems
#define GET_DATA_SIZED(dst, src, size)\
	ENSURE_BUFFER(src, (size), cur);\
	AzAssert(sizeof(dst) >= (size), Stringify("Trying to put ", (size), " bytes into a ", TypeName<decltype(dst)>(), " (size ", sizeof(dst), ")"));\
	memset(&(dst), 0, sizeof(dst));\
	memcpy(&(dst), &(src)[cur], (size));\
	cur += (size);

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

#define GET_PTR(dst, src, ptrSize)\
	switch (ptrSize) {\
		case 1: GET_DATA_PROXY(dst, src, u8); break;\
		case 2: GET_DATA_PROXY(dst, src, u16); break;\
		case 4: GET_DATA_PROXY(dst, src, u32); break;\
		case 8: GET_DATA_PROXY(dst, src, u64); break;\
		default: return Stringify("Invalid ptr size: ", (ptrSize));\
	}

#define GET_RANGE(dst, src, bytes)\
	ENSURE_BUFFER((src), (bytes), cur);\
	(dst).data = &(src)[cur];\
	(dst).size = (bytes);\
	cur += (bytes);

namespace AzCore::dwarf {

struct u24 {
	char bytes[3];
	u24() = default;
	template<
		typename T,
		typename = std::enable_if_t<std::is_integral_v<T>>
	>
	constexpr u24(T value) {
		memcpy(bytes, &value, 3);
	}
	template<
		typename T,
		typename = std::enable_if_t<std::is_integral_v<T>>
	>
	constexpr operator T () const {
		T result = 0;
		memcpy(&result, bytes, 3);
		return result;
	}
};
static_assert(sizeof(u24) == 3);

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
		GET_DATA_PROXY(decl.has_children, binary, u8);
		while (true) {
			AbbrevAttrib attrib;
			GET_ULEB(attrib.name, binary);
			GET_ULEB(attrib.form, binary);
			if ((u64)attrib.name == 0 && (u64)attrib.form == 0) break;
			if (attrib.form == Form::IMPLICIT_CONST) {
				GET_SLEB(attrib.constant, binary);
			}
			decl.attribs.Append(attrib);
		}
		decl.binary = binary.SubRange(startCur, cur-startCur);
		decls.Append(std::move(decl));
	}
	binary.size = cur;
	return None;
}

[[nodiscard]] Ptr<AbbrevDecl> AbbrevUnit::GetDecl(u64 abbrev_code) {
	if (abbrev_code == 0) return nullptr;
	if ((i64)abbrev_code-1 < decls.size && decls[abbrev_code-1].abbrev_code == abbrev_code) {
		return decls.GetPtr((i32)abbrev_code-1);
	}
	// This will probably never actually run if our abbrev_codes are well-behaved like they seem to be, but better safe than sorry.
	for (i32 i = 0; i < decls.size; i++) {
		if (decls[i].abbrev_code == abbrev_code) return decls.GetPtr(i);
	}
	return nullptr;
}

[[nodiscard]] Result<None_t, String> Attrib::Parse(Range<u8> binary, i64 &cur, const AbbrevAttrib &spec, u8 dwarfPtrSize, u8 targetArchPtrSize) {
	io::cout.PrintLnTrace("cur = ", FormatInt(cur, 16), AlignText(16), "Attrib::Parse");
	name = spec.name;
	if (spec.form == Form::INDIRECT) {
		// Our actual form is specified in the DIE
		GET_ULEB(form, binary);
	} else {
		form = spec.form;
	}
	if ((u64)spec.name >= sizeof(AttribNameClasses) / sizeof(AttribNameClasses[0])) {
		return Stringify("Unable to parse Attribute with name ", spec.name, " as idk what that is :P");
	}
	if ((u64)form >= sizeof(FormClasses) / sizeof(FormClasses[0])) {
		return Stringify("Unable to parse Attribute with form ", form, " as idk what that is >:P");
	}
	Range<Class> nameClasses = AttribNameClasses[(u32)spec.name];
	Range<Class> formClasses = FormClasses[(u32)form];
	Class actualClass = (Class)0;
	bool foundClass = false;
	for (Class &nameClass : nameClasses) {
		for (Class &formClass : formClasses) {
			if (nameClass == formClass) {
				if (foundClass) {
					return Stringify("Attribute name ", spec.name, " (classes: ", nameClasses, ") with form ", form, " (classes: ", formClasses, ") can be multiple classes!");
				}
				actualClass = nameClass;
				foundClass = true;
			}
		}
	}
	if (!foundClass) {
		return Stringify("Found no common classes between name ", name, " (classes: ", nameClasses, ") and form ", form, " (classes: ", formClasses, ")");
	}
	_class = actualClass;
	io::cout.PrintLnTrace("cur = ", FormatInt(cur, 16), AlignText(16), "name = ", name, AlignText(48), "form = ", form, AlignText(72), "class = ", _class);
	switch (form) {
		case Form::ADDR:
			GET_PTR(addr, binary, targetArchPtrSize);
			break;
		case Form::REF_ADDR:
		case Form::SEC_OFFSET:
		case Form::STRP:
		case Form::LINE_STRP:
		case Form::STRP_SUP:
			GET_PTR(addr, binary, dwarfPtrSize);
			break;
		case Form::ADDRX:
		case Form::STRX:
		case Form::LOCLISTX:
		case Form::RNGLISTX:
			GET_ULEB(addr, binary);
			break;
		case Form::ADDRX1:
		case Form::STRX1:
		case Form::REF1:
			GET_DATA_PROXY(addr, binary, u8);
			break;
		case Form::ADDRX2:
		case Form::STRX2:
		case Form::REF2:
			GET_DATA_PROXY(addr, binary, u16);
			break;
		case Form::ADDRX3:
		case Form::STRX3:
			GET_DATA_PROXY(addr, binary, u24);
			break;
		case Form::ADDRX4:
		case Form::STRX4:
		case Form::REF4:
		case Form::REF_SUP4:
			GET_DATA_PROXY(addr, binary, u32);
			break;
		case Form::REF8:
		case Form::REF_SUP8:
		case Form::REF_SIG8:
			GET_DATA_PROXY(addr, binary, u64);
			break;
		case Form::REF_UDATA:
			GET_ULEB(addr, binary);
			break;
		case Form::BLOCK1:
			GET_DATA_PROXY(block.size, binary, u8);
			GET_RANGE(block, binary, block.size);
			break;
		case Form::BLOCK2:
			GET_DATA_PROXY(block.size, binary, u16);
			GET_RANGE(block, binary, block.size);
			break;
		case Form::BLOCK4:
			GET_DATA_PROXY(block.size, binary, u32);
			GET_RANGE(block, binary, block.size);
			break;
		case Form::BLOCK:
		case Form::EXPRLOC:
			GET_ULEB(block.size, binary);
			GET_RANGE(block, binary, block.size);
			break;
		case Form::IMPLICIT_CONST:
			constant.lo = spec.constant;
			constant.hi = 0;
			break;
		case Form::DATA1:
			GET_DATA_PROXY(constant.lo, binary, u8);
			constant.hi = 0;
			break;
		case Form::DATA2:
			GET_DATA_PROXY(constant.lo, binary, u16);
			constant.hi = 0;
			break;
		case Form::DATA4:
			GET_DATA_PROXY(constant.lo, binary, u32);
			constant.hi = 0;
			break;
		case Form::DATA8:
			GET_DATA_PROXY(constant.lo, binary, u64);
			constant.hi = 0;
			break;
		case Form::DATA16:
			GET_DATA_PROXY(constant.lo, binary, u64);
			GET_DATA_PROXY(constant.hi, binary, u64);
			break;
		case Form::SDATA:
			GET_SLEB(constant.lo, binary);
			constant.hi = 0; // TODO: Probably allow LEBs to parse into 128 bits
			break;
		case Form::UDATA:
			GET_ULEB(constant.lo, binary);
			constant.hi = 0; // TODO: Probably allow LEBs to parse into 128 bits
			break;
		case Form::STRING:
			for (string.size = 0; binary[string.size+cur] != '\0'; string.size++) {
				if (string.size+cur >= binary.size) {
					return Stringify("Unterminated STRING constant in binary (starting at byte ", cur, ")");
				}
			}
			string.data = (char*)&binary[cur];
			cur += string.size+1; // Add 1 to skip over the null byte
			break;
		case Form::FLAG:
			GET_DATA_PROXY(flag, binary, u8);
			break;
		case Form::INDIRECT:
			return Stringify("Our INDIRECT form just gave us another INDIRECT!!!");
		case Form::FLAG_PRESENT:
			flag = true;
			break;
		default:
			return Stringify("Unreachable with form: ", spec.form);
	}
	return None;
}

[[nodiscard]] Result<None_t, String> InitialLength::Parse(Range<u8> binary, i64 &cur) {
	GET_DATA(initial_length, binary);
	if (initial_length < 0xfffffff0) {
		unit_length = initial_length;
	} else if (initial_length == 0xffffffff) {
		GET_DATA(unit_length, binary);
	} else {
		return Stringify("Unknown special initial_length ", FormatInt(initial_length, 16, true));
	}
	return None;
}

[[nodiscard]] Result<None_t, String> InfoUnitHeader::Parse(Range<u8> debug_info, i64 &cur) {
	binary = debug_info.SubRange(cur);
	if (auto result = unit_length.Parse(debug_info, cur); result.isError) {
		return result.error;
	}
	bool is64bit = unit_length.Is64Bit();
	GET_DATA(version, debug_info);
	if (version != 5) { // TODO: Support older versions maybe?
		return Stringify("Unsupported DWARF version ", version);
	}
	GET_DATA(unit_type, debug_info);
	GET_DATA(address_size, debug_info);
	if (address_size != 4 && address_size != 8) {
		return Stringify("Invalid address_size ", address_size);
	}
	if (is64bit) {
		GET_DATA(debug_abbrev_offset, debug_info);
	} else {
		GET_DATA_PROXY(debug_abbrev_offset, debug_info, u32);
	}
	switch (unit_type) {
		case ComputeUnitType::COMPILE:
		case ComputeUnitType::PARTIAL:
			break; // No additional fields
		case ComputeUnitType::SKELETON:
		case ComputeUnitType::SPLIT_COMPILE: {
			GET_DATA(dwo_id, debug_info);
		} break;
		case ComputeUnitType::TYPE:
		case ComputeUnitType::SPLIT_TYPE: {
			GET_DATA(type_signature, debug_info);
			if (is64bit) {
				GET_DATA(type_offset, debug_info);
			} else {
				GET_DATA_PROXY(type_offset, debug_info, u32);
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

[[nodiscard]] static Result<None_t, String> _ParseDIEs(Array<DIE> &dst, Range<u8> debug_info, Ptr<AbbrevUnit> abbrev_unit, u8 dwarfPtrSize, u8 targetArchPtrSize, i64 &cur, i64 endCur) {
	io::cout.PrintLnTrace("cur = ", FormatInt(cur, 16), AlignText(16), "_ParseDIEs");
	while (cur < endCur) {
		i64 startCur = cur;
		// The humble Reaper:
		DIE die; // die

		GET_ULEB(die.abbrev_code, debug_info);
		io::cout.PrintLnTrace("cur = ", FormatInt(startCur, 16), AlignText(16), "Abbrev Code: ", die.abbrev_code);
		if (die.abbrev_code == 0) break;
		Ptr<AbbrevDecl> decl = abbrev_unit->GetDecl(die.abbrev_code);
		if (!decl.Valid()) {
			return Stringify("Couldn't find abbrev_code ", die.abbrev_code, " in compute unit with ", abbrev_unit->decls.size, " decls!");
		}
		die.abbrev = decl;
		die.attribs.Resize(decl->attribs.size);
		for (i32 i = 0; i < die.attribs.size; i++) {
			Attrib &attrib = die.attribs[i];
			const AbbrevAttrib &spec = decl->attribs[i];
			if (auto result = attrib.Parse(debug_info, cur, spec, dwarfPtrSize, targetArchPtrSize); result.isError) {
				return result.error;
			}
		}
		if (decl->has_children) {
			io::cout.IndentMore();
			if (auto result = _ParseDIEs(die.children, debug_info, abbrev_unit, dwarfPtrSize, targetArchPtrSize, cur, endCur); result.isError) {
				return result.error;
			}
			io::cout.IndentLess();
		}
		die.binary = debug_info.SubRange(startCur, cur-startCur);
		dst.Append(std::move(die));
	}
	return None;
}

[[nodiscard]] Result<None_t, String> InfoUnit::Parse(Range<u8> debug_info, i64 &cur, Ptr<AbbrevUnit> abbrev_unit, u8 targetArchPtrSize) {
	io::cout.PrintLnTrace(AZCORE_PRETTY_FUNCTION);
	i64 startCur = cur;
	AzAssert(abbrev_unit.Valid(), "We need an abbrev_unit, man!");
	if (auto result = header.Parse(debug_info, cur); result.isError) {
		return result.error;
	}
	u8 dwarfPtrSize = header.unit_length.Is64Bit() ? 8 : 4;
	binary = debug_info.SubRange(startCur, header.GetTotalBinarySize());
	if (auto result = _ParseDIEs(dies, debug_info, abbrev_unit, dwarfPtrSize, targetArchPtrSize, cur, startCur+binary.size); result.isError) {
		return result.error;
	}
	if (cur != startCur+binary.size) {
		return Stringify("Our cursor didn't line up with the expected size (cur = ", FormatInt(cur, 16, true), ", expected ", FormatInt(startCur+binary.size, 16, true), ")");
	}
	return None;
}

[[nodiscard]] Result<None_t, String> ARangeUnit::Parse(Range<u8> debug_aranges, i64 &cur, Ptr<InfoUnit> info_unit) {
	io::cout.PrintLnTrace("cur = ", FormatInt(cur, 16), AlignText(16), "ARangeUnit::Parse");
	i64 startCur = cur;
	AzAssert(info_unit.Valid(), "We need an info_unit, man!");
	debug_info_unit = info_unit;
	if (auto result = unit_length.Parse(debug_aranges, cur); result.isError) {
		return result.error;
	}
	bool is64bit = unit_length.Is64Bit();
	binary = debug_aranges.SubRange(startCur, unit_length.GetTotalLength());
	GET_DATA(version, debug_aranges);
	if (is64bit) {
		GET_DATA(debug_info_offset, debug_aranges);
	} else {
		GET_DATA_PROXY(debug_info_offset, debug_aranges, u32);
	}
	GET_DATA(address_size, debug_aranges);
	if (address_size == 0) {
		return String("address_size was zero!");
	}
	GET_DATA(segment_selector_size, debug_aranges);
	cur = alignNonPowerOfTwo(cur, segment_selector_size + address_size*2);
	if (segment_selector_size) {
		while (cur < startCur+binary.size) {
			ARangeDescriptor descriptor;
			GET_DATA_SIZED(descriptor.segment_selector, debug_aranges, segment_selector_size);
			GET_DATA_SIZED(descriptor.offset, debug_aranges, address_size);
			GET_DATA_SIZED(descriptor.length, debug_aranges, address_size);
			if (descriptor.segment_selector == 0 && descriptor.offset == 0 && descriptor.length == 0) break;
			ranges.Append(descriptor);
		}
	} else {
		while (cur < startCur+binary.size) {
			ARangeDescriptor descriptor;
			descriptor.segment_selector = 0;
			GET_DATA_SIZED(descriptor.offset, debug_aranges, address_size);
			GET_DATA_SIZED(descriptor.length, debug_aranges, address_size);
			if (descriptor.offset == 0 && descriptor.length == 0) break;
			ranges.Append(descriptor);
		}
	}
	if (cur != startCur+binary.size) {
		return Stringify("Exected cur to end at ", FormatInt(startCur+binary.size, 16), " but it was ", FormatInt(cur, 16), " instead!");
	}
	return None;
}

[[nodiscard]] Result<None_t, String> DebuggerInfo::ParseFromELF(elf::File &file) {
	debug_info = file.GetSectionByName(".debug_info");
	if (debug_info.size == 0) {
		return String("There is no .debug_info section in the file.");
	}
	debug_abbrev = file.GetSectionByName(".debug_abbrev");
	if (debug_abbrev.size == 0) {
		return String("There is no .debug_abbrev section in the file.");
	}
	debug_aranges = file.GetSectionByName(".debug_aranges");
	if (debug_aranges.size == 0) {
		return String("There is no .debug_aranges section in the file.");
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

	// Then we can parse CUs from .debug_info
	cur = 0;
	for (i32 cu = 0; cur < debug_info.size; cu++) {
		InfoUnit info_unit;
		if (cu >= abbrev_units.size) {
			return Stringify("Trying to parse a ", cu, "th compute unit from .debug_info when we only have ", abbrev_units.size, " CUs from .debug_abbrev");
		}
		if (auto result = info_unit.Parse(debug_info, cur, abbrev_units.GetPtr(cu), file.is64bit ? 8 : 4); result.isError) {
			return result.error;
		}
		info_units.Append(std::move(info_unit));
	}

	cur = 0;
	for (i32 cu = 0; cur < debug_aranges.size; cu++) {
		ARangeUnit arange_unit;
		if (cu >= info_units.size) {
			return Stringify("Trying to parse a ", cu, "th arange unit from .debug_aranges when we only have ", info_units.size, " CUs from .debug_info");
		}
		if (auto result = arange_unit.Parse(debug_aranges, cur, info_units.GetPtr(cu)); result.isError) {
			return result.error;
		}
		arange_units.Append(std::move(arange_unit));
	}
	return None;
}

} // namespace AzCore::dwarf

namespace AzCore {

void AppendToString(String &string, const az::dwarf::Attrib &_value) {
	az::dwarf::Attrib &value = const_cast<az::dwarf::Attrib&>(_value);
	i32 startLen = Utf8CharCount(string);
	AppendToString(string, value.name);
	i32 endLen = Utf8CharCount(string);
	i32 countToAdd = 24-(endLen-startLen);
	for (i32 i = 0; i < countToAdd; i++) {
		string.Append(' ');
	}
	startLen = endLen + countToAdd;
	AppendToString(string, value.form);
	endLen = Utf8CharCount(string);
	countToAdd = 16-(endLen-startLen);
	for (i32 i = 0; i < countToAdd; i++) {
		string.Append(' ');
	}
	switch(value._class) {
		case az::dwarf::Class::STRING:
			if (value.form == az::dwarf::Form::STRING) {
				AppendMultipleToString(string, '"', EscapeString(value.string), '"');
			} else {
				AppendToString(string, FormatInt(value.addr, 16, true));
			}
			break;
		case az::dwarf::Class::BLOCK:
		case az::dwarf::Class::EXPRLOC:
			AppendMultipleToString(string, "Block[", value.block.size, ']');
			if (value.block.size <= 32) {
				AppendToString(string, " = ");
				AppendToStringWithBase(string, value.block, 16);
			}
			break;
		case az::dwarf::Class::ADDRESS:
		case az::dwarf::Class::ADDRPTR:
		case az::dwarf::Class::LINEPTR:
		case az::dwarf::Class::LOCLIST:
		case az::dwarf::Class::LOCLISTPTR:
		case az::dwarf::Class::MACPTR:
		case az::dwarf::Class::RNGLIST:
		case az::dwarf::Class::RNGLISTPTR:
		case az::dwarf::Class::REFERENCE:
		case az::dwarf::Class::STROFFSETSPTR:
			AppendToString(string, FormatInt(value.addr, 16, true));
			break;
		case az::dwarf::Class::CONSTANT:
			if (value.constant.hi) {
				AppendMultipleToString(string, FormatInt(value.constant.hi, 16, true), FormatInt(value.constant.lo, 16));
			} else {
				AppendToString(string, FormatInt(value.constant.lo, 16, true));
			}
			break;
		case az::dwarf::Class::FLAG:
			AppendToString(string, value.flag ? "true" : "false");
			break;
		default: break;
	}
}

} // namespace AzCore
