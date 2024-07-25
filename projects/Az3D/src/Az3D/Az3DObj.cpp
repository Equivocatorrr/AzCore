/*
	File: Az3DObj.cpp
	Author: Philip Haynes
*/

#include "Az3DObj.hpp"
#include "AzCore/IO/Log.hpp"
#include "AzCore/Memory/Endian.hpp"
#include "AzCore/Memory/Util.hpp"

// NOTE: File is little endian, and current target platform is little endian.
// If this ever changes in the future we can do endian stuff. For now we do the easy thing.

namespace Az3D::Az3DObj {

using namespace AzCore;

constexpr Str AZ3D_MAGIC = Str("Az3DObj\0", 8);

constexpr u16 VERSION_MAJOR = 1;
constexpr u16 VERSION_MINOR = 1;

f32 cubic(f32 p1, f32 c1, f32 c2, f32 p2, f32 t) {
	return p1 + 3.0f * t * (c1 - p1) + 3.0f * t*t * (c2 + p1 - 2.0f * c1) + t*t*t * (p2 - p1 + 3.0f * (c1 - c2));
}

f32 cubicDerivative(f32 p1, f32 c1, f32 c2, f32 p2, f32 t) {
	return 3.0f * (c1 - p1 + 2.0f * t * (p1 + c2 - 2.0f * c1) + t*t *(p2 - p1 + 3.0f * (c1 - c2)));
}

f32 cubicBezier(vec2 p1, vec2 c1, vec2 c2, vec2 p2, f32 x) {
	AzAssert(x >= p1.x && x <= p2.x, Stringify("cubicBezier x (", x, ") is out of bounds (", p1.x, " to ", p2.x, ')'));
	// Make it impossible for the bezier to have multiple solutions by clamping the control points within the bounds of the endpoints.
	c1.x = median(c1.x, p1.x, p2.x);
	c2.x = median(c2.x, p1.x, p2.x);
	// Best initial guess is where we'd be if we were a line.
	f32 t = (x - p1.x) / (p2.x - p1.x);
	// Do some newton iterations to get closer. 6 Is maybe a little excessive; should be very precise.
	for (i32 i = 0; i < 6; i++) {
		f32 d = cubicDerivative(p1.x, c1.x, c2.x, p2.x, t);
		if (abs(d) < 0.0001f) break; // Special case, probably would never happen but if we don't handle it we'd get NaNs
		t -= (cubic(p1.x, c1.x, c2.x, p2.x, t) - x) / d;
	}
	return cubic(p1.y, c1.y, c2.y, p2.y, t);
}

f32 cube(f32 factor) {
	return factor * factor * factor;
}

f32 tesseract(f32 factor) {
	return square(square(factor));
}

f32 penteract(f32 factor) {
	return tesseract(factor) * factor;
}

f32 expInterpFactor(f32 factor) {
	return (pow(2.0f, 10.0f * factor - 10.0f)-0.001f) / 0.999f;
}

f32 circInterpFactor(f32 factor) {
	return 1.0f - sqrt(1.0f - square(factor));
}

f32 backInterpFactor(f32 back, f32 t) {
	return cube(t) * (back + 1.0f) - square(t) * back;
}

f32 bounceInterpFactor(f32 t) {
	constexpr f32 n1 = 7.5625f;
	constexpr f32 d1 = 2.75f;

	if (t < 1.0f / d1) {
		return n1 * square(t);
	} else if (t < 2.0f / d1) {
		return n1 * square(t - 1.5f / d1) + 0.75f;
	} else if (t < 2.5f / d1) {
		return n1 * square(t - 2.25f / d1) + 0.9375f;
	} else {
		return n1 * square(t - 2.625f / d1) + 0.984375f;
	}
}

f32 elasticEaseIn(f32 start, f32 end, f32 time, f32 duration, f32 amp, f32 period) {
	f32 scale = 1.0f;
	if (period == 0.0f) {
		period = duration * 0.3f;
	}
	time -= duration;
	f32 t = time / duration;
	f32 phase;
	f32 delta = end - start;
	f32 deltaAbs = abs(delta);
	if (amp == 0.0f || amp < deltaAbs) {
		phase = period / 4.0f;
		if (delta != 0.0f) {
			scale *= amp / deltaAbs;
		} else {
			scale = 0.0f;
		}
		f32 tAbs = abs(t);
		f32 phaseAbs = abs(phase);
		if (tAbs < phaseAbs) {
			scale = lerp(1.0f, scale, tAbs / phaseAbs);
		}
		amp = delta;
	} else {
		phase = period / tau * asin(delta / amp);
	}

	return start - scale * amp * pow(2.0f, 10.0f * time) * sin((t - phase) * tau / period);
}

f32 elasticEaseOut(f32 start, f32 end, f32 time, f32 duration, f32 amp, f32 period) {
	f32 delta = end - start;
	time = duration - time;
	return -delta - elasticEaseIn(start, end, time, duration, amp, period);
}

f32 elasticEaseInOut(f32 start, f32 end, f32 time, f32 duration, f32 amp, f32 period) {
	f32 t = time / duration;
	f32 delta = end - start;
	time = duration - time;
	if (t < 0.5f) {
		return elasticEaseIn(start, end - delta * 0.5f, t * duration, duration * 0.5f, amp * 0.5f, period);
	} else {
		return elasticEaseOut(start + delta * 0.5f, end, (t - 0.5f) * duration, duration * 0.5f, amp * 0.5f, period);
	}
}

f32 Action::Curve::Evaluate(f32 time) const {
	// static bool once = true;
	// if (once) {
	// 	once = false;
	// 	io::Log out("data.csv", false, true);
	// 	for (f32 t = 0; t <= 2.0f; t += 0.01f) {
	// 		out.PrintLn(elasticEaseInOut(5.0f, 0.0f, t, 2.0f, 6.0f, 10.0f / 60.0f));
	// 	}
	// }
	AzAssert(keyframes.size > 0, "Curve has no keyframes!");
	if (keyframes.size == 1) {
		return keyframes[0].point.value;
	}
	time = wrap(time, keyframes.Back().point.time);
	i32 i = 0;
	for (; i < keyframes.size-1; i++) {
		if (keyframes[i+1].point.time > time) break;
	}
	const KeyFrame &kf = keyframes[i];
	KeyFrame::Point p2 = keyframes[i+1].point;
	f32 tweenTime = time - kf.point.time;
	f32 duration = p2.time - kf.point.time;
	f32 factor = tweenTime / duration;
	switch (kf.interpolation) {
		case KeyFrame::CONSTANT: {
			return kf.point.value;
		} break;
		case KeyFrame::LINEAR: {
			return lerp(kf.point.value, p2.value, factor);
		} break;
		case KeyFrame::BEZIER: {
			return cubicBezier(kf.point.vector, kf.bezier.control[0].vector, kf.bezier.control[1].vector, p2.vector, time);
		} break;
		case KeyFrame::SINE: {
			switch (kf.easing) {
				case KeyFrame::EASE_IN: {
					return lerp(kf.point.value, p2.value, 1.0f - cos(factor * halfpi));
				} break;
				case KeyFrame::EASE_OUT: {
					return lerp(kf.point.value, p2.value, sin(factor * halfpi));
				} break;
				case KeyFrame::EASE_IN_OUT: {
					return cosInterp(kf.point.value, p2.value, factor);
				} break;
				default: AzAssert(false, "Invalid sine easing in keyframe");
			}
		} break;
		case KeyFrame::QUADRATIC: {
			switch (kf.easing) {
				case KeyFrame::EASE_IN: {
					return lerp(kf.point.value, p2.value, square(factor));
				} break;
				case KeyFrame::EASE_OUT: {
					return lerp(kf.point.value, p2.value, 1.0f - square(1.0f - factor));
				} break;
				case KeyFrame::EASE_IN_OUT: {
					return lerp(kf.point.value, p2.value, factor <= 0.5f ? 2.0f * square(factor) : 1.0f - 0.5f * square(1.0f - 2.0f*(factor-0.5f)));
				} break;
				default: AzAssert(false, "Invalid quadratic easing in keyframe");
			}
		} break;
		case KeyFrame::CUBIC: {
			switch (kf.easing) {
				case KeyFrame::EASE_IN: {
					return lerp(kf.point.value, p2.value, cube(factor));
				} break;
				case KeyFrame::EASE_OUT: {
					return lerp(kf.point.value, p2.value, 1.0f - cube(1.0f - factor));
				} break;
				case KeyFrame::EASE_IN_OUT: {
					return lerp(kf.point.value, p2.value, factor <= 0.5f ? 4.0f * cube(factor) : 1.0f - 0.5f * cube(1.0f - 2.0f*(factor-0.5f)));
				} break;
				default: AzAssert(false, "Invalid quadratic easing in keyframe");
			}
		} break;
		case KeyFrame::QUARTIC: {
			switch (kf.easing) {
				case KeyFrame::EASE_IN: {
					return lerp(kf.point.value, p2.value, tesseract(factor));
				} break;
				case KeyFrame::EASE_OUT: {
					return lerp(kf.point.value, p2.value, 1.0f - tesseract(1.0f - factor));
				} break;
				case KeyFrame::EASE_IN_OUT: {
					return lerp(kf.point.value, p2.value, factor <= 0.5f ? 8.0f * tesseract(factor) : 1.0f - 0.5f * tesseract(1.0f - 2.0f*(factor-0.5f)));
				} break;
				default: AzAssert(false, "Invalid quartic easing in keyframe");
			}
		} break;
		case KeyFrame::QUINTIC: {
			switch (kf.easing) {
				case KeyFrame::EASE_IN: {
					return lerp(kf.point.value, p2.value, penteract(factor));
				} break;
				case KeyFrame::EASE_OUT: {
					return lerp(kf.point.value, p2.value, 1.0f - penteract(1.0f - factor));
				} break;
				case KeyFrame::EASE_IN_OUT: {
					return lerp(kf.point.value, p2.value, factor <= 0.5f ? 16.0f * penteract(factor) : 1.0f - 0.5f * penteract(1.0f - 2.0f*(factor-0.5f)));
				} break;
				default: AzAssert(false, "Invalid quintic easing in keyframe");
			}
		} break;
		case KeyFrame::EXPONENTIAL: {
			switch (kf.easing) {
				case KeyFrame::EASE_IN: {
					return lerp(kf.point.value, p2.value, expInterpFactor(factor));
				} break;
				case KeyFrame::EASE_OUT: {
					return lerp(kf.point.value, p2.value, 1.0f - expInterpFactor(1.0f - factor));
				} break;
				case KeyFrame::EASE_IN_OUT: {
					return lerp(kf.point.value, p2.value, factor <= 0.5f ? 0.5f * expInterpFactor(2.0f * factor) : 1.0f - 0.5f * expInterpFactor(2.0f * (1.0f - factor)));
				} break;
				default: AzAssert(false, "Invalid exponential easing in keyframe");
			}
		} break;
		case KeyFrame::CIRCULAR: {
			switch (kf.easing) {
				case KeyFrame::EASE_IN: {
					return lerp(kf.point.value, p2.value, circInterpFactor(factor));
				} break;
				case KeyFrame::EASE_OUT: {
					return lerp(kf.point.value, p2.value, 1.0f - circInterpFactor(1.0f - factor));
				} break;
				case KeyFrame::EASE_IN_OUT: {
					return lerp(kf.point.value, p2.value, factor <= 0.5f ? 0.5f * circInterpFactor(2.0f * factor) : 1.0f - 0.5f * circInterpFactor(2.0f * (1.0f - factor)));
				} break;
				default: AzAssert(false, "Invalid circular easing in keyframe");
			}
		} break;
		case KeyFrame::BACK: {
			switch (kf.easing) {
				case KeyFrame::EASE_IN: {
					return lerpUnclamped(kf.point.value, p2.value, backInterpFactor(kf.back.factor, factor));
				} break;
				case KeyFrame::EASE_OUT: {
					return lerpUnclamped(kf.point.value, p2.value, 1.0f - backInterpFactor(kf.back.factor, 1.0f - factor));
				} break;
				case KeyFrame::EASE_IN_OUT: {
					return lerpUnclamped(kf.point.value, p2.value, factor <= 0.5f ? 0.5f * backInterpFactor(kf.back.factor, 2.0f * factor) : 1.0f - 0.5f * backInterpFactor(kf.back.factor, 2.0f * (1.0f - factor)));
				} break;
				default: AzAssert(false, "Invalid back easing in keyframe");
			}
		} break;
		case KeyFrame::BOUNCE: {
			switch (kf.easing) {
				case KeyFrame::EASE_IN: {
					return lerp(kf.point.value, p2.value, 1.0f - bounceInterpFactor(1.0f - factor));
				} break;
				case KeyFrame::EASE_OUT: {
					return lerp(kf.point.value, p2.value, bounceInterpFactor(factor));
				} break;
				case KeyFrame::EASE_IN_OUT: {
					return lerp(kf.point.value, p2.value, factor <= 0.5f ? 0.5f * bounceInterpFactor(2.0f * (1.0f - factor)) : 1.0f - 0.5f * bounceInterpFactor(2.0f * factor));
				} break;
				default: AzAssert(false, "Invalid bounce easing in keyframe");
			}
		} break;
		case KeyFrame::ELASTIC: {
			switch (kf.easing) {
				case KeyFrame::EASE_IN: {
					return elasticEaseIn(kf.point.value, p2.value, tweenTime, duration, kf.elastic.amp, kf.elastic.period);
				} break;
				case KeyFrame::EASE_OUT: {
					return elasticEaseOut(kf.point.value, p2.value, tweenTime, duration, kf.elastic.amp, kf.elastic.period);
				} break;
				case KeyFrame::EASE_IN_OUT: {
					return elasticEaseInOut(kf.point.value, p2.value, tweenTime, duration, kf.elastic.amp, kf.elastic.period);
				} break;
				default: AzAssert(false, "Invalid elastic easing in keyframe");
			}
		} break;
		default: AzAssert(false, "Invalid interpolation in keyframe");
	}
	return 0.0f;
}

// 1st argument must be a "string literal", __VA_ARGS__ breaks somewhere when I add the comma.
#define PRINT_ERROR(...) io::cerr.PrintLn(__FUNCTION__, "[", cur, "] error: " __VA_ARGS__)

#define EXPECT_SPACE_IN_BUFFER(length) if (buffer.size-cur < (length)) {\
	PRINT_ERROR("Buffer underflow!");\
	return false;\
}

