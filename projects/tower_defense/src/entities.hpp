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

struct Entity;

struct Manager : public Objects::Object {
    Array<Entity> entities{};
    i32 selectedEntity = -1;
    i32 texCircle;
    void EventAssetInit();
    void EventAssetAcquire();
    void EventInitialize();
    void EventUpdate();
    void EventDraw(VkCommandBuffer commandBuffer);
};

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

struct Entity {
    i32 index;
    Physical physical;
    bool colliding;
    void Update(f32 timestep);
    void Draw(VkCommandBuffer commandBuffer);
};

} // namespace Entities

#endif // ENTITIES_HPP
