/*
	File: elf.cpp
	Author: Philip Haynes
*/

#include "elf.hpp"
#include "definitions.hpp"

namespace AzCore::elf {

#define ENSURE_BUFFER(bytes, offset) if ((i64)binary.size < (((i64)bytes) + ((i64)offset))) {\
	return Stringify("Expected ", FormatInt(bytes, 16, true), " bytes to be available at offset ", FormatInt(offset, 16, true), " (binary size = ", FormatInt(binary.size, 16, true), ")");\
}

[[nodiscard]] Result<None_t, String> File::ParseElf32() {
	if (parsed) return None;
	return String("ParseElf32 is not yet implemented.");
	// TODO: When ParseElf64 is fully implemented, it should basically just be a copy for most of it
	return None;
}

[[nodiscard]] Result<None_t, String> File::ParseElf64() {
	if (parsed) return None;
	is64bit = true;
	ENSURE_BUFFER(sizeof(elf64_header), 0);
	elf64_header &header = *(elf64_header*)&binary[0];
	const bool doEndianSwap = (header.ident_endian == ElfEndian::MSB && SysEndian.little) || (header.ident_endian == ElfEndian::LSB && SysEndian.big);
	if (doEndianSwap) {
		EndianSwap(header);
	}
	elfHeader64 = &header;
	io::cout.PrintLnTrace("Parsed elf64_header");
	parsed = true;
	return None;
}

[[nodiscard]] Result<None_t, String> File::Parse() {
	if (parsed) return None;
	if (binary.size == 0) {
		return String("No binary data is loaded.");
	} else if ((u64)binary.size < elf_header_min_size) {
		return Stringify("Binary is too small (", FormatInt(binary.size, 16, true), " bytes)!");
	}
	Str myMagic = binary.GetRange(0, 4);
	if (myMagic != magic) {
		return Stringify("Binary is not an ELF file (wrong magic \"", EscapeString(myMagic, '"', false), "\" != \"", EscapeString(magic, '"', false), "\")");
	}
	ElfClass elfClass = (ElfClass)(u8)binary[4];
	switch (elfClass) {
		case ElfClass::ELF32:
			return ParseElf32();
		case ElfClass::ELF64:
			return ParseElf64();
		default:
			return Stringify("Invalid ELF class ", FormatInt((u32)elfClass, 16, true));
	}
}

template<typename PTR_T>
static void _PrintFileHeaderInfo(File &file, io::Log &log) {
	if (!file.parsed) return;
	elf_header<PTR_T> &header = *(elf_header<PTR_T>*)file.elfHeader;
	log.PrintLn(
		"ident_class: ", header.ident_class,
		"\nident_endian: ", header.ident_endian,
		"\nident_version: ", header.ident_version,
		"\nident_abi: ", header.ident_abi,
		"\nident_abi_version: ", header.ident_abi_version,
		"\ntype: ", header.type,
		"\nisa: ", header.isa,
		"\nversion: ", header.version,
		"\nentry: ", FormatInt(header.entry, 16, true),
		"\nprogramHeadersOffset: ", FormatInt(header.programHeadersOffset, 16, true),
		"\nsectionHeadersOffset: ", FormatInt(header.sectionHeadersOffset, 16, true),
		"\nflags: ", FormatInt(header.flags, 16, true),
		"\nelfHeaderSize: ", FormatInt(header.elfHeaderSize, 16, true),
		"\nprogramHeaderEntrySize: ", FormatInt(header.programHeaderEntrySize, 16, true),
		"\nprogramHeaderCount: ", header.programHeaderCount,
		"\nsectionHeaderEntrySize: ", FormatInt(header.sectionHeaderEntrySize, 16, true),
		"\nsectionHeaderCount: ", header.sectionHeaderCount,
		"\nsectionHeaderStringTableIndex: ", header.sectionHeaderStringTableIndex
	);
}

void File::PrintHeaderInfo(io::Log &log) {
	if (is64bit) {
		_PrintFileHeaderInfo<u64>(*this, log);
	} else {
		_PrintFileHeaderInfo<u32>(*this, log);
	}
}

} // namespace AzCore::elf