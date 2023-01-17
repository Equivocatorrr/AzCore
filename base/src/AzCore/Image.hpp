/*
	File: Image.hpp
	Author: Philip Haynes
	A shallow wrapper around stb_image.h
*/

#ifndef AZCORE_IMAGE_HPP
#define AZCORE_IMAGE_HPP

#include "basictypes.hpp"
#include <utility>

namespace AzCore {

/*  struct: Image
	Author: Philip Haynes
	Allows loading and saving image files in many file formats, courtesy of stb_image */
struct Image {
	u8* pixels = nullptr;
	i32 width = 0;
	i32 height = 0;
	i32 channels = 0;
	i32 stride = 0;
	enum Format {
		RGBA,
		BGRA
	} format=RGBA;
	inline void _Acquire(Image &&other) {
		pixels = other.pixels;
		other.pixels = nullptr;
		width = other.width;
		height = other.height;
		channels = other.channels;
	}
	void _Copy(const Image &other);
	Image() = default;
	inline Image(Image &other) {
		_Copy(other);
	}
	inline Image(Image &&other) {
		_Acquire(std::move(other));
	}
	Image(const char *filename, i32 channelsDesired = 4);
	~Image();
	inline Image& operator=(const Image &other) {
		if (this == &other) return *this;
		Dealloc();
		_Copy(other);
		return *this;
	}
	inline Image& operator=(Image &&other) {
		if (this == &other) return *this;
		Dealloc();
		_Acquire(std::move(other));
		return *this;
	}
	// width, height, channels, stride(defaults to width*channels)
	void Alloc(i32 w, i32 h, i32 c, i32 s=0);
	void Alloc();
	void Dealloc();
	bool Copy(u8 *buffer, i32 w, i32 h, i32 c, Format fmt, i32 s=0, u8 def=0);
	bool Load(const char *filename, i32 channelsDesired = 4);
	void Reformat(Format newFormat);
	bool SavePNG(const char *filename);
};

} // namespace AzCore

#endif // AZCORE_IMAGE_HPP
