/*
	File: Image.cpp
	Author: Philip Haynes
*/

#include "basictypes.hpp"
// #define pow(v, e) pow((double)v, (double)e)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
// #undef pow

#include "Image.hpp"
#include "math.hpp"
#include "memory.hpp"

namespace AzCore {

void Image::_Copy(const Image &other) {
	if (other.pixels == nullptr) {
		pixels = nullptr;
	} else {
		size_t size = other.height * other.stride;
		pixels = (u8*)stbi__malloc(size);
		memcpy(pixels, other.pixels, size);
	}
	width = other.width;
	height = other.height;
	channels = other.channels;
	stride = other.stride;
}

Image::Image(const char *filename, i32 channelsDesired)
{
	pixels = stbi_load(filename, &width, &height, &channels, channelsDesired);
	channels = channelsDesired;
	stride = width * channels;
}

Image::~Image() {
	Dealloc();
}

void Image::Alloc(i32 w, i32 h, i32 c, i32 s) {
	Dealloc();
	width = w;
	height = h;
	channels = c;
	if (s) {
		stride = s;
	} else {
		stride = width*c;
	}
	Alloc();
}

void Image::Alloc() {
	size_t size = height * stride;
	pixels = (u8*)malloc(size);
}

bool Image::Copy(u8 *buffer, i32 w, i32 h, i32 c, Image::Format fmt, i32 s, u8 def) {
	if (nullptr == pixels) return false;
	if (s == 0) s = w*c;
	format = fmt;
	i32 copyHeight = min(h, height);
	i32 copyWidth = min(w, width);
	if (s == stride && c == channels) {
		memcpy(pixels, buffer, copyHeight*s);
	} else if (c == channels) {
		for (i32 y = 0; y < copyHeight; y++) {
			memcpy(&pixels[y*stride], &buffer[y*s], copyHeight*copyWidth*c);
		}
	} else {
		// Pain
		i32 copyChannels = min(c, channels);
		for (i32 y = 0; y < copyHeight; y++) {
			for (i32 x = 0; x < copyWidth; x++) {
				size_t dst = y*stride+x*channels;
				memcpy(&pixels[dst], &buffer[y*s + x*c], copyChannels);
				for (i32 cc = c; cc < channels; cc++) {
					pixels[dst+cc] = def;
				}
			}
		}
	}
	return true;
}

void Image::Dealloc() {
	if (pixels) {
		free(pixels);
		pixels = nullptr;
	}
}

bool Image::Load(const char *filename, i32 channelsDesired) {
	pixels = stbi_load(filename, &width, &height, &channels, channelsDesired);
	channels = channelsDesired;
	stride = width * channels;
	format = RGBA;
	return pixels != nullptr;
}

void Image::Reformat(Image::Format newFormat) {
	if (format == newFormat) return;
	for (i32 y = 0; y < height; y++) {
		u8 *line = &pixels[y*stride];
		for (i32 x = 0; x < width; x++) {
			u8 *buf = &line[x*channels];
			// Swap R and B
			Swap(buf[0], buf[2]);
		}
	}
}

bool Image::SavePNG(const char *filename) {
	Reformat(RGBA);
	return stbi_write_png(filename, width, height, channels, pixels, stride);
}

} // namespace AzCore
