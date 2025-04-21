/*
	File: elf.hpp
	Author: Philip Haynes
	ELF File struct for reading from binaries.
*/

#ifndef AZCORE_ELF_HPP
#define AZCORE_ELF_HPP

#include "../BasicTypes.hpp"
#include "../Memory/String.hpp"
#include "../Memory/Result.hpp"

#include "definitions.hpp"

namespace AzCore::elf {

//
// File metastructure
//

struct File {
	Array<char> binary;
	bool parsed = false;
	bool is64bit;
	union {
		struct {
			void *header;
			void *sectionHeaderStringTable;
		} any = {0};
		struct {
			elf32_header *header;
			elf32_section_header *sectionHeaderStringTable;
		} elf32;
		struct {
			elf64_header *header;
			elf64_section_header *sectionHeaderStringTable;
		} elf64;
	};
	Array<char*> programHeaders;
	Array<char*> sectionHeaders;
	static_assert(sizeof(any) == sizeof(elf32) && sizeof(any) == sizeof(elf64), "Union struct size mismatch");
	// Emties all dynamic data and frees associated memory
	inline void Clear() {
		binary.Clear();
		any.header = nullptr;
		parsed = false;
	}
	// Empties all dynamic data but hold on to the associated memory (to use it later)
	inline void ClearSoft() {
		binary.ClearSoft();
		any.header = nullptr;
		parsed = false;
	}
	[[nodiscard]] inline Result<None_t, String> LoadFile(Str filepath) {
		ClearSoft();
		binary = FileContents(filepath, true);
		if (binary.size == 0) {
			return Stringify("File \"", filepath, "\" could not be read.");
		}
		return Parse();
	}
	// Will acquire buffer
	[[nodiscard]] inline Result<None_t, String> LoadFromBuffer(Array<char> &&buffer) {
		ClearSoft();
		binary = std::move(buffer);
		return Parse();
	}
	// Will copy from buffer
	[[nodiscard]] inline Result<None_t, String> LoadFromBuffer(Str buffer) {
		ClearSoft();
		binary = buffer;
		return Parse();
	}
	[[nodiscard]] Result<None_t, String> Parse();
	[[nodiscard]] Result<None_t, String> ParseElf32();
	[[nodiscard]] Result<None_t, String> ParseElf64();

	Str GetSectionName(i32 sectionIndex);

	void PrintHeaderInfo(io::Log &log=io::cout, bool programHeaders=false, bool sectionHeaders=false);
	void PrintSectionHeaderInfo(io::Log &log, i32 sectionIndex);
	void PrintDWARFInfo(io::Log &log=io::cout);
};

} // namespace AzCore::elf

#endif // AZCORE_ELF_HPP