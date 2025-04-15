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
		void *elfHeader = nullptr;
		elf32_header *elfHeader32;
		elf64_header *elfHeader64;
	};
	File() = default;
	// Emties all dynamic data and frees associated memory
	inline void Clear() {
		binary.Clear();
		elfHeader = nullptr;
		parsed = false;
	}
	// Empties all dynamic data but hold on to the associated memory (to use it later)
	inline void ClearSoft() {
		binary.ClearSoft();
		elfHeader = nullptr;
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

	void PrintHeaderInfo(io::Log &log=io::cout);
};

} // namespace AzCore::elf

#endif // AZCORE_ELF_HPP