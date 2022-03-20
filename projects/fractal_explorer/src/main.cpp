/*
	File: main.cpp
	Author: Philip Haynes
	Description: High-level definition of the structure of our program.
*/

#include "AzCore/io.hpp"
#include "AzCore/SoftwareRendering.hpp"

using namespace AzCore;

#include "AzCore/SIMD/SimdAVX.hpp"
#include "AzCore/Image.hpp"
#include "AzCore/math.hpp"
#include "AzCore/Time.hpp"
#include "AzCore/keycodes.hpp"

io::Log cout("test.log");

constexpr i32 fpsLimit = 60;
i64 iterations = 0;
i64 nanoseconds = 0;

#define SIMD_ENABLE 1
#define SIMD_AVX 1

#ifdef __GNUG__
struct SimdInfo {
	bool mmx;
	bool sse;
	bool sse2;
	bool sse3;
	bool ssse3;
	bool sse4_1;
	bool sse4_2;
	bool avx;
	bool f16c;
	bool avx2;
	bool avx512_f;
};

SimdInfo GetSimdInfo() {
	SimdInfo info;
	u32 edx, ecx, ebx;
	__asm__("cpuid"
		: "=d"(edx), "=c"(ecx)
		: "a"(1));
	info.mmx    = 0 != (edx & (1 << 23));
	info.sse    = 0 != (edx & (1 << 25));
	info.sse2   = 0 != (edx & (1 << 26));
	info.sse3   = 0 != (ecx & (1 << 0 ));
	info.ssse3  = 0 != (ecx & (1 << 9 ));
	info.sse4_1 = 0 != (ecx & (1 << 19));
	info.sse4_2 = 0 != (ecx & (1 << 20));
	info.avx    = 0 != (ecx & (1 << 28));
	info.f16c   = 0 != (ecx & (1 << 29));
	__asm__("cpuid"
		: "=b"(ebx), "=c"(ecx), "=d"(edx)
		: "a"(7), "c"(0));
	info.avx2     = 0 != (ebx & (1 << 5 ));
	info.avx512_f = 0 != (ebx & (1 << 16));
	return info;
}
#endif

const char* BoolString(bool in) {
	return in ? "true" : "false";
}

#if !SIMD_ENABLE
typedef f64 Float;
typedef f64 Real;
typedef i32 Integer;
typedef u32 Mask;
constexpr i32 simdLanes = 1;
constexpr i32 intLanes = 1;

u32 SimplifyMask(Mask in) {
	return in;
}
u32 AndNot(u32 a, u32 b) {
	return ~b & a;
}
i32 HorizontalAdd(Integer in) {
	return in;
}
void GetValues(Integer in, i64 *dst) {
	*dst = in;
}
void GetValues(Real in, Float *dst) {
	*dst = in;
}
void SetValues(Real *dst, Float *src) {
	*dst = *src;
}
Mask comparison(Mask in) {
	return in ? 0xffffffff : 0;
}

#else

#if SIMD_AVX
typedef f64 Float;
typedef f64x4 Real;
typedef i32x4 Integer;
typedef u32x4 Mask;
constexpr i32 simdLanes = 4;
constexpr i32 intLanes = 4;

u32 SimplifyMask(Mask in) {
	return _mm_movemask_epi8(in.V);
}

#else

typedef f64 Float;
typedef f64x2 Real;
typedef i32x4 Integer;
typedef u32x4 Mask;
constexpr i32 simdLanes = 2;
constexpr i32 intLanes = 4;

u32 SimplifyMask(Mask in) {
	return _mm_movemask_epi8(in.V);
}

#endif // SIMD_AVX

Mask comparison(Mask in) {
	return in;
}

i32 HorizontalAdd(Integer in) {
	return horizontalAdd(in);
}
void GetValues(Integer in, i32 *dst) {
	i32 values[intLanes];
	in.GetValues(values);
	memcpy(dst, values, sizeof(i32)*simdLanes);
}
void GetValues(Real in, Float *dst) {
	in.GetValues(dst);
}
void SetValues(Real *dst, Float *src) {
	dst->SetValues(src);
}