#define EXPECT_TAG_IN_BUFFER(tagName, len) if (strncmp(&buffer[cur], (tagName), (len)) != 0) {\
	PRINT_ERROR("Incorrect tag \"", Str(&buffer[cur], (len)), "\" (expected \"", (tagName), "\")");\
	return false;\
}

#define COPY_FROM_BUFFER(dst, size)\
	EXPECT_SPACE_IN_BUFFER(i64(size));\
	memcpy(&(dst), &buffer[cur], (size));\
	cur += (size);

namespace Headers {
	// Header that must exist at the start of the file
	struct File {
		static constexpr i64 SIZE = 12;
		char magic[8];
		u16 versionMajor;
		u16 versionMinor;
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER(AZ3D_MAGIC.str, 8);
			memcpy(this, &buffer[cur], SIZE);
			cur += SIZE;
			return true;
		}
	};

	// Header that defines a section of data with a length
	struct Table {
		static constexpr i64 SIZE = 8;
		// ident denotes what kind of data this section contains
		char ident[4];
		// section length includes this header
		u32 length;
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			memcpy(this, &buffer[cur], SIZE);
			cur += SIZE;
			return true;
		}
	};

} // namespace Headers

namespace Types {

	// Encodes a string of text in the file
	struct Name {
		static constexpr i64 SIZE = 8;
		char ident[4];
		u32 length;
		// char name[length];
		Str name;
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER("Name", 4);
			memcpy(ident, &buffer[cur], SIZE);
			cur += SIZE;
			EXPECT_SPACE_IN_BUFFER(length);
			name = Str(&buffer[cur], length);
			cur += align(length, 4);
			return true;
		}
	};

	struct Vert {
		static constexpr i64 SIZE = 12;
		char ident[4];
		u32 count;
		u16 stride;
		// componentCount determines how many components are in the format string
		u16 componentCount;
		// format describes each component's purpose, type, and size in 4 bytes each
		// u8 format[componentCount*4];
		Str format;
		// u8 vertexBuffer[count*stride];
		Array<Vertex> vertices;
		struct SrcScalar {
			enum class Kind {
				F32,
				S16,
				S8,
			} kind;
			enum class DstKind {
				F32,
				U8,
			} dstKind;
			// Stride of a single scalar in bytes
			i32 stride;
			// All values are given by: src*dimension + offset
			f32 dimension;
			f32 offset;
			// Offset into the Vertex struct in bytes.
			i32 dstOffset;
			static Str KindString(Kind kind) {
				switch (kind) {
					case Kind::S8:
						return Str("Byte");
					case Kind::S16:
						return Str("Short");
					case Kind::F32:
						return Str("Float");
					default:
						return Str("Undefined");
				}
			}
		};
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER("Vert", 4);
			memcpy(ident, &buffer[cur], SIZE);
			cur += SIZE;
			Array<SrcScalar> srcScalars;
			if (!ParseFormat(buffer, cur, componentCount, stride, srcScalars)) return false;
			i64 length = count * stride;
			EXPECT_SPACE_IN_BUFFER(length);
			vertices.Resize(count);
			for (i64 i = 0; i < (i64)count; i++) {
				vertices[i] = GetVertex(&buffer[cur], srcScalars);
				cur += stride;
			}
			return true;
		}
		// Fills an array of offsets into our Vertex struct for each 4-byte chunk of data
		// The index into this array increases by one for every component in the input buffer
		bool ParseFormat(Str buffer, i64 &cur, i32 numComponents, u32 stride, Array<SrcScalar> &dst) {
			i32 totalSize = 0;
			for (i32 i = 0; i < numComponents; i++) {
				EXPECT_SPACE_IN_BUFFER(4);
				Str tag = buffer.SubRange(cur, 2);
				char type = buffer[cur + 2];
				u8 count = buffer[cur + 3];
				cur += 4;
				ArrayWithBucket<f32, 4> dimension, srcOffset;
				bool hasDimensionAndSrcOffset;
				SrcScalar::Kind kind;
				i32 scalarStride;
				switch (type) {
					case 'F':
						kind = SrcScalar::Kind::F32;
						dimension = ArrayWithBucket<f32, 4>((i32)count, 1.0f);
						srcOffset = ArrayWithBucket<f32, 4>((i32)count, 0.0f);
						hasDimensionAndSrcOffset = false;
						scalarStride = 4;
						break;
					case 'S':
						kind = SrcScalar::Kind::S16;
						hasDimensionAndSrcOffset = true;
						scalarStride = 2;
						break;
					case 'B':
						kind = SrcScalar::Kind::S8;
						hasDimensionAndSrcOffset = true;
						scalarStride = 1;
						break;
					default:
						io::cerr.PrintLn("Vert Component ", i, " with tag \"", tag, "\" has an invalid scalar type: '", type, "'");
						return false;
				}
				totalSize += scalarStride * count;
				if (hasDimensionAndSrcOffset) {
					EXPECT_SPACE_IN_BUFFER(8*count);
					for (i32 j = 0; j < count; j++) {
						dimension.Append(*(f32*)&buffer[cur]);
						cur += 4;
						srcOffset.Append(*(f32*)&buffer[cur]);
						cur += 4;
					}
				}
				SrcScalar::DstKind dstKind = SrcScalar::DstKind::F32;
				i32 dstOffset;
				i32 dstStride = 4;
				i32 maxInDst;
				if (tag == "Po") {
					dstOffset = offsetof(Vertex, pos);
					maxInDst = 3;
				} else if (tag == "No") {
					dstOffset = offsetof(Vertex, normal);
					maxInDst = 3;
				} else if (tag == "Ta") {
					dstOffset = offsetof(Vertex, tangent);
					maxInDst = 3;
				} else if (tag == "UV") {
					dstOffset = offsetof(Vertex, tex);
					maxInDst = 2;
				} else if (tag == "BI") {
					dstOffset = offsetof(Vertex, boneIDs);
					maxInDst = 4;
					dstKind = SrcScalar::DstKind::U8;
					dstStride = 1;
				} else if (tag == "BW") {
					dstOffset = offsetof(Vertex, boneWeights);
					maxInDst = 4;
					dstKind = SrcScalar::DstKind::U8;
					dstStride = 1;
				} else {
					io::cout.PrintLn("Warning: Vert Format string has tag \"", tag, "\" of ", count, " ", SrcScalar::KindString(kind), count == 1 ? " which is": "s which are", " unused.");
					maxInDst = 0;
				}
				for (i32 j = 0; j < count; j++) {
					SrcScalar scalar;
					scalar.kind = kind;
					scalar.dstKind = dstKind;
					scalar.stride = scalarStride;
					scalar.dimension = dimension[j];
					scalar.offset = srcOffset[j];
					scalar.dstOffset = j < maxInDst ? dstOffset + j * dstStride : -1;
					dst.Append(scalar);
				}
			}
			if (totalSize != (i32)stride) {
				io::cerr.PrintLn("Vert Format string describes a Vertex with a stride of ", totalSize, " when it was supposed to have a stride of ", stride);
				return false;
			}
			return true;
		}

		Vertex GetVertex(char *buffer, Array<SrcScalar> srcScalars) {
			Vertex result;
			// Default values
			result.pos = vec3(0.0f);
			result.normal = vec3(0.0f, 0.0f, 1.0f);
			result.tangent = vec3(1.0f, 0.0f, 0.0f);
			result.tex = vec2(0.0f);
			result.boneIDs = 0xffffffff;
			result.boneWeights = 0;
			u8 *dst = (u8*)&result;
			u8 *src = (u8*)buffer;
			for (SrcScalar srcScalar : srcScalars) {
				AzAssert(srcScalar.dstOffset < i64(sizeof(Vertex)), "dstOffset is out of bounds");
				if (srcScalar.dstOffset >= 0) {
					switch (srcScalar.dstKind) {
						case SrcScalar::DstKind::F32:
							switch (srcScalar.kind) {
								case SrcScalar::Kind::F32:
									(*(f32*)&(dst[srcScalar.dstOffset])) = *(f32*)src * srcScalar.dimension + srcScalar.offset;
									break;
								case SrcScalar::Kind::S16:
									(*(f32*)&(dst[srcScalar.dstOffset])) = f32(*(i16*)src) * srcScalar.dimension / 32767.0f + srcScalar.offset;
									break;
								case SrcScalar::Kind::S8:
									(*(f32*)&(dst[srcScalar.dstOffset])) = f32(*(i8*)src) * srcScalar.dimension / 127.0f + srcScalar.offset;
									break;
								default: AzAssert(false, "Unreachable"); break;
							}
							break;
						case SrcScalar::DstKind::U8:
							switch (srcScalar.kind) {
								case SrcScalar::Kind::S8:
									dst[srcScalar.dstOffset] = u8(round(f32(*(i8*)src) * srcScalar.dimension / 127.0f + srcScalar.offset));
									break;
								default: AzAssert(false, "Unreachable"); break;
							}
							break;
						default: AzAssert(false, "Unreachable"); break;
					}
				}
				src += srcScalar.stride;
			}
			// This is mostly to fix our default value not relating to anything in particular, but if for whatever reason the file's tangent isn't orthogonal to the normal, we still ensure that.
			result.tangent = orthogonalize(result.tangent, result.normal);
			return result;
		}
	};

	struct Indx {
		static constexpr i64 SIZE = 8;
		char ident[4];
		u32 count;
		// u8 indices[count]; when count < 256
		// u16 indices[count]; when count < 65536
		// u32 indices[count]; otherwise
		// implicit from count
		u32 stride;
		Array<u32> indices;
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER("Indx", 4);
			memcpy(ident, &buffer[cur], SIZE);
			cur += SIZE;
			if (count < 0x100) {
				stride = 1;
			} else if (count < 0x10000) {
				stride = 2;
			} else {
				stride = 4;
			}
			i64 length = count * stride;
			EXPECT_SPACE_IN_BUFFER(length);
			indices.Resize(count);
			switch (stride) {
			case 1:
				for (i64 i = 0; i < count; i++) {
					indices[i] = *(u8*)&buffer[cur + i*stride];
				} break;
			case 2:
				for (i64 i = 0; i < count; i++) {
					indices[i] = *(u16*)&buffer[cur + i*stride];
				} break;
			case 4:
				memcpy(indices.data, &buffer[cur], length);
				break;
			}
			cur += align(length, 4);
			return true;
		}
	};

	// Describes a material
	struct Mat0 {
		static constexpr i64 SIZE = 8;
		char ident[4];
		u32 length;
		vec4 albedoColor = vec4(1.0f);
		vec3 emissionColor = vec3(0.0f);
		static_assert(sizeof(az::vec4) == 4*4);
		static_assert(sizeof(az::vec3) == 4*3);
		f32 normalDepth = 1.0f;
		f32 metalnessFactor = 0.0f;
		f32 roughnessFactor = 0.5f;
		// SubSurface Scattering
		f32 sssFactor = 0.0f;
		// Tints the entire subsurf lighting component evenly
		vec3 sssColor = vec3(1.0f);
		// sssRadius has one radius per primary color
		// Tints the subsurf lighting component depending on distance
		vec3 sssRadius = vec3(0.1f);
		// Texture indices are valid within the file
		// 0 indicates no texture
		u32 albedoIndex = 0;
		u32 emissionIndex = 0;
		u32 normalIndex = 0;
		u32 metalnessIndex = 0;
		u32 roughnessIndex = 0;
		// Texture that describes subsurface color. Probably won't really be used since you could just have separate materials for subsurf vs not, even in the same Mesh.
		u32 sssIndex = 0;
		bool isFoliage = false;
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER("Mat0", 4);
			memcpy(this, &buffer[cur], SIZE);
			cur += SIZE;
			EXPECT_SPACE_IN_BUFFER(length);
			i64 endCur = cur + length;
			while (cur < endCur) {
				Str tag = Str(&buffer[cur], 4);
				cur += 4;
				u32 count = (u32)tag[3];
				EXPECT_SPACE_IN_BUFFER(4*count);
				if (tag == "ACF\004") {
					albedoColor = *(vec4*)&buffer[cur];
				} else if (tag == "ECF\003") {
					emissionColor = *(vec3*)&buffer[cur];
				} else if (tag == "NDF\001") {
					normalDepth = *(f32*)&buffer[cur];
				} else if (tag == "MFF\001") {
					metalnessFactor = *(f32*)&buffer[cur];
				} else if (tag == "RFF\001") {
					roughnessFactor = *(f32*)&buffer[cur];
				} else if (tag == "SFF\001") {
					sssFactor = *(f32*)&buffer[cur];
				} else if (tag == "SCF\003") {
					sssColor = *(vec3*)&buffer[cur];
				} else if (tag == "SRF\003") {
					sssRadius = *(vec3*)&buffer[cur];
				} else if (tag == "ATI\001") {
					albedoIndex = *(u32*)&buffer[cur];
				} else if (tag == "ETI\001") {
					emissionIndex = *(u32*)&buffer[cur];
				} else if (tag == "NTI\001") {
					normalIndex = *(u32*)&buffer[cur];
				} else if (tag == "MTI\001") {
					metalnessIndex = *(u32*)&buffer[cur];
				} else if (tag == "RTI\001") {
					roughnessIndex = *(u32*)&buffer[cur];
				} else if (tag == "STI\001") {
					sssIndex = *(u32*)&buffer[cur];
				} else if (tag == "Fol\0") {
					isFoliage = true;
				}
				cur += 4*count;
			}
			if (cur != endCur) {
				io::cerr.PrintLn("Mat0 data is misaligned somehow (cur is ", cur, " but expected ", endCur, ")");
				return false;
			}
			return true;
		}
	};

	// Contains an entire image file
	struct ImageData {
		Name filename;
		u32 length;
		u32 isLinear;
		// char data[length];
		Str data;
		bool FromBuffer(Str buffer, i64 &cur) {
			if (!filename.FromBuffer(buffer, cur)) return false;
			EXPECT_SPACE_IN_BUFFER(8);
			memcpy(&length, &buffer[cur], 8);
			cur += 8;
			EXPECT_SPACE_IN_BUFFER(length);
			data = Str(&buffer[cur], length);
			cur += align(length, 4);
			return true;
		}
	};

} // namespace Types

