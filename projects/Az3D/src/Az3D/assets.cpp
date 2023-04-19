/*
	File: assets.cpp
	Author: Philip Haynes
*/

#include "assets.hpp"
#include "game_systems.hpp"
#include "profiling.hpp"

#include "AzCore/IO/Log.hpp"
#include "AzCore/Simd.hpp"

#include <stb_image.h>
#include <stb_vorbis.c>

namespace Az3D::Assets {

using namespace AzCore;

bool warnFileNotFound = true;

io::Log cout("assets.log");

String error = "No error.";

#define FILE_NOT_FOUND(...) if (warnFileNotFound) {\
	cout.PrintLn(__VA_ARGS__);\
}

const char *typeStrings[6] = {
	"None",
	"Texture",
	"Font",
	"Sound",
	"Stream",
	"Mesh",
};

Type FilenameToType(String filename) {
	const char *texExtensions[] = {
		".tga",
		".png",
		".jpg",
		".jpeg",
		".bmp",
		".hdr"
	};
	const char *fontExtensions[] = {
		".ttf",
		".otf",
		".ttc"
	};
	const char *soundExtensions[] = {
		".ogg"
	};
	const char *meshExtensions[] = {
		".az3d"
	};

	if (4 >= filename.size) {
		return Type::NONE;
	}

	for (const char *ext : meshExtensions) {
		const i32 len = 5;
		bool fnd = true;
		for (i32 i = 0; i < len; i++) {
			if (ext[i] != filename[filename.size-len+i]) {
				fnd = false;
				break;
			}
		}
		if (fnd) {
			return Type::MESH;
		}
	}
	for (const char *ext : soundExtensions) {
		const i32 len = 4;
		bool fnd = true;
		for (i32 i = 0; i < len; i++) {
			if (ext[i] != filename[filename.size-len+i]) {
				fnd = false;
				break;
			}
		}
		if (fnd) {
			return Type::SOUND;
		}
	}
	for (const char *ext : fontExtensions) {
		const i32 len = 4;
		bool fnd = true;
		for (i32 i = 0; i < len; i++) {
			if (ext[i] != filename[filename.size-len+i]) {
				fnd = false;
				break;
			}
		}
		if (fnd) {
			return Type::FONT;
		}
	}
	for (const char *ext : texExtensions) {
		const i32 len = StringLength(ext);
		if (len >= filename.size) {
			return Type::NONE;
		}
		bool fnd = true;
		for (i32 i = 0; i < len; i++) {
			if (ext[i] != filename[filename.size-len+i]) {
				fnd = false;
				break;
			}
		}
		if (fnd) {
			return Type::TEXTURE;
		}
	}
	return Type::NONE;
}

void Texture::PremultiplyAlpha() {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Assets::Texture::PremultiplyAlpha)
	AzAssert(image.channels == 4, "Image must have 4 channels for PremultiplyAlpha");
	i32 i = 0;
	static __m256i alphaMask = _mm256_set_epi16(
		0xff, 0, 0, 0,
		0xff, 0, 0, 0,
		0xff, 0, 0, 0,
		0xff, 0, 0, 0
	);
	AzAssert(((u64)image.pixels & 15) == 0, "We're expecting the pixel array to be aligned on a 16-byte boundary");
	// Premultiply alpha
#if 1
	for (; i <= image.width*image.height-4; i+=4) {
		u8 *pixel = &image.pixels[i*image.channels];
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
	for (; i < image.width*image.height; i++) {
		u32 &pixel = *((u32*)&image.pixels[i*image.channels]);
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

bool Texture::Load(String filename) {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Assets::Texture::Load)
	String path = Stringify("data/textures/", filename);
	if (!image.Load(path.data)) {
		path = Stringify("data/Az3D/textures/", filename);
		if (!image.Load(path.data)) {
			FILE_NOT_FOUND("Failed to load Texture file: \"", filename, "\"");
			return false;
		}
	}
	if (image.channels == 4) {
		// Only multiply alpha if we actually had an alpha channel in the first place
		PremultiplyAlpha();
	}
	if (image.channels == 3) {
		image.SetChannels(4);
	}
	return true;
}


bool Font::Load(String filename) {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Assets::Font::Load)
	font.filename = "data/fonts/" + filename;
	if (!font.Load()) {
		font.filename = "data/Az3D/fonts/" + filename;
		if (!font.Load()) {
			error = "Failed to load font: " + font::error;
			return false;
		}
	}
	fontBuilder.font = &font;
	fontBuilder.AddRange(0, 128);
	if (!fontBuilder.Build()) {
		error = "Failed to load font: " + font::error;
		return false;
	}
	return true;
}

