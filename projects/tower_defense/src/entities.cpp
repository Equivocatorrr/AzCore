/*
    File: entities.cpp
    Author: Philip Haynes
*/

#include "entities.hpp"
#include "globals.hpp"

namespace Entities {

const char* towerStrings[TOWER_MAX_RANGE+1] = {
    "Gun",
    "Shotgun",
    "Fan",
    "Gauss",
    "Shocker",
    "Flak"
};

const Tower towerGunTemplate = Tower(
    BOX,                                        // CollisionType
    {vec2(-20.0), vec2(20.0)},                  // PhysicalBasis
    CIRCLE,                                     // fieldCollisionType
    {vec2(0.0), 400.0},                         // fieldPhysicalBasis
    TOWER_GUN,                                  // TowerType
    400.0,                                      // range
    0.1,                                        // shootInterval
    1.0,                                        // bulletSpread (degrees)
    1,                                          // bulletCount
    10,                                         // damage
    500.0,                                      // bulletSpeed
    50.0,                                       // bulletSpeedVariability
    0,                                          // bulletExplosionDamage
    0.0,                                        // bulletExplosionRange
    vec4(0.1, 0.1, 1.0, 1.0)                    // color
);

const Tower towerShotgunTemplate = Tower(
    BOX,                                        // CollisionType
    {vec2(-16.0), vec2(16.0)},                  // PhysicalBasis
    CIRCLE,                                     // fieldCollisionType
    {vec2(0.0), 250.0},                         // fieldPhysicalBasis
    TOWER_SHOTGUN,                              // TowerType
    250.0,                                      // range
    1.0,                                        // shootInterval
    12.0,                                       // bulletSpread (degrees)
    20,                                         // bulletCount
    10,                                         // damage
    600.0,                                      // bulletSpeed
    200.0,                                      // bulletSpeedVariability
    0,                                          // bulletExplosionDamage
    0.0,                                        // bulletExplosionRange
    vec4(0.1, 1.0, 0.5, 1.0)                    // color
);

const Tower towerFanTemplate = Tower(
    BOX,                                        // CollisionType
    {vec2(-10.0, -32.0), vec2(10.0, 32.0)},     // PhysicalBasis
    BOX,                                        // fieldCollisionType
    {vec2(-50.0, -40.0), vec2(300.0, 40.0)},    // fieldPhysicalBasis
    TOWER_FAN,                                  // TowerType
    300.0,                                      // range
    0.1,                                        // shootInterval
    10.0,                                       // bulletSpread (degrees)
    2,                                          // bulletCount
    1,                                          // damage
    800.0,                                      // bulletSpeed
    200.0,                                      // bulletSpeedVariability
    0,                                          // bulletExplosionDamage
    0.0,                                        // bulletExplosionRange
    vec4(0.5, 1.0, 0.1, 1.0)                    // color
);

const Tower towerGaussTemplate = Tower(
    BOX,                                        // CollisionType
    {vec2(-32.0), vec2(32.0)},                  // PhysicalBasis
    CIRCLE,                                     // fieldCollisionType
    {vec2(0.0, 0.0), 800.0},                    // fieldPhysicalBasis
    TOWER_GAUSS,                                // TowerType
    800.0,                                      // range
    2.0,                                        // shootInterval
    3.0,                                        // bulletSpread (degrees)
    1,                                          // bulletCount
    2000,                                       // damage
    2400.0,                                     // bulletSpeed
    0.0,                                        // bulletSpeedVariability
    0,                                          // bulletExplosionDamage
    0.0,                                        // bulletExplosionRange
    vec4(0.1, 1.0, 0.8, 1.0)                    // color
);

const Tower towerShockerTemplate = Tower(
    CIRCLE,                                     // CollisionType
    {vec2(0.0), 16.0},                          // PhysicalBasis
    CIRCLE,                                     // fieldCollisionType
    {vec2(0.0, 0.0), 100.0},                    // fieldPhysicalBasis
    TOWER_SHOCKWAVE,                            // TowerType
    100.0,                                      // range
    1.0,                                        // shootInterval
    0.0,                                        // bulletSpread (degrees)
    1,                                          // bulletCount
    200,                                        // damage
    1.0,                                        // bulletSpeed
    0.0,                                        // bulletSpeedVariability
    0,                                          // bulletExplosionDamage
    0.0,                                        // bulletExplosionRange
    vec4(1.0, 0.3, 0.1, 1.0)                    // color
);

const Tower towerFlakTemplate = Tower(
    CIRCLE,                                     // CollisionType
    {vec2(0.0), 32.0},                          // PhysicalBasis
    CIRCLE,                                     // fieldCollisionType
    {vec2(0.0), 500.0},                         // fieldPhysicalBasis
    TOWER_FLAK,                                 // TowerType
    500.0,                                      // range
    1.5,                                        // shootInterval
    6.0,                                        // bulletSpread (degrees)
    8,                                          // bulletCount
    50,                                         // damage
    400.0,                                      // bulletSpeed
    100.0,                                      // bulletSpeedVariability
    100,                                        // bulletExplosionDamage
    50.0,                                       // bulletExplosionRange
    vec4(1.0, 0.0, 0.8, 1.0)                    // color
);

void Manager::EventAssetInit() {
}

void Manager::EventAssetAcquire() {
}

void Manager::EventInitialize() {
    towers.granularity = 5;
    enemies.granularity = 25;
    bullets.granularity = 50;
    winds.granularity = 50;
    explosions.granularity = 10;
}

void Manager::EventSync() {
    timestep = globals->objects.timestep * globals->objects.simulationRate;
    towers.Synchronize();
    enemies.Synchronize();
    bullets.Synchronize();
    winds.Synchronize();
    explosions.Synchronize();

    updateChunks.size = 0;

    towers.GetUpdateChunks(updateChunks);
    enemies.GetUpdateChunks(updateChunks);
    bullets.GetUpdateChunks(updateChunks);
    winds.GetUpdateChunks(updateChunks);
    explosions.GetUpdateChunks(updateChunks);
    readyForDraw = true;
}

void Manager::EventUpdate() {
    if (timestep == 0.0) return;

    const i32 concurrency = 4;
    Array<Thread> threads(concurrency);
    for (i32 i = 0; i < updateChunks.size; i++) {
        for (i32 j = 0; j < concurrency; j++) {
            UpdateChunk &chunk = updateChunks[i];
            threads[j] = Thread(chunk.updateCallback, chunk.theThisPointer, j, concurrency);
        }
        for (i32 j = 0; j < concurrency; j++) {
            if (threads[j].joinable()) {
                threads[j].join();
            }
        }
    }

    if (hitpointsLeft > 0) {
        enemyTimer -= timestep;
        while (enemyTimer <= 0.0) {
            Enemy enemy;
            enemies.Create(enemy);
            enemyTimer += enemyInterval;
        }
    }
    if (globals->objects.Pressed(KC_KEY_R)) {
        for (i32 i = 0; i < towers.size; i++) {
            if (towers[i].id.generation >= 0) {
                towers.Destroy(towers[i].id);
            }
        }
    }
    if (globals->objects.Pressed(KC_KEY_T)) {
        placeMode = !placeMode;
    }
    if (globals->objects.Pressed(KC_MOUSE_LEFT)) {
        if (globals->gui.playMenu.list->MouseOver()) {
            placeMode = false;
        }
    }
    for (i32 i = 0; i <= TOWER_MAX_RANGE; i++) {
        if (globals->gui.playMenu.towerButtons[i]->state.Released()) {
            placeMode = true;
            towerType = TowerType(i);
        }
    }
    if (globals->objects.Pressed(KC_KEY_SPACE) || globals->gui.playMenu.buttonStartWave->state.Released()) {
        wave++;
        enemyInterval = max(10.0 / (f32)(wave+19), 0.0001);
        hitpointsLeft += (i64)(pow((f64)wave, (f64)1.5) * 1000.0d);
    }
    if (globals->gui.mouseoverDepth > 0) return; // Don't accept mouse input
    if (!placeMode) {
        if (globals->objects.Pressed(KC_MOUSE_LEFT)) {
            if (selectedTower != -1) {
                selectedTower = -1;
            }
            for (i32 i = 0; i < towers.size; i++) {
                if (towers[i].id.generation < 0) continue;
                if (towers[i].physical.MouseOver()) {
                    selectedTower = towers[i].id;
                    break;
                }
            }
        }
    } else {
        if (globals->objects.Pressed(KC_MOUSE_SCROLLUP)) {
            towerType = TowerType(i32(towerType+1) % i32(TOWER_MAX_RANGE+1));
        } else if (globals->objects.Pressed(KC_MOUSE_SCROLLDOWN)) {
            i32 tt = i32(towerType) - 1;
            if (tt >= 0) {
                towerType = TowerType(tt);
            } else {
                towerType = TOWER_MAX_RANGE;
            }
        }
        if (globals->objects.Down(KC_KEY_LEFT)) {
            placingAngle += timestep * pi;
        } else if (globals->objects.Down(KC_KEY_RIGHT)) {
            placingAngle += -timestep * pi;
        }
        Tower tower(towerType);
        tower.physical.pos = globals->input.cursor;
        tower.physical.angle = placingAngle;
        canPlace = true;
        for (i32 i = 0; i < towers.size; i++) {
            const Tower &other = towers[i];
            if (other.id.generation < 0) continue;
            if (other.physical.Collides(tower.physical)) {
                canPlace = false;
                break;
            }
        }
        if (globals->objects.Pressed(KC_MOUSE_LEFT)) {
            if (canPlace) {
                towers.Create(tower);
                // placeMode = false;
            }
        }
    }
}

void Manager::EventDraw(Array<Rendering::DrawingContext> &contexts) {
    // if (globals->gui.currentMenu != Int::MENU_PLAY) return;

    const i32 concurrency = contexts.size;
    Array<Thread> threads(concurrency);
    for (i32 i = 0; i < updateChunks.size; i++) {
        for (i32 j = 0; j < concurrency; j++) {
            UpdateChunk &chunk = updateChunks[i];
            threads[j] = Thread(chunk.drawCallback, chunk.theThisPointer, &contexts[j], j, concurrency);
        }
        for (i32 j = 0; j < concurrency; j++) {
            if (threads[j].joinable()) {
                threads[j].join();
            }
        }
    }

    if (placeMode) {
        Tower tower(towerType);
        tower.physical.pos = globals->input.cursor;
        tower.physical.angle = placingAngle;
        tower.physical.Draw(contexts.Back(), canPlace ? vec4(0.1, 1.0, 0.1, 0.9) : vec4(1.0, 0.1, 0.1, 0.9));
        tower.field.pos = tower.physical.pos;
        tower.field.angle = tower.physical.angle;
        tower.field.Draw(contexts.Back(), canPlace ? vec4(1.0, 1.0, 1.0, 0.1) : vec4(1.0, 0.5, 0.5, 0.2));
    }
    if (selectedTower != -1) {
        const Tower& selected = towers[selectedTower];
        selected.field.Draw(contexts.Back(), vec4(1.0, 1.0, 1.0, 0.1));
    }
}

bool AABB::Collides(const AABB &other) const {
    return minPos.x <= other.maxPos.x
        && maxPos.x >= other.minPos.x
        && minPos.y <= other.maxPos.y
        && maxPos.y >= other.minPos.y;
}

void AABB::Update(const Physical &physical) {
    switch (physical.type) {
    case SEGMENT: {
        minPos.x = min(physical.actual.segment.a.x, physical.actual.segment.b.x);
        minPos.y = min(physical.actual.segment.a.y, physical.actual.segment.b.y);
        maxPos.x = max(physical.actual.segment.a.x, physical.actual.segment.b.x);
        maxPos.y = max(physical.actual.segment.a.y, physical.actual.segment.b.y);
        break;
    }
    case CIRCLE: {
        minPos.x = physical.actual.circle.c.x - physical.actual.circle.r;
        minPos.y = physical.actual.circle.c.y - physical.actual.circle.r;
        maxPos.x = physical.actual.circle.c.x + physical.actual.circle.r;
        maxPos.y = physical.actual.circle.c.y + physical.actual.circle.r;
        break;
    }
    case BOX: {
        minPos.x = min(physical.actual.box.a.x,
                       min(physical.actual.box.b.x,
                           min(physical.actual.box.c.x,
                               physical.actual.box.d.x)));
        minPos.y = min(physical.actual.box.a.y,
                       min(physical.actual.box.b.y,
                           min(physical.actual.box.c.y,
                               physical.actual.box.d.y)));
        maxPos.x = max(physical.actual.box.a.x,
                       max(physical.actual.box.b.x,
                           max(physical.actual.box.c.x,
                               physical.actual.box.d.x)));
        maxPos.y = max(physical.actual.box.a.y,
                       max(physical.actual.box.b.y,
                           max(physical.actual.box.c.y,
                               physical.actual.box.d.y)));
        break;
    }
    }
}

bool CollisionSegmentSegment(const Physical &a, const Physical &b) {
    const vec2 A = a.actual.segment.a;
    const vec2 B = a.actual.segment.b;
    const vec2 C = b.actual.segment.a;
    const vec2 D = b.actual.segment.b;

    f32 denom, num1, num2;
    denom = ((B.x - A.x) * (D.y - C.y)) - ((B.y - A.y) * (D.x - C.x));
    num1  = ((A.y - C.y) * (D.x - C.x)) - ((A.x - C.x) * (D.y - C.y));
    num2  = ((A.y - C.y) * (B.x - A.x)) - ((A.x - C.x) * (B.y - A.y));

    if (denom == 0) return num1 == 0 && num2 == 0;

    f32 r = num1/denom;
    f32 s = num2/denom;

    return (r >= 0 && r <= 1 && s >= 0 && s <= 1);
}

inline bool CollisionSegmentCircle(const Physical &a, const Physical &b) {
    return distSqrToLine<true>(a.actual.segment.a, a.actual.segment.b, b.actual.circle.c) <= square(b.actual.circle.r);
}

bool SegmentInAABB(const vec2 &A, const vec2 &B, AABB aabb) {
    f32 t = (aabb.minPos.y - A.y) / (B.y - A.y);
    if (t == median(t, 0.0, 1.0)) {
        f32 x = A.x + (B.x - A.x) * t;
        if (x == median(x, aabb.minPos.x, aabb.maxPos.x)) {
            // Segment intersects
            return true;
        }
    }
    t = (aabb.maxPos.y - A.y) / (B.y - A.y);
    if (t == median(t, 0.0, 1.0)) {
        f32 x = A.x + (B.x - A.x) * t;
        if (x == median(x, aabb.minPos.x, aabb.maxPos.x)) {
            // Segment intersects
            return true;
        }
    }
    t = (aabb.minPos.x - A.x) / (B.x - A.x);
    if (t == median(t, 0.0, 1.0)) {
        f32 y = A.y + (B.y - A.y) * t;
        if (y == median(y, aabb.minPos.y, aabb.maxPos.y)) {
            // Segment intersects
            return true;
        }
    }
    t = (aabb.maxPos.x - A.x) / (B.x - A.x);
    if (t == median(t, 0.0, 1.0)) {
        f32 y = A.y + (B.y - A.y) * t;
        if (y == median(y, aabb.minPos.y, aabb.maxPos.y)) {
            // Segment intersects
            return true;
        }
    }
    return false;
}

bool CollisionSegmentBox(const Physical &a, const Physical &b) {
    const mat2 aRotation = mat2::Rotation(-b.angle.value());
    // const vec2 dPos = a.pos - b.pos;
    const vec2 A = (a.actual.segment.a - b.pos) * aRotation;
    if (A.x == median(A.x, b.basis.box.a.x, b.basis.box.b.x) && A.y == median(A.y, b.basis.box.a.y, b.basis.box.b.y)) {
        // Point is inside box
        return true;
    }
    const vec2 B = (a.actual.segment.b - b.pos) * aRotation;
    if (B.x == median(B.x, b.basis.box.a.x, b.basis.box.b.x) && B.y == median(B.y, b.basis.box.a.y, b.basis.box.b.y)) {
        // Point is inside box
        return true;
    }
    return SegmentInAABB(A, B, {b.basis.box.a, b.basis.box.b});
}

bool CollisionCircleCircle(const Physical &a, const Physical &b) {
    return absSqr(a.actual.circle.c - b.actual.circle.c) <= square(a.actual.circle.r + b.actual.circle.r);
}

bool CollisionCircleBox(const Physical &a, const Physical &b) {
    const f32 rSquared = square(a.actual.circle.r);
    if (absSqr(a.actual.circle.c - b.actual.box.a) <= rSquared) return true;
    if (absSqr(a.actual.circle.c - b.actual.box.b) <= rSquared) return true;
    if (absSqr(a.actual.circle.c - b.actual.box.c) <= rSquared) return true;
    if (absSqr(a.actual.circle.c - b.actual.box.d) <= rSquared) return true;

    const mat2 rotation = mat2::Rotation(-b.angle.value());
    const vec2 C = (a.actual.circle.c - b.pos) * rotation;
    if (C.x == median(C.x, b.basis.box.a.x, b.basis.box.b.x)
     && C.y + a.actual.circle.r >= b.basis.box.a.y
     && C.y - a.actual.circle.r <= b.basis.box.b.y) {
        return true;
    }
    if (C.y == median(C.y, b.basis.box.a.y, b.basis.box.b.y)
     && C.x + a.actual.circle.r >= b.basis.box.a.x
     && C.x - a.actual.circle.r <= b.basis.box.b.x) {
        return true;
    }
    return false;
}

bool CollisionBoxBox(const Physical &a, const Physical &b) {
    const mat2 rotation = mat2::Rotation(-b.angle.value());
    const vec2 A = (a.actual.box.a - b.pos) * rotation;
    if (A.x == median(A.x, b.basis.box.a.x, b.basis.box.b.x)
     && A.y == median(A.y, b.basis.box.a.y, b.basis.box.b.y)) {
        return true;
    }
    const vec2 B = (a.actual.box.b - b.pos) * rotation;
    if (B.x == median(B.x, b.basis.box.a.x, b.basis.box.b.x)
     && B.y == median(B.y, b.basis.box.a.y, b.basis.box.b.y)) {
        return true;
    }
    const vec2 C = (a.actual.box.c - b.pos) * rotation;
    if (C.x == median(C.x, b.basis.box.a.x, b.basis.box.b.x)
     && C.y == median(C.y, b.basis.box.a.y, b.basis.box.b.y)) {
        return true;
    }
    const vec2 D = (a.actual.box.d - b.pos) * rotation;
    if (D.x == median(D.x, b.basis.box.a.x, b.basis.box.b.x)
     && D.y == median(D.y, b.basis.box.a.y, b.basis.box.b.y)) {
        return true;
    }

    const AABB aabb = {b.basis.box.a, b.basis.box.b};
    if (SegmentInAABB(A, C, aabb)) return true;
    if (SegmentInAABB(C, B, aabb)) return true;
    if (SegmentInAABB(B, D, aabb)) return true;
    if (SegmentInAABB(D, A, aabb)) return true;
    return false;
}

bool Physical::Collides(const Physical &other) const {
    if (!updated) {
        UpdateActual();
    }
    if (!other.updated) {
        other.UpdateActual();
    }
    // return aabb.Collides(other.aabb);
    if (!aabb.Collides(other.aabb)) {
        return false;
    }
    switch (type) {
    case SEGMENT: {
        switch (other.type) {
        case SEGMENT: {
            return CollisionSegmentSegment(*this, other);
        }
        case CIRCLE: {
            return CollisionSegmentCircle(*this, other);
        }
        case BOX: {
            return CollisionSegmentBox(*this, other);
        }
        }
        break;
    }
    case CIRCLE: {
        switch (other.type) {
        case SEGMENT: {
            return CollisionSegmentCircle(other, *this);
        }
        case CIRCLE: {
            return CollisionCircleCircle(*this, other);
        }
        case BOX: {
            return CollisionCircleBox(*this, other);
        }
        }
        break;
    }
    case BOX: {
        switch (other.type) {
        case SEGMENT: {
            return CollisionSegmentBox(other, *this);
        }
        case CIRCLE: {
            return CollisionCircleBox(other, *this);
        }
        case BOX: {
            return CollisionBoxBox(*this, other);
        }
        }
        break;
    }
    }
    return false;
}

bool Physical::MouseOver() const {
    const vec2 mouse = vec2(globals->input.cursor);
    if (!updated) {
        UpdateActual();
    }
    switch (type) {
    case SEGMENT: {
        return distSqrToLine<true>(actual.segment.a, actual.segment.b, mouse) < 16.0;
    }
    case CIRCLE: {
        return absSqr(actual.circle.c-mouse) <= square(actual.circle.r);
    }
    case BOX: {
        const mat2 rotation = mat2::Rotation(-angle.value());
        const vec2 A = (mouse - pos) * rotation;
        if (A.x == median(A.x, basis.box.a.x, basis.box.b.x)
         && A.y == median(A.y, basis.box.a.y, basis.box.b.y)) {
            return true;
        }
        break;
    }
    }
    return false;
}

void PhysicalAbsFromBasis(PhysicalAbs &actual, const PhysicalBasis &basis, const CollisionType &type, const vec2 &pos, const Angle32 &angle) {
    mat2 rotation(1.0);
    if (angle != 0.0) {
        rotation = mat2::Rotation(angle.value());
    }
    switch (type) {
    case SEGMENT: {
        if (angle != 0.0) {
            actual.segment.a = basis.segment.a * rotation + pos;
            actual.segment.b = basis.segment.b * rotation + pos;
        } else {
            actual.segment.a = basis.segment.a + pos;
            actual.segment.b = basis.segment.b + pos;
        }
        break;
    }
    case CIRCLE: {
        if (angle != 0.0) {
            actual.circle.c = basis.circle.c * rotation + pos;
        } else {
            actual.circle.c = basis.circle.c + pos;
        }
        actual.circle.r = basis.circle.r;
        break;
    }
    case BOX: {
        if (angle != 0.0) {
            actual.box.a = basis.box.a * rotation + pos;
            actual.box.b = basis.box.b * rotation + pos;
            actual.box.c = vec2(basis.box.b.x, basis.box.a.y) * rotation + pos;
            actual.box.d = vec2(basis.box.a.x, basis.box.b.y) * rotation + pos;
        } else {
            actual.box.a = basis.box.a + pos;
            actual.box.b = basis.box.b + pos;
            actual.box.c = vec2(basis.box.b.x, basis.box.a.y) + pos;
            actual.box.d = vec2(basis.box.a.x, basis.box.b.y) + pos;
        }
        break;
    }
    }
}

void Physical::Update(f32 timestep) {
    angle += rot * timestep;
    pos += vel * timestep;
    updated = false;
}

void Physical::UpdateActual() const {
    PhysicalAbsFromBasis(actual, basis, type, pos, angle);
    aabb.Update(*this);
    updated = true;
}

void Physical::Draw(Rendering::DrawingContext &context, vec4 color) const {
    if (type == BOX) {
        const vec2 scale = basis.box.b - basis.box.a;
        globals->rendering.DrawQuad(context, Rendering::texBlank, color, pos, scale, vec2(1.0), -basis.box.a / scale, angle);
    } else if (type == SEGMENT) {
        vec2 scale = basis.segment.b - basis.segment.a;
        scale.y = max(scale.y, 2.0);
        globals->rendering.DrawQuad(context, Rendering::texBlank, color, pos, scale, vec2(1.0), -basis.segment.a / scale, angle);
    } else {
        const vec2 scale = basis.circle.r * 2.0 + 2.0;
        globals->rendering.DrawCircle(context, Rendering::texBlank, color, pos, scale, vec2(1.0), -basis.circle.c / scale + vec2(0.5), angle);
    }
}

template<typename T>
void DoubleBufferArray<T>::Update(void *theThisPointer, i32 threadIndex, i32 concurrency) {
    DoubleBufferArray<T> *theActualThisPointer = (DoubleBufferArray<T>*)theThisPointer;
    i32 g = theActualThisPointer->granularity;
    for (i32 i = threadIndex*g; i < theActualThisPointer->array[theActualThisPointer->buffer].size; i += g*concurrency) {
        for (i32 j = 0; j < g; j++) {
            if (i+j >= theActualThisPointer->array[theActualThisPointer->buffer].size) break;
            T &obj = theActualThisPointer->array[theActualThisPointer->buffer][i+j];
            if (obj.id.generation >= 0)
                obj.Update(globals->entities.timestep);
        }
    }
}

template<typename T>
void DoubleBufferArray<T>::Draw(void *theThisPointer, Rendering::DrawingContext *context, i32 threadIndex, i32 concurrency) {
    DoubleBufferArray<T> *theActualThisPointer = (DoubleBufferArray<T>*)theThisPointer;
    i32 g = theActualThisPointer->granularity;
    for (i32 i = threadIndex*g; i < theActualThisPointer->array[!theActualThisPointer->buffer].size; i += g*concurrency) {
        for (i32 j = 0; j < g; j++) {
            if (i+j >= theActualThisPointer->array[!theActualThisPointer->buffer].size) break;
            T &obj = theActualThisPointer->array[!theActualThisPointer->buffer][i+j];
            if (obj.id.generation >= 0)
                obj.Draw(*context);
        }
    }
}

template<typename T>
void DoubleBufferArray<T>::Synchronize() {
    buffer = globals->objects.buffer;

    for (u16 &index : destroyed) {
        // It should already be negative, so decrement is an increment of the generation.
        array[!buffer][index].id.generation = -(array[!buffer][index].id.generation+1);
        empty.Append(index);
    }
    count -= destroyed.size;
    destroyed.Clear();
    for (T &obj : created) {
        if (empty.size > 0) {
            obj.id.index = empty.Back();
            obj.id.generation = -array[!buffer][obj.id.index].id.generation;
            array[!buffer][obj.id.index] = std::move(obj);
            empty.Erase(empty.size-1);
        } else {
            obj.id.index = array[!buffer].size;
            obj.id.generation = 0;
            array[!buffer].Append(std::move(obj));
        }
    }
    count += created.size;
    created.Clear();
    array[buffer] = array[!buffer];
    size = array[0].size;
}

template<typename T>
void DoubleBufferArray<T>::GetUpdateChunks(Array<UpdateChunk> &dstUpdateChunks) {
    if (count == 0) return;
    UpdateChunk chunk;
    chunk.updateCallback = DoubleBufferArray<T>::Update;
    chunk.drawCallback = DoubleBufferArray<T>::Draw;
    chunk.theThisPointer = this;
    dstUpdateChunks.Append(chunk);
}

template<typename T>
void DoubleBufferArray<T>::Create(T &obj) {
    mutex.lock();
    obj.EventCreate();
    created.Append(obj);
    mutex.unlock();
}

template<typename T>
void DoubleBufferArray<T>::Destroy(Id id) {
    mutex.lock();
    array[!buffer][id.index].EventDestroy();
    for (i32 i = 0; i < destroyed.size; i++) {
        if (destroyed[i] == id.index) {
            mutex.unlock();
            return;
        }
    }
    destroyed.Append(id.index);
    array[!buffer][id.index].id.generation *= -1;
    mutex.unlock();
}

Tower::Tower(TowerType _type) {
    type = _type;
    switch(type) {
        case TOWER_GUN:
            *this = towerGunTemplate;
            break;
        case TOWER_SHOTGUN:
            *this = towerShotgunTemplate;
            break;
        case TOWER_FAN:
            *this = towerFanTemplate;
            break;
        case TOWER_GAUSS:
            *this = towerGaussTemplate;
            break;
        case TOWER_SHOCKWAVE:
            *this = towerShockerTemplate;
            break;
        case TOWER_FLAK:
            *this = towerFlakTemplate;
            break;
        default:
            physical.type = CIRCLE;
            physical.basis.circle.c = vec2(0.0);
            physical.basis.circle.r = 16.0;
            color = vec4(0.0, 0.0, 0.0, 1.0);
            shootInterval = 0.8;
            bulletSpread = 15.0;
            bulletCount = 10;
            bulletSpeed = 800.0;
            break;
    }
}

void Tower::EventCreate() {
    selected = false;
    shootTimer = 0.0;
    field.pos = physical.pos;
    field.angle = physical.angle;
}

void Tower::Update(f32 timestep) {
    physical.Update(timestep);
    selected = globals->entities.selectedTower == id;
    if (shootTimer <= 0.0) disabled = false;
    for (i32 i = 0; i < globals->entities.enemies.size; i++) {
        const Enemy& other = globals->entities.enemies[i];
        if (other.id.generation < 0 || other.hitpoints <= 2000) continue;
        if (physical.Collides(other.physical)) {
            disabled = true;
            shootTimer = 0.5;
            break;
        }
    }
    shootTimer -= timestep;
    if (disabled) return;
    if (shootTimer <= 0.0) {
        if (type != TOWER_SHOCKWAVE && type != TOWER_FAN) {
            f32 maxDist = sqrt(range*range);
            f32 nearestDist = maxDist;
            Id nearestEnemy = -1;
            for (i32 i = 0; i < globals->entities.enemies.size; i++) {
                const Enemy& other = globals->entities.enemies[i];
                if (other.id.generation < 0 || other.hitpoints == 0) continue;
                f32 dist = abs(other.physical.pos - physical.pos) - other.physical.basis.circle.r;
                if (dist < nearestDist) {
                    nearestDist = dist;
                    nearestEnemy = other.id;
                }
            }
            if (nearestEnemy != -1) {
                const Enemy& other = globals->entities.enemies[nearestEnemy];
                Bullet bullet;
                bullet.physical.pos = physical.pos;
                bullet.lifetime = range / (bulletSpeed * 0.9);
                bullet.explosionDamage = bulletExplosionDamage;
                bullet.explosionRange = bulletExplosionRange;
                f32 dist = nearestDist;
                vec2 deltaP;
                for (i32 i = 0; i < 2; i++) {
                    deltaP = other.physical.pos - physical.pos + other.physical.vel * dist / bulletSpeed;
                    dist = abs(deltaP);
                }
                deltaP = other.physical.pos - physical.pos + other.physical.vel * dist / bulletSpeed;
                Angle32 idealAngle = atan2(-deltaP.y, deltaP.x);
                for (i32 i = 0; i < bulletCount; i++) {
                    Angle32 angle = idealAngle + Degrees32(random(-bulletSpread.value(), bulletSpread.value(), globals->rng));
                    bullet.physical.vel.x = cos(angle);
                    bullet.physical.vel.y = -sin(angle);
                    bullet.physical.vel *= bulletSpeed + random(-bulletSpeedVariability, bulletSpeedVariability, globals->rng);
                    bullet.damage = damage;
                    globals->entities.bullets.Create(bullet);
                }
                shootTimer = shootInterval;
            }
        } else if (type == TOWER_SHOCKWAVE) {
            bool shoot = false;
            for (i32 i = 0; i < globals->entities.enemies.size; i++) {
                const Enemy& other = globals->entities.enemies[i];
                if (other.id.generation < 0 || other.hitpoints == 0) continue;
                if (field.Collides(other.physical)) {
                    shoot = true;
                    break;
                }
            }
            if (shoot) {
                Explosion explosion;
                explosion.size = range;
                explosion.growth = 5.0;
                explosion.damage = damage;
                explosion.physical.pos = physical.pos;
                globals->entities.explosions.Create(explosion);
                shootTimer = shootInterval;
            }
        } else if (type == TOWER_FAN) {
            Wind wind;
            wind.physical.pos = physical.pos;
            wind.lifetime = range / bulletSpeed;
            f32 randomPos = random(-20.0, 20.0, globals->rng);
            wind.physical.pos.x += cos(physical.angle.value() + pi * 0.5) * randomPos;
            wind.physical.pos.y -= sin(physical.angle.value() + pi * 0.5) * randomPos;
            for (i32 i = 0; i < bulletCount; i++) {
                Angle32 angle = physical.angle + Degrees32(random(-bulletSpread.value(), bulletSpread.value(), globals->rng));
                wind.physical.vel.x = cos(angle);
                wind.physical.vel.y = -sin(angle);
                wind.physical.vel *= bulletSpeed + random(-bulletSpeedVariability, bulletSpeedVariability, globals->rng);
                wind.physical.pos += wind.physical.vel * 0.03;
                globals->entities.winds.Create(wind);
            }
        }
    }
}

void Tower::Draw(Rendering::DrawingContext &context) {
    vec4 colorTemp;
    if (selected) {
        colorTemp = vec4(0.5) + color * 0.5;
    } else {
        colorTemp = color;
    }
    if (disabled) {
        colorTemp.rgb = (colorTemp.rgb + vec3(0.8 * 3.0)) / 4.0;
    }
    physical.Draw(context, colorTemp);
}

template struct DoubleBufferArray<Tower>;

const f32 honkerSpawnInterval = 5.0;

void Enemy::EventCreate() {
    physical.type = CIRCLE;
    physical.basis.circle.c = vec2(0.0);
    physical.basis.circle.r = 0.0;
    if (!child) {
        physical.pos = vec2(0.0, globals->rendering.screenSize.y * (0.5 + random(-0.2, 0.2, globals->rng)));
        physical.vel = vec2(200.0, random(-50.0, 50.0, globals->rng));
        f32 honker = random(0.0, 100000.0 / pow((f32)globals->entities.hitpointsLeft, 0.75), globals->rng);
        if (honker < 0.001) {
            hitpoints = 250000;
        } else if (honker < 0.1) {
            hitpoints = random(20000, 50000, globals->rng);
        } else if (honker < 1.0) {
            hitpoints = random(5000, 10000, globals->rng);
        } else if (honker < 5.0) {
            hitpoints = random(1000, 2500, globals->rng);
        } else if (honker <= 25.0) {
            hitpoints = random(200, 500, globals->rng);
        } else {
            hitpoints = random(25, 100, globals->rng);
        }
    }
    spawnTimer = honkerSpawnInterval;
    if (!child) {
        i64 limit = median(globals->entities.hitpointsLeft / 2, (i64)500, globals->entities.hitpointsLeft);
        if (hitpoints > limit) {
            hitpoints = limit;
        }
        globals->entities.hitpointsLeft -= hitpoints;
        size = hitpoints;
        color = vec4(hsvToRgb(vec3(sqrt(size)/(tau*16.0) + (f32)globals->entities.wave / 9.0, min(size / 100.0, 1.0), 1.0)), 0.7);
    }
    physical.vel *= 10.0;
    physical.vel /= sqrt((f32)hitpoints);
    targetSpeed = 1000.0 / pow((f32)hitpoints, 1.0 / 3.0);
    size = 0.0;
}

void Enemy::Update(f32 timestep) {
    size = decay(size, (f32)hitpoints, 0.2, timestep);
    physical.basis.circle.r = sqrt(size);
    physical.Update(timestep);
    physical.UpdateActual();
    {
        f32 distToTopBarrier = physical.pos.y - physical.basis.circle.r;
        f32 distToBotBarrier = globals->rendering.screenSize.y - physical.pos.y - physical.basis.circle.r;
        if (distToTopBarrier < 128.0) {
            physical.ImpulseY(1000000.0 / max(distToTopBarrier, 5.0) / size, timestep);
        }
        if (distToBotBarrier < 128.0) {
            physical.ImpulseY(-1000000.0 / max(distToBotBarrier, 5.0) / size, timestep);
        }
    }
    if (physical.aabb.minPos.x > globals->rendering.screenSize.x || physical.aabb.minPos.y > globals->rendering.screenSize.y || (hitpoints <= 0 && size < 0.01)) {
        if (hitpoints > 5) {
            globals->entities.lives -= hitpoints;
        }
        globals->entities.enemies.Destroy(id);
    }
    if (hitpoints == 0) return;
    if (hitpoints > 5000) {
        if (spawnTimer <= 0.0) {
            Enemy newEnemy;
            newEnemy.child = true;
            Angle32 spawnAngle = random(0.0, tau, globals->rng);
            vec2 spawnVector = vec2(cos(spawnAngle), -sin(spawnAngle)) * sqrt(random(0.0, 1.0, globals->rng));
            newEnemy.physical.pos = physical.pos + spawnVector * physical.basis.circle.r;
            newEnemy.physical.vel = physical.vel + spawnVector * 100.0;
            newEnemy.color = color;
            newEnemy.hitpoints = hitpoints/20;
            hitpoints -= newEnemy.hitpoints;
            globals->entities.enemies.Create(newEnemy);
            spawnTimer += honkerSpawnInterval;
        } else {
            spawnTimer -= timestep;
        }
    }
    for (i32 i = 0; i < globals->entities.towers.size; i++) {
        const Tower &other = globals->entities.towers[i];
        if (other.id.generation < 0 || other.type != TOWER_FAN || other.disabled) continue;
        if (physical.Collides(other.field)) {
            vec2 deltaP = physical.pos - other.physical.pos;
            physical.Impulse(normalize(deltaP) * max((other.range + physical.basis.circle.r - abs(deltaP)), 0.0) * 5000.0 / pow(size, 1.5), timestep);
            if (other.damage != 0) {
                if (random(0.0, 1.0, globals->rng) <= (f32)other.damage*timestep) {
                    hitpoints--;
                }
            }
        }
    }
    for (i32 i = 0; i < globals->entities.explosions.size; i++) {
        const Explosion &other = globals->entities.explosions[i];
        if (other.id.generation < 0) continue;
        if (physical.Collides(other.physical)) {
            vec2 deltaP = physical.pos - other.physical.pos;
            physical.Impulse(normalize(deltaP) * max((other.size + physical.basis.circle.r - abs(deltaP)), 0.0) * 500.0 / pow(size, 1.5), timestep);
            if (other.damage != 0) {
                f32 prob = (f32)other.damage*timestep;
                i32 hits = prob;
                prob -= (f32)hits;
                hitpoints -= hits;
                if (random(0.0, 1.0, globals->rng) <= prob) {
                    hitpoints--;
                }
            }
        }
    }
    for (i32 i = 0; i < globals->entities.bullets.size; i++) {
        Bullet &other = globals->entities.bullets.GetMutable(i);
        if (other.id.generation < 0) continue;
        if (physical.Collides(other.physical)) {
            if (other.damage > hitpoints) {
                other.damage -= hitpoints;
                hitpoints = 0;
            } else {
                globals->entities.bullets.Destroy(other.id);
                hitpoints -= other.damage;
                physical.vel += normalize(other.physical.vel) * 100.0 / size;
            }
        }
    }
    physical.vel.x = max(decay(physical.vel.x, targetSpeed + abs(physical.vel.y), 1.0, timestep), targetSpeed * 0.1);
    physical.vel = normalize(physical.vel) * targetSpeed;
}

void Enemy::Draw(Rendering::DrawingContext &context) {
    physical.Draw(context, color);
}

template struct DoubleBufferArray<Enemy>;

void Bullet::EventCreate() {
    f32 length = abs(physical.vel) * 0.5 / 30.0;
    physical.type = SEGMENT;
    physical.basis.segment.a = vec2(-length, -1.0);
    physical.basis.segment.b = vec2(length, 1.0);
    physical.angle = atan2(-physical.vel.y, physical.vel.x);
}

void Bullet::EventDestroy() {
    if (explosionRange != 0.0) {
        Explosion explosion;
        explosion.damage = explosionDamage;
        explosion.size = explosionRange;
        explosion.growth = 8.0;
        explosion.physical.pos = physical.pos;
        explosion.physical.vel = physical.vel;
        globals->entities.explosions.Create(explosion);
    }
}

void Bullet::Update(f32 timestep) {
    physical.Update(timestep);
    physical.UpdateActual();
    lifetime -= timestep;
    if (physical.aabb.minPos.x > globals->rendering.screenSize.x || physical.aabb.minPos.y > globals->rendering.screenSize.y || lifetime <= 0.0) {
        globals->entities.bullets.Destroy(id);
    }
}

void Bullet::Draw(Rendering::DrawingContext &context) {
    vec4 color = vec4(1.0, 1.0, 0.5, clamp(0.0, 1.0, lifetime * 8.0));
    if (explosionDamage != 0) {
        color.rgb = vec3(1.0, 0.25, 0.0);
    }
    physical.Draw(context, color);
}

template struct DoubleBufferArray<Bullet>;

void Wind::EventCreate() {
    physical.type = CIRCLE;
    physical.basis.circle.c = vec2(random(-8.0, 8.0, globals->rng), random(-8.0, 8.0, globals->rng));
    physical.basis.circle.r = random(16.0, 32.0, globals->rng);
    physical.angle = random(0.0, tau, globals->rng);
    physical.rot = random(-tau, tau, globals->rng);
}

void Wind::Update(f32 timestep) {
    physical.Update(timestep);
    physical.UpdateActual();
    lifetime -= timestep;
    if (physical.aabb.minPos.x > globals->rendering.screenSize.x || physical.aabb.minPos.y > globals->rendering.screenSize.y || lifetime <= 0.0) {
        globals->entities.winds.Destroy(id);
    }
}

void Wind::Draw(Rendering::DrawingContext &context) {
    vec4 color = vec4(1.0, 1.0, 1.0, clamp(0.0, 0.1, lifetime * 0.1));
    const vec2 scale = physical.basis.circle.r * 2.0;
    globals->rendering.DrawCircle(context, Rendering::texBlank, color, physical.pos, scale * 0.1, vec2(10.0), -physical.basis.circle.c / scale + vec2(0.5), physical.angle);
}

template struct DoubleBufferArray<Wind>;

void Explosion::EventCreate() {
    physical.type = CIRCLE;
    physical.basis.circle.c = vec2(0.0);
    physical.basis.circle.r = 0.0;
}

void Explosion::Update(f32 timestep) {
    physical.basis.circle.r = decay(physical.basis.circle.r, size, 1.0 / growth, timestep);
    physical.Update(timestep);
    physical.UpdateActual();
    if (physical.basis.circle.r >= size * 0.9375) {
        globals->entities.explosions.Destroy(id);
    }
}

void Explosion::Draw(Rendering::DrawingContext &context) {
    f32 prog = physical.basis.circle.r / size / 0.9375;
    vec4 color = vec4(
        hsvToRgb(vec3(
            0.5 - prog * 0.5,
            prog,
            1.0
        )),
        clamp((1.0 - prog) * 5.0, 0.0, 0.8)
    );
    const vec2 scale = physical.basis.circle.r * 2.0;
    globals->rendering.DrawCircle(context, Rendering::texBlank, color, physical.pos, scale * 0.05, vec2(20.0), -physical.basis.circle.c / scale + vec2(0.5), physical.angle);
}

template struct DoubleBufferArray<Explosion>;

} // namespace Objects
