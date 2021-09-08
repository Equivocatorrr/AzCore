/*
	File: font.cpp
	Author: Philip Haynes
*/

#include "font.hpp"

// #define LOG_VERBOSE

#include "Font/FontTables.cpp"

namespace AzCore {

String ToString(font::Tag_t tag) {
	String string(4);
	for (u32 x = 0; x < 4; x++) {
		string[x] = tag.name[x];
	}
	return string;
}

namespace font {

const f32 sdfDistance = 0.12f; // Units in the Em square
// const f32 sdfDistance = 0.05f; // Units in the Em square

inline f32 BezierDerivative(f32 t, f32 p1, f32 p2, f32 p3) {
	return 2.0f * ((1.0f-t) * (p2-p1) + t * (p3-p2));
}

inline i32 signToWinding(f32 d) {
	if (d > 0.0f) {
		return 1;
	} else if (d < 0.0f) {
		return -1;
	} else {
		return 0;
	}
}

inline i32 BezierDerivativeSign(f32 t, f32 p1, f32 p2, f32 p3) {
	return signToWinding((1.0f-t) * (p2-p1) + t*(p3-p2));
}

inline f32 BezierDerivative(f32 t, f32 p1, f32 p2, f32 p3, f32 p4) {
	f32 tInv = 1.0f - t;
	return 3.0f * (square(tInv)*(p2-p1) + 2.0f*tInv*t*(p3-p2) + square(t)*(p4-p3));
}

inline i32 BezierDerivativeSign(f32 t, f32 p1, f32 p2, f32 p3, f32 p4) {
	f32 tInv = 1.0f - t;
	return signToWinding(square(tInv)*(p2-p1) + 2.0f*tInv*t*(p3-p2) + square(t)*(p4-p3));
}

i32 Line::Intersection(const vec2 &point) const {
	// Bezier(t) = (1-t)*p1 + t*p3
	// Bezier(t) = t(p3 - p1) + p1
	// t = -p1 / (p3 - p1)

	// if (p2.x == p1.x) {
	//	 // Vertical line
	//	 if (p2.x >= point.x) {
	//		 if (point.y >= p1.y && point.y < p2.y) {
	//			 return 1;
	//		 } else if (point.y >= p2.y && point.y < p1.y) {
	//			 return -1;
	//		 } else {
	//			 return 0;
	//		 }
	//	 }
	// } else {
	//	 f32 a = p2.y - p1.y, b; // coefficients of the line: y = ax + b
	//	 if (a == 0.0f) {
	//		 // Horizontal line
	//		 return 0;
	//	 } else {
	//		 b = -p1.y + point.y;
	//		 f32 t = b / a;
	//		 if (a > 0.0f) {
	//			 if (t >= 0.0f && t < 1.0f) {
	//				 f32 x = (p2.x - p1.x) * t + p1.x;
	//				 if (x >= point.x) {
	//					 return 1;
	//				 }
	//			 }
	//		 } else {
	//			 if (t > 0.0f && t <= 1.0f) {
	//				 f32 x = (p2.x - p1.x) * t + p1.x;
	//				 if (x >= point.x) {
	//					 return -1;
	//				 }
	//			 }
	//		 }
	//	 }
	// }
	// return 0;
	if (point.x > max(p1.x, p2.x)) {
		return 0;
	}
	if (point.y < min(p1.y, p2.y)-0.00001f) {
		return 0;
	}
	if (point.y > max(p1.y, p2.y)+0.00001f) {
		return 0;
	}
	i32 winding = 0;

	f32 a, b; // coefficients of the line: y = bx + c
	a = p2.y - p1.y;
	if (a == 0.0f) {
		return 0;
	}
	if (abs(point.y-p1.y) < 0.00001f && p1.x >= point.x) {
		return signToWinding(p2.y - p1.y);
	}
	if (abs(point.y-p2.y) < 0.00001f && p2.x >= point.x) {
		return 0;
	}
	b = p1.y - point.y;

	SolutionLinear<f32> solution = SolveLinear(a, b);

	a = p2.x - p1.x;
	b = p1.x;
	if (solution.nReal) {
		f32 t = solution.root;
		bool tInRange;
		// if (p1.y < p2.y) {
		//	 tInRange = t >= 0.0f && t < 1.0f;
		// } else {
		//	 tInRange = t > 0.0f && t <= 1.0f;
		// }
		tInRange = t > 0.00001f && t < 0.99999f;
		if (tInRange) {
			f32 x = a*t + b;
			if (x >= point.x) {
				// We have an intersection
				winding += signToWinding(p2.y - p1.y);
			}
		}
	}
	return winding;
};

f32 Line::DistanceLess(const vec2 &point, f32 distSquared) const {
	f32 dist = distSqrToLine<true>(p1, p2, point);
	if (dist < distSquared) {
		distSquared = dist;
	}
	return distSquared;
}

void Line::Scale(const mat2 &scale) {
	p1 = scale * p1;
	p2 = scale * p2;
}

void Line::Offset(const vec2 &offset) {
	p1 = p1 + offset;
	p2 = p2 + offset;
}

void Line::Print(io::Log &cout) {
	cout.Print("p1={", p1.x, ",", p1.y, "}, p2={", p2.x, ",", p2.y, "}");
}

i32 Curve::Intersection(const vec2 &point) const {
	if (point.x > max(max(p1.x, p2.x), p3.x)) {
		return 0;
	}
	if (point.y < min(min(p1.y, p2.y), p3.y)-0.00001f) {
		return 0;
	}
	if (point.y > max(max(p1.y, p2.y), p3.y)+0.00001f) {
		return 0;
	}
	i32 winding = 0;
	// Bezier(t) = (1-t)*((1-t)*p1 + t*p2) + t*((1-t)*p2 + t*p3)
	// Bezier(t) = (1-t)*(1-t)*p1 + (1-t)t*(2*p2) + t^2*p3
	// Bezier(t) = p1 - t(2*p1) + t^2(p1) + t(2*p2) - t^2(2*p2) + t^2(p3)
	// Bezier(t) = t^2(p3 - 2*p2 + p1) + t(2*p2 - 2*p1) + p1
	// Bezier'(t) = 2(1-t)(p2-p1) + 2t(p3-p2);

	f32 a, b, c; // coefficients of the curve: y = ax^2 + bx + c
	a = p3.y - 2.0f*p2.y + p1.y;
	b = 2.0f*(p2.y - p1.y);
	if (abs(a) < 0.0000001f) {
		Line line{p1, p3};
		return line.Intersection(point);
	}
	c = p1.y - point.y;
	if (abs(p1.y-point.y) < 0.00001f && p1.x >= point.x) {
		c = b;
		b = a;
		a = 0.0f;
		winding += signToWinding(p2.y - p1.y);
	}
	if (abs(p3.y-point.y) < 0.00001f && p3.x >= point.x) {
		c = b + a;
		b = a;
		a = 0.0f;
		// winding += signToWinding(p3.y - p2.y);
	}

	SolutionQuadratic<f32> solution = SolveQuadratic(a, b, c);

	if (solution.nReal) {
		a = (p3.x - 2.0f*p2.x + p1.x);
		b = 2.0f*(p2.x - p1.x);
		c = p1.x;
	}
	for (i32 i = 0; i < solution.nReal; i++) {
		f32 t = solution.root[i];
		bool tInRange;
		// if (p1.y < p2.y) {
		//	 tInRange = t >= 0.0f;
		// } else {
			// tInRange = t > 0.0f;
		// }
		// if (p2.y < p3.y) {
			// tInRange = tInRange && t < 1.0f;
		// } else {
		//	 tInRange = tInRange && t <= 1.0f;
		// }
		tInRange = t > 0.00001f && t < 0.99999f;
		if (tInRange) {
			f32 x = a*square(t) + b*t + c;
			if (x >= point.x) {
				// We have an intersection
				winding += BezierDerivativeSign(t, p1.y, p2.y, p3.y);
			}
		}
	}
	return winding;
}

f32 Curve::DistanceLess(const vec2 &point, f32 distSquared) const {
	// Try to do an early out if we can
	{
		f32 maxPointDistSquared = max(max(absSqr(p1-p2), absSqr(p2-p3)), absSqr(p3-p1));
		f32 minDistSquared = min(min(absSqr(p1-point), absSqr(p2-point)), absSqr(p3-point));
		if (minDistSquared > distSquared + maxPointDistSquared * 0.25f) {
			// NOTE: Should this be maxPointDist*square(sin(pi/4)) ???
			return distSquared;
		}
	}
	// B(t) = (1-t)^2*p1 + 2t(1-t)*p2 + t^2*p3
	// B'(t) = 2((1-t)(p2-p1) + t(p3-p2))
	// M = p2 - p1
	// N = p3 - p2 - M
	// B'(t) = 2(M + t*N)
	// dot(B(t) - point, B'(t)) = 0
	// dot((1-t)^2*p1 + 2t(1-t)*p2 + t^2*p3 - point, 2(M + t*N)) = 0
	// at^3 + bt^2 + ct + d = 0
	// a = N^2
	// b = 3*dot(M, N)
	// c = 2*M^2 + dot(p1-point, N)
	// d = dot(p1-point, M)
	const vec2 m = p2 - p1;
	const vec2 n = p3 - p2 - m;
	const vec2 o = p1 - point;

	const f32 a = absSqr(n);
	const f32 b = dot(m, n) * 3.0f;
	const f32 c = absSqr(m) * 2.0f + dot(o, n);
	const f32 d = dot(o, m);
	SolutionCubic<f32> solution = SolveCubic<f32>(a, b, c, d);
	// We're guaranteed at least 1 solution.
	for (i32 i = 0; i < solution.nReal; i++) {
		f32 dist;
		if (solution.root[i] < 0.0f) {
			dist = absSqr(p1-point);
		} else if (solution.root[i] > 1.0f) {
			dist = absSqr(p3-point);
		} else {
			dist = absSqr(Point(solution.root[i])-point);
		}
		if (dist < distSquared) {
			distSquared = dist;
		}
	}
	return distSquared;
}

void Curve::Scale(const mat2 &scale) {
	p1 = scale * p1;
	p2 = scale * p2;
	p3 = scale * p3;
}

void Curve::Offset(const vec2 &offset) {
	p1 = p1 + offset;
	p2 = p2 + offset;
	p3 = p3 + offset;
}

void Curve::Print(io::Log &cout) {
	cout.Print("p1={", p1.x, ",", p1.y, "}, p2={", p2.x, ",", p2.y, "}, p3={", p3.x, ",", p3.y, "}");
}

i32 Curve2::Intersection(const vec2 &point) const {
	if (point.x > max(max(max(p1.x, p2.x), p3.x), p4.x)) {
		return 0;
	}
	if (point.y < min(min(min(p1.y, p2.y), p3.y), p4.y)-0.00001f) {
		return 0;
	}
	if (point.y > max(max(max(p1.y, p2.y), p3.y), p4.y)+0.00001f) {
		return 0;
	}
	i32 winding = 0;

	// B(t) = p1 + t*(3*(p2-p1)) + t^2*(3*(p1 - 2*p2 + p3)) + t^3*(-p1 + 3*(p2 - p3) + p4)

	f32 a, b, c, d; // coefficients of the curve: y = ax^3 + bx^2 + cx + d
	a = p4.y + 3.0f * (p2.y - p3.y) - p1.y;
	if (abs(a) < 0.00001f) {
		Curve curve{p1, p1 + (p2-p1) * 3.0f/2.0f, p4};
		return curve.Intersection(point);
	}
	b = 3.0f * (p3.y - 2.0f * p2.y + p1.y);
	c = 3.0f * (p2.y - p1.y);
	d = p1.y - point.y;
	if (abs(p1.y-point.y) < 0.00001f && p1.x >= point.x) {
		d = c;
		c = b;
		b = a;
		a = 0.0f;
		winding += signToWinding(p2.y - p1.y);
	}
	if (abs(p4.y-point.y) < 0.00001f && p4.x >= point.x) {
		d = c + b + a;
		c = b + a;
		b = a;
		a = 0.0f;
		// winding += signToWinding(p4.y - p3.y);
	}

	// SolutionCubic<f64> solution = SolveCubic((f64)a, (f64)b, (f64)c, (f64)d);
	SolutionCubic<f32> solution = SolveCubic(a, b, c, d);

	a = (p4.x + 3.0f*(p2.x - p3.x) - p1.x);
	b = 3.0f * (p3.x - 2.0f * p2.x + p1.x);
	c = 3.0f * (p2.x - p1.x);
	d = p1.x;
	for (i32 i = 0; i < solution.nReal; i++) {
		bool tInRange;
		f32 t = (f32)solution.root[i];
		// if (p1.y < p2.y) {
		//	 tInRange = t >= 0.0f;
		// } else {
			// tInRange = t > 0.0f;
		// }
		// if (p3.y < p4.y) {
			// tInRange = tInRange && t < 1.0;
		// } else {
		//	 tInRange = tInRange && t <= 1.0f;
		// }
		tInRange = t > 0.00001f && t < 0.99999f;
		if (tInRange) {
			f32 x = a*(t*t*t) + b*square(t) + c*t + d;
			if (x >= point.x) {
				// We have an intersection
				winding += BezierDerivativeSign(t, p1.y, p2.y, p3.y, p4.y);
			}
		}
	}
	return winding;
}

#if 0

f32 Curve2::DistanceLess(const vec2 &point, f32 distSquared) const {
	// Try to do an early out if we can
	{
		// NOTE: This could probably be refined
		f32 maxPointDistSquared = max(
			max(max(absSqr(p1-p2), absSqr(p1-p3)), absSqr(p1-p4)),
			max(max(absSqr(p2-p3), absSqr(p2-p4)), absSqr(p3-p4))
		);
		f32 minDistSquared = min(
			min(
				min(absSqr(p1-point), absSqr(p2-point)),
				absSqr(p3-point)
			), absSqr(p4-point)
		);
		if (minDistSquared > distSquared + maxPointDistSquared * 0.25f) {
			return distSquared;
		}
	}
	// A huge thanks to desmos for making all this work smooth and confirmable
	// B(t) = (1-t)^3*p1 + 3t(1-t)^2*p2 + 3t^2(1-t)p3 + t^3*p4
	// B'(t) = 3((1-t)^2(p2-p1) + 2t(1-t)(p3-p2) + t^2(p4-p3))
	// B'(t) = 3((t^2-2t+1)(p2-p1) + 2(t-t^2)(p3-p2) + t^2(p4-p3))
	// B'(t) = 3(t^2(p4 - 3*p3 + 3*p2 - p1) + 2t(p3 - 2*p2 + p1) + (p2-p1))
	// M = p2 - p1
	// N = p3 - 2*p2 + p1
	// O = p4 - 3*p3 + 3*p2 - p1
	// B'(t) = 3(M + 2t*N + t^2*O)
	// Rather than doing all that work for B(t) again I'll just integrate
	// B(t) = 3t*M + 3t^2*N + t^3*O + p1
	// dot(B(t) - point, B'(t)) = 0
	// dot(3t*M + 3t^2*N + t^3*O + p1 - point, 3M + 6t*N + 3t^2*O)) = 0
	// Rearrange everything into the following form:
	// at^5 + bt^4 + ct^3 + dt^2 + et + f = 0
	// a = 3*|O^2|
	// b = 15*dot(O, N)
	// c = 12*dot(O, M) + 18*|N^2|
	// d = 27*dot(N, M) + 3*dot(p1, O) - 3*dot(point, O)
	// e = 9*|M^2| + 6*dot(p1, N) - 6*dot(point, N)
	// f = 3*dot(p1, M) - 3*dot(point, M)
	const vec2 m = p2 - p1;
	const vec2 n = p3 - p2 - m; // Equates to p3 - 2*p2 + p1, but is faster
	const vec2 o = p4 + (p2 - p3) * 3.0f - p1;

	const f32 a = 3.0f * absSqr(o);
	const f32 b = 15.0f * dot(o, n);
	const f32 c = 12.0f * dot(o, m) + 18.0f * absSqr(n);
	const f32 d = 27.0f * dot(n, m) + 3.0f * dot(p1, o) - 3.0f * dot(point, o);
	const f32 e = 9.0f * absSqr(m) + 6.0f * dot(p1, n) - 6.0f * dot(point, n);
	const f32 f = 3.0f * dot(p1, m) - 3.0f * dot(point, m);
	SolutionQuintic<f32> solution = SolveQuintic<f32>(a, b, c, d, e, f);
	// We're guaranteed at least 1 solution.
	for (i32 i = 0; i < solution.nReal; i++) {
		if (isnan(solution.root[i])) {
			throw std::runtime_error("Hahah fuck");
		}
		f32 dist;
		if (solution.root[i] <= 0.0f) {
			dist = absSqr(p1-point);
		} else if (solution.root[i] >= 1.0f) {
			dist = absSqr(p4-point);
		} else {
			dist = absSqr(Point(solution.root[i])-point);
		}
		if (dist < distSquared) {
			distSquared = dist;
		}
	}
	return distSquared;
}

#else

f32 Curve2::DistanceLess(const vec2 &point, f32 distSquared) const {
	// Try to do an early out if we can
	{
		// NOTE: This could probably be refined
		f32 maxPointDistSquared = max(
			max(max(absSqr(p1-p2), absSqr(p1-p3)), absSqr(p1-p4)),
			max(max(absSqr(p2-p3), absSqr(p2-p4)), absSqr(p3-p4))
		);
		f32 minDistSquared = min(
			min(
				min(absSqr(p1-point), absSqr(p2-point)),
				absSqr(p3-point)
			), absSqr(p4-point)
		);
		if (minDistSquared > distSquared + maxPointDistSquared * 0.25f) {
			return distSquared;
		}
	}

	constexpr i32 accuracy = 13;
	Curve2 curve = *this;
	curve.Offset(-point);
	// Thanks to Freya Holmér for this algorithm
	f32 sqDistStart = absSqr(curve.p1);
	f32 sqDistEnd = absSqr(curve.p4);
	if (sqDistStart < distSquared) {
		distSquared = sqDistStart;
	}
	if (sqDistEnd < distSquared) {
		distSquared = sqDistEnd;
	}
	BucketArray<f32, accuracy+3> samples(accuracy+3);
	BucketArray<f32, 4> pits;
	for (i32 i = 0; i <= accuracy; i++) {
		f32 t = (f32)i / (f32)accuracy;
		samples[i+1] = absSqr(curve.Point(t));
	}
	samples[0] = samples[1];
	samples.Back() = samples[samples.size-2];
	for (i32 i = 1; i < accuracy+2; i++) {
		if (samples[i] <= samples[i-1] && samples[i] <= samples[i+1]) {
			pits.Append((f32)(i-1) / (f32)accuracy);
		}
	}
	for (f32 &t : pits) {
		for (i32 i = 0; i < 4; i++) {
			vec2 f, fp, fpp;
			f = curve.Point(t, fp, fpp);
			f32 distDerivative = dot(f, fp);
			t -= distDerivative / (dot(f, fpp) + absSqr(fp));
		}
		t = clamp(t, 0.0f, 1.0f);
		f32 dist = absSqr(curve.Point(t));
		if (dist < distSquared) {
			distSquared = dist;
		}
	}
	return distSquared;
}

#endif

void Curve2::Scale(const mat2 &scale) {
	p1 = scale * p1;
	p2 = scale * p2;
	p3 = scale * p3;
	p4 = scale * p4;
}

void Curve2::Offset(const vec2 &offset) {
	p1 = p1 + offset;
	p2 = p2 + offset;
	p3 = p3 + offset;
	p4 = p4 + offset;
}

void Curve2::Print(io::Log &cout) {
	cout.Print("p1={", p1.x, ",", p1.y, "}, p2={", p2.x, ",", p2.y, "}, p3={", p3.x, ",", p3.y, "}, p4={", p4.x, ",", p4.y, "}");
}

bool Glyph::Inside(const vec2 &point) const {
	i32 winding = 0;
	for (const Curve2& curve2 : curve2s) {
		winding += curve2.Intersection(point);
	}
	for (const Curve& curve : curves) {
		winding += curve.Intersection(point);
	}
	for (const Line& line : lines) {
		winding += line.Intersection(point);
	}
	return winding != 0;
}

f32 Glyph::MinDistance(vec2 point, const f32& startingDist) const {
	f32 minDistSquared = startingDist*startingDist; // Glyphs should be normalized to the em square more or less.
	for (const Curve2& curve2 : curve2s) {
		minDistSquared = curve2.DistanceLess(point, minDistSquared);
	}
	for (const Curve& curve : curves) {
		minDistSquared = curve.DistanceLess(point, minDistSquared);
	}
	for (const Line& line : lines) {
		minDistSquared = line.DistanceLess(point, minDistSquared);
	}
	return sqrt(minDistSquared);
}

void Glyph::AddFromGlyfPoints(glyfPoint *glyfPoints, i32 count) {
	Curve curve;
	Line line;
	for (i32 i = 0; i < count; i++) {
		if (glyfPoints[i%count].onCurve) {
			if (glyfPoints[(i+1)%count].onCurve) {
				// Line segment
				line.p1 = glyfPoints[i%count].coords;
				line.p2 = glyfPoints[(i+1)%count].coords;
				lines.Append(line);
			} else {
				// Bezier
				curve.p1 = glyfPoints[i%count].coords;
				curve.p2 = glyfPoints[(i+1)%count].coords;
				if (glyfPoints[(i+2)%count].onCurve) {
					curve.p3 = glyfPoints[(i+2)%count].coords;
					i++; // next iteration starts at i+2
				} else {
					curve.p3 = (glyfPoints[(i+1)%count].coords + glyfPoints[(i+2)%count].coords) * 0.5f;
				}
				curves.Append(curve);
			}
		} else {
			if (glyfPoints[(i+1)%count].onCurve) {
				// I don't think this should happen???
				continue;
			} else {
				// Implied on-curve points on either side
				curve.p1 = (glyfPoints[i%count].coords + glyfPoints[(i+1)%count].coords) * 0.5;
				curve.p2 = glyfPoints[(i+1)%count].coords;
				if (glyfPoints[(i+2)%count].onCurve) {
					curve.p3 = glyfPoints[(i+2)%count].coords;
					i++;
				} else {
					curve.p3 = (glyfPoints[(i+1)%count].coords + glyfPoints[(i+2)%count].coords) * 0.5;
				}
				curves.Append(curve);
			}
		}
	}
}

void Glyph::Scale(const mat2 &scale) {
	for (Curve2& curve2 : curve2s) {
		curve2.Scale(scale);
	}
	for (Curve& curve : curves) {
		curve.Scale(scale);
	}
	for (Line& line : lines) {
		line.Scale(scale);
	}
}

void Glyph::Offset(const vec2 &offset) {
	for (Curve2& curve2 : curve2s) {
		curve2.Offset(offset);
	}
	for (Curve& curve : curves) {
		curve.Offset(offset);
	}
	for (Line& line : lines) {
		line.Offset(offset);
	}
}

void Glyph::Print(io::Log &cout) {
	cout.PrintLn("Curve2s: ", curve2s.size, "");
	for (Curve2& curve2 : curve2s) {
		cout.Print("\t");
		curve2.Print(cout);
		cout.Print("\n");
	}
	cout.PrintLn("Curves: ", curves.size, "");
	for (Curve& curve : curves) {
		cout.Print("\t");
		curve.Print(cout);
		cout.Print("\n");
	}
	cout.PrintLn("Lines: ", lines.size, "");
	for (Line& line : lines) {
		cout.Print("\t");
		line.Print(cout);
		cout.Print("\n");
	}
	cout.Print("\n");
}

Glyph& Glyph::Simplify() {
	for (i32 i = 0; i < curve2s.size; i++) {
		Curve2 &curve2 = curve2s[i];
		vec2 normal = normalize(curve2.p4 - curve2.p1);
		if (dot(normal, normalize(curve2.p2 - curve2.p1)) == 1.0f
		 && dot(normal, normalize(curve2.p4 - curve2.p3)) == 1.0f) {
			// We're actually a line
			Line line = {curve2.p1, curve2.p4};
			lines.Append(line);
			curve2s.Erase(i--);
			continue;
		}
	}
	for (i32 i = 0; i < curves.size; i++) {
		Curve &curve = curves[i];
		vec2 normal = normalize(curve.p3 - curve.p1);
		if (dot(normal, normalize(curve.p2 - curve.p1)) == 1.0f) {
			// We're actually a line
			Line line = {curve.p1, curve.p3};
			lines.Append(line);
			curves.Erase(i--);
		}
	}
	return *this;
}

bool Font::Load() {
	if (filename.size == 0) {
		error = "No filename specified!";
		return false;
	}
	cout.PrintLn("Attempting to load \"", filename, "\"");

	data.file.open(filename.data, std::ios::in | std::ios::binary);
	if (!data.file.is_open()) {
		error = "Failed to open font with filename: \"" + filename + "\"";
		return false;
	}

	data.ttcHeader.Read(data.file);

	if (data.ttcHeader.ttcTag == 0x10000_Tag) {
		cout.PrintLn("TrueType outline");
	} else if (data.ttcHeader.ttcTag == "true"_Tag) {
		cout.PrintLn("TrueType");
	} else if (data.ttcHeader.ttcTag == "OTTO"_Tag) {
		cout.PrintLn("OpenType with CFF");
	} else if (data.ttcHeader.ttcTag == "typ1"_Tag) {
		cout.PrintLn("Old-style PostScript");
	} else if (data.ttcHeader.ttcTag == "ttcf"_Tag) {
		cout.PrintLn("TrueType Collection");
	} else {
		error = "Unknown font type for file: \"" + filename + "\"";
		return false;
	}

	data.offsetTables.Resize(data.ttcHeader.numFonts);
	HashSet<u32, 32> uniqueOffsets;
	for (u32 i = 0; i < data.ttcHeader.numFonts; i++) {
		data.file.seekg(data.ttcHeader.offsetTables[i]);
		tables::Offset &offsetTable = data.offsetTables[i];
		offsetTable.Read(data.file);
#ifdef LOG_VERBOSE
		cout << "Font[" << i << "]\n\tnumTables: " << offsetTable.numTables
			 << "\n\tsearchRange: " << offsetTable.searchRange
			 << "\n\tentrySelector: " << offsetTable.entrySelector
			 << "\n\trangeShift: " << offsetTable.rangeShift << std::endl;
#endif
		for (u32 ii = 0; ii < offsetTable.numTables; ii++) {
			tables::Record &record = offsetTable.tables[ii];
			if (record.offset < data.offsetMin) {
				data.offsetMin = record.offset;
			}
			if (record.offset + record.length > data.offsetMax) {
				data.offsetMax = record.offset + record.length;
			}
			// Keep track of unique tables
			if (!uniqueOffsets.Exists(record.offset)) {
				data.uniqueTables.Append(record);
				uniqueOffsets.Emplace(record.offset);
			}
#ifdef LOG_VERBOSE
			cout.PrintLn("\tTable: \"", ToString(record.tableTag), "\"\n\t\toffset = ", record.offset, ", length = ", record.length);
#endif
		}
	}

#ifdef LOG_VERBOSE
	cout.PrintLn("offsetMin = ", data.offsetMin, ", offsetMax = ", data.offsetMax);
#endif
	data.tableData.Resize(align(data.offsetMax-data.offsetMin, 4), 0);

	data.file.seekg(data.offsetMin); // Should probably already be here but you never know ;)
	data.file.read(data.tableData.data, data.offsetMax-data.offsetMin);

	data.file.close();

	// Checksum verifications

	u32 checksumsCompleted = 0;
	u32 checksumsCorrect = 0;

	for (i32 i = 0; i < data.uniqueTables.size; i++) {
		tables::Record &record = data.uniqueTables[i];
		char *ptr = data.tableData.data + record.offset - data.offsetMin;
		if (record.tableTag == "head"_Tag || record.tableTag == "bhed"_Tag) {
			((tables::head*)ptr)->checkSumAdjustment = 0;
		}
		u32 checksum = tables::Checksum((u32*)ptr, record.length);
		if (checksum != record.checkSum) {
			cout.PrintLn("Checksum (", checksum, ") for table ", ToString(record.tableTag), " didn't match record (", record.checkSum, "), trying backup!");
			checksum = tables::ChecksumV2((u32*)ptr, record.length);
			if (checksum == record.checkSum) {
				checksumsCorrect++;
				cout.PrintLn("...backup worked!");
			} else {
				cout.PrintLn("...no good (", checksum, ")");
			}
		} else {
			checksumsCorrect++;
		}
		checksumsCompleted++;
	}
	cout.PrintLn("Checksums completed. ", checksumsCorrect, "/", checksumsCompleted, " correct.\n");

	// File is in big-endian, so we may need to swap the data before we can use it.

	if (SysEndian.little) {
		u16 numGlyphs = 0;
		bool longOffsets = false;
		u16 numOfLongHorMetrics = 0;
		for (i32 i = 0; i < data.uniqueTables.size; i++) {
			tables::Record &record = data.uniqueTables[i];
			char *ptr = data.tableData.data + record.offset - data.offsetMin;
			const Tag_t &tag = record.tableTag;
			Array<u32> uniqueEncodingOffsets;
			if (tag == "head"_Tag || tag == "bhed"_Tag) {
				tables::head *head = (tables::head*)ptr;
				head->EndianSwap();
				if (head->indexToLocFormat == 1) {
					longOffsets = true;
				}
			} else if (tag == "cmap"_Tag) {
				tables::cmap_index *index = (tables::cmap_index*)ptr;
				index->EndianSwap();
				for (u32 enc = 0; enc < index->numberSubtables; enc++) {
					tables::cmap_encoding *encoding = (tables::cmap_encoding*)(ptr + 4 + enc * sizeof(tables::cmap_encoding));
					encoding->EndianSwap();
					if (!uniqueEncodingOffsets.Contains(encoding->offset)) {
						uniqueEncodingOffsets.Append(encoding->offset);
						tables::cmap_format_any *cmap = (tables::cmap_format_any*)(ptr + encoding->offset);
						if (!cmap->EndianSwap()) {
#ifdef LOG_VERBOSE
							cout.PrintLn("Unsupported cmap table format ", cmap->format);
#endif
						}
					}
				}
			} else if (tag == "maxp"_Tag) {
				tables::maxp *maxp = (tables::maxp*)ptr;
				maxp->EndianSwap();
				numGlyphs = maxp->numGlyphs;
			} else if (tag == "hhea"_Tag) {
				tables::hhea *hhea = (tables::hhea*)ptr;
				hhea->EndianSwap();
				numOfLongHorMetrics = hhea->numOfLongHorMetrics;
			}
		}
		// To parse the 'loca' table correctly, head needs to be parsed first
		// To parse the 'hmtx' table correctly, hhea needs to be parsed first
		tables::loca *loca = nullptr;
		for (i32 i = 0; i < data.uniqueTables.size; i++) {
			tables::Record &record = data.uniqueTables[i];
			char *ptr = data.tableData.data + record.offset - data.offsetMin;
			const Tag_t &tag = record.tableTag;
			if (tag == "loca"_Tag) {
				loca = (tables::loca*)ptr;
				loca->EndianSwap(numGlyphs, longOffsets);
				// u32 locaGlyphs = record.length / (2 << (u32)longOffsets) - 1;
				// if (numGlyphs != locaGlyphs) {
				//	 error = "Glyph counts don't match between maxp(" + ToString(numGlyphs)
				//		   + ") and loca(" + ToString(locaGlyphs) + ")";
				//	 return false;
				// }
			} else if (tag == "hmtx"_Tag) {
				tables::hmtx *hmtx = (tables::hmtx*)ptr;
				hmtx->EndianSwap(numOfLongHorMetrics, numGlyphs);
			}
		}
		// To parse the 'glyf' table correctly, loca needs to be parsed first
		for (i32 i = 0; i < data.uniqueTables.size; i++) {
			tables::Record &record = data.uniqueTables[i];
			char *ptr = data.tableData.data + record.offset - data.offsetMin;
			const Tag_t &tag = record.tableTag;
			if (tag == "glyf"_Tag) {
				if (loca == nullptr) {
					error = "Cannot parse glyf table without a loca table!";
					return false;
				}
				tables::glyf *glyf = (tables::glyf*)ptr;
				glyf->EndianSwap(loca, numGlyphs, longOffsets);
			}
		}
	}

	// Figuring out what data to actually use

	data.cmaps.Resize(data.ttcHeader.numFonts); // One per font

	for (i32 i = 0; i < data.offsetTables.size; i++) {
#ifdef LOG_VERBOSE
		cout.PrintLn("\nFont[", i, "]\n");
#endif
		tables::Offset &offsetTable = data.offsetTables[i];
		i16 chosenCmap = -1; // -1 is not found, bigger is better
		for (i32 ii = 0; ii < offsetTable.numTables; ii++) {
			tables::Record &record = offsetTable.tables[ii];
			char *ptr = data.tableData.data + record.offset - data.offsetMin;
			Tag_t &tag = record.tableTag;
#ifdef LOG_VERBOSE
			if (tag == "head"_Tag || tag == "bhed"_Tag) {
				tables::head *head = (tables::head*)ptr;
				cout << "\thead table:\nVersion " << head->version.major << "." << head->version.minor
					 << " Revision: " << head->fontRevision.major << "." << head->fontRevision.minor
					 << "\nFlags: 0x" << std::hex << head->flags << " MacStyle: 0x" << head->macStyle
					 << std::dec << " unitsPerEm: " << head->unitsPerEm
					 << "\nxMin: " << head->xMin << " xMax: " << head->xMax
					 << " yMin: " << head->yMin << " yMax " << head->yMax << "\n" << std::endl;
			} else if (tag == "CFF "_Tag) {
				tables::cff *cff = (tables::cff*)ptr;
				cout << "\tCFF version: " << (u32)cff->header.versionMajor << "." << (u32)cff->header.versionMinor
					 << "\nheader.size: " << (u32)cff->header.size
					 << ", header.offSize: " << (u32)cff->header.offSize << std::endl;
			} else if (tag == "maxp"_Tag) {
				tables::maxp *maxp = (tables::maxp*)ptr;
				cout.PrintLn("\tmaxp table:\nnumGlyphs: ", maxp->numGlyphs, "\n");
			}
#endif
			if (tag == "cmap"_Tag) {
				tables::cmap_index *index = (tables::cmap_index*)ptr;
#ifdef LOG_VERBOSE
				cout << "\tcmap table:\nVersion " << index->version
					 << " numSubtables: " << index->numberSubtables << "\n";
#endif
				for (u32 enc = 0; enc < index->numberSubtables; enc++) {
					tables::cmap_encoding *encoding = (tables::cmap_encoding*)(ptr + 4 + enc * sizeof(tables::cmap_encoding));
// #ifdef LOG_VERBOSE
//						 cout << "\tEncoding[" << enc << "]:\nPlatformID: " << encoding->platformID
//							  << " PlatformSpecificID: " << encoding->platformSpecificID
//							  << "\nOffset: " << encoding->offset
//							  << "\nFormat: " << *((u16*)(ptr+encoding->offset)) << "\n";
// #endif
					#define CHOOSE(in) chosenCmap = in; \
							data.cmaps[i] = u32((char*)index - data.tableData.data) + encoding->offset
					if (encoding->platformID == 0 && encoding->platformSpecificID == 4) {
						CHOOSE(4);
					} else if (encoding->platformID == 0 && encoding->platformSpecificID == 3 && chosenCmap < 4) {
						CHOOSE(3);
					} else if (encoding->platformID == 3 && encoding->platformSpecificID == 10 && chosenCmap < 3) {
						CHOOSE(2);
					} else if (encoding->platformID == 3 && encoding->platformSpecificID == 1 && chosenCmap < 2) {
						CHOOSE(1);
					} else if (encoding->platformID == 3 && encoding->platformSpecificID == 0 && chosenCmap < 1) {
						CHOOSE(0);
					}
					#undef CHOOSE
				}
#ifdef LOG_VERBOSE
				cout.Print(std::endl);
#endif
			} else if (tag == "CFF "_Tag && !data.cffParsed.active) {
				tables::cff *cff = (tables::cff*)ptr;
				if (!cff->Parse(&data.cffParsed, SysEndian.little)) {
					return false;
				}
			} else if (tag == "glyf"_Tag && !data.glyfParsed.active) {
				data.glyfParsed.active = true;
				data.glyfParsed.glyphData = (tables::glyf*)ptr;
			} else if (tag == "loca"_Tag) {
				data.glyfParsed.indexToLoc = (tables::loca*)ptr;
			} else if (tag == "maxp"_Tag) {
				data.glyfParsed.maxProfile = (tables::maxp*)ptr;
				data.cffParsed.maxProfile = (tables::maxp*)ptr;
			} else if (tag == "head"_Tag || tag == "bhed"_Tag) {
				data.glyfParsed.header = (tables::head*)ptr;
				data.cffParsed.header = (tables::head*)ptr;
			} else if (tag == "hhea"_Tag) {
				data.glyfParsed.horHeader = (tables::hhea*)ptr;
				data.cffParsed.horHeader = (tables::hhea*)ptr;
			} else if (tag == "hmtx"_Tag) {
				data.glyfParsed.horMetrics = (tables::hmtx*)ptr;
				data.cffParsed.horMetrics = (tables::hmtx*)ptr;
			}
		}
		if (chosenCmap == -1) {
			cout.PrintLn("Font[", i, "] doesn't have a supported cmap table.");
			data.offsetTables.Erase(i--);
		} else {
#ifdef LOG_VERBOSE
			cout << "chosenCmap = " << chosenCmap << " offset = " << data.cmaps[i]
				 << " format = " << *((u16*)(data.tableData.data+data.cmaps[i])) << "\n" << std::endl;
#endif
		}
	}
	if (data.offsetTables.size == 0) {
		error = "Font file not supported.";
		return false;
	}

	if (data.glyfParsed.active) {
		// This at least guarantees glyphData is valid
		// NOTE: These tests are probably overkill, but then again if we're a big-endian system
		//	   they wouldn't have been done in the EndianSwap madness. Better safe than sorry???
		if (data.glyfParsed.header == nullptr) {
			error = "Can't use glyf without head!";
			return false;
		}
		if (data.glyfParsed.maxProfile == nullptr) {
			error = "Can't use glyf without maxp!";
			return false;
		}
		if (data.glyfParsed.indexToLoc == nullptr) {
			error = "Can't use glyf without loca!";
			return false;
		}
		if (data.glyfParsed.horHeader == nullptr) {
			error = "Can't use glyf without hhea!";
			return false;
		}
		if (data.glyfParsed.horMetrics == nullptr) {
			error = "Can't use glyf without hmtx!";
			return false;
		}
		data.glyfParsed.glyfOffsets.Resize((u32)data.glyfParsed.maxProfile->numGlyphs+1);
		if (data.glyfParsed.header->indexToLocFormat == 1) { // Long Offsets
			for (u32 i = 0; i < (u32)data.glyfParsed.maxProfile->numGlyphs + 1; i++) {
				u32 *offsets = (u32*)data.glyfParsed.indexToLoc;
				data.glyfParsed.glyfOffsets[i] = offsets[i];
			}
		} else { // Short Offsets
			for (u32 i = 0; i < (u32)data.glyfParsed.maxProfile->numGlyphs + 1; i++) {
				u16 *offsets = (u16*)data.glyfParsed.indexToLoc;
				data.glyfParsed.glyfOffsets[i] = offsets[i] * 2;
			}
		}
	}
	if (data.cffParsed.active) {
		if (data.cffParsed.header == nullptr) {
			error = "Can't use CFF without head!";
			return false;
		}
		if (data.cffParsed.maxProfile == nullptr) {
			error = "Can't use CFF without maxp!";
			return false;
		}
		if (data.cffParsed.horHeader == nullptr) {
			error = "Can't use CFF without hhea!";
			return false;
		}
		if (data.cffParsed.horMetrics == nullptr) {
			error = "Can't use CFF without hmtx!";
			return false;
		}
	}

	cout.PrintLn("Successfully prepared \"", filename, "\" for usage.");

	return true;
}

u16 Font::GetGlyphIndex(char32 unicode) const {
	for (i32 i = 0; i < data.cmaps.size; i++) {
		tables::cmap_format_any *cmap = (tables::cmap_format_any*)(data.tableData.data + data.cmaps[i]);
		u16 glyphIndex = cmap->GetGlyphIndex(unicode);
		if (glyphIndex) {
			return glyphIndex;
		}
	}
	return 0;
}

Glyph Font::GetGlyphByIndex(u16 index) const {
	if (data.cffParsed.active) {
		return data.cffParsed.GetGlyph(index);
	} else if (data.glyfParsed.active) {
		return data.glyfParsed.GetGlyph(index);
	}
	throw std::runtime_error("No glyph data available/supported!");
}

Glyph Font::GetGlyph(char32 unicode) const {
	u16 glyphIndex = GetGlyphIndex(unicode);
	return GetGlyphByIndex(glyphIndex);
}

GlyphInfo Font::GetGlyphInfoByIndex(u16 index) const {
	if (data.cffParsed.active) {
		return data.cffParsed.GetGlyphInfo(index);
	} else if (data.glyfParsed.active) {
		return data.glyfParsed.GetGlyphInfo(index);
	}
	throw std::runtime_error("No glyph data available/supported!");
}

GlyphInfo Font::GetGlyphInfo(char32 unicode) const {
	u16 glyphIndex = GetGlyphIndex(unicode);
	return GetGlyphInfoByIndex(glyphIndex);
}

void Font::PrintGlyph(char32 unicode) const {
	static u64 totalParseTime = 0;
	static u64 totalDrawTime = 0;
	static u32 iterations = 0;
	Nanoseconds glyphParseTime, glyphDrawTime;
	glyphDrawTime = Nanoseconds(0);
	ClockTime start = Clock::now();
	Glyph glyph;
	try {
		glyph = GetGlyph(unicode);
	} catch (std::runtime_error &err) {
		cout.PrintLn("Failed to get glyph: ", err.what());
		return;
	}
	glyphParseTime = Clock::now()-start;
	const f32 margin = 0.03f;
	const i32 scale = 4;
	i32 stepsX = 16 * scale;
	i32 stepsY = 16 * scale;
	const char distSymbolsPos[] = "X-.";
	const char distSymbolsNeg[] = "@*'";
	const f32 factorX = 1.0f / (f32)stepsX;
	const f32 factorY = 1.0f / (f32)stepsY;
	stepsY += i32(ceil(f32(stepsY) * margin * 2.0f));
	stepsX += i32(ceil(f32(stepsX) * margin * 2.0f));
	for (i32 y = stepsY-1; y >= 0; y--) {
		f32 prevDist = 1000.0f;
		for (i32 x = 0; x < stepsX; x++) {
			vec2 point((f32)x * factorX - margin, (f32)y * factorY - margin);
			// point *= max(glyph.size.x, glyph.size.y);
			if (point.x > glyph.info.size.x + margin) {
				break;
			}
			start = Clock::now();
			f32 dist = glyph.MinDistance(point, prevDist);
			glyphDrawTime += Clock::now()-start;
			prevDist = dist + factorX; // Assume the worst change possible
			if (glyph.Inside(point)) {
				if (dist < margin) {
					cout.Print(distSymbolsNeg[i32(dist/margin*3.0f)]);
				} else {
					cout.Print(' ');
				}
			} else {
				if (dist < margin) {
					cout.Print(distSymbolsPos[i32(dist/margin*3.0f)]);
				} else {
					cout.Print(' ');
				}
			}
		}
		cout.Print("\n");
	}
	cout.Print("\n");
	totalParseTime += glyphParseTime.count();
	totalDrawTime += glyphDrawTime.count();
	iterations++;
	// cout << "Glyph Parse Time: " << glyphParseTime.count() << "ns\n"
	//	 "Glyph Draw Time: " << glyphDrawTime.count() << "ns" << std::endl;
	if (iterations%64 == 0) {
		cout.PrintLn("After ", iterations, " iterations, average glyph parse time is ", totalParseTime/iterations, "ns and average glyph draw time is ", totalDrawTime/iterations, "ns.\nTotal glyph parse time is ", totalParseTime/1000000, "ms and total glyph draw time is ", totalDrawTime/1000000, "ms.");
	}
}

//
//	  Some helper functions for FontBuilder
//

bool Intersects(const Box &a, const Box &b) {
	return (a.min.x <= b.max.x
		 && a.max.x >= b.min.x
		 && a.min.y <= b.max.y
		 && a.max.y >= b.min.y);
}

bool Intersects(const Box &box, const vec2 &point) {
	const f32 epsilon = 0.001f;
	return (point.x == median(box.min.x-epsilon, point.x, box.max.x+epsilon)
		 && point.y == median(box.min.y-epsilon, point.y, box.max.y+epsilon));
}

void InsertCorner(Array<vec2> &array, vec2 toInsert) {
	// Make sure the corners are sorted by how far they are from the origin
	float dist = max(toInsert.x, toInsert.y);
	i32 insertPos = array.size;
	for (i32 i = 0; i < array.size; i++) {
		float dist2 = max(array[i].x, array[i].y);
		if (dist == dist2) {
			if (absSqr(toInsert) > absSqr(array[i]))
				continue;
		}
		if (dist <= dist2) {
			insertPos = i;
			break;
		}
	}
	array.Insert(insertPos, toInsert);
}

void PurgeCorners(Array<vec2> &corners, const Box &bounds) {
	for (i32 i = 0; i < corners.size; i++) {
		if (Intersects(bounds, corners[i])) {
			corners.Erase(i);
			i--;
		}
	}
}

const i32 boxListScale = 1;

bool BoxListXNode::Intersects(Box box) {
	for (i32 i = 0; i < boxes.size; i++) {
		if (font::Intersects(box, boxes[i])) {
			return true;
		}
	}
	return false;
}

void BoxListX::AddBox(Box box) {
	i32 minX = (i32)box.min.x / boxListScale;
	i32 maxX = (i32)box.max.x / boxListScale + 1;
	if (nodes.size < maxX) {
		nodes.Resize(maxX);
	}
	for (i32 x = minX; x < maxX; x++) {
		nodes[x].boxes.Append(box);
	}
}

bool BoxListX::Intersects(Box box) {
	i32 minX = (i32)box.min.x / boxListScale;
	i32 maxX = min((i32)box.max.x / boxListScale + 1, nodes.size);
	for (i32 x = minX; x < maxX; x++) {
		if (nodes[x].Intersects(box)) {
			return true;
		}
	}
	return false;
}

void BoxListY::AddBox(Box box) {
	i32 minY = (i32)box.min.y / boxListScale;
	i32 maxY = (i32)box.max.y / boxListScale + 1;
	if (lists.size < maxY) {
		lists.Resize(maxY);
	}
	for (i32 y = minY; y < maxY; y++) {
		lists[y].AddBox(box);
	}
}

bool BoxListY::Intersects(Box box) {
	i32 minY = (i32)box.min.y / boxListScale;
	i32 maxY = min((i32)box.max.y / boxListScale + 1, lists.size);
	for (i32 y = minY; y < maxY; y++) {
		if (lists[y].Intersects(box)) {
			return true;
		}
	}
	return false;
}

void FontBuilder::ResizeImage(i32 w, i32 h) {
	if (w == dimensions.x && h == dimensions.y) {
		return;
	}
	Array<u8> newPixels(w*h);
	for (i32 i = 0; i < newPixels.size; i+=8) {
		u64 *px = (u64*)(newPixels.data + i);
		*px = 0;
	}
	for (i32 y = 0; y < dimensions.y; y++) {
		for (i32 x = 0; x < dimensions.x; x++) {
			newPixels[y*w + x] = pixels[y*dimensions.x + x];
		}
	}
	dimensions = vec2i(w, h);
	pixels = std::move(newPixels);
}

bool FontBuilder::AddRange(char32 min, char32 max) {
	if (font == nullptr) {
		error = "You didn't give FontBuilder a Font*!";
		return false;
	}
	for (char32 c = min; c <= max; c++) { // OI, THAT'S THE LANGUAGE THIS IS WRITTEN IN!!!
		// Let the hype for such trivial things flow through you!
		u16 glyphIndex = font->GetGlyphIndex(c);
		if (!allIndices.Exists(glyphIndex)) {
			indicesToAdd += glyphIndex;
			allIndices.Emplace(glyphIndex);
		}
	}
	return true;
}

bool FontBuilder::AddString(WString string) {
	if (font == nullptr) {
		error = "You didn't give FontBuilder a Font*!";
		return false;
	}
	for (char32 c : string) {
		u16 glyphIndex = font->GetGlyphIndex(c);
		if (!allIndices.Exists(glyphIndex)) {
			indicesToAdd += glyphIndex;
			allIndices.Emplace(glyphIndex);
		}
	}
	return true;
}

void RenderThreadProc(FontBuilder *fontBuilder, Array<Glyph> *glyphsToAdd, const f32 boundSquare, const i32 numThreads, const i32 threadId) {
	vec2i &dimensions = fontBuilder->dimensions;
	for (i32 i = threadId; i < glyphsToAdd->size; i+=numThreads) {
		Glyph &glyph = (*glyphsToAdd)[i];
		if (glyph.info.size.x == 0.0f || glyph.info.size.y == 0.0f || glyph.components.size != 0) {
			continue;
		}
		/*
		cout.MutexLock();
		glyph.Print(cout);
		cout.MutexUnlock();
		*/
		const i32 texX = i32(glyph.info.pos.x*dimensions.x);
		const i32 texY = i32(glyph.info.pos.y*dimensions.y);
		const f32 offsetX = glyph.info.pos.x * (f32)dimensions.x - (f32)texX;
		const f32 offsetY = glyph.info.pos.y * (f32)dimensions.y - (f32)texY;
		const i32 texW = i32(glyph.info.size.x*dimensions.x);
		const i32 texH = i32(glyph.info.size.y*dimensions.y);

		const f32 factorX = boundSquare / (f32)dimensions.x;
		const f32 factorY = boundSquare / (f32)dimensions.y;

		// i32 lastPercentage = 0;

		for (i32 y = 0; y <= texH; y++) {
			// {
			//	 i32 percentage = i32(100.0f * (f32)y / (f32)texH);
			//	 if (percentage > lastPercentage) {
			//		 // cout.PrintLn(percentage, "%");
			//		 lastPercentage = percentage;
			//	 }
			// }
			f32 prevDist = sdfDistance;
			for (i32 x = 0; x <= texW; x++) {
				vec2 point = vec2(((f32)x - offsetX) * factorX - sdfDistance,
								  ((f32)y + offsetY) * factorY - sdfDistance);
				i32 xx = texX + x;
				if (xx >= dimensions.x || xx < 0) {
					break;
				}
				i32 yy = texY + texH - y;
				if (yy >= dimensions.y || yy < 0) {
					break;
				}
				u8& pixel = fontBuilder->pixels[dimensions.x * yy + xx];
				// if (lastPercentage == 23 && x == 42)
				//	 cout.PrintLn("TEST HERE");
				f32 dist = glyph.MinDistance(point, prevDist);
				// if (lastPercentage == 23 && x == 42) cout.PrintLn("\tMinDistance Passed");
				prevDist = dist + factorX; // Assume the worst change possible
				if (glyph.Inside(point)) {
					if (dist < sdfDistance) {
						dist = (1.0f + dist / sdfDistance) * 0.5f;
					} else {
						dist = 1.0f;
					}
				} else {
					if (dist < sdfDistance) {
						dist = (1.0f - dist / sdfDistance) * 0.5f;
					} else {
						dist = 0.0f;
					}
				}
				pixel = u8(dist*255.0f);
				// if (lastPercentage == 23 && x == 42) cout.PrintLn("\tInside passed");
			}
		}
	}
}

bool FontBuilder::Build() {
	if (font == nullptr) {
		error = "You didn't give FontBuilder a Font*!";
		return false;
	}
	if (indicesToAdd.size == 0) {
		// Nothing to do.
		return true;
	}
	if (renderThreadCount < 1) {
		renderThreadCount = Thread::HardwareConcurrency();
		// renderThreadCount = 1;
		if (renderThreadCount == 0) {
			// Couldn't get actual concurrency so this is just a guess.
			// More than actual concurrency isn't really a bad thing since it's lockless, unless the kernel am dum.
			renderThreadCount = 8;
			cout.PrintLn("Using default concurrency: ", renderThreadCount);
		} else {
			cout.PrintLn("Using concurrency: ", renderThreadCount);
		}
	}
	ClockTime start = Clock::now();
	Array<Glyph> glyphsToAdd;
	glyphsToAdd.Reserve(indicesToAdd.size);
	for (i32 i = 0; i < indicesToAdd.size; i++) {
		Glyph glyph = font->GetGlyphByIndex(indicesToAdd[i]);
		if (glyph.components.size != 0) {
			for (Component& component : glyph.components) {
				if (!allIndices.Exists(component.glyphIndex)) {
					indicesToAdd += component.glyphIndex;
					allIndices.Emplace(component.glyphIndex);
				}
			}
			glyph.info.size = vec2(0.0f);
			glyph.curves.Clear();
			glyph.lines.Clear();
		}
		glyphsToAdd.Append(std::move(glyph));
	}
	for (i32 i = 0; i < indicesToAdd.size; i++) {
		indexToId.Set(indicesToAdd[i], glyphs.size + i);
	}
	indicesToAdd.Clear();
	cout.PrintLn("Took ", FormatTime(Nanoseconds(Clock::now()-start)), " to parse glyphs.");
	cout.PrintLn("Packing ", glyphsToAdd.size, " glyphs...");
	struct SizeIndex {
		i32 index;
		vec2 size;
	};
	start = Clock::now();
	Array<SizeIndex> sortedIndices;
	sortedIndices.Reserve(glyphsToAdd.size/2);
	for (i32 i = glyphsToAdd.size-1; i >= 0; i--) {
		if (glyphsToAdd[i].info.size.x == 0.0f || glyphsToAdd[i].info.size.y == 0.0f) {
			continue;
		}
		SizeIndex sizeIndex = {i, glyphsToAdd[i].info.size};
		i32 insertPos = 0;
		for (i32 ii = 0; ii < sortedIndices.size; ii++) {
			insertPos = ii;
			if (sortedIndices[ii].size.x == sizeIndex.size.x) {
				if (sortedIndices[ii].size.y >= sizeIndex.size.y)
					continue;
			}
			if (sortedIndices[ii].size.x <= sizeIndex.size.x) {
				break;
			}
		}
		sortedIndices.Insert(insertPos, sizeIndex);
	}
	cout.PrintLn("Took ", FormatTime(Nanoseconds(Clock::now()-start)), " to sort by size.");
	if (corners.size == 0) {
		corners.Append(vec2(0.0f));
		bounding = vec2(0.0f);
		boundSquare = 0.0f;
		area = 0.0f;
	}
	start = Clock::now();
	for (SizeIndex& si : sortedIndices) {
		Glyph &glyph = glyphsToAdd[si.index];
		Box box;
		for (i32 i = 0; i < corners.size; i++) {
			box.min = corners[i];
			box.max = box.min + glyph.info.size + vec2(sdfDistance*2.0f);
			if (boxes.Intersects(box)) {
				// Go to the next corner
				continue;
			}
			glyph.info.pos = corners[i];
			area += (box.max.x-box.min.x) * (box.max.y-box.min.y);
			boxes.AddBox(box);
			PurgeCorners(corners, box);
			if (box.max.x > bounding.x) {
				bounding.x = box.max.x;
			}
			if (box.max.y > bounding.y) {
				bounding.y = box.max.y;
			}
			box.max += vec2(0.002f);
			InsertCorner(corners, vec2(box.max.x, box.min.y));
			InsertCorner(corners, vec2(box.min.x, box.max.y));
			if (box.min.x != corners[i].x) {
				box.min.x = corners[i].x;
				InsertCorner(corners, vec2(box.min.x, box.max.y));
			}
			if (box.min.y != corners[i].y) {
				box.min.y = corners[i].y;
				InsertCorner(corners, vec2(box.max.x, box.min.y));
			}
			break;
		}
	}
	Nanoseconds packingTime = Clock::now()-start;
	cout.PrintLn("Took ", FormatTime(packingTime), " to pack glyphs.");
	f32 totalArea = bounding.x * bounding.y;
	cout.PrintLn("Of a total page area of ", totalArea, ", ", u32(area/totalArea*100.0f), "% was used.");
	bounding.x = max(bounding.x, 1.0f);
	bounding.y = max(bounding.y, 1.0f);
	f32 oldBoundSquare = boundSquare;
	// if (boundSquare == 0.0) {
		boundSquare = i32(ceil(max(bounding.x, bounding.y))*64.0f)/64;
	// } else {
	//	 if (max(bounding.x, bounding.y) > boundSquare)
	//		 boundSquare *= 2.0;
	// }
	scale = boundSquare;
	edge = sdfDistance*32.0f;

	vec2i dimensionsNew = vec2i(i32(boundSquare)*resolution);
	cout.PrintLn("Texture dimensions = {", dimensionsNew.x, ", ", dimensionsNew.y, "}");
	ResizeImage(dimensionsNew.x, dimensionsNew.y);
	for (i32 i = 0; i < glyphs.size; i++) {
		Glyph& glyph = glyphs[i];
		glyph.info.pos /= boundSquare/oldBoundSquare;
		glyph.info.size /= boundSquare/oldBoundSquare;
		glyph.info.offset /= boundSquare/oldBoundSquare;
	}
	for (i32 i = 0; i < glyphsToAdd.size; i++) {
		Glyph& glyph = glyphsToAdd[i];
		glyph.info.size += sdfDistance*2.0f;
		glyph.info.offset += sdfDistance;
		glyph.info.pos /= boundSquare;
		glyph.info.size /= boundSquare;
		glyph.info.offset /= boundSquare;
	}
	start = Clock::now();
	// Now do the rendering
	Array<Thread> threads(renderThreadCount);
	for (i32 i = 0; i < renderThreadCount; i++) {
		threads[i] = Thread(RenderThreadProc, this, &glyphsToAdd, boundSquare, renderThreadCount, i);
	}
	for (i32 i = 0; i < renderThreadCount; i++) {
		if (threads[i].Joinable()) {
			threads[i].Join();
		}
	}
	Nanoseconds renderingTime = Clock::now() - start;
	cout.PrintLn("Rendering took ", FormatTime(renderingTime));
	glyphs.Append(std::move(glyphsToAdd));
	return true;
}

} // namespace font

} // namespace AzCore
