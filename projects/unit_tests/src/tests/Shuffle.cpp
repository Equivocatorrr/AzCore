/*
	File: Shuffle.cpp
	Author: Philip Haynes
*/

#include "../UnitTests.hpp"
#include "AzCore/Math/RandomNumberGenerator.hpp"
#include "AzCore/Memory/HashSet.hpp"

namespace ShuffleTestNamespace {

using namespace AzCore;

void ShuffleTest();
UT::Register shuffleTest("Shuffle", ShuffleTest);

void ShuffleTest() {
	HashSet<i32> set;
	i32 shuffleId = genShuffleId();
	for (i32 i = 0; i < 1000; i++) {
		i32 index = shuffle(shuffleId, 1000);
		UTExpect(!set.Exists(index), "shuffle() returned the same value twice");
		set.Emplace(index);
	}
	for (i32 i = 0; i < 1000; i++) {
		UTExpect(set.Exists(shuffle(shuffleId, 1000)), "shuffle() returned a value that hasn't been seen before");
	}
}

} // namespace ShuffleTestNamespace