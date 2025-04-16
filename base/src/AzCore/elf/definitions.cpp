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
		AppendToString(string, FormatInt((u32)elfClass, 16, true));
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
		AppendToString(string, FormatInt((u32)elfEndian, 16, true));
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
		case elf::ElfOsAbi::ARM:
			AppendToString(string, "ARM");
			break;
		case elf::ElfOsAbi::STANDALONE:
			AppendToString(string, "STANDALONE");
			break;
		default:
			AppendToString(string, FormatInt((u32)elfOsAbi, 16, true));
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
		AppendToString(string, FormatInt((u32)elfType, 16, true));
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
		case elf::ElfISA::FUJITSU_VPP500:
			AppendToString(string, "FUJITSU_VPP500");
			break;
		case elf::ElfISA::SPARC32PLUS:
			AppendToString(string, "SPARC32PLUS");
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
		case elf::ElfISA::DONALD_KNUTH_MMIX:
			AppendToString(string, "DONALD_KNUTH_MMIX");
			break;
		case elf::ElfISA::HARVARD_ANY:
			AppendToString(string, "HARVARD_ANY");
			break;
		case elf::ElfISA::SITERA_PRISM:
			AppendToString(string, "SITERA_PRISM");
			break;
		case elf::ElfISA::AMTEL_AVR:
			AppendToString(string, "AMTEL_AVR");
			break;
		case elf::ElfISA::FUJITSU_FR30:
			AppendToString(string, "FUJITSU_FR30");
			break;
		case elf::ElfISA::MITSUBISHI_D10V:
			AppendToString(string, "MITSUBISHI_D10V");
			break;
		case elf::ElfISA::MITSUBISHI_D30V:
			AppendToString(string, "MITSUBISHI_D30V");
			break;
		case elf::ElfISA::NEC_V850:
			AppendToString(string, "NEC_V850");
			break;
		case elf::ElfISA::MITSUBISHI_M32R:
			AppendToString(string, "MITSUBISHI_M32R");
			break;
		case elf::ElfISA::MATSUSHITA_MN10300:
			AppendToString(string, "MATSUSHITA_MN10300");
			break;
		case elf::ElfISA::MATSUSHITA_MN10200:
			AppendToString(string, "MATSUSHITA_MN10200");
			break;
		case elf::ElfISA::PICO_JAVA:
			AppendToString(string, "PICO_JAVA");
			break;
		case elf::ElfISA::OPENRISC:
			AppendToString(string, "OPENRISC");
			break;
		case elf::ElfISA::ARC_COMPACT:
			AppendToString(string, "ARC_COMPACT");
			break;
		case elf::ElfISA::TENSILICA_XTENSA:
			AppendToString(string, "TENSILICA_XTENSA");
			break;
		case elf::ElfISA::ALPHAMOSAIC_VIDEOCORE:
			AppendToString(string, "ALPHAMOSAIC_VIDEOCORE");
			break;
		case elf::ElfISA::THOMPSON_MULTIMEDIA_GPP:
			AppendToString(string, "THOMPSON_MULTIMEDIA_GPP");
			break;
		case elf::ElfISA::NATIONAL_SEMI_32K:
			AppendToString(string, "NATIONAL_SEMI_32K");
			break;
		case elf::ElfISA::TENOR_NETWORK_TPC:
			AppendToString(string, "TENOR_NETWORK_TPC");
			break;
		case elf::ElfISA::TREBIA_SNP1K:
			AppendToString(string, "TREBIA_SNP1K");
			break;
		case elf::ElfISA::STM_ST200:
			AppendToString(string, "STM_ST200");
			break;
		case elf::ElfISA::UBICOM_IP2K:
			AppendToString(string, "UBICOM_IP2K");
			break;
		case elf::ElfISA::MAX:
			AppendToString(string, "MAX");
			break;
		case elf::ElfISA::NATIONAL_SEMI_CR:
			AppendToString(string, "NATIONAL_SEMI_CR");
			break;
		case elf::ElfISA::FUJITSU_F2MC16:
			AppendToString(string, "FUJITSU_F2MC16");
			break;
		case elf::ElfISA::TI_MSP430:
			AppendToString(string, "TI_MSP430");
			break;
		case elf::ElfISA::AD_BLACKFIN_DSP:
			AppendToString(string, "AD_BLACKFIN_DSP");
			break;
		case elf::ElfISA::SEIKO_EPSON_S1C33:
			AppendToString(string, "SEIKO_EPSON_S1C33");
			break;
		case elf::ElfISA::SHARP_EMBEDDED_PROCESSOR:
			AppendToString(string, "SHARP_EMBEDDED_PROCESSOR");
			break;
		case elf::ElfISA::ARCA_RISC:
			AppendToString(string, "ARCA_RISC");
			break;
		case elf::ElfISA::UNICORE:
			AppendToString(string, "UNICORE");
			break;
		case elf::ElfISA::EXCESS:
			AppendToString(string, "EXCESS");
			break;
		case elf::ElfISA::ICERA_SEMI_DXP:
			AppendToString(string, "ICERA_SEMI_DXP");
			break;
		case elf::ElfISA::ALTERA_NIOS_II:
			AppendToString(string, "ALTERA_NIOS_II");
			break;
		case elf::ElfISA::NATIONAL_SEMI_CRX:
			AppendToString(string, "NATIONAL_SEMI_CRX");
			break;
		case elf::ElfISA::MOTOROLA_XGATE:
			AppendToString(string, "MOTOROLA_XGATE");
			break;
		case elf::ElfISA::INFINEON_C166:
			AppendToString(string, "INFINEON_C166");
			break;
		case elf::ElfISA::RENESAS_M16C:
			AppendToString(string, "RENESAS_M16C");
			break;
		case elf::ElfISA::MICROCHIP_TECH_DSPIC30F:
			AppendToString(string, "MICROCHIP_TECH_DSPIC30F");
			break;
		case elf::ElfISA::FREESCALE_CE_RISC:
			AppendToString(string, "FREESCALE_CE_RISC");
			break;
		case elf::ElfISA::RENESAS_M32C:
			AppendToString(string, "RENESAS_M32C");
			break;
		case elf::ElfISA::ALTIUM_TSK3000:
			AppendToString(string, "ALTIUM_TSK3000");
			break;
		case elf::ElfISA::FREESCALE_RS08:
			AppendToString(string, "FREESCALE_RS08");
			break;
		case elf::ElfISA::AD_SHARC:
			AppendToString(string, "AD_SHARC");
			break;
		case elf::ElfISA::CYAN_TECH_ECOG2:
			AppendToString(string, "CYAN_TECH_ECOG2");
			break;
		case elf::ElfISA::SUNPLUS_SCORE7_RISC:
			AppendToString(string, "SUNPLUS_SCORE7_RISC");
			break;
		case elf::ElfISA::NJR_DSP24_DSP:
			AppendToString(string, "NJR_DSP24_DSP");
			break;
		case elf::ElfISA::BROADCOM_VIDEOCORE_III:
			AppendToString(string, "BROADCOM_VIDEOCORE_III");
			break;
		case elf::ElfISA::LATTICEMICO32_RISC:
			AppendToString(string, "LATTICEMICO32_RISC");
			break;
		case elf::ElfISA::SEIKO_EPSON_C17:
			AppendToString(string, "SEIKO_EPSON_C17");
			break;
		case elf::ElfISA::TI_C6000_DSP:
			AppendToString(string, "TI_C6000_DSP");
			break;
		case elf::ElfISA::TI_C2000_DSP:
			AppendToString(string, "TI_C2000_DSP");
			break;
		case elf::ElfISA::TI_C5500_DSP:
			AppendToString(string, "TI_C5500_DSP");
			break;
		case elf::ElfISA::TI_ARP32_RISC:
			AppendToString(string, "TI_ARP32_RISC");
			break;
		case elf::ElfISA::TI_PRU:
			AppendToString(string, "TI_PRU");
			break;
		case elf::ElfISA::STM_MMDSP_PLUS:
			AppendToString(string, "STM_MMDSP_PLUS");
			break;
		case elf::ElfISA::CYPRESS_M8C:
			AppendToString(string, "CYPRESS_M8C");
			break;
		case elf::ElfISA::RENESAS_R32C:
			AppendToString(string, "RENESAS_R32C");
			break;
		case elf::ElfISA::NXP_SEMI_TRIMEDIA:
			AppendToString(string, "NXP_SEMI_TRIMEDIA");
			break;
		case elf::ElfISA::QUALCOMM_DSP6:
			AppendToString(string, "QUALCOMM_DSP6");
			break;
		case elf::ElfISA::INTEL_8051:
			AppendToString(string, "INTEL_8051");
			break;
		case elf::ElfISA::STM_STXP7X:
			AppendToString(string, "STM_STXP7X");
			break;
		case elf::ElfISA::ANDES_TECH_NDS32_RISC:
			AppendToString(string, "ANDES_TECH_NDS32_RISC");
			break;
		case elf::ElfISA::CYAN_TECH_ECOG1X:
			AppendToString(string, "CYAN_TECH_ECOG1X");
			break;
		case elf::ElfISA::DALLAS_SEMI_MAXQ30:
			AppendToString(string, "DALLAS_SEMI_MAXQ30");
			break;
		case elf::ElfISA::NJR_XIMO16_DSP:
			AppendToString(string, "NJR_XIMO16_DSP");
			break;
		case elf::ElfISA::MANIK:
			AppendToString(string, "MANIK");
			break;
		case elf::ElfISA::CRAYNV2:
			AppendToString(string, "CRAYNV2");
			break;
		case elf::ElfISA::RX:
			AppendToString(string, "RX");
			break;
		case elf::ElfISA::METAG:
			AppendToString(string, "METAG");
			break;
		case elf::ElfISA::MCST_ELBRUS_E2K:
			AppendToString(string, "MCST_ELBRUS_E2K");
			break;
		case elf::ElfISA::ECOG16:
			AppendToString(string, "ECOG16");
			break;
		case elf::ElfISA::CR16:
			AppendToString(string, "CR16");
			break;
		case elf::ElfISA::ETPU:
			AppendToString(string, "ETPU");
			break;
		case elf::ElfISA::SLE9X:
			AppendToString(string, "SLE9X");
			break;
		case elf::ElfISA::L10M:
			AppendToString(string, "L10M");
			break;
		case elf::ElfISA::K10M:
			AppendToString(string, "K10M");
			break;
		case elf::ElfISA::ARM_64:
			AppendToString(string, "ARM_64");
			break;
		case elf::ElfISA::AMTEL_AVR32:
			AppendToString(string, "AMTEL_AVR32");
			break;
		case elf::ElfISA::STM_STM8:
			AppendToString(string, "STM_STM8");
			break;
		case elf::ElfISA::TILERA_TILE64:
			AppendToString(string, "TILERA_TILE64");
			break;
		case elf::ElfISA::TILERA_TILEPRO:
			AppendToString(string, "TILERA_TILEPRO");
			break;
		case elf::ElfISA::XILINX_MICROBLAZE:
			AppendToString(string, "XILINX_MICROBLAZE");
			break;
		case elf::ElfISA::NVIDIA_CUDA:
			AppendToString(string, "NVIDIA_CUDA");
			break;
		case elf::ElfISA::TILERA_TILEGX:
			AppendToString(string, "TILERA_TILEGX");
			break;
		case elf::ElfISA::CLOUDSHIELD:
			AppendToString(string, "CLOUDSHIELD");
			break;
		case elf::ElfISA::KIPO_KAIST_COREA_1ST:
			AppendToString(string, "KIPO_KAIST_COREA_1ST");
			break;
		case elf::ElfISA::KIPO_KAIST_COREA_2ND:
			AppendToString(string, "KIPO_KAIST_COREA_2ND");
			break;
		case elf::ElfISA::SYNOPSYS_ARCV2:
			AppendToString(string, "SYNOPSYS_ARCV2");
			break;
		case elf::ElfISA::OPEN8_RISC:
			AppendToString(string, "OPEN8_RISC");
			break;
		case elf::ElfISA::RENESAS_RL78:
			AppendToString(string, "RENESAS_RL78");
			break;
		case elf::ElfISA::BROADCOM_VIDEOCORE_V:
			AppendToString(string, "BROADCOM_VIDEOCORE_V");
			break;
		case elf::ElfISA::RENESAS_78KOR:
			AppendToString(string, "RENESAS_78KOR");
			break;
		case elf::ElfISA::FREESCALE_56800EX_DSC:
			AppendToString(string, "FREESCALE_56800EX_DSC");
			break;
		case elf::ElfISA::BEYOND_BA1:
			AppendToString(string, "BEYOND_BA1");
			break;
		case elf::ElfISA::BEYOND_BA2:
			AppendToString(string, "BEYOND_BA2");
			break;
		case elf::ElfISA::XMOS_XCORE:
			AppendToString(string, "XMOS_XCORE");
			break;
		case elf::ElfISA::MICROCHIP_PIC:
			AppendToString(string, "MICROCHIP_PIC");
			break;
		case elf::ElfISA::INTEL_GRAPHICS_TECH:
			AppendToString(string, "INTEL_GRAPHICS_TECH");
			break;
		case elf::ElfISA::KM211_KM32:
			AppendToString(string, "KM211_KM32");
			break;
		case elf::ElfISA::KM211_KMX32:
			AppendToString(string, "KM211_KMX32");
			break;
		case elf::ElfISA::KM211_KMX16:
			AppendToString(string, "KM211_KMX16");
			break;
		case elf::ElfISA::KM211_KMX8:
			AppendToString(string, "KM211_KMX8");
			break;
		case elf::ElfISA::KM211_KVARC:
			AppendToString(string, "KM211_KVARC");
			break;
		case elf::ElfISA::PANEVE_CDP:
			AppendToString(string, "PANEVE_CDP");
			break;
		case elf::ElfISA::COGE:
			AppendToString(string, "COGE");
			break;
		case elf::ElfISA::BLUECHIP_COOLENGINE:
			AppendToString(string, "BLUECHIP_COOLENGINE");
			break;
		case elf::ElfISA::NANORADIO_OPTIMIZED_RISC:
			AppendToString(string, "NANORADIO_OPTIMIZED_RISC");
			break;
		case elf::ElfISA::CSR_KALIMBA:
			AppendToString(string, "CSR_KALIMBA");
			break;
		case elf::ElfISA::ZILOG_Z80:
			AppendToString(string, "ZILOG_Z80");
			break;
		case elf::ElfISA::CDS_VISIUM:
			AppendToString(string, "CDS_VISIUM");
			break;
		case elf::ElfISA::FTDI_FT32:
			AppendToString(string, "FTDI_FT32");
			break;
		case elf::ElfISA::MOXIE:
			AppendToString(string, "MOXIE");
			break;
		case elf::ElfISA::AMD_GPU:
			AppendToString(string, "AMD_GPU");
			break;
		case elf::ElfISA::RISC_V:
			AppendToString(string, "RISC_V");
			break;
		case elf::ElfISA::BERKELEY_PACKET_FILTER:
			AppendToString(string, "BERKELEY_PACKET_FILTER");
			break;
		case elf::ElfISA::C_SKY:
			AppendToString(string, "C_SKY");
			break;
		case elf::ElfISA::WDC_65C816:
			AppendToString(string, "WDC_65C816");
			break;
		case elf::ElfISA::LOONG_ARCH:
			AppendToString(string, "LOONG_ARCH");
			break;
		default:
			AppendToString(string, FormatInt((u32)elfISA, 16, true));
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
		switch (programType) {
			case elf::ProgramType::GNU_EH_FRAME:
				AppendToString(string, "GNU_EH_FRAME");
				break;
			case elf::ProgramType::GNU_STACK:
				AppendToString(string, "GNU_STACK");
				break;
			case elf::ProgramType::GNU_RELRO:
				AppendToString(string, "GNU_RELRO");
				break;
			case elf::ProgramType::GNU_PROPERTY:
				AppendToString(string, "GNU_PROPERTY");
				break;
			case elf::ProgramType::SUN_WBSS:
				AppendToString(string, "SUN_WBSS");
				break;
			case elf::ProgramType::SUN_WSTACK:
				AppendToString(string, "SUN_WSTACK");
				break;
			case elf::ProgramType::AARCH64_MEMTAG_MTE:
				AppendToString(string, "AARCH64_MEMTAG_MTE");
				break;
			default:
				AppendToString(string, FormatInt((u32)programType, 16, true));
				break;
		}
	}
}

