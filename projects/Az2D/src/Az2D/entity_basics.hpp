/*
	File: entity_basics.hpp
	Author: Philip Haynes
	The basic building blocks that help define interactions
	between entities and how they're stored in memory.
*/

#ifndef AZ2D_ENTITY_BASICS_HPP
#define AZ2D_ENTITY_BASICS_HPP

#include "game_systems.hpp"
#include "AzCore/Thread.hpp"
#include "AzCore/math.hpp"
#include "rendering.hpp"
#include <type_traits> // is_base_of

namespace Az2D::Entities {

using az::vec2, az::vec3, az::vec4;
using GameSystems::sys;

extern struct ManagerBasic *entitiesBasic;

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
	az::Angle32 angle = 0.0f;
	PhysicalBasis basis; // What you set to define the collider
	mutable PhysicalAbs actual; // Will be updated at most once a frame (only when collision checking is happening)
	mutable bool updated = false;
	vec2 pos;
	vec2 vel = vec2(0.0f);
	az::Radians32 rot = 0.0f;

	bool Collides(const Physical &other) const;
	bool MouseOver(vec2 mouse) const;
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
	Used to identify unique objects with a known type.    */
struct Id {
	union {
		i64 data;
		struct {
			i32 index;
			i32 generation; // If generation is negative, then the object doesn't exist.
		};
	};

	Id() : data(0) {}
	Id(i64 a) : data(a) {}
	inline bool operator==(Id other) const {
		return data == other.data;
	}
	inline bool operator!=(Id other) const {
		return data != other.data;
	}
	inline Id& operator=(i64 a) {
		data = a;
		return *this;
	}
	// inline Id& operator=(const Id &other) {
	//	 data = other.data;
	//	 return *this;
	// }
	inline bool operator<(Id other) const {
		return data < other.data;
	}
};

typedef u64 TypeID;

extern az::Array<void*> _DoubleBufferArrays;
TypeID GenTypeId(void *ptr);


/*  struct: IdGeneric */
struct IdGeneric {
	Id id;
	TypeID type=UINT64_MAX;
	
	const struct Entity& GetConst() const;
	struct Entity& GetMut() const;
	inline bool Valid() const;
	inline bool operator==(IdGeneric other) const {
		return id == other.id && type == other.type;
	}
};


/*  struct: Entity
	Author: Philip Haynes
	Baseline entity. Anything in a DoubleBufferArray must be inherited from this.    */
struct Entity {
	union {
		Id id; // Occupies the same place as IdGeneric, so just a nice accessor
		IdGeneric idGeneric;
	};
	Physical physical;
	inline Entity() : idGeneric() {}
	inline Entity(const Entity &other) : idGeneric(other.idGeneric), physical(other.physical) {}
	inline Entity(Entity &&other) : idGeneric(other.idGeneric), physical(other.physical) {}
	inline Entity& operator=(const Entity &other) {
		 idGeneric = other.idGeneric;
		 physical = other.physical;
		 return *this;
	}
	inline Entity& operator=(Entity &&other) {
		 idGeneric = other.idGeneric;
		 physical = other.physical;
		 return *this;
	}
	// void EventCreate();
	// void Update(f32 timestep);
	// void Draw(Rendering::DrawingContext &context);
	void EventDestroy() {};
};

inline bool IdGeneric::Valid() const {
	if (type == UINT64_MAX) return false;
	const Entity &entity = GetConst();
	return entity.id.generation > 0;
}

typedef void (*fpUpdateCallback)(void*,i32,i32);
typedef void (*fpDrawCallback)(void*,Rendering::DrawingContext*,i32,i32);

struct WorkChunk {
	fpUpdateCallback updateCallback;
	fpDrawCallback drawCallback;
	void *theThisPointer;
};

/*  struct: DoubleBufferArray
	Author: Philip Haynes
	Stores a copy of objects that are read-only and a copy that get updated.    */
template<typename T>
struct DoubleBufferArray {
	// Our IdGeneric assumes we have Entity subclasses
	static_assert(std::is_base_of<Entity, T>::value == true);
	// Data is stored in the balls.
	az::Array<T> array[2];
	// New objects created every frame, added during Synchronize.
	az::Array<T> created;
	// Indices of array that can be refilled
	az::Array<i32> empty;
	// Indices of array that must be destroyed during Synchronize.
	az::Array<i32> destroyed;
	// Used to synchronize access to created and destroyed
	size_t typeStride = sizeof(T);
	TypeID typeId = GenTypeId(this);
	az::Mutex mutex;
	i32 size = 0;
	i32 count = 0;
	i32 claimedEmpty = 0;
	i32 claimedNew = 0;
	bool buffer = false;
	i32 granularity = 10;
	
	DoubleBufferArray() = default;
	// Delete these because _DoubleBufferArrays will be invalidated if we move
	DoubleBufferArray(const DoubleBufferArray&) = delete;
	DoubleBufferArray(DoubleBufferArray&&) = delete;
	
