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
	FENIXOS    = 0x10, // FenixOS, a multicore research OS       2011?-????
	CLOUDABI   = 0x11, // Nuxi CloudABI                          2010-2020 (deprecated)
	OPENVOS    = 0x12, // Stratus Technologies OpenVOS           1984-present

	ARM_EABI   = 0x40, // Arm Embedded ABI

	ARM        = 0x61,

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
	NOT_SPECIFIED             = 0x00,
	ATT_WE_32100              = 0x01, // AT&T WE 32100
	SPARC                     = 0x02,
	X86                       = 0x03, // x86
	M68K                      = 0x04, // Motorola 68000
	M88K                      = 0x05, // Motorola 88000
	INTEL_MCU                 = 0x06,
	INTEL_80860               = 0x07,
	MIPS                      = 0x08,
	IBM_SYSTEM_370            = 0x09, // IBM System/370
	MIPS_RS3000_LE            = 0x0A, // MIPS RS3000 Little-endian
	// 0x0B-0x0E reserved
	HP_PA_RISC                = 0x0F, // Hewlett-Packard PA-RISC
	// 0x10 reserved
	FUJITSU_VPP500            = 0x11,
	SPARC32PLUS               = 0x12, // Sun's v8plus
	INTEL_80960               = 0x13,
	POWERPC                   = 0x14, // PowerPC
	POWERPC_64                = 0x15, // PowerPC 64-bit
	S390                      = 0x16,
	IBM_SPU_SPC               = 0x17, // IBM SPU/SPC
	// 0x18-0x23 reserved
	NEC_V800                  = 0x24,
	FUJITSU_FR20              = 0x25,
	TRW_RH_32                 = 0x26, // TRW RH-32
	MOTOROLA_RCE              = 0x27,
	ARM                       = 0x28, // Arm (up to Armv7/AArch32)
	DIGITAL_ALPHA             = 0x29,
	SUPERH                    = 0x2A, // SuperH
	SPARC_V9                  = 0x2B, // SPARC Version 9
	SIEMENS_TRICORE           = 0x2C, // Siemens TriCore embedded processor
	ARGONAUT_RISC_CORE        = 0x2D,
	HITACHI_H8_300            = 0x2E, // Hitachi H8/300
	HITACHI_H8_300H           = 0x2F, // Hitachi H8/300H
	HITACHI_H8S               = 0x30, // Hitachi H8S
	HITACHI_H8_500            = 0x31, // Hitachi H8/500
	IA_64                     = 0x32,
	STANFORD_MIPS_X           = 0x33, // Stanford MIPS-X
	MOTOROLA_COLDFIRE         = 0x34,
	MOTOROLA_M68HC12          = 0x35,
	FUJITSU_MMA               = 0x36, // Fujitsu Multimedia Accelerator
	SIEMENS_PCP               = 0x37,
	SONY_NCPU_RISC            = 0x38, // Sony nCPU embedded RISC processor
	DENSO_NDR1                = 0x39, // Denso NDR1 microprocessor
	MOTOROLA_STAR_CORE        = 0x3A, // Motorola Star*Core processor
	TOYOTA_ME16               = 0x3B, // Toyota ME16 processor
	STM_ST100                 = 0x3C, // STMicroelectronics ST100 processor
	ALC_TINYJ                 = 0x3D, // Advanced Logic Corp. TinyJ embedded processors
	X86_64                    = 0x3E, // AMD x86-64
	SONY_DSP                  = 0x3F, // Sony DSP processor
	DEC_PDP_10                = 0x40, // Digital Equipment Corp. PDP-10
	DEC_PDP_11                = 0x41, // Digital Equipment Corp. PDP-11
	SIEMENS_FX66              = 0x42, // Siemens FX66 microcontroller
	STM_ST9                   = 0x43, // STMicroelectronics ST9+ 8/16 bit microcontroller
	STM_ST7                   = 0x44, // STMicroelectronics ST8 8-bit microcontroller
	MOTOROLA_MC68HC16         = 0x45,
	MOTOROLA_MC68HC11         = 0x46,
	MOTOROLA_MC68HC08         = 0x47,
	MOTOROLA_MC68HC05         = 0x48,
	SILICON_GRAPHICS_SVX      = 0x49,
	STM_ST19                  = 0x4A, // STMicroelectronics ST19 8-bit microcontroller
	DIGITAL_VAX               = 0x4B,
	AXIS_COMMS_32             = 0x4C, // Axis Communications 32-bit embedded processor
	INFINEON_TECH_32          = 0x4D, // Infineon Technologies 32-bit embedded processor
	ELEMENT_14_DSP            = 0x4E, // Element 14 64-bit DSP processor
	LSI_LOGIC_DSP             = 0x4F, // LSI Logic 16-bit DSP processor

	DONALD_KNUTH_MMIX         = 0x50, // Donald Knuth's educational 64-bit proc
	HARVARD_ANY               = 0x51, // Harvard University machine-independent object files
	SITERA_PRISM              = 0x52, // SiTera Prism
	AMTEL_AVR                 = 0x53, // Atmel AVR 8-bit microcontroller
	FUJITSU_FR30              = 0x54,
	MITSUBISHI_D10V           = 0x55,
	MITSUBISHI_D30V           = 0x56,
	NEC_V850                  = 0x57,
	MITSUBISHI_M32R           = 0x58,
	MATSUSHITA_MN10300        = 0x59,
	MATSUSHITA_MN10200        = 0x5A,
	PICO_JAVA                 = 0x5B,
	OPENRISC                  = 0x5C, // OpenRISC 32-bit embedded processor
	ARC_COMPACT               = 0x5D, // ARC International ARCompact
	TENSILICA_XTENSA          = 0x5E, // Tensilica Xtensa Architecture
	ALPHAMOSAIC_VIDEOCORE     = 0x5F, // Alphamosaic VideoCore
	THOMPSON_MULTIMEDIA_GPP   = 0x60, // Thompson Multimedia General Purpose Proc
	NATIONAL_SEMI_32K         = 0x61, // National Semi. 32000
	TENOR_NETWORK_TPC         = 0x62,
	TREBIA_SNP1K              = 0x63,
	STM_ST200                 = 0x64, // STMicroelectronics ST200
	UBICOM_IP2K               = 0x65, // Ubicom IP2000 family
	MAX                       = 0x66, // MAX processor
	NATIONAL_SEMI_CR          = 0x67, // National Semi. CompactRISC
	FUJITSU_F2MC16            = 0x68,
	TI_MSP430                 = 0x69, // Texas Instruments msp430
	AD_BLACKFIN_DSP           = 0x6A, // Analog Devices Blackfin DSP
	SEIKO_EPSON_S1C33         = 0x6B, // Seiko Epson S1C33 family
	SHARP_EMBEDDED_PROCESSOR  = 0x6C, // Sharp embedded microprocessor
	ARCA_RISC                 = 0x6D,
	UNICORE                   = 0x6E, // Peking University MPRC mc series
	EXCESS                    = 0x6F, // eXcess configurable cpu
	ICERA_SEMI_DXP            = 0x70, // Icera Semi. Deep Execution Processor
	ALTERA_NIOS_II            = 0x71,
	NATIONAL_SEMI_CRX         = 0x72, // National Semi. CompactRISC CRX
	MOTOROLA_XGATE            = 0x73,
	INFINEON_C166             = 0x74, // Infineon C16x/XC16x
	RENESAS_M16C              = 0x75,
	MICROCHIP_TECH_DSPIC30F   = 0x76, // Microchip Technology dsPIC30F
	FREESCALE_CE_RISC         = 0x77, // Freescale Communication Engine RISC
	RENESAS_M32C              = 0x78,
	// 0x79-0x82 reserved
	ALTIUM_TSK3000            = 0x83,
	FREESCALE_RS08            = 0x84,
	AD_SHARC                  = 0x85,
	CYAN_TECH_ECOG2           = 0x86,
	SUNPLUS_SCORE7_RISC       = 0x87, // Sunplus S+core7 RISC
	NJR_DSP24_DSP             = 0x88, // New Japan Radio (NJR) 24-bit DSP
	BROADCOM_VIDEOCORE_III    = 0x89,
	LATTICEMICO32_RISC        = 0x8A, // RISC for Lattice FPGA
	SEIKO_EPSON_C17           = 0x8B,
	TI_C6000_DSP              = 0x8C, // Texas Instruments TMS320C6000 DSP
	TI_C2000_DSP              = 0x8D, // Texas Instruments TMS320C2000 DSP
	TI_C5500_DSP              = 0x8E, // Texas Instruments TMS320C55x DSP
	TI_ARP32_RISC             = 0x8F, // Texas Instruments App. Specific RISC
	TI_PRU                    = 0x90, // Texas Instruments Prog. Realtime Unit
	// 0x91-0x9f reserved
	STM_MMDSP_PLUS            = 0xA0, // STMicroelectronics 64bit VLIW DSP
	CYPRESS_M8C               = 0xA1,
	RENESAS_R32C              = 0xA2,
	NXP_SEMI_TRIMEDIA         = 0xA3,
	QUALCOMM_DSP6             = 0xA4,
	INTEL_8051                = 0xA5, // Intel 8051 and variants
	STM_STXP7X                = 0xA6, // STMicroelectronics STxP7x
	ANDES_TECH_NDS32_RISC     = 0xA7, // Andes Tech. compact code emb. RISC
	CYAN_TECH_ECOG1X          = 0xA8,
	DALLAS_SEMI_MAXQ30        = 0xA9,
	NJR_XIMO16_DSP            = 0xAA, // New Japan Radio (NJR) 16-bit DSP
	MANIK                     = 0xAB, // M2000 Reconfigurable RISC
	CRAYNV2                   = 0xAC, // Cray NV2 vector architecture
	RX                        = 0xAD, // Renesas RX
	METAG                     = 0xAE, // Imagination Tech. META
	MCST_ELBRUS_E2K           = 0xAF,
	ECOG16                    = 0xB0, // Cyan Technology eCOG16
	CR16                      = 0xB1, // National Semi. CompactRISC CR16
	ETPU                      = 0xB2, // Freescale Extended Time Processing Unit
	SLE9X                     = 0xB3, // Infineon Tech. SLE9X
	L10M                      = 0xB4, // Intel L10M
	K10M                      = 0xB5, // Intel K10M
	// 0xB6 reserved
	ARM_64                    = 0xB7, // Arm 64-bits (Armv8/AArch64)
	// 0xB8 reserved
	AMTEL_AVR32               = 0xB9, // Amtel 32-bit microprocessor
	STM_STM8                  = 0xBA, // STMicroelectronics STM8
	TILERA_TILE64             = 0xBB,
	TILERA_TILEPRO            = 0xBC,
	XILINX_MICROBLAZE         = 0xBD,
	NVIDIA_CUDA               = 0xBE,
	TILERA_TILEGX             = 0xBF,
	CLOUDSHIELD               = 0xC0,
	KIPO_KAIST_COREA_1ST      = 0xC1, // KIPO-KAIST Core-A 1st gen.
	KIPO_KAIST_COREA_2ND      = 0xC2, // KIPO-KAIST Core-A 2nd gen.
	SYNOPSYS_ARCV2            = 0xC3, // Synopsys ARCv2 ISA.
	OPEN8_RISC                = 0xC4,
	RENESAS_RL78              = 0xC5,
	BROADCOM_VIDEOCORE_V      = 0xC6,
	RENESAS_78KOR             = 0xC7,
	FREESCALE_56800EX_DSC     = 0xC8,
	BEYOND_BA1                = 0xC9,
	BEYOND_BA2                = 0xCA,
	XMOS_XCORE                = 0xCB,
	MICROCHIP_PIC             = 0xCC, // Microchip 8-bit PIC(r)
	INTEL_GRAPHICS_TECH       = 0xCD,
	// 0xCD-0xD1 reserved
	KM211_KM32                = 0xD2,
	KM211_KMX32               = 0xD3,
	KM211_KMX16               = 0xD4,
	KM211_KMX8                = 0xD5,
	KM211_KVARC               = 0xD6,
	PANEVE_CDP                = 0xD7,
	COGE                      = 0xD8, // Cognitive Smart Memory Processor
	BLUECHIP_COOLENGINE       = 0xD9,
	NANORADIO_OPTIMIZED_RISC  = 0xDA,
	CSR_KALIMBA               = 0xDB,
	ZILOG_Z80                 = 0xDC,
	CDS_VISIUM                = 0xDD, // Controls and Data Services VISIUMcore
	FTDI_FT32                 = 0xDE, // FTDI Chip FT32
	MOXIE                     = 0xDF, // Moxie processor
	AMD_GPU                   = 0xE0,
	// 0xE1-0xF2 reserved
	RISC_V                    = 0xF3,

	BERKELEY_PACKET_FILTER    = 0xF7,

	C_SKY                     = 0xFC,

	WDC_65C816                = 0x101,
	LOONG_ARCH                = 0x102,
};