bool Sound::Load(String filename) {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Assets::Sound::Load)
	String path = Stringify("data/sound/", filename);
	if (!buffer.Create()) {
		error = "Sound::Load: Failed to create buffer: " + Az3D::Sound::error;
		return false;
	}
	valid = true;
	i16 *decoded;
	i32 channels, samplerate, length;
	length = stb_vorbis_decode_filename(path.data, &channels, &samplerate, &decoded);
	if (length <= 0) {
		path = Stringify("data/Az3D/sound/", filename);
		length = stb_vorbis_decode_filename(path.data, &channels, &samplerate, &decoded);
		if (length <= 0) {
			FILE_NOT_FOUND("Failed to decode sound file (", filename, ")");
			return false;
		}
	}
	if (!decoded) {
		error = "Decoded is nullptr!";
		return false;
	}
	if (channels > 2 || channels < 1) {
		error = "Unsupported number of channels in sound file (" + filename + "): " + ToString(channels);
		free(decoded);
		return false;
	}
	if (!buffer.Load(decoded, channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, length * 2 * channels, samplerate)) {
		error = "Sound::Load: Failed to load buffer: " + Az3D::Sound::error
			+ " channels=" + ToString(channels) + " length=" + ToString(length)
			+ " samplerate=" + ToString(samplerate) + " bufferid=" + ToString(buffer.buffer) + " &decoded=0x" + ToString((i64)decoded, 16);
		free(decoded);
		return false;
	}
	free(decoded);
	return true;
}

Sound::~Sound() {
	if (valid) {
		if (!buffer.Clean()) {
			cout.PrintLn("Failed to clean Sound buffer: ", Az3D::Sound::error);
		}
	}
}

bool Stream::Open(String filename) {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Assets::Stream::Open)
	String path = Stringify("data/sound/", filename);
	for (i32 i = 0; i < numStreamBuffers; i++) {
		if (!buffers[i].Create()) {
			error = "Stream::Open: Failed to create buffer: " + Az3D::Sound::error;
			return false;
		}
	}
	i32 iError = 0;
	vorbis = stb_vorbis_open_filename(path.data, &iError, nullptr);
	if (!vorbis) {
		path = Stringify("data/Az3D/sound/", filename);
		vorbis = stb_vorbis_open_filename(path.data, &iError, nullptr);
		if (!vorbis) {
			FILE_NOT_FOUND("Stream::Open: Failed to open \"", filename, "\", error code ", iError);
			return false;
		}
	}
	data.totalSamples = stb_vorbis_stream_length_in_samples(vorbis);
	stb_vorbis_info info = stb_vorbis_get_info(vorbis);
	data.channels = info.channels;
	data.samplerate = info.sample_rate;
	if (data.channels > 2 || data.channels < 1) {
		error = "Unsupported number of channels in sound file (" + filename + "): "
			  + ToString(data.channels);
		stb_vorbis_close(vorbis);
		return false;
	}
	valid = true;
	return true;
}

constexpr i32 crossfadeSamples = 2205;

