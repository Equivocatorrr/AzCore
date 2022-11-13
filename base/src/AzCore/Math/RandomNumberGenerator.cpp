/*
	File: RandomNumberGenerator.cpp
	Author: Philip Haynes
*/

#include "RandomNumberGenerator.hpp"
#include "../Time.hpp"
#include "../memory.hpp"

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
	u32 num = rng->Generate() & 0x7fffff;
	return (f32)num / (f32)0x7fffff * (max - min) + min;
}

f64 random(f64 min, f64 max, RandomNumberGenerator *rng) {
	if (nullptr == rng) rng = &globalRNG;
	u64 num = rng->Generate() | (((u64)rng->Generate() & 0xfffff) << 32);
	return (f64)num / (f64)0xfffffffffffff * (max - min) + min;
}

i32 random(i32 min, i32 max, RandomNumberGenerator *rng) {
	if (nullptr == rng) rng = &globalRNG;
	AzAssert(min <= max, "random() min must be <= max");
	u32 span = max - min + 1;
	AzAssert(span != 0, "random() min and max are invalid!");
	return i32(rng->Generate() % span) + min;
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
	i32 carryOver;
	if (playlist.indices.size) {
		carryOver = playlist.indices.Back();
	} else {
		carryOver = rng->Generate() % size;
	}
	playlist.indices.Reserve(size);
	playlist.indices.size = 0;
	playlist.current = 0;
	for (i32 i = 0; i < size; i++) {
		i32 index = rng->Generate() % (playlist.indices.size+1);
		playlist.indices.Insert(index, i);
	}
	if (playlist.indices.size > 1 && playlist.indices[0] == carryOver) {
		Swap(playlist.indices[0], playlist.indices[1]);
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
