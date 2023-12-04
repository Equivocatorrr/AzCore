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
#include "Profiling.hpp"
#include "Simd.hpp"

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
	format = other.format;
	colorSpace = other.colorSpace;
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
	AZCORE_PROFILING_FUNC_TIMER()
	pixels = stbi_load(filename, &width, &height, &channels, channelsDesired);
	if (channelsDesired != 0) channels = channelsDesired;
	stride = width * channels;
	format = RGBA;
	return pixels != nullptr;
}

bool Image::LoadFromBuffer(Str buffer, i32 channelsDesired) {
	AZCORE_PROFILING_FUNC_TIMER()
	pixels = stbi_load_from_memory((const stbi_uc*)buffer.str, buffer.size, &width, &height, &channels, channelsDesired);
	if (channelsDesired != 0) channels = channelsDesired;
	stride = width * channels;
	format = RGBA;
	return pixels != nullptr;
}

void Image::Reformat(Image::Format newFormat) {
	AZCORE_PROFILING_FUNC_TIMER()
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

void Image::SetChannels(i32 newChannels) {
	AZCORE_PROFILING_FUNC_TIMER()
	if (newChannels == channels) return;
	AzAssert(newChannels <= 4 && newChannels >= 1, "Invalid channel count");
	u8 *newPixels = (u8*)calloc(width * height, newChannels);
	for (i64 i = 0; i < width * height; i++) {
		memcpy(newPixels + newChannels * i, pixels + channels * i, min(channels, newChannels));
		if (newChannels == 4) {
			newPixels[i*4+3] = 255;
		}
	}
	free(pixels);
	pixels = newPixels;
	channels = newChannels;
	stride = newChannels;
}

bool Image::SavePNG(const char *filename) {
	AZCORE_PROFILING_FUNC_TIMER()
	Reformat(RGBA);
	return stbi_write_png(filename, width, height, channels, pixels, stride);
}

void Image::PremultiplyAlpha() {
	AZCORE_PROFILING_FUNC_TIMER()
	AzAssert(channels == 4, "Image must have 4 channels for PremultiplyAlpha");
	i32 i = 0;
	static __m256i alphaMask = _mm256_set_epi16(
		0xff, 0, 0, 0,
		0xff, 0, 0, 0,
		0xff, 0, 0, 0,
		0xff, 0, 0, 0
	);
	AzAssert(((u64)pixels & 15) == 0, "We're expecting the pixel array to be aligned on a 16-byte boundary");
	// Premultiply alpha
#if 1
	for (; i <= width*height-4; i+=4) {
		u8 *pixel = &pixels[i*channels];
		__m128i &rgba8 = *(__m128i*)pixel;
		__m256i RGBA = _mm256_cvtepu8_epi16(rgba8);
		// Shuffle our alpha channel into all the rgb channels
		__m256i AAA1 = _mm256_shufflelo_epi16(
			_mm256_shufflehi_epi16(RGBA, _MM_SHUFFLE(3,3,3,3)),
			_MM_SHUFFLE(3,3,3,3)
		);
		// RGBA = _mm256_set1_epi16(0xff);
		// Set our alpha to 1.0 so it doesn't get squared
		AAA1 = _mm256_or_si256(AAA1, alphaMask);
		// Multiply RGBA by AAA1
		RGBA = _mm256_mullo_epi16(RGBA, AAA1);
		// now divide by 255 by multiplying by a magic number and shifting
		{
			// NOTE: MSVC gives the warning "C4309: 'initializing': truncation of constant value"
			//       I would assume that truncation means it's losing data, but the static_assert
			//       passes so that means this code is valid.
			constexpr short test = 0x8081;
			static_assert((unsigned short)test == (unsigned short)0x8081);
		}
		RGBA = _mm256_srli_epi16(
			_mm256_mulhi_epu16(RGBA, _mm256_set1_epi16(0x8081)),
			7
		);
		// Pack 16-bit integers into 8-bit integers using unsigned saturation
		// Shuffle 64-bit integers to get the parts we want in the lower 128 bits
		// cast to __m128i so we just have the parts we want.
		__m256i packed = _mm256_packus_epi16(RGBA, RGBA);
		rgba8 = _mm256_castsi256_si128(
			_mm256_permute4x64_epi64(packed, _MM_SHUFFLE(2, 0, 2, 0))
		);
	}
#endif
	for (; i < width*height; i++) {
		u32 &pixel = *((u32*)&pixels[i*channels]);
		u16 r = (pixel >> 8*0) & 0xff;
		u16 g = (pixel >> 8*1) & 0xff;
		u16 b = (pixel >> 8*2) & 0xff;
		u16 a = (pixel >> 8*3) & 0xff;
		r = (r * a) / 0xff;
		g = (g * a) / 0xff;
		b = (b * a) / 0xff;
		pixel = (u32)a << 8*3;
		pixel |= (u32)r << 8*0;
		pixel |= (u32)g << 8*1;
		pixel |= (u32)b << 8*2;
	}
}

} // namespace AzCore
