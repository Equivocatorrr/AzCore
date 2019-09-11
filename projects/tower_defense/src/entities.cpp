/*
    File: entities.cpp
    Author: Philip Haynes
*/

#include "entities.hpp"
#include "globals.hpp"

namespace Entities {


void Manager::EventAssetInit() {
}

void Manager::EventAssetAcquire() {
}

void Manager::EventInitialize() {

}

void Manager::EventUpdate() {
    if (globals->gui.currentMenu != Int::MENU_PLAY) return;
    towers.Synchronize();
    enemies.Synchronize();
    bullets.Synchronize();
    towers.Update(globals->objects.timestep);
    enemies.Update(globals->objects.timestep);
    bullets.Update(globals->objects.timestep);

    if (generateEnemies) {
        enemyTimer -= globals->objects.timestep;
        if (enemyTimer <= 0.0) {
            Enemy enemy;
            enemies.Create(enemy);
            enemyTimer += 0.1;
        }
    }

    if (globals->gui.mouseoverDepth > 0) return; // Don't accept mouse input
    if (globals->objects.Pressed(KC_KEY_T)) {
        placeMode = !placeMode;
    }
    if (globals->objects.Pressed(KC_KEY_E)) {
        generateEnemies = !generateEnemies;
    }
    if (!placeMode) {
        if (globals->objects.Pressed(KC_MOUSE_LEFT)) {
            if (selectedTower != -1) {
                selectedTower = -1;
            }
            for (i32 i = 0; i < towers.size; i++) {
                if (towers[i].physical.MouseOver()) {
                    selectedTower = towers[i].id;
                    break;
                }
            }
        }
    } else {
        Tower tower;
        tower.physical.type = BOX;
        tower.physical.basis.box = {vec2(-16.0), vec2(16.0)};
        tower.physical.pos = globals->input.cursor;
        canPlace = true;
        for (i32 i = 0; i < towers.size; i++) {
            const Tower &other = towers[i];
            if (other.physical.Collides(tower.physical)) {
                canPlace = false;
                break;
            }
        }
        if (globals->objects.Pressed(KC_MOUSE_LEFT)) {
            if (canPlace) {
                towers.Create(tower);
                placeMode = false;
            }
        }
    }
}

void Manager::EventDraw(Rendering::DrawingContext &context) {
    if (globals->gui.currentMenu != Int::MENU_PLAY) return;
    towers.Draw(context);
    enemies.Draw(context);
    bullets.Draw(context);
    if (placeMode) {
        globals->rendering.DrawQuad(context, Rendering::texBlank, canPlace ? vec4(0.1, 1.0, 0.1, 0.9) : vec4(1.0, 0.1, 0.1, 0.9), globals->input.cursor, vec2(32.0), vec2(1.0), vec2(0.5));
    }
    if (selectedTower != -1) {
        const Tower& selected = towers[selectedTower];
        globals->rendering.DrawCircle(context, Rendering::texBlank, vec4(0.8, 0.8, 1.0, 0.2), selected.physical.pos, vec2(selected.range*2.0), vec2(1.0), vec2(0.5));
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
    mat2 rotation;
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

void Physical::Draw(Rendering::DrawingContext &context, vec4 color) {
    if (type == BOX) {
        const vec2 scale = basis.box.b - basis.box.a;
        globals->rendering.DrawQuad(context, Rendering::texBlank, color, pos, scale, vec2(1.0), -basis.box.a / scale, angle);
    } else if (type == SEGMENT) {
        vec2 scale = basis.segment.b - basis.segment.a;
        scale.y = max(scale.y, 2.0);
        globals->rendering.DrawQuad(context, Rendering::texBlank, color, pos, scale, vec2(1.0), -basis.segment.a / scale, angle);
    } else {
        const vec2 scale = basis.circle.r * 2.0;
        globals->rendering.DrawCircle(context, Rendering::texBlank, color, pos, scale, vec2(1.0), -basis.circle.c / scale + vec2(0.5), angle);
    }
}

template<typename T>
void DoubleBufferArray<T>::Update(f32 timestep) {
    for (T &obj : array[buffer]) {
        if (obj.id.generation >= 0)
            obj.Update(timestep);
    }
}

template<typename T>
void DoubleBufferArray<T>::Draw(Rendering::DrawingContext &context) {
    for (T& obj : array[!buffer]) {
        if (obj.id.generation >= 0)
            obj.Draw(context);
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
void DoubleBufferArray<T>::Create(T &obj) {
    mutex.lock();
    obj.EventCreate();
    created.Append(obj);
    mutex.unlock();
}

template<typename T>
void DoubleBufferArray<T>::Destroy(Id id) {
    mutex.lock();
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

void Tower::EventCreate() {
    shootTimer = 0.0;
}

void Tower::Update(f32 timestep) {
    physical.Update(timestep);
    selected = globals->entities.selectedTower == id;
    if (shootTimer <= 0.0) {
        f32 maxDistSquared = range*range;
        f32 nearestDistSquared = maxDistSquared;
        Id nearestEnemy = -1;
        for (i32 i = 0; i < globals->entities.enemies.size; i++) {
            const Enemy& other = globals->entities.enemies[i];
            if (other.id.generation < 0) continue;
            f32 distSquared = absSqr(other.physical.pos - physical.pos);
            if (distSquared < nearestDistSquared) {
                nearestDistSquared = distSquared;
                nearestEnemy = other.id;
            }
        }
        if (nearestEnemy != -1) {
            const Enemy& other = globals->entities.enemies[nearestEnemy];
            vec2 deltaP = other.physical.pos - physical.pos;
            f32 dist = sqrt(nearestDistSquared);
            Bullet bullet;
            bullet.physical.pos = physical.pos;
            bullet.physical.vel = normalize(deltaP + other.physical.vel * dist / 300.0) * 300.0;
            globals->entities.bullets.Create(bullet);
            shootTimer = 0.3;
        }
    } else {
        shootTimer -= timestep;
    }
}

void Tower::Draw(Rendering::DrawingContext &context) {
    vec4 color;
    if (selected) {
        color = vec4(0.5, 0.5, 1.0, 1.0);
    } else {
        color = vec4(0.1, 0.1, 1.0, 1.0);
    }
    physical.Draw(context, color);
}

template struct DoubleBufferArray<Tower>;

void Enemy::EventCreate() {
    physical.type = CIRCLE;
    physical.basis.circle.c = vec2(0.0);
    physical.basis.circle.r = 16.0;
    physical.pos = vec2(-16.0, random(0.0, 512.0, globals->rng));
    physical.vel = vec2(200.0, 80.0);
}

void Enemy::Update(f32 timestep) {
    physical.Update(timestep);
    physical.UpdateActual();
    if (physical.aabb.minPos.x > globals->rendering.screenSize.x || physical.aabb.minPos.y > globals->rendering.screenSize.y) {
        globals->entities.enemies.Destroy(id);
    }
    Id hitBullet = -1;
    for (i32 i = 0; i < globals->entities.bullets.size; i++) {
        const Bullet &other = globals->entities.bullets[i];
        if (other.id.generation < 0) continue;
        if (physical.Collides(other.physical)) {
            hitBullet = other.id;
            break;
        }
    }
    if (hitBullet != -1) {
        globals->entities.bullets.Destroy(hitBullet);
        globals->entities.enemies.Destroy(id);
    }
}

void Enemy::Draw(Rendering::DrawingContext &context) {
    vec4 color = vec4(1.0, 0.5, 0.1, 1.0);
    physical.Draw(context, color);
}

template struct DoubleBufferArray<Enemy>;

void Bullet::EventCreate() {
    physical.type = SEGMENT;
    physical.basis.segment.a = vec2(-8.0, -1.0);
    physical.basis.segment.b = vec2(8.0, 1.0);
    physical.angle = atan2(-physical.vel.y, physical.vel.x);
}

void Bullet::Update(f32 timestep) {
    physical.Update(timestep);
    physical.UpdateActual();
    if (physical.aabb.minPos.x > globals->rendering.screenSize.x || physical.aabb.minPos.y > globals->rendering.screenSize.y) {
        globals->entities.bullets.Destroy(id);
    }
}

void Bullet::Draw(Rendering::DrawingContext &context) {
    vec4 color = vec4(1.0, 1.0, 0.5, 1.0);
    physical.Draw(context, color);
}

template struct DoubleBufferArray<Bullet>;

} // namespace Objects
