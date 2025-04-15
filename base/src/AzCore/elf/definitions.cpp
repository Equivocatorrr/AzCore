/*
	File: definitions.cpp
	Author: Philip Haynes
*/

#include "definitions.hpp"

namespace AzCore {

static Str elfClassStrings[3] = {
	"NONE",
	"ELF32",
	"ELF64",
};

void AppendToString(String &string, elf::ElfClass elfClass) {
	if ((u32)elfClass < 3) {
		AppendToString(string, elfClassStrings[(u32)elfClass]);
	} else {
		AppendToString(string, "0x");
		AppendToStringWithBase(string, (u32)elfClass, 16);
	}
}

static Str elfEndianStrings[3] = {
	"NONE",
	"LSB",
	"MSB",
};

void AppendToString(String &string, elf::ElfEndian elfEndian) {
	if ((u32)elfEndian < 3) {
		AppendToString(string, elfEndianStrings[(u32)elfEndian]);
	} else {
		AppendToString(string, "0x");
		AppendToStringWithBase(string, (u32)elfEndian, 16);
	}
}

void AppendToString(String &string, elf::ElfOsAbi elfOsAbi) {
	switch (elfOsAbi) {
		case elf::ElfOsAbi::SYSV:
			AppendToString(string, "SYSV");
			break;
		case elf::ElfOsAbi::HPUX:
			AppendToString(string, "HPUX");
			break;
		case elf::ElfOsAbi::NETBSD:
			AppendToString(string, "NETBSD");
			break;
		case elf::ElfOsAbi::GNU_LINUX:
			AppendToString(string, "GNU_LINUX");
			break;
		case elf::ElfOsAbi::GNU_HURD:
			AppendToString(string, "GNU_HURD");
			break;
		case elf::ElfOsAbi::SOLARIS:
			AppendToString(string, "SOLARIS");
			break;
		case elf::ElfOsAbi::AIX:
			AppendToString(string, "AIX");
			break;
		case elf::ElfOsAbi::IRIX:
			AppendToString(string, "IRIX");
			break;
		case elf::ElfOsAbi::FREEBSD:
			AppendToString(string, "FREEBSD");
			break;
		case elf::ElfOsAbi::TRU64:
			AppendToString(string, "TRU64");
			break;
		case elf::ElfOsAbi::MODESTO:
			AppendToString(string, "MODESTO");
			break;
		case elf::ElfOsAbi::OPENBSD:
			AppendToString(string, "OPENBSD");
			break;
		case elf::ElfOsAbi::OPENVMS:
			AppendToString(string, "OPENVMS");
			break;
		case elf::ElfOsAbi::HP_NSK:
			AppendToString(string, "HP_NSK");
			break;
		case elf::ElfOsAbi::AROS:
			AppendToString(string, "AROS");
			break;
		case elf::ElfOsAbi::FENIXOS:
			AppendToString(string, "FENIXOS");
			break;
		case elf::ElfOsAbi::CLOUDABI:
			AppendToString(string, "CLOUDABI");
			break;
		case elf::ElfOsAbi::OPENVOS:
			AppendToString(string, "OPENVOS");
			break;
		case elf::ElfOsAbi::ARM_EABI:
			AppendToString(string, "ARM_EABI");
			break;
		case elf::ElfOsAbi::STANDALONE:
			AppendToString(string, "STANDALONE");
			break;
		default:
			AppendToString(string, "0x");
			AppendToStringWithBase(string, (u32)elfOsAbi, 16);
			break;
	}
}

static Str elfTypeStrings[5] = {
	"NONE",
	"RELOCATABLE",
	"EXECUTABLE",
	"DYNAMIC",
	"CORE",
};

void AppendToString(String &string, elf::ElfType elfType) {
	if ((u32)elfType < 5) {
		AppendToString(string, elfTypeStrings[(u32)elfType]);
	} else {
		AppendToString(string, "0x");
		AppendToStringWithBase(string, (u32)elfType, 16);
	}
}

void AppendToString(String &string, elf::ElfISA elfISA) {
	switch(elfISA) {
		case elf::ElfISA::NOT_SPECIFIED:
			AppendToString(string, "NOT_SPECIFIED");
			break;
		case elf::ElfISA::ATT_WE_32100:
			AppendToString(string, "ATT_WE_32100");
			break;
		case elf::ElfISA::SPARC:
			AppendToString(string, "SPARC");
			break;
		case elf::ElfISA::X86:
			AppendToString(string, "X86");
			break;
		case elf::ElfISA::M68K:
			AppendToString(string, "M68K");
			break;
		case elf::ElfISA::M88K:
			AppendToString(string, "M88K");
			break;
		case elf::ElfISA::INTEL_MCU:
			AppendToString(string, "INTEL_MCU");
			break;
		case elf::ElfISA::INTEL_80860:
			AppendToString(string, "INTEL_80860");
			break;
		case elf::ElfISA::MIPS:
			AppendToString(string, "MIPS");
			break;
		case elf::ElfISA::IBM_SYSTEM_370:
			AppendToString(string, "IBM_SYSTEM_370");
			break;
		case elf::ElfISA::MIPS_RS3000_LE:
			AppendToString(string, "MIPS_RS3000_LE");
			break;
		case elf::ElfISA::HP_PA_RISC:
			AppendToString(string, "HP_PA_RISC");
			break;
		case elf::ElfISA::INTEL_80960:
			AppendToString(string, "INTEL_80960");
			break;
		case elf::ElfISA::POWERPC:
			AppendToString(string, "POWERPC");
			break;
		case elf::ElfISA::POWERPC_64:
			AppendToString(string, "POWERPC_64");
			break;
		case elf::ElfISA::S390:
			AppendToString(string, "S390");
			break;
		case elf::ElfISA::IBM_SPU_SPC:
			AppendToString(string, "IBM_SPU_SPC");
			break;
		case elf::ElfISA::NEC_V800:
			AppendToString(string, "NEC_V800");
			break;
		case elf::ElfISA::FUJITSU_FR20:
			AppendToString(string, "FUJITSU_FR20");
			break;
		case elf::ElfISA::TRW_RH_32:
			AppendToString(string, "TRW_RH_32");
			break;
		case elf::ElfISA::MOTOROLA_RCE:
			AppendToString(string, "MOTOROLA_RCE");
			break;
		case elf::ElfISA::ARM:
			AppendToString(string, "ARM");
			break;
		case elf::ElfISA::DIGITAL_ALPHA:
			AppendToString(string, "DIGITAL_ALPHA");
			break;
		case elf::ElfISA::SUPERH:
			AppendToString(string, "SUPERH");
			break;
		case elf::ElfISA::SPARC_V9:
			AppendToString(string, "SPARC_V9");
			break;
		case elf::ElfISA::SIEMENS_TRICORE:
			AppendToString(string, "SIEMENS_TRICORE");
			break;
		case elf::ElfISA::ARGONAUT_RISC_CORE:
			AppendToString(string, "ARGONAUT_RISC_CORE");
			break;
		case elf::ElfISA::HITACHI_H8_300:
			AppendToString(string, "HITACHI_H8_300");
			break;
		case elf::ElfISA::HITACHI_H8_300H:
			AppendToString(string, "HITACHI_H8_300H");
			break;
		case elf::ElfISA::HITACHI_H8S:
			AppendToString(string, "HITACHI_H8S");
			break;
		case elf::ElfISA::HITACHI_H8_500:
			AppendToString(string, "HITACHI_H8_500");
			break;
		case elf::ElfISA::IA_64:
			AppendToString(string, "IA_64");
			break;
		case elf::ElfISA::STANFORD_MIPS_X:
			AppendToString(string, "STANFORD_MIPS_X");
			break;
		case elf::ElfISA::MOTOROLA_COLDFIRE:
			AppendToString(string, "MOTOROLA_COLDFIRE");
			break;
		case elf::ElfISA::MOTOROLA_M68HC12:
			AppendToString(string, "MOTOROLA_M68HC12");
			break;
		case elf::ElfISA::FUJITSU_MMA:
			AppendToString(string, "FUJITSU_MMA");
			break;
		case elf::ElfISA::SIEMENS_PCP:
			AppendToString(string, "SIEMENS_PCP");
			break;
		case elf::ElfISA::SONY_NCPU_RISC:
			AppendToString(string, "SONY_NCPU_RISC");
			break;
		case elf::ElfISA::DENSO_NDR1:
			AppendToString(string, "DENSO_NDR1");
			break;
		case elf::ElfISA::MOTOROLA_STAR_CORE:
			AppendToString(string, "MOTOROLA_STAR_CORE");
			break;
		case elf::ElfISA::TOYOTA_ME16:
			AppendToString(string, "TOYOTA_ME16");
			break;
		case elf::ElfISA::STM_ST100:
			AppendToString(string, "STM_ST100");
			break;
		case elf::ElfISA::ALC_TINYJ:
			AppendToString(string, "ALC_TINYJ");
			break;
		case elf::ElfISA::X86_64:
			AppendToString(string, "X86_64");
			break;
		case elf::ElfISA::SONY_DSP:
			AppendToString(string, "SONY_DSP");
			break;
		case elf::ElfISA::DEC_PDP_10:
			AppendToString(string, "DEC_PDP_10");
			break;
		case elf::ElfISA::DEC_PDP_11:
			AppendToString(string, "DEC_PDP_11");
			break;
		case elf::ElfISA::SIEMENS_FX66:
			AppendToString(string, "SIEMENS_FX66");
			break;
		case elf::ElfISA::STM_ST9:
			AppendToString(string, "STM_ST9");
			break;
		case elf::ElfISA::STM_ST7:
			AppendToString(string, "STM_ST7");
			break;
		case elf::ElfISA::MOTOROLA_MC68HC16:
			AppendToString(string, "MOTOROLA_MC68HC16");
			break;
		case elf::ElfISA::MOTOROLA_MC68HC11:
			AppendToString(string, "MOTOROLA_MC68HC11");
			break;
		case elf::ElfISA::MOTOROLA_MC68HC08:
			AppendToString(string, "MOTOROLA_MC68HC08");
			break;
		case elf::ElfISA::MOTOROLA_MC68HC05:
			AppendToString(string, "MOTOROLA_MC68HC05");
			break;
		case elf::ElfISA::SILICON_GRAPHICS_SVX:
			AppendToString(string, "SILICON_GRAPHICS_SVX");
			break;
		case elf::ElfISA::STM_ST19:
			AppendToString(string, "STM_ST19");
			break;
		case elf::ElfISA::DIGITAL_VAX:
			AppendToString(string, "DIGITAL_VAX");
			break;
		case elf::ElfISA::AXIS_COMMS_32:
			AppendToString(string, "AXIS_COMMS_32");
			break;
		case elf::ElfISA::INFINEON_TECH_32:
			AppendToString(string, "INFINEON_TECH_32");
			break;
		case elf::ElfISA::ELEMENT_14_DSP:
			AppendToString(string, "ELEMENT_14_DSP");
			break;
		case elf::ElfISA::LSI_LOGIC_DSP:
			AppendToString(string, "LSI_LOGIC_DSP");
			break;
		case elf::ElfISA::TMS320C6000:
			AppendToString(string, "TMS320C6000");
			break;
		case elf::ElfISA::MCST_ELBRUS_E2K:
			AppendToString(string, "MCST_ELBRUS_E2K");
			break;
		case elf::ElfISA::ARM_64:
			AppendToString(string, "ARM_64");
			break;
		case elf::ElfISA::ZILOG_Z80:
			AppendToString(string, "ZILOG_Z80");
			break;
		case elf::ElfISA::RISC_V:
			AppendToString(string, "RISC_V");
			break;
		case elf::ElfISA::BERKELEY_PACKET_FILTER:
			AppendToString(string, "BERKELEY_PACKET_FILTER");
			break;
		case elf::ElfISA::WDC_65C816:
			AppendToString(string, "WDC_65C816");
			break;
		case elf::ElfISA::LOONG_ARCH:
			AppendToString(string, "LOONG_ARCH");
			break;
		default:
			AppendToString(string, "0x");
			AppendToStringWithBase(string, (u32)elfISA, 16);
			break;
	}
}

static Str programTypeStrings[8] = {
	"UNUSED",
	"LOADABLE",
	"DYNAMIC",
	"INTERPRETER",
	"NOTE",
	"SHLIB",
	"PROGRAM_HEADER",
	"THREAD_LOCAL_STORAGE",
};

void AppendToString(String &string, elf::ProgramType programType) {
	if ((u32)programType < 8) {
		AppendToString(string, programTypeStrings[(u32)programType]);
	} else {
		AppendToString(string, "0x");
		AppendToStringWithBase(string, (u32)programType, 16);
	}
}

void AppendToString(String &string, elf::SegmentFlags segmentFlags) {
	u32 flags = segmentFlags;
	if (flags == 0) {
		AppendToString(string, "NONE");
		return;
	}
	if (flags & elf::PF_X) {
		AppendToString(string, 'X');
		flags &= ~elf::PF_X;
	}
	if (flags & elf::PF_W) {
		AppendToString(string, 'W');
		flags &= ~elf::PF_W;
	}
	if (flags & elf::PF_R) {
		AppendToString(string, 'R');
		flags &= ~elf::PF_R;
	}
	if (flags) {
		AppendToString(string, "+0x");
		AppendToStringWithBase(string, flags, 16);
	}
}

void AppendToString(String &string, elf::SectionType sectionType) {
	switch(sectionType) {
		case elf::SectionType::UNUSED:
			AppendToString(string, "UNUSED");
			break;
		case elf::SectionType::PROGRAM_DATA:
			AppendToString(string, "PROGRAM_DATA");
			break;
		case elf::SectionType::SYMBOL_TABLE:
			AppendToString(string, "SYMBOL_TABLE");
			break;
		case elf::SectionType::STRING_TABLE:
			AppendToString(string, "STRING_TABLE");
			break;
		case elf::SectionType::RELOCATION_ADDENDS:
			AppendToString(string, "RELOCATION_ADDENDS");
			break;
		case elf::SectionType::HASH_TABLE:
			AppendToString(string, "HASH_TABLE");
			break;
		case elf::SectionType::DYNAMIC:
			AppendToString(string, "DYNAMIC");
			break;
		case elf::SectionType::NOTE:
			AppendToString(string, "NOTE");
			break;
		case elf::SectionType::NOBITS:
			AppendToString(string, "NOBITS");
			break;
		case elf::SectionType::RELOCATION:
			AppendToString(string, "RELOCATION");
			break;
		case elf::SectionType::SHLIB:
			AppendToString(string, "SHLIB");
			break;
		case elf::SectionType::DYNAMIC_SYMBOL_TABLE:
			AppendToString(string, "DYNAMIC_SYMBOL_TABLE");
			break;
		case elf::SectionType::INIT_ARRAY:
			AppendToString(string, "INIT_ARRAY");
			break;
		case elf::SectionType::DEINIT_ARRAY:
			AppendToString(string, "DEINIT_ARRAY");
			break;
		case elf::SectionType::PREINIT_ARRAY:
			AppendToString(string, "PREINIT_ARRAY");
			break;
		case elf::SectionType::GROUP:
			AppendToString(string, "GROUP");
			break;
		case elf::SectionType::SYMBOL_TABLE_EXT:
			AppendToString(string, "SYMBOL_TABLE_EXT");
			break;
		case elf::SectionType::NUM:
			AppendToString(string, "NUM");
			break;
		default:
			AppendToString(string, "0x");
			AppendToStringWithBase(string, (u32)sectionType, 16);
			break;
	}
}

void AppendToString(String &string, elf::SectionFlags sectionFlags) {
	u64 flags = sectionFlags;
	if (flags == 0) {
		AppendToString(string, "NONE");
		return;
	}
	bool once = false;
	if (flags & elf::SHF_WRITE) {
		AppendToString(string, "WRITE|");
		flags &= ~elf::SHF_WRITE;
		once = true;
	}
	if (flags & elf::SHF_ALLOC) {
		AppendToString(string, "ALLOC|");
		flags &= ~elf::SHF_ALLOC;
		once = true;
	}
	if (flags & elf::SHF_EXECINSTR) {
		AppendToString(string, "EXECINSTR|");
		flags &= ~elf::SHF_EXECINSTR;
		once = true;
	}
	if (flags & elf::SHF_MERGE) {
		AppendToString(string, "MERGE|");
		flags &= ~elf::SHF_MERGE;
		once = true;
	}
	if (flags & elf::SHF_STRINGS) {
		AppendToString(string, "STRINGS|");
		flags &= ~elf::SHF_STRINGS;
		once = true;
	}
	if (flags & elf::SHF_INFO_LINK) {
		AppendToString(string, "INFO_LINK|");
		flags &= ~elf::SHF_INFO_LINK;
		once = true;
	}
	if (flags & elf::SHF_LINK_ORDER) {
		AppendToString(string, "LINK_ORDER|");
		flags &= ~elf::SHF_LINK_ORDER;
		once = true;
	}
	if (flags & elf::SHF_OS_NONCONFORMING) {
		AppendToString(string, "OS_NONCONFORMING|");
		flags &= ~elf::SHF_OS_NONCONFORMING;
		once = true;
	}
	if (flags & elf::SHF_GROUP) {
		AppendToString(string, "GROUP|");
		flags &= ~elf::SHF_GROUP;
		once = true;
	}
	if (flags & elf::SHF_THREAD_LOCAL) {
		AppendToString(string, "THREAD_LOCAL|");
		flags &= ~elf::SHF_THREAD_LOCAL;
		once = true;
	}
	if (flags & elf::SHF_ORDERED) {
		AppendToString(string, "ORDERED|");
		flags &= ~elf::SHF_ORDERED;
		once = true;
	}
	if (flags & elf::SHF_EXCLUDE) {
		AppendToString(string, "EXCLUDE|");
		flags &= ~elf::SHF_EXCLUDE;
		once = true;
	}
	if (once) {
		string.size--; // Remove trailing '|'
	}
	if (flags) {
		AppendToString(string, "+0x");
		AppendToStringWithBase(string, flags, 16);
	}
}

} // namespace AzCore