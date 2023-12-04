/*
	File: Image.hpp
	Author: Philip Haynes
	A shallow wrapper around stb_image.h
*/

#ifndef AZCORE_IMAGE_HPP
#define AZCORE_IMAGE_HPP

#include "basictypes.hpp"
#include "Memory/String.hpp"
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
	enum ColorSpace {
		SRGB,
		LINEAR
	} colorSpace=SRGB;
	inline void _Acquire(Image &&other) {
		pixels = other.pixels;
		other.pixels = nullptr;
		width = other.width;
		height = other.height;
		channels = other.channels;
		stride = other.stride;
		format = other.format;
		colorSpace = other.colorSpace;
	}
	void _Copy(const Image &other);
	Image() = default;
	inline Image(Image &other) {
		_Copy(other);
	}
	inline Image(Image &&other) {
		_Acquire(std::move(other));
	}
	Image(const char *filename, i32 channelsDesired = 0);
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
	// Opens the file to load
	bool Load(const char *filename, i32 channelsDesired = 0);
	// Decodes the file stored in buffer
	bool LoadFromBuffer(Str buffer, i32 channelsDesired = 0);
	void Reformat(Format newFormat);
	void SetChannels(i32 newChannels);
	bool SavePNG(const char *filename);
	void PremultiplyAlpha();
};

} // namespace AzCore

#endif // AZCORE_IMAGE_HPP