#endif // !SIMD_ENABLE

typedef complex_t<Real> Complex;

Integer GetIterations(Complex z, Complex c, i64 limit) {
	ClockTime start = Clock::now();
	Integer result(0);
	Integer increment(1);
	Mask incompleteMask(0xffffffff);
	for (i64 i = 0; i < limit; i++) {
		 z = z*z + c;
		 Real sqDist = z.real*z.real + z.imag*z.imag;
		 Mask notEscaped = comparison(sqDist <= Real(4));
		 increment &= notEscaped;
		 result += increment;
		 incompleteMask &= notEscaped;
		 if (0 == SimplifyMask(incompleteMask)) break;
	}
	result = AndNot(result, incompleteMask);
	iterations += HorizontalAdd(result);
	nanoseconds += Nanoseconds(Clock::now() - start).count();
	return result;
}

void Render(SoftwareRenderer &renderer, vec2_t<Float> pos, bool julia, Complex juliaPoint, Float zoom, i64 iterations, vec2i offset, vec2i scale, vec2i stride, i32 finalScale) {
	scale *= finalScale;
	stride *= finalScale;
	offset *= finalScale;
	scale -= vec2i(1);
	Float aspect = (Float)renderer.height / (Float)renderer.width;
	for (i32 y = 0; y < renderer.height; y += simdLanes*stride.y) {
		Complex c;
		Float yy[simdLanes];
		for (i32 i = 0; i < simdLanes; i++) {
			yy[i] = Float(y + offset.y + i * stride.y);
		}
		SetValues(&c.imag, yy);
		c.imag /= (Float)renderer.height;
		c.imag -= Float(0.5);
		c.imag *= zoom * aspect;
		c.imag += pos.y;
		for (i32 x = 0; x < renderer.width; x+=stride.x) {
			Float xx = (Float)(x+offset.x) / (Float)renderer.width;
			xx = (xx-Float(0.5)) * zoom + pos.x;
			c.real = xx;
			Real its = GetIterations(c, julia ? juliaPoint : c, iterations);
			Float values[simdLanes];
			its /= Float(1024);
			its = sqrt(its+1)-1;
			GetValues(its, values);
			for (i32 i = 0; i < simdLanes; i++) {
				Float value = values[i];
				vec3 color;
				f32 control = (f32)value;
				f32 hue = control/6.0f;
				f32 sat = sin(control*tau*2.0f)/4.0f
				        + 0.75f;
				f32 val = min(1.0f, control*16.0f);
				color = hsvToRgb(vec3(hue, sat, val)) * 255.0f;
				vec2i p1(x, y + i*stride.y);
				p1 += offset;
				renderer.DrawBox(p1, p1+scale, Color((u8)color.r, (u8)color.g, (u8)color.b, 255));
			}
		}
	}
}
struct Pattern {
	vec2i offset;
	vec2i scale;
};

void RemoveDuplicates(Array<Pattern> &pattern) {
	Array<vec2i> offsets;
	for (i32 i = 0; i < pattern.size; i++) {
		if (offsets.Contains(pattern[i].offset)) {
			pattern.Erase(i);
			i--;
		} else {
			offsets.Append(pattern[i].offset);
		}
	}
}

i32 LowestBit(i32 in) {
	return in & -in;
}

i32 Fitness(Pattern pattern) {
	return pattern.scale.x * pattern.scale.y;
}

void SortBySize(Array<Pattern> &pattern) {
	// Just gonna do a simple insertion sort
	Array<Pattern> intermediate;
	for (i32 i = 0; i < pattern.size; i++) {
		Pattern p = pattern[i];
		i32 fitness = Fitness(p);
		i32 j = 0;
		for (; j < intermediate.size; j++) {
			if (fitness > Fitness(intermediate[j])) break;
		}
		intermediate.Insert(j, p);
	}
	pattern = std::move(intermediate);
}