i32 Stream::Decode(i32 sampleCount) {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Assets::Stream::Decode)
	if (!valid) {
		error = "Stream::Decode: Stream not valid!";
		return -1;
	}
	Array<i16> samples(sampleCount * data.channels);
	i32 length; // How many samples were decoded
	if (data.loopEndSample <= 0) {
		if (data.cursorSample >= data.totalSamples) {
			SeekStart();
			return 0;
		}
		length =
		stb_vorbis_get_samples_short_interleaved(vorbis, data.channels, samples.data, samples.size);
		data.cursorSample += length;
	} else {
		if (data.cursorSample + crossfadeSamples + sampleCount >= data.loopEndSample) {
			// Don't go past the loop point
			sampleCount = max(data.loopEndSample - data.cursorSample, crossfadeSamples);
			samples.Resize(sampleCount*data.channels);
			length =
			stb_vorbis_get_samples_short_interleaved(vorbis, data.channels, samples.data, samples.size);
			Array<i16> crossfade(crossfadeSamples * data.channels);
			if (data.loopBeginSample > crossfadeSamples) {
				// crossfade can be actual audio
				stb_vorbis_seek(vorbis, data.loopBeginSample-crossfadeSamples);
				stb_vorbis_get_samples_short_interleaved(vorbis, data.channels, crossfade.data, crossfade.size);
			} else if (data.loopBeginSample > 0) {
				// crossfadeSamples > loopBeginSample
				// some of the crossfade is audio
				memset(crossfade.data, 0, crossfade.size * sizeof(i16));
				stb_vorbis_seek_start(vorbis);
				stb_vorbis_get_samples_short_interleaved(vorbis, data.channels, crossfade.data + (crossfadeSamples - data.loopBeginSample) * data.channels, data.loopBeginSample * data.channels);
			} else {
				// crossfade is silence
				memset(crossfade.data, 0, crossfade.size * sizeof(i16));
				stb_vorbis_seek_start(vorbis);
			}
			// I'll do a linear crossfade for now
			for (i32 i = 0; i < crossfadeSamples; i++) {
				for (i32 c = 0; c < data.channels; c++) {
					i16 &sample1 = samples[(sampleCount-crossfadeSamples+i) * data.channels + c];
					i16 &sample2 = crossfade[i * data.channels + c];
					f32 s = lerp((f32)sample1, (f32)sample2, f32(i+1) / f32(crossfadeSamples+1));
					sample1 = (i16)s;
				}
			}
			data.cursorSample = data.loopBeginSample;
		} else {
			length =
			stb_vorbis_get_samples_short_interleaved(vorbis, data.channels, samples.data, samples.size);
			data.cursorSample += length;
		}
	}

	if (data.fadeoutSamples > 0) {
		if (data.fadeoutCompleted >= data.fadeoutSamples) {
			memset(samples.data, 0, samples.size * sizeof(i16));
			data.fadeoutSamples = -1;
		} else {
			for (i32 i = 0; i < length; i++) {
				for (i32 c = 0; c < data.channels; c++) {
					i16 &sample = samples[i*data.channels + c];
					f32 fadePos = f32(i + data.fadeoutCompleted);
					fadePos = min(fadePos / (f32)data.fadeoutSamples, 1.0f);
					f32 s = ease<2>((f32)sample, 0.0f, pow(fadePos, 2.0f/3.0f));
					sample = (i16)s;
				}
			}
			data.fadeoutCompleted += length;
		}
	}

	Az3D::Sound::Buffer &buffer = buffers[(i32)data.currentBuffer];
	ALenum format = data.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	if (!buffer.Load(samples.data, format, length * 2 * data.channels, data.samplerate)) {
		error = "Stream::Decode: Failed to load buffer: " + Az3D::Sound::error
			+ " channels=" + ToString(data.channels) + " length=" + ToString(length)
			+ " samplerate=" + ToString(data.samplerate) + " bufferid=" + ToString(buffer.buffer)
			+ " &decoded=0x" + ToString((i64)samples.data, 16);
		return -1;
	}
	data.lastBuffer = data.currentBuffer;
	data.currentBuffer = (data.currentBuffer + 1) % numStreamBuffers;
	return length;
}

void Stream::SeekStart() {
	data.cursorSample = 0;
	stb_vorbis_seek_start(vorbis);
}

ALuint Stream::LastBuffer() {
	return buffers[(i32)data.lastBuffer].buffer;
}

bool Stream::Close() {
	if (!valid) {
		error = "Stream::Close: Stream not valid!";
		return false;
	}
	stb_vorbis_close(vorbis);
	return true;
}

Stream::~Stream() {
	if (valid) {
		Close();
		for (i32 i = 0; i < numStreamBuffers; i++) {
			if (!buffers[i].Clean()) {
				cout.PrintLn("Failed to clean Stream buffer: ", Az3D::Sound::error);
			}
		}
	}
}

