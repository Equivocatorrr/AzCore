/*
    File: entities.hpp
    Author: Philip Haynes
    All the different types of objects that can interact with each other with collision.
*/

#ifndef ENTITIES_HPP
#define ENTITIES_HPP

#include "objects.hpp"
#include "AzCore/math.hpp"

namespace Entities {

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
    Angle32 angle = 0.0;
    PhysicalBasis basis; // What you set to define the collider
    mutable PhysicalAbs actual; // Will be updated at most once a frame (only when collision checking is happening)
    mutable bool updated = false;
    vec2 pos;
    vec2 vel = vec2(0.0);
    Radians32 rot = 0.0;

    bool Collides(const Physical &other) const;
    bool MouseOver() const;
    void Update(f32 timestep);
    void UpdateActual() const;
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
            i16 generation = 0; // If generation is negative, then the object doesn't exist.
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
    inline Id& operator=(const Id &other) {
        data = other.data;
        return *this;
    }
};

/*  struct: Entity
    Author: Philip Haynes
    Baseline entity. Anything in a DoubleBufferArray must be inherited from this.    */
struct Entity {
    Id id;
    Physical physical;
    bool colliding;
    void EventCreate();
    void Update(f32 timestep);
    void Draw(VkCommandBuffer commandBuffer);
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
    bool buffer = false;

    void Update(f32 timestep);
    void Draw(VkCommandBuffer commandBuffer);
    // Done between frames. Must be done synchronously.
    void Synchronize();
    void Create(T &obj);
    void Destroy(Id id);
    // Provides read-only access for interactions
    inline const T& operator[](const i32 &index) {
        return array[!buffer][index];
    }
    inline const T& operator[](const Id &id) {
        return array[!buffer][id.index];
    }
};

struct Manager : public Objects::Object {
    DoubleBufferArray<Entity> entities{};
    Id selectedEntity = -1;
    i32 texCircle;
    void EventAssetInit();
    void EventAssetAcquire();
    void EventInitialize();
    void EventUpdate();
    void EventDraw(VkCommandBuffer commandBuffer);
};

extern template struct DoubleBufferArray<Entity>;

} // namespace Entities

#endif // ENTITIES_HPP