void AppendToString(String &string, elf::SegmentFlags segmentFlags) {
	u32 flags = segmentFlags;
	if (flags == 0) {
		AppendToString(string, "NONE");
		return;
	}
	if (flags & elf::PF_R) {
		string.Append('R');
		flags &= ~elf::PF_R;
	} else {
		string.Append('_');
	}
	if (flags & elf::PF_W) {
		string.Append('W');
		flags &= ~elf::PF_W;
	} else {
		string.Append('_');
	}
	if (flags & elf::PF_X) {
		string.Append('X');
		flags &= ~elf::PF_X;
	} else {
		string.Append('_');
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
		case elf::SectionType::GNU_ATTRIBUTES:
			AppendToString(string, "GNU_ATTRIBUTES");
			break;
		case elf::SectionType::GNU_HASH:
			AppendToString(string, "GNU_HASH");
			break;
		case elf::SectionType::GNU_LIBLIST:
			AppendToString(string, "GNU_LIBLIST");
			break;
		case elf::SectionType::CHECKSUM:
			AppendToString(string, "CHECKSUM");
			break;
		case elf::SectionType::SUNW_MOVE:
			AppendToString(string, "SUNW_MOVE");
			break;
		case elf::SectionType::SUNW_COMDAT:
			AppendToString(string, "SUNW_COMDAT");
			break;
		case elf::SectionType::SUNW_SYMINFO:
			AppendToString(string, "SUNW_SYMINFO");
			break;
		case elf::SectionType::GNU_VERSION_DEFINITION:
			AppendToString(string, "GNU_VERSION_DEFINITION");
			break;
		case elf::SectionType::GNU_VERSION_NEEDS:
			AppendToString(string, "GNU_VERSION_NEEDS");
			break;
		case elf::SectionType::GNU_VERSION_SYMBOL_TABLE:
			AppendToString(string, "GNU_VERSION_SYMBOL_TABLE");
			break;
		default:
			AppendToString(string, FormatInt((u32)sectionType, 16, true));
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
	if (flags & elf::SHF_COMPRESSED) {
		AppendToString(string, "COMPRESSED|");
		flags &= ~elf::SHF_COMPRESSED;
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