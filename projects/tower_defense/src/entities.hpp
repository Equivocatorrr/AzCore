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

enum TowerType {
	TOWER_GUN=0,
	TOWER_SHOTGUN=1,
	TOWER_FAN=2,
	TOWER_SHOCKWAVE=3,
	TOWER_GAUSS=4,
	TOWER_FLAK=5,
	TOWER_MAX_RANGE=5
};

struct TowerUpgradeables {
	bool data[5];
};

extern const char* towerStrings[TOWER_MAX_RANGE+1];
extern const i32 towerCosts[TOWER_MAX_RANGE+1];
extern const bool towerHasPriority[TOWER_MAX_RANGE+1];
extern const TowerUpgradeables towerUpgradeables[TOWER_MAX_RANGE+1];
extern const char* towerDescriptions[TOWER_MAX_RANGE+1];

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
	az::Degrees32 bulletSpread;
	i32 bulletCount;
	i32 damage;
	f32 bulletSpeed;
	f32 bulletSpeedVariability;
	i32 bulletExplosionDamage;
	f32 bulletExplosionRange;
	i64 sunkCost;
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
			f32 _range, f32 _shootInterval, az::Degrees32 _bulletSpread, i32 _bulletCount,
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
	enum Type {
		BASIC,
		HONKER,
		ORBITER,
		STUNNER
	} type;
	i32 hitpoints;
	f32 size;
	f32 targetSpeed;
	f32 spawnTimer;
	vec4 color;
	i32 value;
	az::BinarySet<Id> damageContributors;
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

struct FailureText {
	vec2 position;
	f32 angle;
	f32 size;
	vec2 velocity;
	f32 rotation;
	f32 scaleSpeed;
	vec2 targetPosition;
	f32 targetAngle;
	f32 targetSize;
	az::WString text;

	void Reset();
	void Update(f32 timestep);
	void Draw(Rendering::DrawingContext &context);
};

struct Manager : public ManagerBasic {
	DoubleBufferArray<Tower> towers{};
	DoubleBufferArray<Enemy> enemies{};
	DoubleBufferArray<Bullet> bullets{};
	DoubleBufferArray<Wind> winds{};
	DoubleBufferArray<Explosion> explosions{};
	
	Sound::Source sndMoney;
	Sound::Stream streamSegment1;
	Sound::Stream streamSegment2;
	Id selectedTower = -1;
	bool focusMenu = false;
	bool placeMode = false;
	TowerType towerType = TOWER_GUN;
	az::Angle32 placingAngle = 0.0f;
	bool canPlace = false;
	f32 enemyTimer = 0.0;
	i32 wave = 0;
	i64 hitpointsLeft = 0;
	f64 hitpointsPerSecond = 200.0;
	i64 lives = 1000;
	i64 money = 5000;
	f32 timestep;
	bool waveActive = true;
	bool failed = false;
	f32 backgroundTransition = -1.0;
	vec3 backgroundFrom;
	vec3 backgroundTo;
	vec2 mouse = 0.0f;
	FailureText failureText;
	Physical basePhysical{};
	az::Array<Physical> enemySpawns{};
	
	Manager();
	
	void EventAssetsQueue() override;
	void EventAssetsAcquire() override;
	void EventInitialize() override;
	void EventSync() override;
	void EventDraw(az::Array<Rendering::DrawingContext> &contexts) override;
	
	void CreateSpawn();
	void Reset();

	inline void HandleUI();
	inline void HandleGamepadUI();
	inline void HandleMouseUI();
	inline void HandleGamepadCamera();
	inline void HandleMouseCamera();
	inline void HandleTowerPlacement(u8 keycodePlace);
	inline void HandleMusicLoops(i32 w);
	inline bool CursorVisible() const;
};

extern Manager *entities;

} // namespace Az2D::Entities

#endif // ENTITIES_HPP
