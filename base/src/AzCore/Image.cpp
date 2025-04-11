/*
	File: Image.cpp
	Author: Philip Haynes
*/

#include "BasicTypes.hpp"
// #define pow(v, e) pow((double)v, (double)e)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
// #undef pow

#include "Image.hpp"
#include "Utility/Profiling.hpp"
#include "Math/Basic.hpp"
#include "SIMD/Simd.hpp"

namespace AzCore {

void Image::_Copy(const Image &other) {
	if (other.pixels == nullptr) {
		Dealloc();
		pixels = nullptr;
		pixelsCapacity = 0;
	} else {
		u64 requiredCapacity = other.height * other.stride;
		if (pixelsCapacity >= requiredCapacity) {
			// We're good, just copy
			// TODO: Maybe have some logic to shrink in some extreme scenarios?
		} else {
			Dealloc();
			pixelsCapacity = requiredCapacity;
			pixels = (u8*)malloc(pixelsCapacity);
		}
		memcpy(pixels, other.pixels, requiredCapacity);
	}
	_CopyProperties(other);
}

Image::~Image() {
	Dealloc();
}

static void ValidateStride(const Image &image, i32 stride) {
#ifndef NDEBUG
	i32 minStrideReq = image.GetNeededStride();
	AzAssert(stride >= minStrideReq, Stringify("Manually specified stride (", stride, ") is too small for our width (", image.width, ") and format (", image.format.bitdepth, "bpp, ", image.strideAlignment, " byte alignment). Needed at least a stride of ", minStrideReq, "."));
#endif
}

void Image::Alloc() {
	if (autoStride) {
		stride = GetNeededStride();
	} else {
		ValidateStride(*this, stride);
	}
	u64 requiredCapacity = height * stride;
	if (pixelsCapacity >= requiredCapacity) {
		// We're good, no alloc needed
		// TODO: Maybe have some logic to shrink in some extreme scenarios?
	} else {
		Dealloc();
		pixelsCapacity = requiredCapacity;
		pixels = (u8*)malloc(pixelsCapacity);
	}
}

// Prepares the value for use in the pixel operations by reinterpreting the bytes as uints
static void BitSimplifyDefaultValue(Any &defaultValue) {
	if (defaultValue.IsSomething()) {
		if (defaultValue.IsType<i8>()) {
			defaultValue.Reinterpret<u8>();
		} else if (defaultValue.IsType<i16>()) {
			defaultValue.Reinterpret<u16>();
		} else if (defaultValue.IsType<i32>()) {
			defaultValue.Reinterpret<u32>();
		} else if (defaultValue.IsType<i64>()) {
			defaultValue.Reinterpret<u64>();
		} else if (defaultValue.IsType<f32>()) {
			defaultValue.Reinterpret<u32>();
		} else if (defaultValue.IsType<f64>()) {
			defaultValue.Reinterpret<u64>();
		} else if (!(
			   defaultValue.IsType<u8>()
			|| defaultValue.IsType<u16>()
			|| defaultValue.IsType<u32>()
			|| defaultValue.IsType<u64>()
		)) {
			AzAssert(false, "defaultValue is not a valid type for image data");
		}
	}
}

void Image::Copy(const Image &other, Any defaultValue) {
	AzAssert(width == other.width, Stringify("In Image::Copy, expected width (", width, ") to match other.width (", other.width, ")"));
	AzAssert(height == other.height, Stringify("In Image::Copy, expected height (", height, ") to match other.height (", other.height, ")"));
	AzAssert(pixels != nullptr, "In Image::Copy, pixels is null!");
	AzAssert(other.pixels != nullptr, "In Image::Copy, other.pixels is null!");
	if (format == other.format && stride == other.stride) {
		// EZPZ
		memcpy(pixels, other.pixels, GetTotalSizeBytes());
		return;
	}
	if (format == other.format) {
		// Different stride, same pixel format
		for (i32 y = 0; y < height; y++) {
			memcpy(&pixels[y*stride], &other.pixels[y*other.stride], width*GetBytesPerPixel());
		}
		return;
	}
	if (format.bitdepth == other.format.bitdepth && format.dataType == other.format.dataType && format.ColorSpaceMatches(other.format)) {
		// Same bitdepth, dataType, and colorSpace
		if (format.channels == other.format.channels) {
			// Only need to swap channel orders
			AzAssert(!format.ChannelOrderMatches(other.format), "We should only be able to get here if the channel orders don't match! What gives?");
			for (i32 y = 0; y < height; y++) {
				u8 *dstLine = &pixels[y*stride];
				u8 *srcLine = &other.pixels[y*other.stride];
				// Stands for bytes per pixel in this case
				i32 bpp = GetBytesPerPixel();
				memcpy(dstLine, srcLine, width*bpp);
				switch (format.bitdepth) {
					case 8: {
						for (i32 x = 0; x < width*bpp; x += bpp) {
							Swap(dstLine[x], dstLine[x+2*sizeof(u8)]);
						}
					} break;
					case 16: {
						for (i32 x = 0; x < width*bpp; x += bpp) {
							Swap(*((u16*)&dstLine[x]), *((u16*)&dstLine[x+2*sizeof(u16)]));
						}
					} break;
					case 32: {
						for (i32 x = 0; x < width*bpp; x += bpp) {
							Swap(*((u32*)&dstLine[x]), *((u32*)&dstLine[x+2*sizeof(u32)]));
						}
					} break;
					case 64: {
						for (i32 x = 0; x < width*bpp; x += bpp) {
							Swap(*((u64*)&dstLine[x]), *((u64*)&dstLine[x+2*sizeof(u64)]));
						}
					} break;
					default: AzAssert(false, Stringify("format bitdepth (", format.bitdepth, ") is unsupported."));
				}
			}
			return;
		}
		if (format.ChannelOrderMatches(other.format)) {
			// Only need to change channel counts
			AzAssert(format.channels != other.format.channels, "We should only be able to get here if the channel counts don't match! What gives?");
			BitSimplifyDefaultValue(defaultValue);
			u32 dstBpp = GetBytesPerPixel();
			u32 srcBpp = other.GetBytesPerPixel();
			u32 minBpp = min(dstBpp, srcBpp);
			for (i32 y = 0; y < height; y++) {
				u8 *dstLine = &pixels[y*stride];
				u8 *srcLine = &other.pixels[y*other.stride];
				if (srcBpp < dstBpp && defaultValue.IsSomething()) {
					switch (format.bitdepth) {
						case 8: {
							u8 def = defaultValue.Get<u8>();
							for (i32 x = 0; x < width; x++) {
								u8 *dstPixel = &dstLine[x*dstBpp];
								u8 *srcPixel = &srcLine[x*srcBpp];
								i32 i = 0;
								for (; i < other.format.channels; i++) {
									dstPixel[i] = srcPixel[i];
								}
								for (; i < format.channels; i++) {
									dstPixel[i] = def;
								}
							}
						} break;
						case 16: {
							u16 def = defaultValue.Get<u16>();
							for (i32 x = 0; x < width; x++) {
								u16 *dstPixel = (u16*)&dstLine[x*dstBpp];
								u16 *srcPixel = (u16*)&srcLine[x*srcBpp];
								i32 i = 0;
								for (; i < other.format.channels; i++) {
									dstPixel[i] = srcPixel[i];
								}
								for (; i < format.channels; i++) {
									dstPixel[i] = def;
								}
							}
						} break;
						case 32: {
							u32 def = defaultValue.Get<u32>();
							for (i32 x = 0; x < width; x++) {
								u32 *dstPixel = (u32*)&dstLine[x*dstBpp];
								u32 *srcPixel = (u32*)&srcLine[x*srcBpp];
								i32 i = 0;
								for (; i < other.format.channels; i++) {
									dstPixel[i] = srcPixel[i];
								}
								for (; i < format.channels; i++) {
									dstPixel[i] = def;
								}
							}
						} break;
						case 64: {
							u64 def = defaultValue.Get<u64>();
							for (i32 x = 0; x < width; x++) {
								u64 *dstPixel = (u64*)&dstLine[x*dstBpp];
								u64 *srcPixel = (u64*)&srcLine[x*srcBpp];
								i32 i = 0;
								for (; i < other.format.channels; i++) {
									dstPixel[i] = srcPixel[i];
								}
								for (; i < format.channels; i++) {
									dstPixel[i] = def;
								}
							}
						} break;
						default: AzAssert(false, Stringify("format bitdepth (", format.bitdepth, ") is unsupported."));
					}
				} else {
					// Don't worry about the uninitialized channels
					for (i32 x = 0; x < width; x++) {
						memcpy(&dstLine[x*dstBpp], &srcLine[x*srcBpp], minBpp);
					}
				}
			}
			return;
		} else {
			// Welcome to swizzle heaven (swap orders and have different channel counts)
			AzAssert(!format.ChannelOrderMatches(other.format), "We should only be able to get here if the channel orders don't match! What gives?");
			AzAssert(format.channels != other.format.channels, "We should only be able to get here if the channel counts don't match! What gives?");
			BitSimplifyDefaultValue(defaultValue);
			// stands for bytes per pixel in this case
			u32 dstBpp = GetBytesPerPixel();
			u32 srcBpp = other.GetBytesPerPixel();
			for (i32 y = 0; y < height; y++) {
				u8 *dstLine = &pixels[y*stride];
				u8 *srcLine = &other.pixels[y*other.stride];
				if (srcBpp < dstBpp && defaultValue.IsSomething()) {
					switch (format.bitdepth) {
						case 8: {
							u8 def = defaultValue.Get<u8>();
							for (i32 x = 0; x < width; x++) {
								u8 *dstPixel = &dstLine[x*dstBpp];
								u8 *srcPixel = &srcLine[x*srcBpp];
								i32 i = 0;
								for (; i < 3; i++) {
									dstPixel[i] = srcPixel[2-i];
								}
								for (; i < other.format.channels; i++) {
									dstPixel[i] = srcPixel[i];
								}
								for (; i < format.channels; i++) {
									dstPixel[i] = def;
								}
							}
						} break;
						case 16: {
							u16 def = defaultValue.Get<u16>();
							for (i32 x = 0; x < width; x++) {
								u16 *dstPixel = (u16*)&dstLine[x*dstBpp];
								u16 *srcPixel = (u16*)&srcLine[x*srcBpp];
								i32 i = 0;
								for (; i < 3; i++) {
									dstPixel[i] = srcPixel[2-i];
								}
								for (; i < other.format.channels; i++) {
									dstPixel[i] = srcPixel[i];
								}
								for (; i < format.channels; i++) {
									dstPixel[i] = def;
								}
							}
						} break;
						case 32: {
							u32 def = defaultValue.Get<u32>();
							for (i32 x = 0; x < width; x++) {
								u32 *dstPixel = (u32*)&dstLine[x*dstBpp];
								u32 *srcPixel = (u32*)&srcLine[x*srcBpp];
								i32 i = 0;
								for (; i < 3; i++) {
									dstPixel[i] = srcPixel[2-i];
								}
								for (; i < other.format.channels; i++) {
									dstPixel[i] = srcPixel[i];
								}
								for (; i < format.channels; i++) {
									dstPixel[i] = def;
								}
							}
						} break;
						case 64: {
							u64 def = defaultValue.Get<u64>();
							for (i32 x = 0; x < width; x++) {
								u64 *dstPixel = (u64*)&dstLine[x*dstBpp];
								u64 *srcPixel = (u64*)&srcLine[x*srcBpp];
								i32 i = 0;
								for (; i < 3; i++) {
									dstPixel[i] = srcPixel[2-i];
								}
								for (; i < other.format.channels; i++) {
									dstPixel[i] = srcPixel[i];
								}
								for (; i < format.channels; i++) {
									dstPixel[i] = def;
								}
							}
						} break;
						default: AzAssert(false, Stringify("format bitdepth (", format.bitdepth, ") is unsupported."));
					}
				} else {
					u8 minChannels = min(format.channels, other.format.channels);
					switch (format.bitdepth) {
						case 8: {
							for (i32 x = 0; x < width; x++) {
								u8 *dstPixel = &dstLine[x*dstBpp];
								u8 *srcPixel = &srcLine[x*srcBpp];
								i32 i = 0;
								for (; i < 3; i++) {
									dstPixel[i] = srcPixel[2-i];
								}
								for (; i < minChannels; i++) {
									dstPixel[i] = srcPixel[i];
								}
							}
						} break;
						case 16: {
							for (i32 x = 0; x < width; x++) {
								u16 *dstPixel = (u16*)&dstLine[x*dstBpp];
								u16 *srcPixel = (u16*)&srcLine[x*srcBpp];
								i32 i = 0;
								for (; i < 3; i++) {
									dstPixel[i] = srcPixel[2-i];
								}
								for (; i < minChannels; i++) {
									dstPixel[i] = srcPixel[i];
								}
							}
						} break;
						case 32: {
							for (i32 x = 0; x < width; x++) {
								u32 *dstPixel = (u32*)&dstLine[x*dstBpp];
								u32 *srcPixel = (u32*)&srcLine[x*srcBpp];
								i32 i = 0;
								for (; i < 3; i++) {
									dstPixel[i] = srcPixel[2-i];
								}
								for (; i < minChannels; i++) {
									dstPixel[i] = srcPixel[i];
								}
							}
						} break;
						case 64: {
							for (i32 x = 0; x < width; x++) {
								u64 *dstPixel = (u64*)&dstLine[x*dstBpp];
								u64 *srcPixel = (u64*)&srcLine[x*srcBpp];
								i32 i = 0;
								for (; i < 3; i++) {
									dstPixel[i] = srcPixel[2-i];
								}
								for (; i < minChannels; i++) {
									dstPixel[i] = srcPixel[i];
								}
							}
						} break;
						default: AzAssert(false, Stringify("format bitdepth (", format.bitdepth, ") is unsupported."));
					}
				}
			}
		}
		return;
	}
	// TODO: Total reformatting of the heart
	AzAssertRel(false, "Unimplemented");
}

void Image::Dealloc() {
	if (pixelsCapacity) {
		free(pixels);
		pixelsCapacity = 0;
	}
	pixels = nullptr;
}

bool Image::LoadFromFile(const char *filename, i32 channelsDesired) {
	AZCORE_PROFILING_FUNC_TIMER()
	Dealloc();
	i32 c;
	pixels = stbi_load(filename, &width, &height, &c, channelsDesired);
	if (!pixels) return false;
	strideAlignment = 1;
	format.channels = channelsDesired ? channelsDesired : c;
	stride = width * format.channels;
	pixelsCapacity = height * stride;
	format.bitdepth = 8;
	format.dataType = Format::UNORM;
	format.channelOrder = Format::RGBA;
	return true;
}

bool Image::LoadFromBuffer(Str buffer, i32 channelsDesired) {
	AZCORE_PROFILING_FUNC_TIMER()
	Dealloc();
	i32 c;
	pixels = stbi_load_from_memory((const stbi_uc*)buffer.data, buffer.size, &width, &height, &c, channelsDesired);
	if (!pixels) return false;
	strideAlignment = 1;
	format.channels = channelsDesired ? channelsDesired : c;
	stride = width * format.channels;
	pixelsCapacity = height * stride;
	format.bitdepth = 8;
	format.dataType = Format::UNORM;
	format.channelOrder = Format::RGBA;
	return true;
}

static bool FormatsAreSameExceptChannelLayout(Image::Format a, Image::Format b) {
	a.channelOrder = b.channelOrder;
	return a == b;
}

void Image::Reformat(Image::Format newFormat, Any defaultValue) {
	AZCORE_PROFILING_FUNC_TIMER()
	if (format == newFormat) {
		format = newFormat; // Because values that might not be counted in the above comparison can change
		return;
	}
	if (FormatsAreSameExceptChannelLayout(format, newFormat)) {
		// Just channel swapping
		for (i32 y = 0; y < height; y++) {
			u8 *line = &pixels[y*stride];
			// Stands for bytes per pixel in this case
			i32 bpp = GetBytesPerPixel();
			switch (format.bitdepth) {
				case 8: {
					for (i32 x = 0; x < width*bpp; x += bpp) {
						Swap(line[x], line[x+2*sizeof(u8)]);
					}
				} break;
				case 16: {
					for (i32 x = 0; x < width*bpp; x += bpp) {
						Swap(*((u16*)&line[x]), *((u16*)&line[x+2*sizeof(u16)]));
					}
				} break;
				case 32: {
					for (i32 x = 0; x < width*bpp; x += bpp) {
						Swap(*((u32*)&line[x]), *((u32*)&line[x+2*sizeof(u32)]));
					}
				} break;
				case 64: {
					for (i32 x = 0; x < width*bpp; x += bpp) {
						Swap(*((u64*)&line[x]), *((u64*)&line[x+2*sizeof(u64)]));
					}
				} break;
				default: AzAssert(false, Stringify("format bitdepth (", format.bitdepth, ") is unsupported."));
			}
		}
		format = newFormat;
	} else {
		// We'll need 2 buffers to do the reformat, so just do the simple thing
		Image copy;
		copy.width = width;
		copy.height = height;
		copy.format = newFormat;
		copy.Alloc();
		copy.Copy(*this, defaultValue);
		*this = std::move(copy);
	}
}

bool Image::SavePNG(const char *filename) {
	AZCORE_PROFILING_FUNC_TIMER()
	bool result;
	if (format.bitdepth == 8 && format.dataType == Format::DataType::UNORM) {
		// We can do non-destructive formatting if need be
		Format oldFormat = format;
		Reformat(format.WithChannelOrder(Format::ChannelOrder::RGBA));
		result = stbi_write_png(filename, width, height, format.channels, pixels, stride);
		Reformat(oldFormat);
	} else {
		// Our reformatting will likely be destructive, so make a copy
		Image copy;
		copy.width = width;
		copy.height = height;
		copy.format = format.WithBitdepth(8).WithChannelOrder(Format::ChannelOrder::RGBA).WithDataType(Format::DataType::UNORM);
		copy.Alloc();
		copy.Copy(*this);
		result = copy.SavePNG(filename);
	}
	return result;
}

void Image::PremultiplyAlpha() {
	AZCORE_PROFILING_FUNC_TIMER()
	AzAssert(format.channels == 4, Stringify("Image must have 4 channels for PremultiplyAlpha (had ", format.channels, ")"));
	AzAssert(format.bitdepth == 8, Stringify("Image must have a bitdepth of 8 for PremultiplyAlpha (had ", format.bitdepth, ")"));
	i32 i = 0;
	AzAssert(((u64)pixels & 15) == 0, "We're expecting the pixel array to be aligned on a 16-byte boundary");
	// Premultiply alpha
	// TODO: Make this work with AVX (AVX2 is too new D: )
#if __AVX2__
	static __m256i alphaMask = _mm256_set_epi16(
		0xff, 0, 0, 0,
		0xff, 0, 0, 0,
		0xff, 0, 0, 0,
		0xff, 0, 0, 0
	);
	for (; i <= width*height-4; i+=4) {
		u8 *pixel = &pixels[i*format.channels];
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
		u32 &pixel = *((u32*)&pixels[i*format.channels]);
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

Image Image::PointingToBuffer(u8 *buffer, i32 width, i32 height, Format format, i32 stride) {
	Image result;
	result.pixels = buffer;
	result.width = width;
	result.height = height;
	result.format = format;
	if (stride) {
		ValidateStride(result, stride);
		result.stride = stride;
		result.autoStride = false;
	} else {
		result.stride = result.GetNeededStride();
		result.autoStride = true;
	}
	return result;
}

Image Image::SubImage(i32 x, i32 y, i32 w, i32 h) const {
	AzAssert(pixels, "Can't get a subimage with no pixels");
	AzAssert(x >= 0, Stringify("SubImage x (", x, ") is out of bounds (0 to ", width, ")"));
	AzAssert(y >= 0, Stringify("SubImage y (", y, ") is out of bounds (0 to ", height, ")"));
	AzAssert(x+w <= width, Stringify("SubImage x+w (", x, "+", w, "=", x+w, ") is out of bounds (0 to ", width, ")"));
	AzAssert(y+h <= height, Stringify("SubImage y+h (", y, "+", h, "=", y+h, ") is out of bounds (0 to ", height, ")"));
	Image result;
	result.pixels = &pixels[x * GetBytesPerPixel() + y * stride];
	result.stride = stride;
	result.width = w;
	result.height = h;
	result.format = format;
	return result;
}

} // namespace AzCore