template<typename Word_t>
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
	ElfType type;                // object file type
	ElfISA isa;                  // Instruction Set Architecture
	u32 version;                 // object file version (should be 1)
	Word_t entry;                // entry point virtual address
	Word_t programHeadersOffset; // file offset
	Word_t sectionHeadersOffset; // file offset
	u32 flags;                   // processor-specific flags
	u16 elfHeaderSize;           // total size of this struct (should be 52 for 32-bit and 64 for 64-bit)
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
	UNUSED               = 0x00, // This program header table entry is unused.
	LOADABLE             = 0x01, // Loadable segment
	DYNAMIC              = 0x02, // Dynamic linking information
	INTERPRETER          = 0x03, // Interpreter information
	NOTE                 = 0x04,
	SHLIB                = 0x05,
	PROGRAM_HEADER       = 0x06,
	THREAD_LOCAL_STORAGE = 0x07,
	// GNU
	GNU_EH_FRAME         = 0x6474E550, // GCC .eh_frame_hdr segment (exception handlers)
	GNU_STACK            = 0x6474E551, // Indicates stack executability
	GNU_RELRO            = 0x6474E552, // Read-only after relocation
	GNU_PROPERTY         = 0x6474E553, // Segment containing .note.gnu.property section
	// GNU_MBIND_LO      = 0x6474E555, // Mbind segments start
	// GNU_MBIND_HI      = 0x6474F554, // Mbind segments end
	// SUN
	SUN_WBSS             = 0x6ffffffa,
	SUN_WSTACK           = 0x6ffffffb,

	AARCH64_MEMTAG_MTE   = 0x70000002, // ARM MTE memory tag segment
};

