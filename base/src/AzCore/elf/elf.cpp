/*
	File: elf.cpp
	Author: Philip Haynes
*/

#include "elf.hpp"
#include "definitions.hpp"
#include <type_traits>

namespace AzCore::elf {

#define ENSURE_BUFFER(bytes, offset) if ((i64)file.binary.size < (((i64)bytes) + ((i64)offset))) {\
	return Stringify("Expected ", FormatInt(bytes, 16, true), " bytes to be available at offset ", FormatInt(offset, 16, true), " (binary size = ", FormatInt(file.binary.size, 16, true), ")");\
}

template<typename Word_t>
inline Result<None_t, String> _ParseElf(File &file) {
	constexpr Str ELF_HEADER_NAME = std::is_same_v<u64, Word_t> ? "elf64_header" : "elf32_header";
	constexpr Str PROGRAM_HEADER_NAME = std::is_same_v<u64, Word_t> ? "elf64_program_header" : "elf32_program_header";
	constexpr Str SECTION_HEADER_NAME = std::is_same_v<u64, Word_t> ? "elf64_section_header" : "elf32_section_header";
	using elf_header_t = elf_header<Word_t>;
	using program_header_t = program_header<Word_t>;
	using section_header_t = section_header<Word_t>;
	if (file.parsed) return None;
	file.is64bit = std::is_same_v<u64, Word_t>;
	ENSURE_BUFFER(sizeof(elf_header_t), 0);
	elf_header_t &header = *(elf_header_t*)&file.binary[0];
	const bool doEndianSwap = (header.ident_endian == ElfEndian::MSB && SysEndian.little) || (header.ident_endian == ElfEndian::LSB && SysEndian.big);
	if (doEndianSwap) {
		EndianSwap(header);
	}
	file.any.header = &header;
	io::cout.PrintLnTrace("Parsed ", ELF_HEADER_NAME);
	if (header.programHeaderCount) {
		// Parse programs
		if (header.programHeaderEntrySize < sizeof(program_header_t)) {
			return Stringify("programHeaderEntrySize (", FormatInt(header.programHeaderEntrySize,16, true), " bytes) is too small! (Expected ", FormatInt(sizeof(program_header_t), 16, true), " bytes)");
		} // We're probably okay with bigger than expected, just not sure what to do with the rest yet if there is any.

		// Check alignment
		// TODO: If this becomes a problem we'll have to copy the data out.
		if (header.programHeaderEntrySize % alignof(program_header_t) != 0) {
			return Stringify("programHeaderEntrySize (", header.programHeaderEntrySize, ") is not adequately aligned (", alignof(program_header_t), ")");
		}
		if (header.programHeadersOffset % alignof(program_header_t) != 0) {
			return Stringify("programHeadersOffset (", header.programHeadersOffset, ") is not adequately aligned (", alignof(program_header_t), ")");
		}
		ENSURE_BUFFER(header.programHeaderEntrySize * header.programHeaderCount, header.programHeadersOffset);
		char *programHeadersStart = &file.binary[header.programHeadersOffset];
		file.programHeaders.Resize(header.programHeaderCount);
		for (i32 i = 0; i < header.programHeaderCount; i++) {
			program_header_t &programHeader = *(program_header_t*)(programHeadersStart + header.programHeaderEntrySize * i);
			file.programHeaders[i] = (char*)&programHeader;
			if (doEndianSwap) {
				EndianSwap(programHeader);
			}
			if ((u64)file.binary.size < (programHeader.fileSize + programHeader.fileOffset)) {
				return Stringify("Program ", i, " data is out of bounds, expecting ", FormatInt(programHeader.fileSize, 16, true), " bytes to be available at offset ", FormatInt(programHeader.fileOffset, 16, true), " (binary size = ", FormatInt(file.binary.size, 16, true), ")");
			}
		}
		io::cout.PrintLnTrace("Parsed ", PROGRAM_HEADER_NAME);
	}
	if (header.sectionHeaderCount) {
		if (header.sectionHeaderStringTableIndex >= header.sectionHeaderCount) {
			return Stringify("sectionHeaderStringTableIndex (", header.sectionHeaderStringTableIndex, ") is out of bounds (we have ", header.sectionHeaderCount, " sections)");
		}
		// Parse sections
		if (header.sectionHeaderEntrySize < sizeof(section_header_t)) {
			return Stringify("sectionHeaderEntrySize (", FormatInt(header.sectionHeaderEntrySize,16, true), " bytes) is too small! (Expected ", FormatInt(sizeof(section_header_t), 16, true), " bytes)");
		} // We're probably okay with bigger than expected, just not sure what to do with the rest yet if there is any.

		// Check alignment
		// TODO: If this becomes a problem we'll have to copy the data out.
		if (header.sectionHeaderEntrySize % alignof(section_header_t) != 0) {
			return Stringify("sectionHeaderEntrySize (", header.sectionHeaderEntrySize, ") is not adequately aligned (", alignof(section_header_t), ")");
		}
		if (header.sectionHeadersOffset % alignof(section_header_t) != 0) {
			return Stringify("sectionHeadersOffset (", header.sectionHeadersOffset, ") is not adequately aligned (", alignof(section_header_t), ")");
		}
		ENSURE_BUFFER(header.sectionHeaderEntrySize * header.sectionHeaderCount, header.programHeadersOffset);
		char *sectionHeadersStart = &file.binary[header.sectionHeadersOffset];
		file.sectionHeaders.Resize(header.sectionHeaderCount);
		for (i32 i = 0; i < header.sectionHeaderCount; i++) {
			section_header_t &sectionHeader = *(section_header_t*)(sectionHeadersStart + header.sectionHeaderEntrySize * i);
			file.sectionHeaders[i] = (char*)&sectionHeader;
			if (doEndianSwap) {
				EndianSwap(sectionHeader);
			}
			if (sectionHeader.type != SectionType::NOBITS && (u64)file.binary.size < (sectionHeader.size + sectionHeader.fileOffset)) {
				return Stringify("Section ", i, " data is out of bounds, expecting ", FormatInt(sectionHeader.size, 16, true), " bytes to be available at offset ", FormatInt(sectionHeader.fileOffset, 16, true), " (binary size = ", FormatInt(file.binary.size, 16, true), ")");
			}
		}
		file.any.sectionHeaderStringTable = file.sectionHeaders[header.sectionHeaderStringTableIndex];
		io::cout.PrintLnTrace("Parsed ", SECTION_HEADER_NAME);
	}
	file.parsed = true;
	return None;
}