namespace Tables {

	struct Mesh {
		Headers::Table header;
		Types::Name name;
		Types::Vert vert;
		Types::Indx indx;
		Types::Mat0 mat0;
		Types::Name armatureName;
		// Expects header to already be set and cur to be immediately after
		bool FromBuffer(Str buffer, i64 &cur) {
			if (strncmp(header.ident, "Mesh", 4) != 0) {
				PRINT_ERROR("Parsing a \"", Str(header.ident, 4), "\" as though it's a \"Mesh\"");
				return false;
			}
			EXPECT_SPACE_IN_BUFFER(header.length - Headers::Table::SIZE);
			i64 endCur = cur + header.length - Headers::Table::SIZE;
			bool hasName = false;
			bool hasVert = false;
			bool hasIndx = false;
			bool hasMat0 = false;
			bool skippedToEnd = false;
			while (cur < endCur) {
				Str tag = Str(&buffer[cur], 4);
				if (tag == "Name") {
					if (!name.FromBuffer(buffer, cur)) return false;
					hasName = true;
				} else if (tag == "Vert") {
					if (!vert.FromBuffer(buffer, cur)) return false;
					hasVert = true;
				} else if (tag == "Indx") {
					if (!indx.FromBuffer(buffer, cur)) return false;
					hasIndx = true;
				} else if (tag == "Mat0") {
					if (!mat0.FromBuffer(buffer, cur)) return false;
					hasMat0 = true;
				} else if (tag == Str("Arm\0", 4)) {
					cur += 4;
					if (!armatureName.FromBuffer(buffer, cur)) return false;
				} else {
					io::cout.PrintLn("Unknown tag \"", tag, "\" in Mesh. Skipping to the end...");
					cur = endCur;
					skippedToEnd = true;
				}
				cur = align(cur, 4);
			}
			if (!hasName) {
				PRINT_ERROR("\"Mesh\" has no \"Name\"", skippedToEnd ? "... was it skipped?" : "");
				return false;
			}
			if (!hasVert) {
				PRINT_ERROR("\"Mesh\" has no \"Vert\"", skippedToEnd ? "... was it skipped?" : "");
				return false;
			}
			if (!hasIndx) {
				PRINT_ERROR("\"Mesh\" has no \"Indx\"", skippedToEnd ? "... was it skipped?" : "");
				return false;
			}
			if (!hasMat0) {
				PRINT_ERROR("\"Mesh\" has no \"Mat0\"", skippedToEnd ? "... was it skipped?" : "");
				return false;
			}
			return true;
		}
	};

