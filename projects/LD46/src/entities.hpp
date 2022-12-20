/*
	File: entities.hpp
	Author: Philip Haynes
	All the different types of objects that can interact with each other with collision.
*/

#ifndef ENTITIES_HPP
#define ENTITIES_HPP

#include "Az2D/game_systems.hpp"
#include "AzCore/math.hpp"

#include "Az2D/entity_basics.hpp"

namespace Az2D::Entities {

using az::vec2, az::vec3, az::vec4;

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
	az::WString text;

	void Reset();
	void Update(f32 timestep);
	void Draw(Rendering::DrawingContext &context);
};

struct Lantern {
	vec2 pos;
	vec2 posPrev = 0.0f;
	vec2 vel = 0.0f;
	vec2 velPrev = 0.0f;
	az::Angle32 angle = 0.0f;
	az::Radians32 rot = 0.0f;
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
	az::Angle32 angle = az::pi/2.0f;
	az::Radians32 rot = az::pi/2.0f;
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
	az::vec2i size = 0;
	az::Array<u8> data{};
	inline void Resize(az::vec2i newSize) {
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
	inline u8& operator[](az::vec2i pos) {
		return data[pos.y*size.x + pos.x];
	}
	bool Solid(AABB aabb);
	bool Water(AABB aabb);
	bool Goal(AABB aabb);
	bool Save(az::String filename);
	bool Load(az::String filename);
};

struct Manager : public ManagerBasic {
	DoubleBufferArray<Player> players{};
	DoubleBufferArray<Sprinkler> sprinklers{};
	DoubleBufferArray<Droplet> droplets{};
	DoubleBufferArray<Flame> flames{};
	az::Array<az::String> levelNames;
	i32 level = 0;

	// sprites
	Assets::TexIndex texPlayerJump;
	Assets::TexIndex texPlayerFloat;
	Assets::TexIndex texPlayerStand;
	Assets::TexIndex texPlayerRun;
	Assets::TexIndex texPlayerWallTouch;
	Assets::TexIndex texPlayerWallBack;
	Assets::TexIndex texLantern;
	Assets::TexIndex texBeacon;
	Assets::TexIndex texSprinkler;

	// sounds
	Sound::Source jump1Sources[4];
	Sound::MultiSource jump1;
	Sound::Source jump2Sources[3];
	Sound::MultiSource jump2;
	Sound::Source stepSources[8];
	Sound::MultiSource step;
	Sound::MultiSource *jump = &jump2;
	Sound::Stream music;

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

	Manager();

	void EventAssetsQueue() override;
	void EventAssetsAcquire() override;
	void EventInitialize() override;
	void EventSync() override;
	void EventDraw(az::Array<Rendering::DrawingContext> &contexts) override;

	void Reset();

	inline void HandleUI();
};

extern Manager *entities;

} // namespace Entities

#endif // ENTITIES_HPP