bool Mesh::Load(String filename) {
	AZ3D_PROFILING_FUNC_TIMER()
	String path = Stringify("data/models/", filename);
	Az3DObj::File file;
	if (!file.Load(path)) return false;
	// Use this to offset material texture indices
	i32 texOffset = GameSystems::sys->assets.textures.size - 1;
	for (Az3DObj::Mesh &meshData : file.meshes) {
		UniquePtr<MeshPart> meshPart;
		meshPart->name = std::move(meshData.name);
		meshPart->vertices = std::move(meshData.vertices);
		meshPart->indices = std::move(meshData.indices);
		meshPart->material.color = meshData.material.color;
		meshPart->material.emit = meshData.material.emit;
		meshPart->material.normal = meshData.material.normal;
		meshPart->material.metalness = meshData.material.metalness;
		meshPart->material.roughness = meshData.material.roughness;
		meshPart->material.sssFactor = meshData.material.sssFactor;
		meshPart->material.sssColor = meshData.material.sssColor;
		meshPart->material.sssRadius = meshData.material.sssRadius;
		meshPart->material.isFoliage = meshData.material.isFoliage;
		for (i32 i = 0; i < 5; i++) {
			if (meshData.material.tex[i] == 0) {
				meshPart->material.tex[i] = i == 2 ? texIndexBlankNormal : texIndexBlank;
			} else {
				meshPart->material.tex[i] = meshData.material.tex[i] + texOffset;
			}
		}
		parts.Append(GameSystems::sys->assets.meshParts.Append(std::move(meshPart)).RawPtr());
	}
	for (az::Image &image : file.images) {
		Texture &texture = GameSystems::sys->assets.textures.Append(Texture());
		texture.image = std::move(image);
		if (texture.image.channels == 4) {
			texture.PremultiplyAlpha();
		}
		if (texture.image.channels == 3) {
			texture.image.SetChannels(4);
		}
	}
	return true;
}

bool Manager::LoadAll() {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Assets::Manager::LoadAll)
	for (i32 i = 0; i < filesToLoad.size; i++) {
		cout.Print("Loading asset \"", filesToLoad[i].filename, "\": ");
		Type type;
		if (filesToLoad[i].type == Type::NONE) {
			type = FilenameToType(filesToLoad[i].filename);
		} else {
			type = filesToLoad[i].type;
		}
		i32 nextTexIndex = textures.size;
		i32 nextFontIndex = fonts.size;
		i32 nextSoundIndex = sounds.size;
		i32 nextStreamIndex = streams.size;
		i32 nextMeshIndex = meshes.size;
		Mapping mapping;
		mapping.type = type;
		switch (type) {
		default:
		case Type::NONE:
			cout.PrintLn("Unknown file type.");
			continue;
		case Type::FONT:
			cout.PrintLn("as font.");
			fonts.Append(Font());
			fonts[nextFontIndex].fontBuilder.resolution = font::FontBuilder::HIGH;
			if (!fonts[nextFontIndex].Load(filesToLoad[i].filename)) {
				return false;
			}
			mapping.index = nextFontIndex;
			break;
		case Type::TEXTURE:
			cout.PrintLn("as texture.");
			textures.Append(Texture());
			if (!textures[nextTexIndex].Load(filesToLoad[i].filename)) {
				textures.size--;
				continue;
			}
			textures.Back().image.colorSpace = filesToLoad[i].isLinearTexture ? Image::LINEAR : Image::SRGB;
			mapping.index = nextTexIndex;
			break;
		case Type::SOUND:
			cout.PrintLn("as sound.");
			sounds.Append(Sound());
			if (!sounds[nextSoundIndex].Load(filesToLoad[i].filename)) {
				sounds.size--;
				continue;
			}
			mapping.index = nextSoundIndex;
			break;
		case Type::STREAM:
			cout.PrintLn("as stream.");
			streams.Append(Stream());
			if (!streams[nextStreamIndex].Open(filesToLoad[i].filename)) {
				streams.size--;
				continue;
			}
			mapping.index = nextStreamIndex;
			break;
		case Type::MESH:
			cout.PrintLn("as mesh.");
			meshes.Append(Mesh());
			if (!meshes[nextMeshIndex].Load(filesToLoad[i].filename)) {
				meshes.size--;
				continue;
			}
			mapping.index = nextMeshIndex;
			break;
		}
		mappings.Emplace(filesToLoad[i].filename, mapping);
	}
	return true;
}

i32 Manager::FindMapping(SimpleRange<char> filename, Type type) {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Assets::Manager::FindMapping)
	auto *node = mappings.Find(filename);
	if (node == nullptr) {
		FILE_NOT_FOUND("No mapping found for \"", filename, "\"");
		return 0;
	}
	Mapping &mapping = node->value;
	if (mapping.type != type) {
		cout.PrintLn("\"", filename, "\" is not a ", typeStrings[(u32)type], "!");
		return 0;
	}
	return mapping.index;
}

f32 Manager::CharacterWidth(char32 c, i32 fontIndex) const {
	return GameSystems::sys->rendering.CharacterWidth(c, &fonts[fontIndex], &fonts[0]);
}

}