	struct Empt {
		Headers::Table header;
		Types::Name name;
		az::vec3 pos;
		az::vec3 eulerAngles;
		static_assert(sizeof(az::vec3) == 4*3);
		// Expects header to already be set and cur to be immediately after
		bool FromBuffer(Str buffer, i64 &cur) {
			if (strncmp(header.ident, "Empt", 4) != 0) {
				PRINT_ERROR("Parsing a \"", Str(header.ident, 4), "\" as though it's a \"Empt\"");
				return false;
			}
			EXPECT_SPACE_IN_BUFFER(header.length - Headers::Table::SIZE);
			i64 endCur = cur + header.length - Headers::Table::SIZE;
			if (!name.FromBuffer(buffer, cur)) return false;
			if (header.length < Headers::Table::SIZE + Types::Name::SIZE + name.length + 4*6) {
				PRINT_ERROR("\"Empt\" length (", header.length, ") is too short!");
				return false;
			}
			memcpy(&pos, &buffer[cur], 4*6);
			cur += 4*6;
			if (cur < endCur) {
				io::cout.PrintLn("Skipping ", endCur - cur, " bytes in \"Empt\"");
			}
			cur = endCur;
			return true;
		}
	};

	struct Imgs {
		Headers::Table header;
		u32 count;
		// Types::ImageData files[count];
		Array<Types::ImageData> files;
		// Expects header to already be set and cur to be immediately after
		bool FromBuffer(Str buffer, i64 &cur) {
			i64 startCur = cur - Headers::Table::SIZE;
			if (strncmp(header.ident, "Imgs", 4) != 0) {
				PRINT_ERROR("Parsing a \"", Str(header.ident, 4), "\" as though it's a \"Imgs\"");
				return false;
			}
			EXPECT_SPACE_IN_BUFFER(header.length - Headers::Table::SIZE);
			i64 endCur = cur + align(header.length, 4) - Headers::Table::SIZE;
			COPY_FROM_BUFFER(count, 4);
			files.Resize(count);
			for (u32 i = 0; i < count; i++) {
				if (!files[i].FromBuffer(buffer, cur)) return false;
			}
			if (cur > endCur) {
				io::cerr.PrintLn("\"Imgs\" had more data (", cur - startCur, " bytes) than expected (", header.length, " bytes reported in header)");
				return false;
			} else if (cur < endCur) {
				io::cout.PrintLn("Skipping ", endCur - cur, " bytes in \"Imgs\"");
			}
			cur = endCur;
			return true;
		}
	};

