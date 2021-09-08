/*
	File: entities.hpp
	Author: Philip Haynes
	All the different types of objects that can interact with each other with collision.
*/

#ifndef ENTITIES_HPP
#define ENTITIES_HPP

#include "objects.hpp"
#include "AzCore/math.hpp"
#include "rendering.hpp"
#include "sound.hpp"

#include "entity_basics.hpp"

namespace Entities {

using namespace AzCore;

struct MessageText {
	vec2 position;
	f32 angle;
	f32 size;
	vec2 velocity;
	f32 rotation;
	f32 scaleSpeed;
	vec2 targetPosition;
	f32 targetAngle;
	f32 targetSize;
	vec4 color;
	WString text;

	void Reset();
	void Update(f32 timestep);
	void Draw(Rendering::DrawingContext &context);
};

struct Player;
struct Sprinkler;
struct Droplet;
struct Flame;

struct World {
	enum Block {
		BLOCK_AIR = 0,
		BLOCK_PLAYER,
		BLOCK_WALL,
		BLOCK_WATER_FULL,
		BLOCK_WATER_TOP,
		BLOCK_GOAL,
		BLOCK_SPRINKLER,
		BLOCK_TYPE_COUNT
	};
	vec2i size = 0;
	Array<u8> data{};
	inline void Resize(vec2i newSize) {
		data.Resize(newSize.x * newSize.y, BLOCK_AIR);
		size = newSize;
		for (i32 i = 0; i < data.size; i++) {
			if (i < size.x || i >= data.size-size.x || i % size.x == 0 || (i+1) % size.x == 0) {
				data[i] = BLOCK_WALL;
			} else {
				data[i] = BLOCK_AIR;
			}
		}
	}
	void Draw(Rendering::DrawingContext &context, bool playing, bool under);
	inline u8& operator[](vec2i pos) {
		return data[pos.y*size.x + pos.x];
	}
	bool Solid(AABB aabb);
	bool Water(AABB aabb);
	bool Goal(AABB aabb);
	bool Save(String filename);
	bool Load(String filename);
};

struct Manager : public Objects::Object {
	DoubleBufferArray<Player> players{};
	DoubleBufferArray<Sprinkler> sprinklers{};
	DoubleBufferArray<Droplet> droplets{};
	DoubleBufferArray<Flame> flames{};
	Array<UpdateChunk> updateChunks{};
	Array<String> levelNames{};
	i32 level = 0;

	// sprites
	i32 playerJump;
	i32 playerFloat;
	i32 playerStand;
	i32 playerRun;
	i32 playerWallTouch;
	i32 playerWallBack;
	i32 lantern;
	i32 beacon;
	i32 sprinkler;

	// sounds
	Sound::Source jump1Sources[4];
	Sound::MultiSource jump1;
	Sound::Source jump2Sources[3];
	Sound::MultiSource jump2;
	Sound::Source stepSources[8];
	Sound::MultiSource step;
	Sound::MultiSource *jump = &jump2;
	Sound::Stream music;

	f32 timestep = 1.0f/60.0f;
	f32 camZoom = 0.00001f;
	vec2 camPos = vec2(0.0f);
	vec2 mouse = 0.0f;
	f32 gas = 15.0f;
	f32 flame = 1.0f;
	f32 goalFlame = 0.0f;
	f32 flameTimer = 0.0f;
	vec2 goalPos;
	vec2 lanternPos = 0.0f;
	f32 nextLevelTimer = 0.0f;
	World::Block toPlace = World::BLOCK_WALL;
	MessageText failureText;
	MessageText successText;
	MessageText winText;
	World world;
	void EventAssetInit();
	void EventAssetAcquire();
	void EventInitialize();
	void EventSync();
	void EventUpdate();
	void EventDraw(Array<Rendering::DrawingContext> &contexts);
	vec2 WorldPosToScreen(vec2 in) const;
	vec2 ScreenPosToWorld(vec2 in) const;

	void Reset();

	inline void HandleUI();
	inline void HandleGamepadUI();
	inline void HandleMouseUI();
	inline bool CursorVisible() const;
};

struct Lantern {
	vec2 pos;
	vec2 posPrev = 0.0f;
	vec2 vel = 0.0f;
	vec2 velPrev = 0.0f;
	Angle32 angle = 0.0f;
	Radians32 rot = 0.0f;
	f32 particleTimer = 0.0f;

	void Update(f32 timestep);
	void Draw(Rendering::DrawingContext &context);
};

struct Player : public Entity {
	enum Anim {
		RUN,
		JUMP,
		FLOAT,
		WALL_TOUCH,
		WALL_JUMP
	} anim = RUN;
	f32 animTime = 0.0f;
	bool facingRight = true;
	Lantern lantern;
	void EventCreate();
	void Update(f32 timestep);
	void Draw(Rendering::DrawingContext &context);
};
extern template struct DoubleBufferArray<Player>;

struct Sprinkler : public Entity {
	Angle32 angle = pi/2.0f;
	Radians32 rot = pi/2.0f;
	f32 shootTimer = 0.0f;
	void EventCreate();
	void Update(f32 timestep);
	void Draw(Rendering::DrawingContext &context);
};
extern template struct DoubleBufferArray<Sprinkler>;

struct Droplet : public Entity {
	f32 lifetime;
	void EventCreate();
	void Update(f32 timestep);
	void Draw(Rendering::DrawingContext &context);
};
extern template struct DoubleBufferArray<Droplet>;

struct Flame : public Entity {
	f32 size;
	void EventCreate();
	void Update(f32 timestep);
	void Draw(Rendering::DrawingContext &context);
};
extern template struct DoubleBufferArray<Flame>;

} // namespace Entities

#endif // ENTITIES_HPP
