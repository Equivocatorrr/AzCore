/*
	File: assets.cpp
	Author: Philip Haynes
*/

#include "assets.hpp"
#include "globals.hpp"

#include "AzCore/IO/Log.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <stb_vorbis.c>

namespace Assets {

io::Log cout("assets.log");

String error = "No error.";

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

	if (4 >= filename.size) {
		return Type::NONE;
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

void Mapping::SetFilename(String name) {
	filename = std::move(name);
	checkSum = CheckSum(filename);
}

bool Mapping::FilenameEquals(String name, u32 sum) {
	if (checkSum == sum) {
		return filename == name;
	}
	return false;
}

u32 Mapping::CheckSum(String name) {
	u32 checkSum = 0;
	for (i32 i = 0; i < name.size; i++) {
		checkSum += (name[i] << (i%4)*8);
	}
	return checkSum;
}

bool Texture::Load(String filename) {
	filename = "data/textures/" + filename;
	pixels.data = stbi_load(filename.data, &width, &height, &channels, 4);
	if (pixels.data == nullptr) {
		error = "Failed to load Texture file: \"" + filename + "\"";
		return false;
	}
	channels = 4;
	pixels.allocated = width * height* channels;
	pixels.size = pixels.allocated;
	return true;
}

bool Font::Load(String filename) {
	font.filename = "data/fonts/" + filename;
	if (!font.Load()) {
		error = "Failed to load font: " + font::error;
		return false;
	}
	fontBuilder.font = &font;
	fontBuilder.AddRange(0, 128);
	if (!fontBuilder.Build()) {
		error = "Failed to load font: " + font::error;
		return false;
	}
	return true;
}

void Font::SaveAtlas() {
	cout.PrintLn("Saving png of font ", font.filename);
	stbi_write_png((font.filename + ".png").data, fontBuilder.dimensions.x, fontBuilder.dimensions.y, 1, fontBuilder.pixels.data, fontBuilder.dimensions.x);
}

bool Sound::Load(String filename) {
	filename = "data/sound/" + filename;
	if (!buffer.Create()) {
		error = "Sound::Load: Failed to create buffer: " + ::Sound::error;
		return false;
	}
	valid = true;
	i16 *decoded;
	i32 channels, samplerate, length;
	length = stb_vorbis_decode_filename(filename.data, &channels, &samplerate, &decoded);
	if (length <= 0) {
		error = "Failed to decode sound file (" + filename + ")";
		return false;
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
		error = "Sound::Load: Failed to load buffer: " + ::Sound::error
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
			cout.PrintLn("Failed to clean Sound buffer: ", ::Sound::error);
		}
	}
}

bool Stream::Open(String filename) {
	filename = "data/sound/" + filename;
	for (i32 i = 0; i < numStreamBuffers; i++) {
		if (!buffers[i].Create()) {
			error = "Stream::Open: Failed to create buffer: " + ::Sound::error;
			return false;
		}
	}
	i32 iError = 0;
	vorbis = stb_vorbis_open_filename(filename.data, &iError, nullptr);
	if (!vorbis) {
		error = "Stream::Open: Failed to open \"" + filename + "\", error code " + ToString(iError);
		return false;
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
			sampleCount = data.loopEndSample - data.cursorSample;
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

	::Sound::Buffer &buffer = buffers[(i32)data.currentBuffer];
	ALenum format = data.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	if (!buffer.Load(samples.data, format, length * 2 * data.channels, data.samplerate)) {
		error = "Stream::Decode: Failed to load buffer: " + ::Sound::error
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
		for (i32 i = 0; i < numStreamBuffers; i++) {
			if (!buffers[i].Clean()) {
				cout.PrintLn("Failed to clean Stream buffer: ", ::Sound::error);
			}
		}
	}
}

bool Manager::LoadAll() {
	for (i32 i = 0; i < filesToLoad.size; i++) {
		cout.Print("Loading asset \"", filesToLoad[i].filename, "\": ");
		Type type;
		if (filesToLoad[i].type == NONE) {
			type = FilenameToType(filesToLoad[i].filename);
		} else {
			type = filesToLoad[i].type;
		}
		i32 nextTexIndex = textures.size;
		i32 nextFontIndex = fonts.size;
		i32 nextSoundIndex = sounds.size;
		i32 nextStreamIndex = streams.size;
		Mapping mapping;
		mapping.type = type;
		switch (type) {
		case NONE:
			cout.PrintLn("Unknown file type.");
			continue;
		case FONT:
			cout.PrintLn("as font.");
			fonts.Append(Font());
			fonts[nextFontIndex].fontBuilder.resolution = font::FontBuilder::HIGH;
			if (!fonts[nextFontIndex].Load(filesToLoad[i].filename)) {
				return false;
			}
			mapping.index = nextFontIndex;
			break;
		case TEXTURE:
			cout.PrintLn("as texture.");
			textures.Append(Texture());
			if (!textures[nextTexIndex].Load(filesToLoad[i].filename)) {
				return false;
			}
			mapping.index = nextTexIndex;
			break;
		case SOUND:
			cout.PrintLn("as sound.");
			sounds.Append(Sound());
			if (!sounds[nextSoundIndex].Load(filesToLoad[i].filename)) {
				return false;
			}
			mapping.index = nextSoundIndex;
			break;
		case STREAM:
			cout.PrintLn("as stream.");
			streams.Append(Stream());
			if (!streams[nextStreamIndex].Open(filesToLoad[i].filename)) {
				return false;
			}
			mapping.index = nextStreamIndex;
			break;
		}
		mapping.SetFilename(filesToLoad[i].filename);
		mappings.Append(std::move(mapping));
	}
	return true;
}

i32 Manager::FindMapping(String filename) {
	i32 checkSum = Mapping::CheckSum(filename);
	for (Mapping& mapping : mappings) {
		if (mapping.FilenameEquals(filename, checkSum)) {
			return mapping.index;
		}
	}
	cout.PrintLn("No mapping found for \"", filename, "\"");
	return 0;
}

f32 Manager::CharacterWidth(char32 c, i32 fontIndex) const {
	return globals->rendering.CharacterWidth(c, &fonts[fontIndex], &fonts[0]);
}

}