	struct Arm0 {
		Headers::Table header;
		Types::Name name;
		u32 boneCount;
		struct Bone {
			Types::Name name;
			u8 parent;
			u8 ikTarget;
			u8 ikPole;
			enum BoneFlags {
				USE_DEFORM     = 1 << 0,
				IS_IN_IK_CHAIN = 1 << 1,
				HAS_IK_INFO    = 1 << 2,
			};
			u8 bitflags;
			mat3 basis;
			vec3 offset;
			f32 length;
			// The following only exists if bitflags has the HAS_IK_INFO bit
			char tag[2]; // Should be "IK"
			u16 ikInfoLen; // Length of the following info not including padding at the end
			enum IkInfoFlags {
				IK_STRETCH = 1 << 0,
				IK_LOCK_X  = 1 << 1,
				IK_LOCK_Y  = 1 << 2,
				IK_LOCK_Z  = 1 << 3,
				IK_LIMIT_X = 1 << 4,
				IK_LIMIT_Y = 1 << 5,
				IK_LIMIT_Z = 1 << 6,
			};
			u32 ikInfoFlags;
			// Stretch exists when IK_STRETCH is specified
			u16 ikStretch;
			// Stiffness exists when IK_LOCK_ is NOT specified
			u16 ikStiffnessX;
			u16 ikStiffnessY;
			u16 ikStiffnessZ;
			// Min and Max exist when IK_LIMIT_ is specified
			u16 ikMinX;
			u16 ikMaxX;
			u16 ikMinY;
			u16 ikMaxY;
			u16 ikMinZ;
			u16 ikMaxZ;
			bool FromBuffer(Str buffer, i64 &cur) {
				if (!name.FromBuffer(buffer, cur)) return false;
				COPY_FROM_BUFFER(parent, 4 + sizeof(mat3) + sizeof(vec3) + sizeof(f32));
				if (bitflags & HAS_IK_INFO) {
					i64 startCur = cur;
					EXPECT_TAG_IN_BUFFER("IK", 2);
					COPY_FROM_BUFFER(tag, 8);
					i64 endCur = startCur + ikInfoLen;
					if (ikInfoFlags & IK_STRETCH) {
						COPY_FROM_BUFFER(ikStretch, 2);
					}
					if (!(ikInfoFlags & IK_LOCK_X)) {
						COPY_FROM_BUFFER(ikStiffnessX, 2);
					}
					if (!(ikInfoFlags & IK_LOCK_Y)) {
						COPY_FROM_BUFFER(ikStiffnessY, 2);
					}
					if (!(ikInfoFlags & IK_LOCK_Z)) {
						COPY_FROM_BUFFER(ikStiffnessZ, 2);
					}
					if (ikInfoFlags & IK_LIMIT_X) {
						COPY_FROM_BUFFER(ikMinX, 4);
					}
					if (ikInfoFlags & IK_LIMIT_Y) {
						COPY_FROM_BUFFER(ikMinY, 4);
					}
					if (ikInfoFlags & IK_LIMIT_Z) {
						COPY_FROM_BUFFER(ikMinZ, 4);
					}
					if (cur != endCur) {
						io::cerr.PrintLn("Bone IK actual length (", cur - startCur, ") doesn't match declared length (", endCur - startCur, ")");
						return false;
					}
				}
				cur = align(cur, 4);
				return true;
			}
		};
		az::Array<Bone> bones;

