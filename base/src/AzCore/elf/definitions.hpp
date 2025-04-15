/*
	File: definitions.hpp
	Author: Philip Haynes
	structs and datatypes for working with ELF files
*/

#ifndef AZCORE_ELF_DEFINITIONS_HPP
#define AZCORE_ELF_DEFINITIONS_HPP

#include "../BasicTypes.hpp"
#include "../Memory/String.hpp"
#include "../Utility/Endian.hpp"
#include <type_traits>

namespace AzCore::elf {

//
// ELF File Header
//

constexpr Str magic("\x7F""ELF", 4);

enum class ElfClass : u8 {
	NONE=0,
	ELF32=1,
	ELF64=2,
};

enum class ElfEndian : u8 {
	NONE=0,
	LSB=1, // Data structures are little-endian
	MSB=2, // Data structures are big-endian
};

enum class ElfOsAbi : u8 {
	SYSV       = 0x00, // No extensions, Unix System V           1983-present
	HPUX       = 0x01, // Hewlett-Packard HP-UX                  1984-present
	NETBSD     = 0x02, // NetBSD                                 1993-present
	GNU_LINUX  = 0x03, // Or, as I like to call it, GNU+Linux    1991-present
	GNU_HURD   = 0x04, // GNU Hurd                               1990-present

	SOLARIS    = 0x06, // Sun Solaris                            1992-present
	AIX        = 0x07, // IBM AIX                                1986-present
	IRIX       = 0x08, // SGI IRIX                               1988-2013 (end of support)
	FREEBSD    = 0x09, // FreeBSD                                1993-present
	TRU64      = 0x0A, // Tru64 UNIX (formerly Digital UNIX)     1992-2012 (end of support)
	MODESTO    = 0x0B, // Novell Modesto                         1998-????
	OPENBSD    = 0x0C, // OpenBSD                                1996-present
	OPENVMS    = 0x0D, // OpenVMS                                1977-present
	HP_NSK     = 0x0E, // Hewlett-Packard NonStop Kernel         1976-present
	AROS       = 0x0F, // "AR-OS" Research Operating System      1995-present
	FENIXOS    = 0x10, // FENIX by Aura Vulpes                   ????-present
	CLOUDABI   = 0x11, // Nuxi CloudABI                          2010-2020 (deprecated)
	OPENVOS    = 0x12, // Stratus Technologies OpenVOS           1984-present

	ARM_EABI   = 0x40, // Arm Embedded ABI

	STANDALONE = 0xFF,
};

enum class ElfType : u16 {
	NONE        = 0x00,
	RELOCATABLE = 0x01,
	EXECUTABLE  = 0x02,
	DYNAMIC     = 0x03, // Shared object
	CORE        = 0x04,
};

enum class ElfISA : u16 {
	NOT_SPECIFIED          = 0x00,
	ATT_WE_32100           = 0x01, // AT&T WE 32100
	SPARC                  = 0x02,
	X86                    = 0x03, // x86
	M68K                   = 0x04, // Motorola 68000
	M88K                   = 0x05, // Motorola 88000
	INTEL_MCU              = 0x06,
	INTEL_80860            = 0x07,
	MIPS                   = 0x08,
	IBM_SYSTEM_370         = 0x09, // IBM System/370
	MIPS_RS3000_LE         = 0x0A, // MIPS RS3000 Little-endian

	HP_PA_RISC             = 0x0F, // Hewlett-Packard PA-RISC

	INTEL_80960            = 0x13,
	POWERPC                = 0x14, // PowerPC
	POWERPC_64             = 0x15, // PowerPC 64-bit
	S390                   = 0x16,
	IBM_SPU_SPC            = 0x17, // IBM SPU/SPC

