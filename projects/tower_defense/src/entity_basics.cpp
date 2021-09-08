/*
	File: entity_basics.cpp
	Author: Philip Haynes
*/

#include "entity_basics.hpp"
#include "globals.hpp"

namespace Entities {

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
	const vec2 mouse = globals->entities.mouse;
	if (!updated) {
		UpdateActual();
	}
	switch (type) {
	case SEGMENT: {
		return distSqrToLine<true>(actual.segment.a, actual.segment.b, mouse) < 16.0f;
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
	const vec2 p = (pos - globals->entities.camPos) * globals->entities.camZoom
				 + vec2(globals->window.width / 2, globals->window.height / 2);
	if (type == BOX) {
		const vec2 scale = basis.box.b - basis.box.a;
		globals->rendering.DrawQuad(context, Rendering::texBlank, color, p, scale * globals->entities.camZoom, vec2(1.0f), -basis.box.a / scale, angle);
	} else if (type == SEGMENT) {
		vec2 scale = basis.segment.b - basis.segment.a;
		scale.y = max(scale.y, 2.0f);
		globals->rendering.DrawQuad(context, Rendering::texBlank, color, p, scale * globals->entities.camZoom, vec2(1.0f), -basis.segment.a / scale, angle);
	} else {
		const vec2 scale = basis.circle.r * 2.0f;
		globals->rendering.DrawCircle(context, Rendering::texBlank, color, p, scale * globals->entities.camZoom + 2.0f, vec2(1.0f), -basis.circle.c / (scale + 2.0f) + vec2(0.5f), angle);
	}
}

template<typename T>
void DoubleBufferArray<T>::Update(void *theThisPointer, i32 threadIndex, i32 concurrency) {
	DoubleBufferArray<T> *theActualThisPointer = (DoubleBufferArray<T>*)theThisPointer;
	i32 g = theActualThisPointer->granularity;
	bool doTwice = globals->entities.timestep < 1.0f/30.0f;
	f32 timestep = globals->entities.timestep;
	if (doTwice) timestep /= 2.0f;
	for (i32 i = threadIndex*g; i < theActualThisPointer->array[theActualThisPointer->buffer].size; i += g*concurrency) {
		for (i32 j = 0; j < g; j++) {
			if (i+j >= theActualThisPointer->array[theActualThisPointer->buffer].size) break;
			T &obj = theActualThisPointer->array[theActualThisPointer->buffer][i+j];
			if (obj.id.generation >= 0) {
				obj.Update(timestep);
			}
			if (doTwice) {
				if (obj.id.generation >= 0) {
					obj.Update(timestep);
				}
			}
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
		// array[!buffer][index].EventDestroy();
		array[!buffer][index].id.generation = -array[!buffer][index].id.generation-1;
		empty.Append(index);
	}
	count -= destroyed.size;
	destroyed.Clear();
	for (T &obj : created) {
		if (empty.size > 0) {
			obj.id.index = empty.Back();
			obj.id.generation = -array[!buffer][obj.id.index].id.generation;
			// obj.EventCreate();
			array[!buffer][obj.id.index] = std::move(obj);
			empty.Erase(empty.size-1);
		} else {
			obj.id.index = array[!buffer].size;
			obj.id.generation = 0;
			// obj.EventCreate();
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
	mutex.Lock();
	obj.EventCreate();
	created.Append(obj);
	mutex.Unlock();
}

template<typename T>
void DoubleBufferArray<T>::Destroy(Id id) {
	mutex.Lock();
	if (array[!buffer][id.index].id != id) {
		// std::cout << "Attempt to destroy an object of the wrong generation! Expected gen = "
		//		   << id.generation << ", actual gen = " << array[!buffer][id.index].id.generation << std::endl;
		mutex.Unlock();
		return;
	}
	for (i32 i = 0; i < destroyed.size; i++) {
		if (destroyed[i] == id.index) {
			mutex.Unlock();
			return;
		}
	}
	array[!buffer][id.index].EventDestroy();
	destroyed.Append(id.index);
	array[!buffer][id.index].id.generation *= -1;
	mutex.Unlock();
}

} // namespace Entities
