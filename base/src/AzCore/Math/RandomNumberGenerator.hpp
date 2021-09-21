/*
	File: RandomNumberGenerator.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_RANDOMNUMBERGENERATOR_HPP
#define AZCORE_RANDOMNUMBERGENERATOR_HPP

#include "../basictypes.hpp"

namespace AzCore {

/*  struct: RandomNumberGenerator
	Author: Philip Haynes
	Uses the JKISS generator by David Jones
	From http://www0.cs.ucl.ac.uk/staff/d.jones/GoodPracticeRNG.pdf */
struct RandomNumberGenerator {
	u32 x, y, z, c;
	RandomNumberGenerator(); // Automatically seeds itself based on time.
	inline RandomNumberGenerator(u64 seed) {
		Seed(seed);
	}
	u32 Generate();
	void Seed(u64 seed);
};

f32 random(f32 min, f32 max, RandomNumberGenerator *rng=nullptr);
i32 random(i32 min, i32 max, RandomNumberGenerator *rng=nullptr);
// Gets a unique id for shuffling.
// Generally only call this once and store the result as a handle to use in shuffle(...)
i32 genShuffleId();
// id and size are used to keep track of the playlist.
// If the same id but different size is used, that is considered to be a different playlist.
i32 shuffle(i32 id, i32 size, RandomNumberGenerator *rng=nullptr);
// Resets the playlist used by shuffle(...).
void shuffleReset(i32 id, i32 size, RandomNumberGenerator *rng=nullptr);

} // namespace AzCore

#endif // AZCORE_RANDOMNUMBERGENERATOR_HPP