	NEC_V800               = 0x24,
	FUJITSU_FR20           = 0x25,
	TRW_RH_32              = 0x26, // TRW RH-32
	MOTOROLA_RCE           = 0x27,
	ARM                    = 0x28, // Arm (up to Armv7/AArch32)
	DIGITAL_ALPHA          = 0x29,
	SUPERH                 = 0x2A, // SuperH
	SPARC_V9               = 0x2B, // SPARC Version 9
	SIEMENS_TRICORE        = 0x2C, // Siemens TriCore embedded processor
	ARGONAUT_RISC_CORE     = 0x2D,
	HITACHI_H8_300         = 0x2E, // Hitachi H8/300
	HITACHI_H8_300H        = 0x2F, // Hitachi H8/300H
	HITACHI_H8S            = 0x30, // Hitachi H8S
	HITACHI_H8_500         = 0x31, // Hitachi H8/500
	IA_64                  = 0x32,
	STANFORD_MIPS_X        = 0x33, // Stanford MIPS-X
	MOTOROLA_COLDFIRE      = 0x34,
	MOTOROLA_M68HC12       = 0x35,
	FUJITSU_MMA            = 0x36, // Fujitsu Multimedia Accelerator
	SIEMENS_PCP            = 0x37,
	SONY_NCPU_RISC         = 0x38, // Sony nCPU embedded RISC processor
	DENSO_NDR1             = 0x39, // Denso NDR1 microprocessor
	MOTOROLA_STAR_CORE     = 0x3A, // Motorola Star*Core processor
	TOYOTA_ME16            = 0x3B, // Toyota ME16 processor
	STM_ST100              = 0x3C, // STMicroelectronics ST100 processor
	ALC_TINYJ              = 0x3D, // Advanced Logic Corp. TinyJ embedded processors
	X86_64                 = 0x3E, // AMD x86-64
	SONY_DSP               = 0x3F, // Sony DSP processor
	DEC_PDP_10             = 0x40, // Digital Equipment Corp. PDP-10
	DEC_PDP_11             = 0x41, // Digital Equipment Corp. PDP-11
	SIEMENS_FX66           = 0x42, // Siemens FX66 microcontroller
	STM_ST9                = 0x43, // STMicroelectronics ST9+ 8/16 bit microcontroller
	STM_ST7                = 0x44, // STMicroelectronics ST8 8-bit microcontroller
	MOTOROLA_MC68HC16      = 0x45,
	MOTOROLA_MC68HC11      = 0x46,
	MOTOROLA_MC68HC08      = 0x47,
	MOTOROLA_MC68HC05      = 0x48,
	SILICON_GRAPHICS_SVX   = 0x49,
	STM_ST19               = 0x4A, // STMicroelectronics ST19 8-bit microcontroller
	DIGITAL_VAX            = 0x4B,
	AXIS_COMMS_32          = 0x4C, // Axis Communications 32-bit embedded processor
	INFINEON_TECH_32       = 0x4D, // Infineon Technologies 32-bit embedded processor
	ELEMENT_14_DSP         = 0x4E, // Element 14 64-bit DSP processor
	LSI_LOGIC_DSP          = 0x4F, // LSI Logic 16-bit DSP processor

	TMS320C6000            = 0x8C, // Texas Instruments didn't want anyone to remember their processors by name

	MCST_ELBRUS_E2K        = 0xAF,

	ARM_64                 = 0xB7, // Arm 64-bits (Armv8/AArch64)

	ZILOG_Z80              = 0xDC,

	RISC_V                 = 0xF3, // RISC-V

	BERKELEY_PACKET_FILTER = 0xF7,

