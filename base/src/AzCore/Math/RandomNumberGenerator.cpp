/*
	File: RandomNumberGenerator.cpp
	Author: Philip Haynes
*/

#include "RandomNumberGenerator.hpp"
#include "../Time.hpp"
#include "../Memory/HashMap.hpp"
#include "../Memory/Array.hpp"

namespace AzCore {

RandomNumberGenerator::RandomNumberGenerator() {
	Seed(Clock::now().time_since_epoch().count());
}

u32 RandomNumberGenerator::Generate() {
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

void RandomNumberGenerator::Seed(u64 seed) {
	// The power of keysmashes!
	if (seed == 0)
		seed += 3478596;
	x = seed;
	y = seed * 16807;
	z = seed * 47628;
	c = seed * 32497;
}

RandomNumberGenerator globalRNG;

f32 random(f32 min, f32 max, RandomNumberGenerator *rng) {
	if (nullptr == rng) rng = &globalRNG;
	u32 num = rng->Generate();
	return (f32)(((f64)num * (f64)(max - min) / (f64)UINT32_MAX) + (f64)min);
}

i32 random(i32 min, i32 max, RandomNumberGenerator *rng) {
	if (nullptr == rng) rng = &globalRNG;
	return i32(rng->Generate() % (max - min + 1)) + min;
}

i32 genShuffleId() {
	static i32 id = 0;
	return ++id;
}

struct Playlist {
	Array<i32> indices;
	i32 current;
};

HashMap<u32, Playlist> playlists;

u32 GetActualId(i32 id, i32 size) {
	u64 idSeed = ((u64)id << 32) + (u64)size;
	RandomNumberGenerator idGenerator(idSeed);
	return idGenerator.Generate();
}

void shuffleReset(i32 id, i32 size, RandomNumberGenerator *rng) {
	if (nullptr == rng) rng = &globalRNG;
	u32 actualId = GetActualId(id, size);
	Playlist &playlist = playlists[actualId];
	playlist.indices.Reserve(size);
	playlist.indices.size = 0;
	playlist.current = 0;
	for (i32 i = 0; i < size; i++) {
		i32 index = rng->Generate() % (playlist.indices.size+1);
		playlist.indices.Insert(index, i);
	}
}

i32 shuffle(i32 id, i32 size, RandomNumberGenerator *rng) {
	if (nullptr == rng) rng = &globalRNG;
	u32 actualId = GetActualId(id, size);
	Playlist &playlist = playlists[actualId];
	++playlist.current;
	if (playlist.current == size || playlist.indices.size != size) {
		shuffleReset(id, size, rng);
	}
	return playlist.indices[playlist.current];
}

} // namespace AzCore