		bool FromBuffer(Str buffer, i64 &cur) {
			i64 startCur = cur - Headers::Table::SIZE;
			if (strncmp(header.ident, "Arm0", 4) != 0) {
				PRINT_ERROR("Parsing a \"", Str(header.ident, 4), "\" as though it's a \"Arm0\"");
				return false;
			}
			if (!name.FromBuffer(buffer, cur)) return false;
			COPY_FROM_BUFFER(boneCount, 4);
			io::cout.PrintLn("Armature \"", name.name, "\" has got ", boneCount, " bones in ya shit!");
			if (boneCount >= 255) {
				io::cerr.PrintLn("There are ", boneCount, " bones when we have a hard limit of 254 bones");
				return false;
			}
			bones.Resize(boneCount);
			for (Bone &bone : bones) {
				if (!bone.FromBuffer(buffer, cur)) return false;
			}
			if (startCur + header.length != cur) {
				PRINT_ERROR("Arm0 table actual length (", cur - startCur, ") doesn't match declared length (", header.length, ")");
				return false;
			}
			return true;
		}
	};

	struct Act0 {
		Headers::Table header;
		Types::Name name;
		u32 numCurves;
		struct Curve {
			// Encodes the target scalar as a string
			Types::Name name;
			u32 numKeyframes;
			az::Array<KeyFrame> keyframes;
			bool FromBuffer(Str buffer, i64 &cur) {
				if (!name.FromBuffer(buffer, cur)) return false;
				COPY_FROM_BUFFER(numKeyframes, 4);
				keyframes.Resize(numKeyframes);
				for (KeyFrame &keyframe : keyframes) {
					i64 startCur = cur;
					EXPECT_SPACE_IN_BUFFER(4);
					EXPECT_TAG_IN_BUFFER("KF", 2);
					cur += 2;
					u16 len;
					COPY_FROM_BUFFER(len, 2);
					COPY_FROM_BUFFER(keyframe.point, 12);
					if (keyframe.interpolation == KeyFrame::BEZIER) {
						COPY_FROM_BUFFER(keyframe.bezier, 16);
					} else if ((u32)keyframe.interpolation >= (u32)KeyFrame::SINE) {
						COPY_FROM_BUFFER(keyframe.easing, 4);
					}
					if (keyframe.interpolation == KeyFrame::BACK) {
						COPY_FROM_BUFFER(keyframe.back, 4);
					} else if (keyframe.interpolation == KeyFrame::ELASTIC) {
						COPY_FROM_BUFFER(keyframe.elastic, 8);
					}
					if (cur != startCur + len) {
						PRINT_ERROR("KeyFrame actual length (", cur - startCur, ") doesn't match declared length (", len, ")");
						return false;
					}
				}
				return true;
			}
		};
		az::Array<Curve> curves;