	WDC_65C816             = 0x101,
	LOONG_ARCH             = 0x102,
};

template<typename PTR_T>
struct elf_header {
	// 0     1    2    3    4      5       6        7      8           9..15
	// 0x7f, 'E', 'L', 'F', class, endian, version, osabi, abiversion, padding
	union {
		u8 ident[16];
		struct {
			u8 ident_magic[4];
			ElfClass ident_class;
			ElfEndian ident_endian;
			u8 ident_version; // = 1
			ElfOsAbi ident_abi;
			u8 ident_abi_version;
		};
	};
	ElfType type;               // object file type
	ElfISA isa;                 // Instruction Set Architecture
	u32 version;                // object file version (should be 1)
	PTR_T entry;                // entry point virtual address
	PTR_T programHeadersOffset; // file offset
	PTR_T sectionHeadersOffset; // file offset
	u32 flags;                  // processor-specific flags
	u16 elfHeaderSize;          // total size of this struct (should be 52 for 32-bit and 64 for 64-bit)
	u16 programHeaderEntrySize;
	u16 programHeaderCount;
	u16 sectionHeaderEntrySize;
	u16 sectionHeaderCount;
	u16 sectionHeaderStringTableIndex;
};

using elf32_header = elf_header<u32>;
static_assert(sizeof(elf32_header) == 52);
using elf64_header = elf_header<u64>;
static_assert(sizeof(elf64_header) == 64);

constexpr u64 elf_header_min_size = 52;

//
// Program Header
//

enum class ProgramType : u32 {
	UNUSED               = 0, // This program header table entry is unused.
	LOADABLE             = 1, // Loadable segment
	DYNAMIC              = 2, // Dynamic linking information
	INTERPRETER          = 3, // Interpreter information
	NOTE                 = 4,
	SHLIB                = 5,
	PROGRAM_HEADER       = 6,
	THREAD_LOCAL_STORAGE = 7,
};

enum SegmentFlags : u32 {
	PF_X = 0x01,
	PF_W = 0x02,
	PF_R = 0x04,
};

struct elf32_program_header {
	ProgramType type;
	u32 fileOffset;
	u32 virtualAddress;
	u32 physicalAddress;
	u32 fileSize;  // Size in bytes of the segment in the file image.
	u32 memSize;   // Size in bytes of the segment in memory.
	u32 flags;
	u32 alignment; // 0 and 1 specify no alignment. Powers of 2. virtualAddress % alignment == fileOffset % alignment
};
static_assert(sizeof(elf32_program_header) == 32);

struct elf64_program_header {
	ProgramType type;
	u32 flags;
	u64 fileOffset;
	u64 virtualAddress;
	u64 physicalAddress;
	u64 fileSize;  // Size in bytes of the segment in the file image.
	u64 memSize;   // Size in bytes of the segment in memory.
	u64 alignment; // 0 and 1 specify no alignment. Powers of 2. virtualAddress % alignment == fileOffset % alignment
};
static_assert(sizeof(elf64_program_header) == 56);

//
// Section Header
//

enum class SectionType : u32 {
	UNUSED                = 0x00,
	PROGRAM_DATA          = 0x01,
	SYMBOL_TABLE          = 0x02,
	STRING_TABLE          = 0x03,
	RELOCATION_ADDENDS    = 0x04,
	HASH_TABLE            = 0x05,
	DYNAMIC               = 0x06,
	NOTE                  = 0x07,
	NOBITS                = 0x08,
	RELOCATION            = 0x09,
	SHLIB                 = 0x0A,
	DYNAMIC_SYMBOL_TABLE  = 0x0B,

	INIT_ARRAY            = 0x0E,
	DEINIT_ARRAY          = 0x0F,
	PREINIT_ARRAY         = 0x10,
	GROUP                 = 0x11,
	SYMBOL_TABLE_EXT      = 0x12,
	NUM                   = 0x13,
};

enum SectionFlags : u64 {
	SHF_WRITE            = 0x001, // Writeable
	SHF_ALLOC            = 0x002, // Occupies memory during execution
	SHF_EXECINSTR        = 0x004, // Executable

	SHF_MERGE            = 0x010, // Might be merged
	SHF_STRINGS          = 0x020, // Contains strings
	SHF_INFO_LINK        = 0x040, // section_header::info contains SHT index
	SHF_LINK_ORDER       = 0x080, // Preserve order after combining
	SHF_OS_NONCONFORMING = 0x100, // Non-standard OS-specific handling is required
	SHF_GROUP            = 0x200, // Section is a member of a group
	SHF_THREAD_LOCAL     = 0x400, // Section holds thread-local data