enum SegmentFlags : u32 {
	PF_X = 0x01,
	PF_W = 0x02,
	PF_R = 0x04,
};

template<typename Word_t>
struct program_header {
	static_assert(std::is_same_v<Word_t, u32> || std::is_same_v<Word_t, u64>, "program_header<Word_t> must have a Word_t of u32 or u64");
};

template<>
struct program_header<u32> {
	ProgramType type;
	u32 fileOffset;
	u32 virtualAddress;
	u32 physicalAddress;
	u32 fileSize;  // Size in bytes of the segment in the file image.
	u32 memSize;   // Size in bytes of the segment in memory.
	u32 flags;
	u32 alignment; // 0 and 1 specify no alignment. Powers of 2. virtualAddress % alignment == fileOffset % alignment
};
using elf32_program_header = program_header<u32>;
static_assert(sizeof(elf32_program_header) == 32);

template<>
struct program_header<u64> {
	ProgramType type;
	u32 flags;
	u64 fileOffset;
	u64 virtualAddress;
	u64 physicalAddress;
	u64 fileSize;  // Size in bytes of the segment in the file image.
	u64 memSize;   // Size in bytes of the segment in memory.
	u64 alignment; // 0 and 1 specify no alignment. Powers of 2. virtualAddress % alignment == fileOffset % alignment
};
using elf64_program_header = program_header<u64>;
static_assert(sizeof(elf64_program_header) == 56);