		bool FromBuffer(Str buffer, i64 &cur) {
			// i64 startCur = cur - Headers::Table::SIZE;
			if (strncmp(header.ident, "Act0", 4) != 0) {
				PRINT_ERROR("Parsing a \"", Str(header.ident, 4), "\" as though it's a \"Act0\"");
				return false;
			}
			if (!name.FromBuffer(buffer, cur)) return false;
			COPY_FROM_BUFFER(numCurves, 4);
			io::cout.PrintLn("Action \"", name.name, "\" has ", numCurves, " curves.");
			curves.Resize(numCurves);
			for (Curve &curve : curves) {
				if (!curve.FromBuffer(buffer, cur)) return false;
			}
			return true;
		}
	};

} // namespace Tables

Str StringUntil(Str source, char delimiter) {
	i32 i = 0;
	for (; i < source.size; i++) {
		char c = source[i];
		if (c == '\\') {
			i++;
		} else if (c == delimiter) {
			break;
		}
	}
	return source.SubRange(0, i);
}

bool GetActionCurveFromAct0Curve(Action::Curve &dst, Tables::Act0::Curve &curveData) {
	Str toParse = curveData.name.name;
	Str prefix = "pose.bones[\"";
	if (StartsWith(toParse, prefix)) {
		toParse = toParse.SubRange(prefix.size);
		dst.boneName = StringUntil(toParse, '"');
		toParse = toParse.SubRange(dst.boneName.size + 3); // +3 to pass over `"].`
	} else {
		dst.boneName = String();
	}
	i32 indexLimit;
	if (prefix = "rotation_quaternion["; StartsWith(toParse, prefix)) {
		dst.isOffset = false;
		indexLimit = 3;
	} else if (prefix = "location["; StartsWith(toParse, prefix)) {
		dst.isOffset = true;
		indexLimit = 2;
	} else {
		io::cerr.PrintLn("Unknown data target in Act0 Curve \"", StringUntil(toParse, '['), "\"");
		return false;
	}
	toParse = toParse.SubRange(prefix.size);
	i32 index;
	if (!StringToI32(StringUntil(toParse, ']'), &index)) {
		io::cerr.PrintLn("Couldn't parse \"", StringUntil(toParse, ']'), "\" as an integer");
		return false;
	}
	if (index < 0 || index > indexLimit) {
		io::cerr.PrintLn("Index (", index, ") is out of bounds (0 to ", indexLimit, " inclusive)");
		return false;
	}
	dst.index = index;
	dst.keyframes = std::move(curveData.keyframes);
	return true;
}

bool File::Load(Str filepath) {
	Array<char> buffer = FileContents(filepath, true);
	if (buffer.size == 0) {
		io::cerr.PrintLn("Failed to open '", filepath, "'");
		return false;
	}
	return LoadFromBuffer(buffer);
}

