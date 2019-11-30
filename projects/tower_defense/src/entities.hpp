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
    inline void Impulse(vec2 amount, f32 timestep) {
        amount *= timestep;
        vel += amount;
        pos += 0.5 * amount * timestep;
    }
    inline void ImpulseX(f32 amount, f32 timestep) {
        amount *= timestep;
        vel.x += amount;
        pos.x += 0.5 * amount * timestep;
    }
    inline void ImpulseY(f32 amount, f32 timestep) {
        amount *= timestep;
        vel.y += amount;
        pos.y += 0.5 * amount * timestep;
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

enum TowerType {
    TOWER_GUN=0,
    TOWER_SHOTGUN=1,
    TOWER_FAN=2,
    TOWER_SHOCKWAVE=3,
    TOWER_GAUSS=4,
    TOWER_FLAK=5,
    TOWER_MAX_RANGE=5
};

extern const char* towerStrings[TOWER_MAX_RANGE+1];
extern const i32 towerCosts[TOWER_MAX_RANGE+1];
extern const char* towerDescriptions[TOWER_MAX_RANGE+1];

struct Tower;
struct Enemy;
struct Bullet;
struct Wind;
struct Explosion;

struct Manager : public Objects::Object {
    DoubleBufferArray<Tower> towers{};
    DoubleBufferArray<Enemy> enemies{};
    DoubleBufferArray<Bullet> bullets{};
    DoubleBufferArray<Wind> winds{};
    DoubleBufferArray<Explosion> explosions{};
    Array<UpdateChunk> updateChunks{};
    Id selectedTower = -1;
    bool placeMode = false;
    TowerType towerType = TOWER_GUN;
    Angle32 placingAngle = 0.0;
    bool canPlace = false;
    f32 enemyTimer = 0.0;
    i32 wave = 0;
    i64 hitpointsLeft = 0;
    f32 hitpointsPerSecond = 200.0;
    i32 lives = 1000;
    i32 money = 5000;
    f32 timestep;
    bool waveActive = false;
    f32 camZoom = 1.0;
    vec2 camPos = 0.0;
    vec2 mouse = 0.0;
    Physical basePhysical{};
    Array<Physical> enemySpawns{};
    void EventAssetInit();
    void EventAssetAcquire();
    void EventInitialize();
    void EventSync();
    void EventUpdate();
    void EventDraw(Array<Rendering::DrawingContext> &contexts);
    void CreateSpawn();
};

struct Tower : public Entity {
    TowerType type;
    // field is for AOE effects, and is only used by certain types of towers
    // For those that don't have AOE, it's used to illustrate range.
    Physical field;
    bool selected;
    bool disabled;
    f32 range;
    f32 shootTimer;
    f32 shootInterval;
    Degrees32 bulletSpread;
    i32 bulletCount;
    i32 damage;
    f32 bulletSpeed;
    f32 bulletSpeedVariability;
    i32 bulletExplosionDamage;
    f32 bulletExplosionRange;
    i32 sunkCost;
    vec4 color;

    enum TargetPriority {
        PRIORITY_NEAREST,
        PRIORITY_FURTHEST,
        PRIORITY_WEAKEST,
        PRIORITY_STRONGEST,
        PRIORITY_NEWEST,
        PRIORITY_OLDEST
    } priority;
    static const char *priorityStrings[6];
    i64 kills;
    i64 damageDone;

    Tower() = default;
    inline Tower(CollisionType collisionType, PhysicalBasis physicalBasis,
            CollisionType fieldCollisionType, PhysicalBasis fieldPhysicalBasis, TowerType _type,
            f32 _range, f32 _shootInterval, Degrees32 _bulletSpread, i32 _bulletCount,
            i32 _damage, f32 _bulletSpeed, f32 _bulletSpeedVariability, i32 _bulletExplosionDamage, f32 _bulletExplosionRange, vec4 _color) :
            type(_type), range(_range), shootInterval(_shootInterval), bulletSpread(_bulletSpread),
            bulletCount(_bulletCount), damage(_damage), bulletSpeed(_bulletSpeed),
            bulletSpeedVariability(_bulletSpeedVariability), bulletExplosionDamage(_bulletExplosionDamage),
            bulletExplosionRange(_bulletExplosionRange), color(_color) {
        physical.type = collisionType;
        physical.basis = physicalBasis;
        field.type = fieldCollisionType;
        field.basis = fieldPhysicalBasis;
    }
    Tower(TowerType _type);
    void EventCreate();
    void Update(f32 timestep);
    void Draw(Rendering::DrawingContext &context);
};
extern template struct DoubleBufferArray<Tower>;

struct Enemy : public Entity {
    i32 hitpoints;
    f32 size;
    f32 targetSpeed;
    f32 spawnTimer;
    vec4 color;
    i32 value;
    Set<Id> damageContributors;
    f32 age;
    bool child = false;
    void EventCreate();
    void EventDestroy();
    void Update(f32 timestep);
    void Draw(Rendering::DrawingContext &context);
};
extern template struct DoubleBufferArray<Enemy>;

struct Bullet : public Entity {
    f32 lifetime;
    i32 damage;
    i32 explosionDamage;
    f32 explosionRange;
    Id owner;
    void EventCreate();
    void EventDestroy();
    void Update(f32 timestep);
    void Draw(Rendering::DrawingContext &context);
};
extern template struct DoubleBufferArray<Bullet>;

struct Wind : public Entity {
    f32 lifetime;
    void EventCreate();
    void Update(f32 timestep);
    void Draw(Rendering::DrawingContext &context);
};
extern template struct DoubleBufferArray<Wind>;

struct Explosion : public Entity {
    f32 size, growth;
    i32 damage; // per second
    Id owner;
    void EventCreate();
    void Update(f32 timestep);
    void Draw(Rendering::DrawingContext &context);
};
extern template struct DoubleBufferArray<Explosion>;

} // namespace Entities

#endif // ENTITIES_HPP
