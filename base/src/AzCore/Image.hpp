/*
	File: Image.hpp
	Author: Philip Haynes
	A shallow wrapper around stb_image.h
*/

#ifndef AZCORE_IMAGE_HPP
#define AZCORE_IMAGE_HPP

#include "BasicTypes.hpp"
#include "Memory/String.hpp"
#include "Memory/Any.hpp"
#include <utility>

namespace AzCore {

/*  struct: Image
	Author: Philip Haynes
	Allows loading and saving image files in many file formats, courtesy of stb_image */
struct Image {
	u8* pixels = nullptr;
	u64 pixelsCapacity = 0; // Size in bytes of the buffer pointed to by pixels. Should be 0 if we don't own it.
	i32 stride = 0;  // Line stride in bytes (if you want to specify it manually, set autoStride to false)
	i32 width = 0;   // Image width in pixels
	i32 height = 0;  // Image height in pixels
	u8 strideAlignment = 1; // alignment of stride in bytes (you probably want bitdepth/8, but you can also specify a greater alignment if you need)
	bool autoStride = true; // if this is true, calculates the stride based on width and strideAlignment
	inline i32 GetBytesPerPixel() const {
		return format.channels*(format.bitdepth>>3);
	}
	inline i32 GetNeededStride() const {
		return align(width*GetBytesPerPixel(), strideAlignment);
	}
	// Assumes stride has been set
	inline u64 GetTotalSizeBytes() const {
		return stride * height * GetBytesPerPixel();
	}
	struct Format {
		u8 channels = 4; // How many channels per pixel
		u8 bitdepth = 8; // Bits per channel (currently expects a multiple of 8 for simplicity)
		enum DataType : u8 {
			UNORM,  // Integer values represent values from 0 to 1 inclusive
			SFLOAT, // Signed float
		} dataType=UNORM;
		enum ChannelOrder : u8 {
			RGBA,
			BGRA,
		} channelOrder=RGBA;
		enum ColorSpace : u8 {
			SRGB, // Only valid for 8-bit color depth, and doesn't apply to the alpha channel.
			LINEAR,
		} colorSpace=SRGB;
		// TODO: Standardized texture compression
		inline Format WithChannels(u8 newChannels) const {
			Format newFormat = *this;
			newFormat.channels = newChannels;
			return newFormat;
		}
		inline Format WithBitdepth(u8 newBitdepth) const {
			AzAssert((newBitdepth & (8-1)) == 0, "Expected newBitdepth to be a multiple of 8");
			Format newFormat = *this;
			newFormat.bitdepth = newBitdepth;
			return newFormat;
		}
		inline Format WithDataType(DataType newDataType) const {
			Format newFormat = *this;
			newFormat.dataType = newDataType;
			return newFormat;
		}
		inline Format WithChannelOrder(ChannelOrder newChannelOrder) const {
			Format newFormat = *this;
			newFormat.channelOrder = newChannelOrder;
			return newFormat;
		}
		inline Format WithColorSpace(ColorSpace newColorSpace) const {
			AzAssert(!(newColorSpace == SRGB && bitdepth != 8), Stringify("SRGB is only valid for bitdepths of 8 (bitdepth is ", bitdepth, ")"));
			Format newFormat = *this;
			newFormat.colorSpace = newColorSpace;
			return newFormat;
		}
		inline bool ChannelOrderMatches(const Format &other) const {
			// Channel order only matters if you have at least 3 channels
			return channels < 3 || channelOrder == other.channelOrder;
		}
		inline bool ColorSpaceMatches(const Format &other) const {
			// colorSpace only matters if you have a bitdepth of 8
			return bitdepth != 8 || colorSpace == other.colorSpace;
		}
		inline bool operator==(const Format &other) const {
			return channels == other.channels
			    && bitdepth == other.bitdepth
			    && dataType == other.dataType
			    && ChannelOrderMatches(other)
			    && ColorSpaceMatches(other);
		}
	} format;
	inline void _CopyProperties(const Image &other) {
		stride = other.stride;
		width = other.width;
		height = other.height;
		format = other.format;
	}
	inline void _Acquire(Image &&other) {
		pixels = other.pixels;
		pixelsCapacity = other.pixelsCapacity;
		other.pixelsCapacity = 0;
		_CopyProperties(other);
	}
	inline void _PointTo(Image &other) {
		pixels = other.pixels;
		pixelsCapacity = 0;
		_CopyProperties(other);
	}
	// Will preserve our owned memory if it can fit the contents of other
	void _Copy(const Image &other);
	Image() = default;
	// Makes an owned copy
	inline Image(const Image &other) {
		_Copy(other);
	}
	inline Image(Image *other) {
		_PointTo(*other);
	}
	inline Image(Image &&other) {
		_Acquire(std::move(other));
	}
	// Calls LoadFromFile
	inline Image(const char *filename, i32 channelsDesired = 0) {
		LoadFromFile(filename, channelsDesired);
	}
	// Calls LoadFromBuffer
	inline Image(Str buffer, i32 channelsDesired = 0) {
		LoadFromBuffer(buffer, channelsDesired);
	}
	~Image();
	inline Image& operator=(const Image &other) {
		if (this == &other) return *this;
		_Copy(other);
		return *this;
	}
	inline Image& operator=(Image *other) {
		if (this == other) return *this;
		Dealloc();
		_PointTo(*other);
		return *this;
	}
	inline Image& operator=(Image &&other) {
		if (this == &other) return *this;
		Dealloc();
		_Acquire(std::move(other));
		return *this;
	}
	void Alloc();
	inline void Alloc(i32 w, i32 h, u8 c, i32 s=0) {
		width = w;
		height = h;
		format.channels = c;
		if (s) {
			stride = s;
			autoStride = false;
		} else {
			stride = 0;
			autoStride = true;
		}
		Alloc();
	}
	void Dealloc();
	// Expects other to have the same dimensions as we do. Can do format conversions. If src image has fewer channels, uses defaultValue to set the rest of the channels.
	void Copy(const Image &other, Any defaultValue=None);
	bool LoadFromFile(const char *filename, i32 channelsDesired = 0);
	// Decodes the file stored in buffer
	bool LoadFromBuffer(Str buffer, i32 channelsDesired = 0);
	// If increasing the number of channels, defaultValue (which must have a type with the same bitdepth and dataType as newFormat) specifies what to initialize the new channels, else they're uninitialized.
	void Reformat(Format newFormat, Any defaultValue=None);
	// Reformats the existing image. For new, unallocated images, just set the field directly before allocating instead.
	// If increasing the number of channels, defaultValue (which must have a type with the same bitdepth and dataType as newFormat) specifies what to initialize the new channels, else they're uninitialized.
	inline void SetChannels(u8 newChannels, Any defaultValue=None) {
		Reformat(format.WithChannels(newChannels), defaultValue);
	}
	// Reformats the existing image. For new, unallocated images, just set the field directly before allocating instead.
	inline void SetBitdepth(u8 newBitdepth) {
		Reformat(format.WithBitdepth(newBitdepth));
	}
	// Reformats the existing image. For new, unallocated images, just set the field directly before allocating instead.
	inline void SetDataType(Format::DataType newDataType) {
		Reformat(format.WithDataType(newDataType));
	}
	// Reformats the existing image. For new, unallocated images, just set the field directly before allocating instead.
	inline void SetChannelOrder(Format::ChannelOrder newChannelOrder) {
		Reformat(format.WithChannelOrder(newChannelOrder));
	}
	// Reformats the existing image. For new, unallocated images, just set the field directly before allocating instead.
	inline void SetColorSpace(Format::ColorSpace newColorSpace) {
		Reformat(format.WithColorSpace(newColorSpace));
	}
	bool SavePNG(const char *filename);
	void PremultiplyAlpha();
	// Makes an image that points to an existing pixel buffer in memory
	static Image PointingToBuffer(u8 *buffer, i32 width, i32 height, Format format, i32 stride=0);
	// Creates an image that points to a subsection of our image
	Image SubImage(i32 x, i32 y, i32 w, i32 h) const;
};

} // namespace AzCore

#endif // AZCORE_IMAGE_HPP
