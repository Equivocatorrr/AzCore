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
	elf64.header = &header;
	io::cout.PrintLnTrace("Parsed elf64_header");
	if (header.programHeaderCount) {
		// Parse programs
		if (header.programHeaderEntrySize < sizeof(elf64_program_header)) {
			return Stringify("programHeaderEntrySize (", FormatInt(header.programHeaderEntrySize,16, true), " bytes) is too small! (Expected ", FormatInt(sizeof(elf64_program_header), 16, true), " bytes)");
		} // We're probably okay with bigger than expected, just not sure what to do with the rest yet if there is any.
		ENSURE_BUFFER(header.programHeaderEntrySize * header.programHeaderCount, header.programHeadersOffset);
		char *programHeadersStart = &binary[header.programHeadersOffset];
		programHeaders.Resize(header.programHeaderCount);
		for (i32 i = 0; i < header.programHeaderCount; i++) {
			elf64_program_header &programHeader = *(elf64_program_header*)(programHeadersStart + header.programHeaderEntrySize * i);
			programHeaders[i] = (char*)&programHeader;
			if (doEndianSwap) {
				EndianSwap(programHeader);
			}
			if ((u64)binary.size < (programHeader.fileSize + programHeader.fileOffset)) {
				return Stringify("Program ", i, " data is out of bounds, expecting ", FormatInt(programHeader.fileSize, 16, true), " bytes to be available at offset ", FormatInt(programHeader.fileOffset, 16, true), " (binary size = ", FormatInt(binary.size, 16, true), ")");
			}
		}
		io::cout.PrintLnTrace("Parsed elf64_program_headers");
	}
	if (header.sectionHeaderCount) {
		if (header.sectionHeaderStringTableIndex >= header.sectionHeaderCount) {
			return Stringify("sectionHeaderStringTableIndex (", header.sectionHeaderStringTableIndex, ") is out of bounds (we have ", header.sectionHeaderCount, " sections)");
		}
		// Parse sections
		if (header.sectionHeaderEntrySize < sizeof(elf64_section_header)) {
			return Stringify("sectionHeaderEntrySize (", FormatInt(header.sectionHeaderEntrySize,16, true), " bytes) is too small! (Expected ", FormatInt(sizeof(elf64_section_header), 16, true), " bytes)");
		} // We're probably okay with bigger than expected, just not sure what to do with the rest yet if there is any.
		ENSURE_BUFFER(header.sectionHeaderEntrySize * header.sectionHeaderCount, header.programHeadersOffset);
		char *sectionHeadersStart = &binary[header.sectionHeadersOffset];
		sectionHeaders.Resize(header.sectionHeaderCount);
		for (i32 i = 0; i < header.sectionHeaderCount; i++) {
			elf64_section_header &sectionHeader = *(elf64_section_header*)(sectionHeadersStart + header.sectionHeaderEntrySize * i);
			sectionHeaders[i] = (char*)&sectionHeader;
			if (doEndianSwap) {
				EndianSwap(sectionHeader);
			}
			if ((u64)binary.size < (sectionHeader.size + sectionHeader.fileOffset)) {
				return Stringify("Section ", i, " data is out of bounds, expecting ", FormatInt(sectionHeader.size, 16, true), " bytes to be available at offset ", FormatInt(sectionHeader.fileOffset, 16, true), " (binary size = ", FormatInt(binary.size, 16, true), ")");
			}
		}
		elf64.sectionHeaderStringTable = (elf64_section_header*)sectionHeaders[header.sectionHeaderStringTableIndex];
		io::cout.PrintLnTrace("Parsed elf64_section_headers");
	}
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
	elf_header<PTR_T> &header = *(elf_header<PTR_T>*)file.any.header;
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

static void _PrintProgramHeaderInfo32(File &file, io::Log &log) {
	log.PrintLn("Program headers (", file.programHeaders.size, "):");
	for (i32 i = 0; i < file.programHeaders.size; i++) {
		elf32_program_header &header = *(elf32_program_header*)file.programHeaders[i];
		log.PrintLn(i);
		log.IndentMore();
		log.PrintLn(
			"type: ", header.type,
			"\nfileOffset: ", FormatInt(header.fileOffset, 16, true),
			"\nvirtualAddress: ", FormatInt(header.virtualAddress, 16, true),
			"\nphysicalAddress: ", FormatInt(header.physicalAddress, 16, true),
			"\nfileSize: ", FormatInt(header.fileSize, 16, true),
			"\nmemSize: ", FormatInt(header.memSize, 16, true),
			"\nflags: ", (SegmentFlags)header.flags,
			"\nalignment: ", FormatInt(header.alignment, 16, true)
		);
		log.IndentLess();
	}
}