	SHF_ORDERED      = 0x4000000, // Special ordering requirements
	SHF_EXCLUDE      = 0x8000000, // Section is excluded unless referenced or allocated
};

struct elf32_section_header {
	u32 name;
	SectionType type;
	u32 flags;
	u32 virtualAddress;
	u32 fileOffset;
	u32 size;
	u32 link;      // Section index of an associated section.
	u32 info;
	u32 alignment; // Required alignment of the section (must be a power of 2)
	u32 entrySize; // Size of entries for segments that contain fixed-size entries.
};
static_assert(sizeof(elf32_section_header) == 40);

struct elf64_section_header {
	u32 name;
	SectionType type;
	u64 flags;
	u64 virtualAddress;
	u64 fileOffset;
	u64 size;
	u32 link;      // Section index of an associated section.
	u32 info;
	u64 alignment; // Required alignment of the section (must be a power of 2)
	u64 entrySize; // Size of entries for segments that contain fixed-size entries.
};
static_assert(sizeof(elf64_section_header) == 64);

} // namespace AzCore::elf

namespace AzCore {

template<typename PTR_T>
inline void EndianSwap(elf::elf_header<PTR_T> &data) {
	EndianSwap(data.type);
	EndianSwap(data.isa);
	EndianSwap(data.version);
	EndianSwap(data.entry);
	EndianSwap(data.programHeadersOffset);
	EndianSwap(data.sectionHeadersOffset);
	EndianSwap(data.flags);
	EndianSwap(data.elfHeaderSize);
	EndianSwap(data.programHeaderEntrySize);
	EndianSwap(data.programHeaderCount);
	EndianSwap(data.sectionHeaderEntrySize);
	EndianSwap(data.sectionHeaderCount);
	EndianSwap(data.sectionHeaderStringTableIndex);
}

inline void EndianSwap(elf::elf32_program_header &data) {
	EndianSwap(data.type);
	EndianSwap(data.fileOffset);
	EndianSwap(data.virtualAddress);
	EndianSwap(data.physicalAddress);
	EndianSwap(data.fileSize);
	EndianSwap(data.memSize);
	EndianSwap(data.flags);
	EndianSwap(data.alignment);
};

inline void EndianSwap(elf::elf64_program_header &data) {
	EndianSwap(data.type);
	EndianSwap(data.flags);
	EndianSwap(data.fileOffset);
	EndianSwap(data.virtualAddress);
	EndianSwap(data.physicalAddress);
	EndianSwap(data.fileSize);
	EndianSwap(data.memSize);
	EndianSwap(data.alignment);
};

inline void EndianSwap(elf::elf32_section_header &data) {
	EndianSwap(data.name);
	EndianSwap(data.type);
	EndianSwap(data.flags);
	EndianSwap(data.virtualAddress);
	EndianSwap(data.fileOffset);
	EndianSwap(data.size);
	EndianSwap(data.link);
	EndianSwap(data.info);
	EndianSwap(data.alignment);
	EndianSwap(data.entrySize);
}

inline void EndianSwap(elf::elf64_section_header &data) {
	EndianSwap(data.name);
	EndianSwap(data.type);
	EndianSwap(data.flags);
	EndianSwap(data.virtualAddress);
	EndianSwap(data.fileOffset);
	EndianSwap(data.size);
	EndianSwap(data.link);
	EndianSwap(data.info);
	EndianSwap(data.alignment);
	EndianSwap(data.entrySize);
}

void AppendToString(String &string, elf::ElfClass elfClass);
void AppendToString(String &string, elf::ElfEndian elfEndian);
void AppendToString(String &string, elf::ElfOsAbi elfOsAbi);
void AppendToString(String &string, elf::ElfType elfType);
void AppendToString(String &string, elf::ElfISA elfISA);
void AppendToString(String &string, elf::ProgramType programType);
void AppendToString(String &string, elf::SegmentFlags segmentFlags);
void AppendToString(String &string, elf::SectionType sectionType);
void AppendToString(String &string, elf::SectionFlags sectionFlags);

} // namespace AzCore

#endif // AZCORE_ELF_DEFINITIONS_HPP