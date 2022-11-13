/*
    File: String.cpp
    Author: Philip Haynes
*/

#include "../UnitTests.hpp"
#include "../Utilities.hpp"

#include "AzCore/Math/RandomNumberGenerator.hpp"
#include "AzCore/Memory/String.hpp"

namespace StringTestNamespace {

void StringTest();
UT::Register stringTest("String", StringTest);

using namespace AzCore;

FPError<f32> fpError;

#define COMPARE_FP(lhs, rhs, magnitude) \
	fpError.Compare(lhs, rhs, magnitude, __LINE__)

#define CHECK_F32_STRING(_real) \
	real = (_real); \
	str = ToString(real); \
	UTExpectEquals(str, #_real); \
	UTAssert(StringToF32(str, &real2)); \
	COMPARE_FP(real, real2, real);

#define CHECK_F32(_real) \
	real = (_real); \
	str = ToString(real); \
	UTAssert(StringToF32(str, &real2)); \
	COMPARE_FP(real, real2, real);

#define CHECK_I32_STRING(_integer) \
	integer1 = (_integer); \
	str = ToString(integer1); \
	UTExpectEquals(str, #_integer); \
	UTAssert(StringToI32(str, &integer2)); \
	UTExpectEquals(integer1, integer2);

#define CHECK_I32(_integer) \
	integer1 = (_integer); \
	str = ToString(integer1); \
	UTAssert(StringToI32(str, &integer2)); \
	UTExpectEquals(integer1, integer2);

void StringTest() {
	f32 real, real2;
	String str;

	// float ToString and StringToF32
	CHECK_F32_STRING(0.0f);
	CHECK_F32_STRING(0.1f);
	CHECK_F32_STRING(0.111f);
	CHECK_F32_STRING(10.0f);
	CHECK_F32_STRING(100.0f);
	CHECK_F32_STRING(1000.0f);
	real = 69.420f;
	str = ToString(real, 10, 5);
	UTExpectEquals(str, "69.42");
	UTAssert(StringToF32(str, &real2));
	COMPARE_FP(real, real2, real);
	real = 1.0f / 10000.0f;
	str = ToString(real);
	UTExpectEquals(str, "1.0e-4");
	UTAssert(StringToF32(str, &real2));
	COMPARE_FP(real, real2, real);
	real = 0.1f - 1.0f / 100000.0f;
	str = ToString(real, 10, 2);
	UTExpectEquals(str, "0.1");
	UTAssert(StringToF32(str, &real2));
	COMPARE_FP(0.1f, real2, 0.1f);

	RandomNumberGenerator rng(69420);

	for (i32 i = 0; i < 100000; i++) {
		CHECK_F32(random(-1000000.0f, 1000000.0f, &rng));
	}

	// int ToString and StringToI32
	i32 integer1, integer2;

	CHECK_I32_STRING(0);
	CHECK_I32_STRING(1);
	CHECK_I32_STRING(2);
	CHECK_I32_STRING(-1);
	CHECK_I32_STRING(-2);

	for (i32 i = 0; i < 100000; i++) {
		CHECK_I32((i32)rng.Generate());
	}

	// String modification

	str = "ha";
	str.Append("HA");
	UTExpectEquals(str, "haHA");
	str.Erase(0, 2);
	UTExpectEquals(str, "HA");
	str.Insert(1, "12");
	UTExpectEquals(str, "H12A");
	String str2 = "What the ";
	str2.Append(std::move(str));
	UTExpectEquals(str2, "What the H12A");
	UTExpectEquals(str, "");
	str2.Back() = '3';
	UTExpectEquals(str2, "What the H123");
	str = "This is a sentence that has to go on the heap";
	str2 = " because our String only has so much stack space.";
	str += str2;
	UTExpectEquals(str, "This is a sentence that has to go on the heap because our String only has so much stack space.");
	UTExpectEquals(str2, " because our String only has so much stack space.");
	UTExpect(str2.Contains('.'));
	UTExpect(!str2.Contains('0'));
	UTExpectEquals(str2.Count(' '), 9);
	UTExpectEquals(str2.Count('b'), 1);
	UTExpectEquals(str2.Count('1'), 0);
	str2.Reverse();
	UTExpectEquals(str2, ".ecaps kcats hcum os sah ylno gnirtS ruo esuaceb ");
	str.Clear();
	UTExpectEquals(str, "");

	fpError.Report(__LINE__);
}

} // namespace StringTestNamespace