/*
	File: SoftwareRendering.hpp
	Author: Philip Haynes
	Implements a simple image-to-window renderer.
*/

#ifndef AZCORE_SOFTWARE_RENDERING_HPP
#define AZCORE_SOFTWARE_RENDERING_HPP

#include "IO/Window.hpp"
#include "math.hpp"
#include "Image.hpp"
#include "memory.hpp"
#include "Math/Color.hpp"

namespace AzCore {

struct Pixel {
	struct {
		u8 r, g, b, a;
	};
	Pixel(u32 in) : r(in), g(in>>8), b(in>>16), a(in>>24) {}
	Pixel(Color<u8> color) : r(color.r), g(color.g), b(color.b), a(color.a) {}
};

struct SoftwareRenderer {
	struct SWData *data;
	io::Window *window;
	i32 width, height, depth;
	u8 *framebuffer;
	i32 stride;
	String error;
	bool initted;

	inline Pixel& GetPixel(i32 x, i32 y) {
#ifndef NDEBUG
		static Pixel errPix{0};
		if (x > width || y > height || x < 0 || y < 0) return errPix;
		if (nullptr == framebuffer) return errPix;
#endif
		return *((Pixel*)&framebuffer[y*stride + x*depth]);
	}
	SoftwareRenderer(io::Window *inWindow);
	SoftwareRenderer(const SoftwareRenderer&) = delete;
	SoftwareRenderer(SoftwareRenderer &&) = delete;
	~SoftwareRenderer();
	bool Init();
	// Makes sure width and height are up-to-date and our framebuffer is current
	bool Update();
	bool Present();
	bool Deinit();

	bool FramebufferToImage(Image *dst);

	void ColorPixel(i32 x, i32 y, Color<u8> in);
	void DarkenBox(vec2i p1, vec2i p2, u8 amount);
	void DrawBox(vec2i p1, vec2i p2, Color<u8> color);
	void DrawBoxBlended(vec2i p1, vec2i p2, Color<u8> color);
	void DrawImage(vec2i p1, Image *image);
	void DrawImageBlended(vec2i p1, Image *image);
};

} // namespace AzCore

#endif // AZCORE_SOFTWARE_RENDERING_HPP