//
// Section Header
//

enum class SectionType : u32 {
	UNUSED                   = 0x00,
	PROGRAM_DATA             = 0x01,
	SYMBOL_TABLE             = 0x02,
	STRING_TABLE             = 0x03,
	RELOCATION_ADDENDS       = 0x04,
	HASH_TABLE               = 0x05,
	DYNAMIC                  = 0x06,
	NOTE                     = 0x07,
	NOBITS                   = 0x08,
	RELOCATION               = 0x09,
	SHLIB                    = 0x0A,
	DYNAMIC_SYMBOL_TABLE     = 0x0B,

	INIT_ARRAY               = 0x0E,
	DEINIT_ARRAY             = 0x0F,
	PREINIT_ARRAY            = 0x10,
	GROUP                    = 0x11,
	SYMBOL_TABLE_EXT         = 0x12,
	NUM                      = 0x13,

	GNU_ATTRIBUTES           = 0x6ffffff5, // Object attributes
	GNU_HASH                 = 0x6ffffff6, // GNU-style hash table
	GNU_LIBLIST              = 0x6ffffff7, // Prelink library list
	CHECKSUM                 = 0x6ffffff8, // Checksum for DSO content

	SUNW_MOVE                = 0x6ffffffa,
	SUNW_COMDAT              = 0x6ffffffb,
	SUNW_SYMINFO             = 0x6ffffffc,
	GNU_VERSION_DEFINITION   = 0x6ffffffd, // Version definition section
	GNU_VERSION_NEEDS        = 0x6ffffffe, // Version needs section
	GNU_VERSION_SYMBOL_TABLE = 0x6fffffff, // Version symbol table
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
	SHF_COMPRESSED       = 0x800, // Section with compressed data

	SHF_ORDERED      = 0x4000000, // Special ordering requirements
	SHF_EXCLUDE      = 0x8000000, // Section is excluded unless referenced or allocated
};

template<typename Word_t>
struct section_header {
	u32 name;
	SectionType type;
	Word_t flags;
	Word_t virtualAddress;
	Word_t fileOffset;
	Word_t size;
	u32 link;      // Section index of an associated section.
	u32 info;
	Word_t alignment; // Required alignment of the section (must be a power of 2)
	Word_t entrySize; // Size of entries for segments that contain fixed-size entries.
};

using elf32_section_header = section_header<u32>;
static_assert(sizeof(elf32_section_header) == 40);
using elf64_section_header = section_header<u64>;
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