static void _PrintProgramHeaderInfo64(File &file, io::Log &log) {
	log.PrintLn("Program headers (", file.programHeaders.size, "):");
	for (i32 i = 0; i < file.programHeaders.size; i++) {
		elf64_program_header &header = *(elf64_program_header*)file.programHeaders[i];
		log.PrintLn(i);
		log.IndentMore();
		log.PrintLn(
			"type: ", header.type,
			"\nflags: ", (SegmentFlags)header.flags,
			"\nfileOffset: ", FormatInt(header.fileOffset, 16, true),
			"\nvirtualAddress: ", FormatInt(header.virtualAddress, 16, true),
			"\nphysicalAddress: ", FormatInt(header.physicalAddress, 16, true),
			"\nfileSize: ", FormatInt(header.fileSize, 16, true),
			"\nmemSize: ", FormatInt(header.memSize, 16, true),
			"\nalignment: ", FormatInt(header.alignment, 16, true)
		);
		log.IndentLess();
	}
}

static void _PrintSectionHeaderInfo32(File &file, io::Log &log) {
	elf32_section_header &nameHeader = *file.elf32.sectionHeaderStringTable;
	Str sectionNameTable = file.binary.GetRange(nameHeader.fileOffset, nameHeader.size);
	log.PrintLn("Section headers (", file.sectionHeaders.size, "):");
	for (i32 i = 0; i < file.sectionHeaders.size; i++) {
		elf32_section_header &header = *(elf32_section_header*)file.sectionHeaders[i];
		Str name = sectionNameTable.SubRange(header.name);
		name.size = (i64)StringLength(name.data, name.size);
		log.PrintLn(i);
		log.IndentMore();
		log.PrintLn(
			"name: \"", EscapeString(name), '"',
			"\ntype: ", header.type,
			"\nflags: ", (SectionFlags)header.flags,
			"\nvirtualAddress: ", FormatInt(header.virtualAddress, 16, true),
			"\nfileOffset: ", FormatInt(header.fileOffset, 16, true),
			"\nsize: ", FormatInt(header.size, 16, true),
			"\nlink: ", header.link,
			"\ninfo: ", FormatInt(header.info, 16, true),
			"\nalignment: ", FormatInt(header.alignment, 16, true),
			"\nentrySize: ", FormatInt(header.entrySize, 16, true)
		);
		log.IndentLess();
	}
}

static void _PrintSectionHeaderInfo64(File &file, io::Log &log) {
	elf64_section_header &nameHeader = *file.elf64.sectionHeaderStringTable;
	Str sectionNameTable = file.binary.GetRange(nameHeader.fileOffset, nameHeader.size);
	log.PrintLn("Section headers (", file.sectionHeaders.size, "):");
	for (i32 i = 0; i < file.sectionHeaders.size; i++) {
		elf64_section_header &header = *(elf64_section_header*)file.sectionHeaders[i];
		Str name = sectionNameTable.SubRange(header.name);
		name.size = (i64)StringLength(name.data, name.size);
		log.PrintLn(i);
		log.IndentMore();
		log.PrintLn(
			"name: \"", EscapeString(name), '"',
			"\ntype: ", header.type,
			"\nflags: ", (SectionFlags)header.flags,
			"\nvirtualAddress: ", FormatInt(header.virtualAddress, 16, true),
			"\nfileOffset: ", FormatInt(header.fileOffset, 16, true),
			"\nsize: ", FormatInt(header.size, 16, true),
			"\nlink: ", header.link,
			"\ninfo: ", FormatInt(header.info, 16, true),
			"\nalignment: ", FormatInt(header.alignment, 16, true),
			"\nentrySize: ", FormatInt(header.entrySize, 16, true)
		);
		log.IndentLess();
	}
}

void File::PrintHeaderInfo(io::Log &log, bool programHeaders, bool sectionHeaders) {
	if (is64bit) {
		_PrintFileHeaderInfo<u64>(*this, log);
		if (programHeaders) {
			_PrintProgramHeaderInfo64(*this, log);
		}
		if (sectionHeaders) {
			_PrintSectionHeaderInfo64(*this, log);
		}
	} else {
		_PrintFileHeaderInfo<u32>(*this, log);
		if (programHeaders) {
			_PrintProgramHeaderInfo32(*this, log);
		}
		if (sectionHeaders) {
			_PrintSectionHeaderInfo32(*this, log);
		}
	}
}

} // namespace AzCore::elf