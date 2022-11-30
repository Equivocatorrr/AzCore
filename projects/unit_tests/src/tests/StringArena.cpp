/*
	File: StringArena.cpp
	Author: Philip Haynes
*/

#include "../UnitTests.hpp"

#include "AzCore/Memory/StringArena.hpp"

namespace StringArenaTestNamespace {

// Use a small page size to test making a new page
// NOTE: This could be done by brute-forcing tons of unique strings,
//   and it probably should be done that way.
AZCORE_CREATE_STRING_ARENA_LOCAL_SIZE(32)

void StringArenaTest();
UT::Register stringArenaTest("StringArena", StringArenaTest);

using namespace AzCore;

void StringArenaTest() {
	AString str1 = "Hey!";
	AString str2 = "Hey hey!";
	UTExpect(str1 != str2);
	String string2 = str2.GetString();
	string2.Erase(3, 4);
	str2 = string2;
	UTExpectEquals(str1, str2);
	AString str3 = Stringify(str2, " What's up?");
	str1 = "Hey! What's up?";
	UTExpectEquals(str1, str3);
	AString str4 = "This should make a new page.";
	UTExpectEquals(str4, "This should make a new page.");
}

} // namespace StringArenaTestNamespace