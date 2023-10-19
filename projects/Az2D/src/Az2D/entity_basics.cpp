/*
	File: entity_basics.cpp
	Author: Philip Haynes
*/

#include "entity_basics.hpp"
#include "game_systems.hpp"
#include "sprite.hpp"
#include "AzCore/Profiling.hpp"

namespace Az2D::Entities {

using namespace AzCore;
using GameSystems::sys;

Array<void*> _DoubleBufferArrays;

TypeID GenTypeId(void *ptr) {
	TypeID result = _DoubleBufferArrays.size;
	_DoubleBufferArrays.Append(ptr);
	return result;
}

const Entity& IdGeneric::GetConst() const {
	AzAssert(type != UINT64_MAX, "IdGeneric not initialized correctly!");
	void *ptr = _DoubleBufferArrays[type];
	bool buffer = *(bool*)((char*)ptr + offsetof(DoubleBufferArray<Entity>, buffer));
	size_t typeStride = *(size_t*)((char*)ptr + offsetof(DoubleBufferArray<Entity>, typeStride));
	char **data = (char**)((char*)ptr + (!buffer) * sizeof(Array<Entity>));
	Entity *result = (Entity*)(*data + typeStride * id.index);
	return *result;
}

Entity& IdGeneric::GetMut() const {
	AzAssert(type != UINT64_MAX, "IdGeneric not initialized correctly!");
	void *ptr = _DoubleBufferArrays[type];
	bool buffer = *(bool*)((char*)ptr + offsetof(DoubleBufferArray<Entity>, buffer));
	size_t typeStride = *(size_t*)((char*)ptr + offsetof(DoubleBufferArray<Entity>, typeStride));
	char **data = (char**)((char*)ptr + (buffer) * sizeof(Array<Entity>));
	Entity *result = (Entity*)(*data + typeStride * id.index);
	return *result;
}

bool IdGeneric::Valid() const {
	if (type == UINT64_MAX) return false;
	void *ptr = _DoubleBufferArrays[type];
	i32 size = *(i32*)((char*)ptr + offsetof(DoubleBufferArray<Entity>, size));
	if (id.index >= size) return false;
	const Entity &entity = GetConst();
	return entity.id.generation > 0;
}

ManagerBasic *entitiesBasic = nullptr;

ManagerBasic::ManagerBasic() {
	entitiesBasic = this;
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
	if (t == median(t, 0.0f, 1.0f)) {
		f32 x = A.x + (B.x - A.x) * t;
		if (x == median(x, aabb.minPos.x, aabb.maxPos.x)) {
			// Segment intersects
			return true;
		}
	}
	t = (aabb.maxPos.y - A.y) / (B.y - A.y);
	if (t == median(t, 0.0f, 1.0f)) {
		f32 x = A.x + (B.x - A.x) * t;
		if (x == median(x, aabb.minPos.x, aabb.maxPos.x)) {
			// Segment intersects
			return true;
		}
	}
	t = (aabb.minPos.x - A.x) / (B.x - A.x);
	if (t == median(t, 0.0f, 1.0f)) {
		f32 y = A.y + (B.y - A.y) * t;
		if (y == median(y, aabb.minPos.y, aabb.maxPos.y)) {
			// Segment intersects
			return true;
		}
	}
	t = (aabb.maxPos.x - A.x) / (B.x - A.x);
	if (t == median(t, 0.0f, 1.0f)) {
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
	return normSqr(a.actual.circle.c - b.actual.circle.c) <= square(a.actual.circle.r + b.actual.circle.r);
}

bool CollisionCircleBox(const Physical &a, const Physical &b) {
	const f32 rSquared = square(a.actual.circle.r);
	if (normSqr(a.actual.circle.c - b.actual.box.a) <= rSquared) return true;
	if (normSqr(a.actual.circle.c - b.actual.box.b) <= rSquared) return true;
	if (normSqr(a.actual.circle.c - b.actual.box.c) <= rSquared) return true;
	if (normSqr(a.actual.circle.c - b.actual.box.d) <= rSquared) return true;

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

bool CollisionBoxBoxPart(const Physical &a, const Physical &b) {
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

bool CollisionBoxBox(const Physical &a, const Physical &b) {
	// have to go both ways otherwise you can fit a smaller one inside a bigger one
	return CollisionBoxBoxPart(a, b) || CollisionBoxBoxPart(b, a);
}

void Physical::FromSpriteAABB(const Sprite &sprite, vec2 scale, vec2 shrinkTopLeft, vec2 shrinkBottomRight) {
	type = BOX;
	basis.box.a = (-sprite.origin + shrinkTopLeft) * scale;
	basis.box.b = (sprite.Size() - sprite.origin - shrinkBottomRight) * scale;
}

bool Physical::Collides(const Physical &other) const {
	AZCORE_PROFILING_SCOPED_TIMER(Az2D::Entities::Physical::Collides)
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

bool Physical::MouseOver(vec2 mouse) const {
	if (!updated) {
		UpdateActual();
	}
	switch (type) {
	case SEGMENT: {
		return distSqrToLine<true>(actual.segment.a, actual.segment.b, mouse) < 16.0f;
	}
	case CIRCLE: {
		return normSqr(actual.circle.c-mouse) <= square(actual.circle.r);
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
	AZCORE_PROFILING_SCOPED_TIMER(Az2D::Entities::Physical::PhysicalAbsFromBasis)
	mat2 rotation(1.0f);
	if (angle != 0.0f) {
		rotation = mat2::Rotation(angle.value());
	}
	switch (type) {
	case SEGMENT: {
		if (angle != 0.0f) {
			actual.segment.a = basis.segment.a * rotation + pos;
			actual.segment.b = basis.segment.b * rotation + pos;
		} else {
			actual.segment.a = basis.segment.a + pos;
			actual.segment.b = basis.segment.b + pos;
		}
		break;
	}
	case CIRCLE: {
		if (angle != 0.0f) {
			actual.circle.c = basis.circle.c * rotation + pos;
		} else {
			actual.circle.c = basis.circle.c + pos;
		}
		actual.circle.r = basis.circle.r;
		break;
	}
	case BOX: {
		if (angle != 0.0f) {
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
	AZCORE_PROFILING_SCOPED_TIMER(Az2D::Entities::Physical::Draw)
	vec2 camZoom = entitiesBasic->camZoom;
	vec2 p = entitiesBasic->WorldPosToScreen(pos);
	if (type == BOX) {
		const vec2 scale = basis.box.b - basis.box.a;
		sys->rendering.DrawQuad(context, p, scale * camZoom, 1.0f, -basis.box.a / scale, angle, Rendering::PIPELINE_BASIC_2D, color);
	} else if (type == SEGMENT) {
		vec2 scale = basis.segment.b - basis.segment.a;
		scale.y = max(scale.y, 2.0f);
		sys->rendering.DrawQuad(context, p, scale * camZoom, 1.0f, -basis.segment.a / scale, angle, Rendering::PIPELINE_BASIC_2D, color);
	} else {
		const vec2 scale = basis.circle.r * 2.0f;
		sys->rendering.DrawCircle(context, Rendering::texBlank, color, p, scale * camZoom + 2.0f, vec2(1.0f), -basis.circle.c / (scale + 2.0f) + vec2(0.5f), angle);
	}
}

vec3 ManagerBasic::WorldPosToScreen3D(vec3 in) const {
	vec3 out = (in - vec3(camPos.x, camPos.y,	0.0f)) * camZoom + vec3(sys->window.width, sys->window.height, sys->window.height) / 2.0f;
	return out;
}
vec2 ManagerBasic::WorldPosToScreen(vec2 in) const {
	vec2 out = (in - camPos) * camZoom + vec2(sys->window.width, sys->window.height) / 2.0f;
	return out;
}
vec2 ManagerBasic::ScreenPosToWorld(vec2 in) const {
	vec2 out = (in - vec2(sys->window.width, sys->window.height) / 2.0f) / camZoom + camPos;
	return out;
}

vec2 ManagerBasic::CamTopLeft() const {
	vec2 out = camPos - vec2(sys->window.width, sys->window.height) / 2.0f / camZoom;
	return out;
}

vec2 ManagerBasic::CamBottomRight() const {
	vec2 out = camPos + vec2(sys->window.width, sys->window.height) / 2.0f / camZoom;
	return out;
}

AABB ManagerBasic::CamBounds() const {
	AABB out;
	out.minPos = CamTopLeft();
	out.maxPos = CamBottomRight();
	return out;
}

void ManagerBasic::EventSync() {
	timestep = sys->timestep * sys->simulationRate;
	workChunks.size = 0;
}

void ManagerBasic::EventUpdate() {
	AZCORE_PROFILING_SCOPED_TIMER(Az2D::Entities::ManagerBasic::EventUpdate)
	if (timestep != 0.0f) {
		const i32 concurrency = 4;
		Array<Thread> threads(concurrency);
		for (i32 i = 0; i < workChunks.size; i++) {
			WorkChunk &chunk = workChunks[i];
			for (i32 j = 0; j < concurrency; j++) {
				// TODO: Use persistent threads
				threads[j] = Thread(chunk.updateCallback, chunk.theThisPointer, j, concurrency);
			}
			for (i32 j = 0; j < concurrency; j++) {
				if (threads[j].Joinable()) {
					threads[j].Join();
				}
			}
		}
	}
}

void ManagerBasic::EventDraw(Array<Rendering::DrawingContext> &contexts) {
	AZCORE_PROFILING_SCOPED_TIMER(Az2D::Entities::ManagerBasic::EventDraw)
	const i32 concurrency = contexts.size;
	Array<Thread> threads(concurrency);
	for (i32 i = 0; i < workChunks.size; i++) {
		for (i32 j = 0; j < concurrency; j++) {
			WorkChunk &chunk = workChunks[i];
			// TODO: Use persistent threads
			threads[j] = Thread(chunk.drawCallback, chunk.theThisPointer, &contexts[j], j, concurrency);
		}
		for (i32 j = 0; j < concurrency; j++) {
			if (threads[j].Joinable()) {
				threads[j].Join();
			}
		}
	}
}

} // namespace Entities
