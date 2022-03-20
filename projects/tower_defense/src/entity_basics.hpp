/*
	File: entity_basics.hpp
	Author: Philip Haynes
	The basic building blocks that help define interactions
	between entities and how they're stored in memory.
*/

#ifndef ENTITY_BASICS_HPP
#define ENTITY_BASICS_HPP

#include "AzCore/Thread.hpp"
#include "AzCore/math.hpp"
#include "rendering.hpp"

namespace Entities {

using namespace AzCore;

struct AABB {
	vec2 minPos, maxPos;

	bool Collides(const AABB &other) const;
	void Update(const struct Physical &physical);
};

enum CollisionType {
	SEGMENT,
	CIRCLE,
	BOX
};

union PhysicalBasis {
	struct {
		vec2 a, b;
	} segment;
	struct {
		vec2 c;
		f32 r;
	} circle;
	struct {
		vec2 a, b;
		// ┌─────────► x
		// │
		// │    a ───┐
		// │    │     │
		// │    └─── b
		// ▼
		// y
	} box;
};

union PhysicalAbs {
	struct {
		vec2 a, b;
	} segment;
	struct {
		vec2 c;
		f32 r;
	} circle;
	struct {
		vec2 a, b, c, d;
		// ┌─────────► x
		// │
		// │    a ─── c
		// │    │      │
		// │    d ─── b
		// ▼
		// y
	} box;
};

struct Physical {
	mutable AABB aabb;
	CollisionType type;
	Angle32 angle = 0.0f;
	PhysicalBasis basis; // What you set to define the collider
	mutable PhysicalAbs actual; // Will be updated at most once a frame (only when collision checking is happening)
	mutable bool updated = false;
	vec2 pos;
	vec2 vel = vec2(0.0f);
	Radians32 rot = 0.0f;

	bool Collides(const Physical &other) const;
	bool MouseOver() const;
	void Update(f32 timestep);
	void UpdateActual() const;
	inline void Impulse(vec2 amount, f32 timestep) {
		amount *= timestep;
		vel += amount;
		pos += 0.5f * amount * timestep;
	}
	inline void ImpulseX(f32 amount, f32 timestep) {
		amount *= timestep;
		vel.x += amount;
		pos.x += 0.5f * amount * timestep;
	}
	inline void ImpulseY(f32 amount, f32 timestep) {
		amount *= timestep;
		vel.y += amount;
		pos.y += 0.5f * amount * timestep;
	}
	void Draw(Rendering::DrawingContext &context, vec4 color) const;
};

/*  struct: Id
	Author: Philip Haynes
	Used to identify unique objects.    */
struct Id {
	// NOTE: We're limited to 65536 objects at once, which should be more than we'll ever need.
	union {
		i32 data;
		struct {
			u16 index;
			i16 generation; // If generation is negative, then the object doesn't exist.
		};
	};

	Id() : data(0) {}
	Id(const i32 &a) : data(a) {}
	inline bool operator==(const Id &other) const {
		return data == other.data;
	}
	inline bool operator!=(const Id &other) const {
		return data != other.data;
	}
	inline Id& operator=(const i32 &a) {
		data = a;
		return *this;
	}
	// inline Id& operator=(const Id &other) {
	//	 data = other.data;
	//	 return *this;
	// }
	inline bool operator<(const Id& other) const {
		return data < other.data;
	}
};


typedef void (*fpUpdateCallback)(void*,i32,i32);
typedef void (*fpDrawCallback)(void*,Rendering::DrawingContext*,i32,i32);

struct UpdateChunk {
	fpUpdateCallback updateCallback;
	fpDrawCallback drawCallback;
	void *theThisPointer;
};

/*  struct: DoubleBufferArray
	Author: Philip Haynes
	Stores a copy of objects that are read-only and a copy that get updated.    */
template<typename T>
struct DoubleBufferArray {
	// Data is stored in the balls.
	Array<T> array[2];
	// New objects created every frame, added during Synchronize.
	Array<T> created;
	// Indices of array that can be refilled
	Array<u16> empty;
	// Indices of array that must be destroyed during Synchronize.
	Array<u16> destroyed;
	// Used to synchronize access to created and destroyed
	Mutex mutex;
	i32 size = 0;
	i32 count = 0;
	bool buffer = false;
	i32 granularity = 10;

	static void Update(void *theThisPointer, i32 threadIndex, i32 concurrency);
	static void Draw(void *theThisPointer, Rendering::DrawingContext *context, i32 threadIndex, i32 concurrency);
	// Done between frames. Must be done synchronously.
	void Synchronize();
	void GetUpdateChunks(Array<UpdateChunk> &dstUpdateChunks);
	void Create(T &obj);
	void Destroy(Id id);
	// Provides read-only access for interactions
	inline const T& operator[](const i32 &index) {
		return array[!buffer][index];
	}
	inline const T& operator[](const Id &id) {
		return array[!buffer][id.index];
	}
	inline T& GetMutable(const i32 &index) {
		return array[buffer][index];
	}
	inline T& GetMutable(const Id &id) {
		return array[buffer][id.index];
	}
	inline void Clear() {
		array[0].Clear();
		array[1].Clear();
		created.Clear();
		empty.Clear();
		destroyed.Clear();
		size = 0;
		count = 0;
		buffer = false;
	}
};

/*  struct: Entity
	Author: Philip Haynes
	Baseline entity. Anything in a DoubleBufferArray must be inherited from this.    */
struct Entity {
	Id id;
	Physical physical;
	// void EventCreate();
	// void Update(f32 timestep);
	// void Draw(Rendering::DrawingContext &context);
	void EventDestroy() {};
};

} // namespace Entities

#endif // ENTITY_BASICS_HPP