bool File::LoadFromBuffer(Str buffer, Array<File::ImageData> *dstImageData) {
	i64 cur = 0;
	Headers::File header;
	if (!header.FromBuffer(buffer, cur)) return false;
	if (header.versionMajor > VERSION_MAJOR || header.versionMinor > VERSION_MINOR) {
		io::cout.PrintLn("Az3DObj version ", header.versionMajor, ".", header.versionMinor, " is newer than our importer (version ", VERSION_MAJOR, ".", VERSION_MINOR, "). Some features may not be available.");
	}
	u32 numTexturesExpected = 0;
	u32 numTexturesActual = 0;
	for (; cur < buffer.size;) {
		Headers::Table tableHeader;
		if (!tableHeader.FromBuffer(buffer, cur)) return false;
		i64 endCur = cur + tableHeader.length - Headers::Table::SIZE;
		if (tableHeader.length <= Headers::Table::SIZE) {
			io::cerr.PrintLn("Header length invalid (", tableHeader.length, ")");
			return false;
		}
		Str tag = Str(tableHeader.ident, 4);
		if (tag == "Mesh") {
			Tables::Mesh meshData;
			meshData.header = tableHeader;
			if (!meshData.FromBuffer(buffer, cur)) return false;
			Mesh &mesh = meshes.Append(Mesh());
			mesh.name = meshData.name.name;
			mesh.vertices = std::move(meshData.vert.vertices);
			mesh.indices = std::move(meshData.indx.indices);
			mesh.material.color = meshData.mat0.albedoColor;
			mesh.material.emit = meshData.mat0.emissionColor;
			mesh.material.normal = meshData.mat0.normalDepth;
			mesh.material.metalness = meshData.mat0.metalnessFactor;
			mesh.material.roughness = meshData.mat0.roughnessFactor;
			mesh.material.sssFactor = meshData.mat0.sssFactor;
			mesh.material.sssColor = meshData.mat0.sssColor;
			mesh.material.sssRadius = meshData.mat0.sssRadius;
			mesh.material.texAlbedo = meshData.mat0.albedoIndex;
			mesh.material.texEmit = meshData.mat0.emissionIndex;
			mesh.material.texNormal = meshData.mat0.normalIndex;
			mesh.material.texMetalness = meshData.mat0.metalnessIndex;
			mesh.material.texRoughness = meshData.mat0.roughnessIndex;
			mesh.material.isFoliage = meshData.mat0.isFoliage;
			for (i32 i = 0; i < 5; i++) {
				if (mesh.material.tex[i] > numTexturesExpected) numTexturesExpected = mesh.material.tex[i];
			}
		} else if (tag == "Empt") {
			Tables::Empt emptyData;
			emptyData.header = tableHeader;
			if (!emptyData.FromBuffer(buffer, cur)) return false;
			Empty &empty = empties.Append(Empty());
			empty.name = emptyData.name.name;
			empty.pos = emptyData.pos;
			empty.eulerAngles = emptyData.eulerAngles;
		} else if (tag == "Imgs") {
			Tables::Imgs imagesData;
			imagesData.header = tableHeader;
			if (!imagesData.FromBuffer(buffer, cur)) return false;
			if (dstImageData) {
				dstImageData->Resize(imagesData.count);
				for (u32 i = 0; i < imagesData.count; i++) {
					ImageData &imageData = (*dstImageData)[i];
					imageData.filename = imagesData.files[i].filename.name;
					imageData.data = imagesData.files[i].data;
					imageData.isLinear = imagesData.files[i].isLinear;
				}
			} else {
				for (u32 i = 0; i < imagesData.count; i++) {
					Image &image = images.Append(Image());
					if (!image.LoadFromBuffer(imagesData.files[i].data)) {
						io::cerr.PrintLn("Failed to decode image data for \"", imagesData.files[i].filename.name, "\" embedded in Az3DObj.");
						return false;
					}
					image.colorSpace = imagesData.files[i].isLinear ? Image::LINEAR : Image::SRGB;
				}
			}
			numTexturesActual += imagesData.count;
		} else if (tag == "Arm0") {
			Tables::Arm0 armatureData;
			armatureData.header = tableHeader;
			if (!armatureData.FromBuffer(buffer, cur)) return false;
			Armature &armature = armatures.Append(Armature());
			armature.name = armatureData.name.name;
			armature.bones.Resize(armatureData.bones.size);
			for (i32 i = 0; i < armatureData.bones.size; i++) {
				Tables::Arm0::Bone &boneData = armatureData.bones[i];
				Bone &bone = armature.bones[i];
				bone.name = boneData.name.name;
				bone.basis = boneData.basis;
				bone.offset = boneData.offset;
				bone.length = boneData.length;
				bone.parent = boneData.parent;
				bone.ikTarget = boneData.ikTarget;
				bone.ikPole = boneData.ikPole;
				bone.deform = 0 != (boneData.bitflags & Tables::Arm0::Bone::USE_DEFORM);
				bone.isInIkChain = 0 != (boneData.bitflags & Tables::Arm0::Bone::IS_IN_IK_CHAIN);
				if (boneData.bitflags & Tables::Arm0::Bone::HAS_IK_INFO) {
					using BoneData = Tables::Arm0::Bone;
					Bone::IkInfo &ikInfo = bone.ikInfo;
					ikInfo.stretch = boneData.ikInfoFlags & BoneData::IK_STRETCH ? boneData.ikStretch : 0.0f;
					ikInfo.limited = {
						bool(boneData.ikInfoFlags & BoneData::IK_LIMIT_X),
						bool(boneData.ikInfoFlags & BoneData::IK_LIMIT_Y),
						bool(boneData.ikInfoFlags & BoneData::IK_LIMIT_Z),
					};
					ikInfo.locked = {
						bool(boneData.ikInfoFlags & BoneData::IK_LOCK_X),
						bool(boneData.ikInfoFlags & BoneData::IK_LOCK_Y),
						bool(boneData.ikInfoFlags & BoneData::IK_LOCK_Z),
					};
					if (boneData.ikInfoFlags & BoneData::IK_LIMIT_X) {
						ikInfo.min.x = map((f32)boneData.ikMinX, 0.0f, f32(0xffff), -pi, 0.0f);
						ikInfo.max.x = map((f32)boneData.ikMaxX, 0.0f, f32(0xffff), 0.0f, pi);
					} else {
						ikInfo.min.x = -180.0f;
						ikInfo.max.x = 180.0f;
					}
					if (boneData.ikInfoFlags & BoneData::IK_LIMIT_Y) {
						ikInfo.min.y = map((f32)boneData.ikMinY, 0.0f, f32(0xffff), -pi, 0.0f);
						ikInfo.max.y = map((f32)boneData.ikMaxY, 0.0f, f32(0xffff), 0.0f, pi);
					} else {
						ikInfo.min.y = -180.0f;
						ikInfo.max.y = 180.0f;
					}
					if (boneData.ikInfoFlags & BoneData::IK_LIMIT_Z) {
						ikInfo.min.z = map((f32)boneData.ikMinZ, 0.0f, f32(0xffff), -pi, 0.0f);
						ikInfo.max.z = map((f32)boneData.ikMaxZ, 0.0f, f32(0xffff), 0.0f, pi);
					} else {
						ikInfo.min.z = -180.0f;
						ikInfo.max.z = 180.0f;
					}
					ikInfo.stiffness.x = ikInfo.locked.x ? 0.0f : map((f32)boneData.ikStiffnessX, 0.0f, f32(0xffff), 0.0f, 1.0f);
					ikInfo.stiffness.y = ikInfo.locked.y ? 0.0f : map((f32)boneData.ikStiffnessY, 0.0f, f32(0xffff), 0.0f, 1.0f);
					ikInfo.stiffness.z = ikInfo.locked.z ? 0.0f : map((f32)boneData.ikStiffnessZ, 0.0f, f32(0xffff), 0.0f, 1.0f);
				} else {
					bone.ikInfo.stretch = 0.0f;
					bone.ikInfo.locked = vec3_t<bool>(false);
					bone.ikInfo.limited = vec3_t<bool>(false);
					bone.ikInfo.min = vec3(-pi);
					bone.ikInfo.max = vec3(pi);
					bone.ikInfo.stiffness = vec3(0.0f);
				}
			}
		} else if (tag == "Act0") {
			Tables::Act0 actionData;
			actionData.header = tableHeader;
			if (!actionData.FromBuffer(buffer, cur)) return false;
			Action &action = actions.Append(Action());
			action.name = actionData.name.name;
			action.curves.Resize(actionData.curves.size);
			for (i32 i = 0; i < action.curves.size; i++) {
				Tables::Act0::Curve &curveData = actionData.curves[i];
				Action::Curve &curve = action.curves[i];
				if (!GetActionCurveFromAct0Curve(curve, curveData)) return false;
			}
		} else {
			io::cout.PrintLn("Ignoring unsupported table '", tag, "'");
			cur = endCur;
		}
		if (cur > endCur) {
			io::cerr.PrintLn("We exceeded the limits of '", tag, "' table by ", cur - endCur, " bytes!");
			return false;
		} else if (cur < endCur) {
			io::cout.PrintLn("There seems to be some trailing information in '", tag, "' table of ", endCur - cur, " bytes. Skipping...");
			cur = endCur;
		}
		cur = align(cur, 4);
	}
	if (numTexturesActual != numTexturesExpected) {
		io::cerr.PrintLn("Materials expected ", numTexturesExpected, " textures, but we actually had ", numTexturesActual);
		return false;
	}
	io::cout.PrintLn("Had ", meshes.size, " meshes, ", empties.size, " empties, ", images.size, " images, ", armatures.size, " armatures, and ", actions.size, " actions.");
	return true;
}

} // namespace Az3D::Az3DObj