	inline az::Array<T>& ArrayConst() {
		return array[!buffer];
	}
	inline az::Array<T>& ArrayMut() {
		return array[buffer];
	}
	inline T& EntityConst(i32 index) {
		return ArrayConst()[index];
	}
	inline T& EntityMut(i32 index) {
		return ArrayMut()[index];
	}
	inline T& EntityConst(Id id) {
		return ArrayConst()[id.index];
	}
	inline T& EntityMut(Id id) {
		return ArrayMut()[id.index];
	}
	
	static void Update(void *theThisPointer, i32 threadIndex, i32 concurrency);
	static void Draw(void *theThisPointer, Rendering::DrawingContext *context, i32 threadIndex, i32 concurrency) {
		DoubleBufferArray<T> *theActualThisPointer = (DoubleBufferArray<T>*)theThisPointer;
		i32 g = theActualThisPointer->granularity;
		for (i32 i = threadIndex*g; i < theActualThisPointer->array[!theActualThisPointer->buffer].size; i += g*concurrency) {
			for (i32 j = 0; j < g; j++) {
				if (i+j >= theActualThisPointer->array[!theActualThisPointer->buffer].size) break;
				T &obj = theActualThisPointer->array[!theActualThisPointer->buffer][i+j];
				if (obj.id.generation > 0)
					obj.Draw(*context);
			}
		}
	}
	// Done between frames. Must be done synchronously.
	void Synchronize() {
		buffer = sys->buffer;

		empty.Erase(0, claimedEmpty);
		for (i32 index : destroyed) {
			// array[!buffer][index].EventDestroy();
			array[!buffer][index].id.generation = -EntityConst(index).id.generation-1;
			empty.Append(index);
		}
		count -= destroyed.size;
		destroyed.Clear();
		for (T &obj : created) {
			if (ArrayConst().size <= obj.id.index) {
				ArrayConst().Resize(obj.id.index+1);
			}
			EntityConst(obj.id) = std::move(obj);
		}
		count += created.size;
		created.Clear();
		claimedEmpty = 0;
		claimedNew = 0;
		array[buffer] = array[!buffer];
		size = array[0].size;
	}
	void GetWorkChunks(az::Array<WorkChunk> &dstWorkChunks) {
		if (count == 0) return;
		WorkChunk chunk;
		chunk.updateCallback = DoubleBufferArray<T>::Update;
		chunk.drawCallback = DoubleBufferArray<T>::Draw;
		chunk.theThisPointer = this;
		dstWorkChunks.Append(chunk);
	}
	az::Ptr<T> Create(T &obj) {
		mutex.Lock();
		if (empty.size > claimedEmpty) {
			obj.id.index = empty[claimedEmpty];
			obj.id.generation = -EntityConst(obj.id).id.generation;
			claimedEmpty++;
		} else {
			obj.id.index = ArrayConst().size + claimedNew;
			obj.id.generation = 1;
			claimedNew++;
		}
		obj.idGeneric.type = typeId;
		obj.EventCreate();
		created.Append(obj);
		mutex.Unlock();
		return created.GetPtr(created.size-1);
	}
	void Destroy(Id id) {
		mutex.Lock();
		if (EntityConst(id).id != id) {
			mutex.Unlock();
			return;
		}
		if (destroyed.Contains(id.index)) {
			mutex.Unlock();
			return;
		}
		EntityConst(id).EventDestroy();
		destroyed.Append(id.index);
		EntityConst(id).id.generation *= -1;
		mutex.Unlock();
	}
	// Provides read-only access for interactions
	inline const T& operator[](i32 index) {
		return array[!buffer][index];
	}
	inline const T& operator[](Id id) {
		return array[!buffer][id.index];
	}
	inline T& GetMutable(i32 index) {
		return array[buffer][index];
	}
	inline T& GetMutable(Id id) {
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

struct ManagerBasic : public GameSystems::System {
	az::Array<WorkChunk> workChunks{};
	// Our version integrates simulationRate
	f32 timestep;
	f32 camZoom = 0.00001f;
	vec2 camPos = vec2(0.0f);
	vec2 WorldPosToScreen(vec2 in) const;
	vec2 ScreenPosToWorld(vec2 in) const;
	AABB CamBounds() const;
	vec2 CamTopLeft() const;
	vec2 CamBottomRight() const;

	ManagerBasic();

	void EventSync() override;
	void EventUpdate() override;
	void EventDraw(az::Array<Rendering::DrawingContext> &contexts) override;
};

template<typename T>
void DoubleBufferArray<T>::Update(void *theThisPointer, i32 threadIndex, i32 concurrency) {
	DoubleBufferArray<T> *theActualThisPointer = (DoubleBufferArray<T>*)theThisPointer;
	i32 g = theActualThisPointer->granularity;
	for (i32 i = threadIndex*g; i < theActualThisPointer->array[theActualThisPointer->buffer].size; i += g*concurrency) {
		for (i32 j = 0; j < g; j++) {
			if (i+j >= theActualThisPointer->array[theActualThisPointer->buffer].size) break;
			T &obj = theActualThisPointer->array[theActualThisPointer->buffer][i+j];
			if (obj.id.generation > 0) {
				obj.Update(entitiesBasic->timestep);
			}
		}
	}
}

} // namespace Az2D::Entities

#endif // AZ2D_ENTITY_BASICS_HPP
