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
struct RandomNumberGenerator
{
    u32 x, y, z, c;
    RandomNumberGenerator(); // Automatically seeds itself based on time.
    u32 Generate();
    void Seed(u64 seed);
};

f32 random(f32 min, f32 max, RandomNumberGenerator &rng);
i32 random(i32 min, i32 max, RandomNumberGenerator &rng);

} // namespace AzCore

#endif // AZCORE_RANDOMNUMBERGENERATOR_HPP