// Generates a size*size kernel
Array<Pattern> GenKernel(vec2i size, vec2i offset=0) {
	Array<Pattern> out;
	out += {{offset.x, offset.y}, size};
	if (size.x > 1 && size.x >= size.y) {
		out += GenKernel({size.x/2, size.y}, offset);
		out += GenKernel({size.x/2, size.y}, offset + vec2i(size.x/2, 0));
	}
	if (size.y > 1 && size.y >= size.x) {
		out += GenKernel({size.x, size.y/2}, offset);
		out += GenKernel({size.x, size.y/2}, offset + vec2i(0, size.y/2));
	}
	if (offset == 0) {
		SortBySize(out);
		RemoveDuplicates(out);
	}
	return out;
}

i32 main(i32 argumentCount, char** argumentValues) {

	i32 kernelSize = 16;
	i32 finalScale = 1;
	Array<Pattern> pattern = GenKernel(kernelSize);
	// for (i32 i = 0; i < pattern.size; i++) {
	// 	Pattern p = pattern[i];
	// 	cout.PrintLn("(", p.offset.x, ",", p.offset.y, ") (", p.scale.x, ",", p.scale.y, ")");
	// }

	f32 scale = 1.0f;
#ifdef __GNUG__
	SimdInfo simdInfo = GetSimdInfo();
	cout.PrintLn(
		"MMX: ", BoolString(simdInfo.mmx),
		"\nSSE: ", BoolString(simdInfo.sse),
		"\nSSE2: ", BoolString(simdInfo.sse2),
		"\nSSE3: ", BoolString(simdInfo.sse3),
		"\nSSSE3: ", BoolString(simdInfo.ssse3),
		"\nSSE4.1: ", BoolString(simdInfo.sse4_1),
		"\nSSE4.2: ", BoolString(simdInfo.sse4_2),
		"\nAVX: ", BoolString(simdInfo.avx),
		"\nF16C: ", BoolString(simdInfo.f16c),
		"\nAVX2: ", BoolString(simdInfo.avx2),
		"\nAVX512_f: ", BoolString(simdInfo.avx512_f)
	);
#endif

	cout.PrintLn("\nTest program received ", argumentCount, " arguments:");

	io::Window window;
	io::Input input;
	window.input = &input;
	window.width = 512;
	window.height = 512;
	if (!window.Open()) {
		cout.PrintLn("Failed to open Window: ", io::error);
		return 1;
	}

	scale = (f32)window.GetDPI() / 96.0f;
	window.Resize(u32((f32)window.width * scale), u32((u32)window.height * scale));

	if(!window.Show()) {
		cout.PrintLn("Failed to show Window: ", io::error);
		return 1;
	}
	SoftwareRenderer renderer(&window);
	if (!renderer.Init()) {
		cout.PrintLn("Failed to init Software Renderer: ", renderer.error);
		return 1;
	}
	ClockTime frameStart, frameNext;
	Nanoseconds frameDuration = Nanoseconds(1000000000/fpsLimit);
	vec2_t<Float> pos(0.0);
	vec2_t<Float> julia(-0.445833333333331, -0.5937499999999968);
	Float zoom(4.0);
	bool updated = true;
	bool skippedPresent = false;
	i32 patternIteration = 0;
	bool renderJulia = false;
	do {
		if (input.Released(KC_KEY_ESC)) {
			break;
		}
		if (window.resized) updated = true;
		if (!skippedPresent) {
			if (abs(Nanoseconds(frameNext - Clock::now()).count()) >= 1000000) {
				// Something must have hung the program. Start fresh.
				frameStart = Clock::now();
			} else {
				frameStart = frameNext;
			}
			frameNext = frameStart + frameDuration;
		}
		if (!renderer.Update()) {
			cout.PrintLn("Failed to update Software Renderer: ", renderer.error);
			return 1;
		}
		Float aspect = (Float)renderer.height / (Float)renderer.width;
		vec2_t<Float> mouse = input.cursor;
		mouse /= vec2_t<Float>(renderer.width, renderer.height);
		mouse -= Float(0.5);
		mouse *= zoom;
		mouse.y *= aspect;
		vec2_t<Float> delta = input.cursor-input.cursorPrevious;
		delta /= vec2_t<Float>(renderer.width, renderer.height);
		delta *= zoom;
		delta.y *= aspect;
		if (input.scroll.y != 0.0f) {
			Float factor = pow(1.2f, input.scroll.y);
			pos += mouse * (factor-1);
			zoom /= factor;
			updated = true;
		}
		if (input.Down(KC_MOUSE_LEFT) && !input.Pressed(KC_MOUSE_LEFT)) {
			if (delta != Float(0)) {
				updated = true;
			}
			pos -= delta;
		}
		if (input.Pressed(KC_KEY_M)) {
			renderJulia = !renderJulia;
			updated = true;
		}
		if (input.Pressed(KC_KEY_P)) {
			cout.PrintLn("Julia Point: ", julia.x, " + ", julia.y, "i");
		}
		if (input.Pressed(KC_KEY_I) && nanoseconds != 0) {
			cout.PrintLn("Iterations: ", iterations, "\nTime: ", FormatTime(Nanoseconds(nanoseconds)), "\nits/msec = ", (1000000*iterations)/nanoseconds);
			iterations = 0;
			nanoseconds = 0;
		}
		if (input.Pressed(KC_KEY_F12)) {
			Image screenshot;
			if (!renderer.FramebufferToImage(&screenshot)) {
				cout.PrintLn("Failed to get framebuffer for screenshot.");
			} else {
				if (!screenshot.SavePNG("screenshot.png")) {
					cout.PrintLn("Failed to save screenshot.");
				}
			}
		}
		if (renderJulia) {
			if (input.Down(KC_MOUSE_RIGHT)) {
				if (input.Down(KC_KEY_LEFTSHIFT)) {
					vec2_t<Float> m = input.cursor*4;
					m /= vec2_t<Float>(renderer.width, renderer.height);
					m -= Float(2.0);;
					if (julia != m) {
						julia = m;
						updated = true;
					}
				} else {
					if (delta != 0) {
						julia -= delta;
						updated = true;
					}
				}
			}
		}
		if (updated) {
			patternIteration = 0;
		}
		if (updated || patternIteration) {
			i64 iterations = i64(f64(128) * pow(f64(1) / f64(zoom), f64(1.0/3.5)));
			Pattern &p = pattern[patternIteration];
			Render(renderer, pos, renderJulia, Complex(julia.x, julia.y), zoom, iterations, p.offset, p.scale, {kernelSize}, finalScale);
			patternIteration++;
			if (patternIteration == pattern.size) {
				patternIteration = 0;
			}
			updated = false;
		}
		input.Tick(1.0f/(f32)fpsLimit);
		Nanoseconds frameSleep = frameNext - Clock::now() - Nanoseconds(1000);
		if (patternIteration && frameSleep > Nanoseconds(1000000)) {
			skippedPresent = true;
			continue;
		}
		skippedPresent = false;
		if (!renderer.Present()) {
			cout.PrintLn("Failed to present Software Renderer: ", renderer.error);
			return 1;
		}
		if (frameSleep.count() >= 1000) {
			Thread::Sleep(frameSleep);
		}
	} while (window.Update());
	if (!renderer.Deinit()) {
		cout.PrintLn("Failed to cleanup Software Renderer.");
	}
	if (!window.Close()) {
		cout.PrintLn("Failed to close Window: ", io::error);
		return 1;
	}
	cout.PrintLn("Last io::error was \"", io::error, "\"");

	return 0;
}
