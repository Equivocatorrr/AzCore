/*
    File: RandomNumberGenerator.cpp
    Author: Philip Haynes
*/

#include "RandomNumberGenerator.hpp"
#include "../Time.hpp"

namespace AzCore {

RandomNumberGenerator::RandomNumberGenerator()
{
    Seed(Clock::now().time_since_epoch().count());
}

u32 RandomNumberGenerator::Generate()
{
    u64 t;
    x = 314527869 * x + 1234567;
    y ^= y << 5;
    y ^= y >> 7;
    y ^= y << 22;
    t = 4294584393ULL * (u64)z + (u64)c;
    c = t >> 32;
    z = t;
    return x + y + z;
}

void RandomNumberGenerator::Seed(u64 seed)
{
    // The power of keysmashes!
    if (seed == 0)
        seed += 3478596;
    x = seed;
    y = seed * 16807;
    z = seed * 47628;
    c = seed * 32497;
}

f32 random(f32 min, f32 max, RandomNumberGenerator &rng)
{
    u32 num = rng.Generate();
    return ((f64)num * (f64)(max - min) / (f64)UINT32_MAX) + (f64)min;
}

i32 random(i32 min, i32 max, RandomNumberGenerator &rng)
{
    return i32(rng.Generate() % (max - min)) + min;
}

} // namespace AzCore