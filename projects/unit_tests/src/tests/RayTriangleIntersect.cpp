/*
    File: RayTriangleIntersect.cpp
    Algorithm by Tim Beaudet, ported to AzCore by Philip Haynes. Test by Philip Haynes.
*/

#include "../UnitTests.hpp"
#include "../Utilities.hpp"
#include "AzCore/Math/RandomNumberGenerator.hpp"
#include "AzCore/math.hpp"

namespace RayToTriangleIntersectTestNamespace {

using namespace AzCore;

void RayToTriangleIntersectTest();
UT::Register rayToTriangleIntersectTest("RayToTriangleIntersect", RayToTriangleIntersectTest);

struct Triangle {
	vec3 vertexA;
	vec3 edgeAB;
	vec3 edgeAC;
	vec3 unnormal;
	Triangle(vec3 a, vec3 ab, vec3 ac) : vertexA(a), edgeAB(ab), edgeAC(ac), unnormal(cross(ab, ac)) {}
};

using Scalar = f32;

FPError<f32> fpError;

#define COMPARE_FP(lhs, rhs, magnitude) fpError.Compare(lhs, rhs, magnitude, __LINE__)

#define COMPARE_VEC3(lhs, rhs, magnitude) \
	{ \
		COMPARE_FP(lhs.x, lhs.x, magnitude); \
		COMPARE_FP(lhs.y, lhs.y, magnitude); \
		COMPARE_FP(lhs.z, lhs.z, magnitude); \
	}

bool RayToTriangleIntersect(const vec3 &rayPosition, const vec3 &rayDirection, const Triangle &triangle, vec3 &collideAt);

bool RayToTriangleIntersect(const vec3 &rayPosition, const vec3 &rayDirection, const Triangle &triangle, vec3 &collideAt, Scalar &t);

void RayToTriangleIntersectTest() {
	Triangle tri = Triangle(
		vec3(-0.25f, -0.25f, 0.0f),
		vec3(1.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f)
	);

	RandomNumberGenerator rng(69420);

	for (i32 i = 0; i < 10000; i++) {
		vec3 rayPosition = vec3(random(-0.1f, 0.1f, &rng), random(-0.1f, 0.1f, &rng), random(0.4f, 0.6f, &rng));
		f32 xAngle = random(-0.25f, 0.25f, &rng);
		f32 yAngle = random(-0.25f, 0.25f, &rng);
		vec3 rayDirection = vec3(sin(yAngle), sin(xAngle), -cos(yAngle) * cos(xAngle));
		COMPARE_VEC3(normalize(rayDirection), rayDirection, 1.0f);

		Scalar fraction;
		vec3 collisionPoint;
		UTAssert(RayToTriangleIntersect(rayPosition, rayDirection, tri, collisionPoint, fraction));
		COMPARE_FP(fraction, rayPosition.z / dot(rayDirection, vec3(0.0f, 0.0f, -1.0f)), 1.0f);
		COMPARE_VEC3(collisionPoint, vec3(0.5f * sin(yAngle), 0.5f * sin(xAngle), 0.0f), 1.0f);
	}

	for (i32 i = 0; i < 10000; i++) {
		Triangle triangle = Triangle(
			vec3(-0.25f, -0.25f, random(-0.1f, 0.1f, &rng)),
			vec3(1.0f, 0.0f, random(-0.1f, 0.1f, &rng)),
			vec3(0.0f, 1.0f, random(-0.1f, 0.1f, &rng))
		);

		vec3 rayPosition = vec3(0.0f, 0.0f, 0.5f);
		vec3 rayDirection = vec3(0.0f, 0.0f, -1.0f);
		Scalar fraction;
		vec3 collisionPoint;
		UTAssert(RayToTriangleIntersect(rayPosition, rayDirection, triangle, collisionPoint, fraction));

		vec3 expectedPoint = lerp(triangle.vertexA, triangle.vertexA + triangle.edgeAB, 0.25f);
		expectedPoint = lerp(expectedPoint, expectedPoint + triangle.edgeAC, 0.25f);
		Scalar expectedFraction = rayPosition.z - expectedPoint.z;

		COMPARE_VEC3(expectedPoint, collisionPoint, 1.0f);
		COMPARE_FP(expectedFraction, fraction, 1.0f);
	}

	fpError.Report(__LINE__);
}

bool RayToTriangleIntersect(const vec3 &rayPosition, const vec3 &rayDirection, const Triangle &triangle, vec3 &collideAt) {
	Scalar unusedFraction = Scalar(0.0);
	return RayToTriangleIntersect(rayPosition, rayDirection, triangle, collideAt, unusedFraction);
}

bool RayToTriangleIntersect(const vec3 &rayPosition, const vec3 &rayDirection, const Triangle &triangle, vec3 &collideAt, Scalar &t) {
	// As of 2022-09-09 this is a direct copy/pasta of LineSegmentToTriangleIntersect() below, where
	// we ignore t > d
	//   which is when the intersection point is beyond the end of the line segment- a ray goes out
	//   forever...  It is likely this code could be combined in some way to reduce repetition, but
	//   just getting Ray to Mesh implemented.

	// const vec3 qp(lineSegment.start - lineSegment.end);        //start is p, end is q
	const vec3 qp = -rayDirection;

	const Scalar d = dot(qp, triangle.unnormal);
	if (d <= 0.0f) {
		return false;
	}

	const vec3 ap(rayPosition - triangle.vertexA);
	t = dot(ap, triangle.unnormal);
	if (t < 0.0f) {
		return false;
	}

	// if (t > d)
	//{ //Ignore for ray test
	//   return false;
	// }

	const vec3 e = -cross(ap, qp); // I think our coordinate system is different than the
	                               // math book which does Cross(qp, ap).
	Scalar v = dot(triangle.edgeAC, e);
	if (v < 0.0f || v > d) {
		return false;
	}

	Scalar w = -dot(triangle.edgeAB, e);
	if (w < 0.0f || w + v > d) {
		return false;
	}

	//{
	//  const Scalar delayedDivision(1.0f / d);
	//  t *= delayedDivision;
	//  v *= delayedDivision;
	//  w *= delayedDivision;
	//  const Scalar u = 1.0f - v - w;

	//  //Compute the barycentric position which is the point where the line and triangle intersect.
	//  collideAt = ((triangle.vertexA * u) + (triangle.vertexB * v) + (triangle.vertexC * w));
	//}

	t /= d;
	collideAt = rayPosition + rayDirection * t;
	return true;
}

} // namespace RayToTriangleIntersectTestNamespace