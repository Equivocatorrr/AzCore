/*
	File: main.cpp
	Author: Philip Haynes
	Description: High-level definition of the structure of our program.
*/

#include "AzCore/io.hpp"
#include "AzCore/SoftwareRendering.hpp"

using namespace AzCore;

#include "AzCore/Math/Noise.hpp"
#include "AzCore/math.hpp"

io::Log cout("test.log", true, true);

constexpr i32 fpsLimit = 60;

using Real = f64;

enum class NoiseType {
	WHITE,
	LINEAR,
	COSINE,
	CUBIC,
};
constexpr i32 NoiseTypeCount = 4;

void RenderWhiteNoise(SoftwareRenderer &renderer, vec2_t<Real> pos, Real zoom, Real aspect, u64 seed=0) {
	for (i32 y = 0; y < renderer.height; y++) {
		i32 pointY = round(((Real)y - 0.5 * (Real)renderer.height) * zoom + pos.y * (Real)renderer.height);
		for (i32 x = 0; x < renderer.width; x++) {
			i32 pointX = round(((Real)x - 0.5 * (Real)renderer.width) * zoom + pos.x * (Real)renderer.width);
			f32 value = Noise::whiteNoise<f32>(vec2i(pointX, pointY), seed);
			renderer.ColorPixel(x, y, Color<u8>(vec3_t<u8>(255.0f*linearTosRGB(value)), 255));
		}
	}
}

void RenderLinearNoise(SoftwareRenderer &renderer, vec2_t<Real> pos, Real zoom, Real aspect, u64 seed=0) {
	for (i32 y = 0; y < renderer.height; y++) {
		Real pointY = ((Real)y - 0.5 * (Real)renderer.height) * zoom + pos.y * (Real)renderer.height;
		for (i32 x = 0; x < renderer.width; x++) {
			Real pointX = ((Real)x - 0.5 * (Real)renderer.width) * zoom + pos.x * (Real)renderer.width;
			f32 value = Noise::linearNoise<f32>(vec2d(pointX, pointY), seed);
			renderer.ColorPixel(x, y, Color<u8>(vec3_t<u8>(255.0f*linearTosRGB(value)), 255));
		}
	}
}

void RenderCosineNoise(SoftwareRenderer &renderer, vec2_t<Real> pos, Real zoom, Real aspect, u64 seed=0) {
	for (i32 y = 0; y < renderer.height; y++) {
		Real pointY = ((Real)y - 0.5 * (Real)renderer.height) * zoom + pos.y * (Real)renderer.height;
		for (i32 x = 0; x < renderer.width; x++) {
			Real pointX = ((Real)x - 0.5 * (Real)renderer.width) * zoom + pos.x * (Real)renderer.width;
			f32 value = Noise::cosineNoise<f32>(vec2d(pointX, pointY), seed);
			renderer.ColorPixel(x, y, Color<u8>(vec3_t<u8>(255.0f*linearTosRGB(value)), 255));
		}
	}
}

void RenderCubicNoise(SoftwareRenderer &renderer, vec2_t<Real> pos, Real zoom, Real aspect, u64 seed=0) {
	for (i32 y = 0; y < renderer.height; y++) {
		Real pointY = ((Real)y - 0.5 * (Real)renderer.height) * zoom + pos.y * (Real)renderer.height;
		for (i32 x = 0; x < renderer.width; x++) {
			Real pointX = ((Real)x - 0.5 * (Real)renderer.width) * zoom + pos.x * (Real)renderer.width;
			f32 value = Noise::cubicNoise<f32>(vec2d(pointX, pointY), seed);
			renderer.ColorPixel(x, y, Color<u8>(vec3_t<u8>(255.0f*linearTosRGB(value)), 255));
		}
	}
}

i32 main(i32 argumentCount, char** argumentValues) {

	io::Window window;
	io::Input input;
	window.input = &input;
	window.width = 512;
	window.height = 512;
	if (!window.Open()) {
		cout.PrintLn("Failed to open Window: ", io::error);
		return 1;
	}

	f32 scale = (f32)window.GetDPI() / 96.0f;
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
	bool updated = true;
	bool skippedPresent = false;
	Real zoom = 1.0f;
	i32 noiseType = 0;
	u64 seed = 0;
	vec2_t<Real> pos = 0.0f;
	do {
		scale = (f32)window.GetDPI() / 96.0f;
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
		Real aspect = (Real)renderer.height / (Real)renderer.width;
		vec2 mouse = input.cursor;
		mouse /= vec2(renderer.width, renderer.height);
		mouse -= 0.5f;
		mouse *= zoom;
		mouse.y *= aspect;
		vec2 delta = input.cursor-input.cursorPrevious;
		delta /= vec2(renderer.width, renderer.height);
		delta *= zoom;
		delta.y *= aspect;
		if (input.scroll.y != 0.0f) {
			f32 factor = pow(1.1f, input.scroll.y);
			pos += mouse - mouse / factor;
			// if (input.scroll.y > 0.0f) {
			// 	// Slight tendency to scroll towards center of screen on zoom in
			// 	pos += mouse * (factor-1) / 8;
			// }
			zoom /= factor;
			updated = true;
		}
		if (input.Down(KC_MOUSE_LEFT) && !input.Pressed(KC_MOUSE_LEFT)) {
			if (delta != f32(0)) {
				updated = true;
			}
			pos -= delta;
		}
		if (input.Pressed(KC_KEY_KPPLUS)) {
			if (input.Down(KC_KEY_LEFTSHIFT) || input.Down(KC_KEY_RIGHTSHIFT)) {
				noiseType = (noiseType+1) % NoiseTypeCount;
			} else {
				seed += 1;
			}
			updated = true;
		}
		if (input.Pressed(KC_KEY_KPMINUS)) {
			if (input.Down(KC_KEY_LEFTSHIFT) || input.Down(KC_KEY_RIGHTSHIFT)) {
				noiseType--;
				if (noiseType < 0) noiseType = NoiseTypeCount-1;
			} else {
				seed -= 1;
			}
			updated = true;
		}
		if (updated) {
			switch ((NoiseType)noiseType) {
				case NoiseType::WHITE:
					RenderWhiteNoise(renderer, pos, zoom, aspect, seed);
					break;
				case NoiseType::LINEAR:
					RenderLinearNoise(renderer, pos, zoom, aspect, seed);
					break;
				case NoiseType::COSINE:
					RenderCosineNoise(renderer, pos, zoom, aspect, seed);
					break;
				case NoiseType::CUBIC:
					RenderCubicNoise(renderer, pos, zoom, aspect, seed);
					break;
			}
			updated = false;
		}
		input.Tick(1.0f/(f32)fpsLimit);
		Nanoseconds frameSleep = frameNext - Clock::now() - Nanoseconds(1000);
		if (frameSleep > Nanoseconds(1000000)) {
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

	return 0;
}