[[nodiscard]] Result<None_t, String> File::ParseElf32() {
	return _ParseElf<u32>(*this);
}

[[nodiscard]] Result<None_t, String> File::ParseElf64() {
	return _ParseElf<u64>(*this);
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

template<typename Word_t>
static void _PrintFileHeaderInfo(File &file, io::Log &log) {
	if (!file.parsed) return;
	elf_header<Word_t> &header = *(elf_header<Word_t>*)file.any.header;
	log.PrintLn(
		"class: ", header.ident_class,
		"\nendian: ", header.ident_endian,
		"\nversion: ", header.ident_version,
		"\nabi: ", header.ident_abi,
		"\nabi_version: ", header.ident_abi_version,
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

constexpr u64 align1 = 36;
constexpr u64 align2 = 68;

template<typename Word_t>
static void _PrintProgramHeaderInfo(File &file, io::Log &log) {
	using program_header_t = program_header<Word_t>;
	log.PrintLn("Program headers (", file.programHeaders.size, "):");
	for (i32 i = 0; i < file.programHeaders.size; i++) {
		program_header_t &header = *(program_header_t*)file.programHeaders[i];
		log.PrintLn(i, AlignText(4, "─"), " type: ", header.type, AlignText(align1), " flags: ", (SegmentFlags)header.flags);
		log.PrintLn(
			"│    fileOffset: ", FormatInt(header.fileOffset, 16, true), AlignText(align1),
			" virtualAddress: ", FormatInt(header.virtualAddress, 16, true), AlignText(align2),
			" physicalAddress: ", FormatInt(header.physicalAddress, 16, true)
		);
		log.PrintLn(
			"└─── fileSize: ", FormatInt(header.fileSize, 16, true), AlignText(align1),
			" memSize: ", FormatInt(header.memSize, 16, true), AlignText(align2),
			" alignment: ", FormatInt(header.alignment, 16, true)
		);
	}
}

template<typename Word_t>
static void _PrintSectionHeaderInfo(File &file, io::Log &log) {
	using section_header_t = section_header<Word_t>;
	section_header_t &nameHeader = *(section_header_t*)file.any.sectionHeaderStringTable;
	Str sectionNameTable = file.binary.GetRange(nameHeader.fileOffset, nameHeader.size);
	log.PrintLn("Section headers (", file.sectionHeaders.size, "):");
	for (i32 i = 0; i < file.sectionHeaders.size; i++) {
		section_header_t &header = *(section_header_t*)file.sectionHeaders[i];
		Str name = sectionNameTable.SubRange(header.name);
		name.size = (i64)StringLength(name.data, name.size);
		log.PrintLn(i, AlignText(4, "─"), " name: \"", EscapeString(name), '"');
		log.PrintLn("│    type: ", header.type, AlignText(align1), " flags: ", (SectionFlags)header.flags);
		log.PrintLn(
			"│    virtualAddress: ", FormatInt(header.virtualAddress, 16, true), AlignText(align1),
			" fileOffset: ", FormatInt(header.fileOffset, 16, true), AlignText(align2),
			" size: ", FormatInt(header.size, 16, true)
		);
		log.PrintLn("│    link: ", header.link, AlignText(align1), " info: ", FormatInt(header.info, 16, true));
		log.PrintLn("└─── alignment: ", FormatInt(header.alignment, 16, true), AlignText(align1), " entrySize: ", FormatInt(header.entrySize, 16, true));
	}
}

void File::PrintHeaderInfo(io::Log &log, bool programHeaders, bool sectionHeaders) {
	if (is64bit) {
		_PrintFileHeaderInfo<u64>(*this, log);
		if (programHeaders) {
			_PrintProgramHeaderInfo<u64>(*this, log);
		}
		if (sectionHeaders) {
			_PrintSectionHeaderInfo<u64>(*this, log);
		}
	} else {
		_PrintFileHeaderInfo<u32>(*this, log);
		if (programHeaders) {
			_PrintProgramHeaderInfo<u32>(*this, log);
		}
		if (sectionHeaders) {
			_PrintSectionHeaderInfo<u32>(*this, log);
		}
	}
}

} // namespace AzCore::elf