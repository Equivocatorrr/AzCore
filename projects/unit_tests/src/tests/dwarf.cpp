/*
	File: dwarf.cpp
	Author: Philip Haynes
*/

#include "../UnitTests.hpp"
#include "../Utilities.hpp"

#include "AzCore/dwarf/dwarf.hpp"

namespace DwarfTestNamespace {

using namespace AzCore::dwarf;

void dwarfTest();
UT::Register DwarfTest("dwarf", dwarfTest);

const ULEB ulebTests[] = {
	{ 2,       { 0x02       } },
	{ 127,     { 0x7f       } },
	{ 128,     { 0x80, 0x01 } },
	{ 129,     { 0x81, 0x01 } },
	{ 12857,   { 0xb9, 0x64 } },
	{ 0x10000, { 0x80, 0x80, 0x04 } },
	{ ((u64)1 << 63), { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01 } },
};
constexpr i32 ulebTestsCount = sizeof(ulebTests) / sizeof(ULEB);

const SLEB slebTests[] = {
	{  2,       { 0x02 } },
	{ -2,       { 0x7e } },
	{  127,     { 0xff, 0x00 } },
	{ -127,     { 0x81, 0x7f } },
	{  128,     { 0x80, 0x01 } },
	{ -128,     { 0x80, 0x7f } },
	{  129,     { 0x81, 0x01 } },
	{ -129,     { 0xff, 0x7e } },
	{  0x10000, { 0x80, 0x80, 0x04 } },
	{ -0x10000, { 0x80, 0x80, 0x7c } },
	{ ((i64)1 << 62), { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xc0, 0x00 } },
	{ ((i64)1 << 63), { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f } },
};
constexpr i32 slebTestsCount = sizeof(slebTests) / sizeof(SLEB);

void dwarfTest() {
	ULEB ulebGen, ulebTest;
	for (i32 i = 0; i < ulebTestsCount; i++) {
		ulebTest = ulebTests[i];
		ulebGen = EncodeULEB(ulebTest.value);
		UTExpectEquals(ulebGen, ulebTest, " EncodeULEB ", i);
		ulebGen = DecodeULEB(ulebTest.binary).AzUnwrap();
		UTExpectEquals(ulebGen, ulebTest, " DecodeULEB ", i);
	}

	SLEB slebGen, slebTest;
	for (i32 i = 0; i < slebTestsCount; i++) {
		slebTest = slebTests[i];
		slebGen = EncodeSLEB(slebTest.value);
		UTExpectEquals(slebGen, slebTest, " EncodeSLEB ", i);
		slebGen = DecodeSLEB(slebTest.binary).AzUnwrap();
		UTExpectEquals(slebGen, slebTest, " DecodeSLEB ", i);
	}
	UT::ReportInfo(__LINE__, "There were ", ulebTestsCount, " ULEB tests and ", slebTestsCount, " SLEB tests!");
}

} // namespace DwarfTestNamespace