/*
	File: assets.cpp
	Author: Philip Haynes
*/

#include "assets.hpp"
#include "game_systems.hpp"
#include "AzCore/Profiling.hpp"

#include "AzCore/IO/Log.hpp"

#include <stb_vorbis.c>

namespace Az2D::Assets {

using namespace AzCore;

io::Log cout("assets.log");

String error = "No error.";

const char *typeStrings[5] = {
	"None",
	"Texture",
	"Font",
	"Sound",
	"Stream",
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

void Texture::Decode() {
	AZCORE_PROFILING_FUNC_TIMER()
	image.LoadFromBuffer(file->data);
	if (image.channels == 4) {
		// Only multiply alpha if we actually had an alpha channel in the first place
		image.PremultiplyAlpha();
	}
	if (image.channels == 3) {
		image.SetChannels(4);
	}
}

void Font::Decode() {
	AZCORE_PROFILING_FUNC_TIMER()
	font.LoadFromBuffer(std::move(file->data));
	fontBuilder.font = &font;
	fontBuilder.AddRange(0, 128);
	fontBuilder.Build();
}

void Sound::Decode() {
	AZCORE_PROFILING_FUNC_TIMER()
	valid = false;
	if (!buffer.Create()) {
		cout.PrintLn("Sound::Decode: Failed to create buffer: ", Az2D::Sound::error);
		return;
	}
	i16 *decoded;
	i32 channels, samplerate, length;
	length = stb_vorbis_decode_memory((u8*)file->data.data, file->data.size, &channels, &samplerate, &decoded);
	if (length <= 0) {
		cout.PrintLn("Failed to decode sound file (", file->filepath, ")");
		return;
	}
	if (nullptr == decoded) {
		cout.PrintLn("Decoded is nullptr!");
		return;
	}
	if (channels > 2 || channels < 1) {
		cout.PrintLn("Unsupported number of channels in sound file (", file->filepath, "): ", channels);
		free(decoded);
		return;
	}
	if (!buffer.Load(decoded, channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, length * 2 * channels, samplerate)) {
		cout.PrintLn("Sound::Load: Failed to load buffer: ", Az2D::Sound::error, " channels=",channels, " length=", length, " samplerate=", samplerate, " bufferid=", buffer.buffer, " &decoded=0x", FormatInt((i64)decoded, 16));
		free(decoded);
		return;
	}
	valid = true;
	free(decoded);
}

Sound::~Sound() {
	if (valid) {
		if (!buffer.Clean()) {
			cout.PrintLn("Failed to clean Sound buffer: ", Az2D::Sound::error);
		}
	}
}

void Stream::Decode() {
	AZCORE_PROFILING_FUNC_TIMER()
	valid = false;
	for (i32 i = 0; i < numStreamBuffers; i++) {
		if (!buffers[i].Create()) {
			cout.PrintLn("Stream::Decode: Failed to create buffer: ", Az2D::Sound::error);
			return;
		}
	}
	i32 iError = 0;
	vorbis = stb_vorbis_open_memory((u8*)file->data.data, file->data.size, &iError, nullptr);
	if (!vorbis) {
		cout.PrintLn("Stream::Decode: Failed to decode \"", file->filepath, "\", error code ", iError);
		return;
	}
	data.totalSamples = stb_vorbis_stream_length_in_samples(vorbis);
	stb_vorbis_info info = stb_vorbis_get_info(vorbis);
	data.channels = info.channels;
	data.samplerate = info.sample_rate;
	if (data.channels > 2 || data.channels < 1) {
		cout.PrintLn("Unsupported number of channels in sound file (", file->filepath, "): ", data.channels);
		stb_vorbis_close(vorbis);
		return;
	}
	valid = true;
	return;
}

constexpr i32 crossfadeSamples = 2205;

i32 Stream::DecodeSamples(i32 sampleCount) {
	AZCORE_PROFILING_FUNC_TIMER()
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

	Az2D::Sound::Buffer &buffer = buffers[(i32)data.currentBuffer];
	ALenum format = data.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	if (!buffer.Load(samples.data, format, length * 2 * data.channels, data.samplerate)) {
		error = "Stream::Decode: Failed to load buffer: " + Az2D::Sound::error
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
				cout.PrintLn("Failed to clean Stream buffer: ", Az2D::Sound::error);
			}
		}
	}
}

void Manager::Init() {
	fileManager.Init();
	fileManager.searchDirectories = {
		"data/",
		"data/Az2D/",
	};
	mappings.Clear();
	textures.Clear();
	fonts.Clear();
	sounds.Clear();
	streams.Clear();
	nextTexIndex = 0;
	nextFontIndex = 0;
	nextSoundIndex = 0;
	nextStreamIndex = 0;
#ifndef NDEBUG
	fileManager.warnFileNotFound = true;
#endif
	
	RequestTexture("TextureMissing.png", false);
	RequestTexture("blank.tga", false);
	RequestTexture("blank_n.tga", true);
	RequestTexture("blank_e.tga", true);
	RequestFont("DroidSansFallback.ttf");
}

void Manager::Deinit() {
	// Delete the OpenAL buffers
	sounds.Clear();
	streams.Clear();
	fileManager.Deinit();
}

struct TextureDecodeMetadata {
	TexIndex texIndex;
	Array<Texture> *dstArray;
	Mutex *dstArrayMutex;
	bool linear;
};

bool textureDecoder(io::File *file, Any &any) {
	Texture texture;
	texture.file = file;
	texture.Decode();
	TextureDecodeMetadata &metadata = any.Get<TextureDecodeMetadata>();
	texture.image.colorSpace = metadata.linear ? Image::LINEAR : Image::SRGB;
	metadata.dstArrayMutex->Lock();
	metadata.dstArray->Resize(max(metadata.texIndex+1, metadata.dstArray->size));
	(*metadata.dstArray)[metadata.texIndex] = std::move(texture);
	metadata.dstArrayMutex->Unlock();
	return false;
};

TexIndex Manager::RequestTexture(az::String filepath, bool linear, i32 priority) {
	if (auto node = mappings.Find(filepath)) {
		AzAssert(node->value.type == Type::TEXTURE, Stringify("RequestTexture for \"", filepath, "\" already exists as a ", typeStrings[(i32)node->value.type]));
		return node->value.index;
	}
	ScopedLock lock(arrayMutex);
	TexIndex result = nextTexIndex++;
	mappings.Emplace(filepath, Mapping{Type::TEXTURE, result});
	textures.Resize(max(result+1, textures.size));
	Texture &texture = textures[result];
	filepath = Stringify("textures/", filepath);
	texture.file = fileManager.RequestFile(filepath, priority, textureDecoder, TextureDecodeMetadata{result, &textures, &arrayMutex, linear});
	return result;
}

TexIndex Manager::RequestTextureDecode(Array<char> &&buffer, az::String filepath, bool linear, i32 priority, bool lock) {
	if (lock) arrayMutex.Lock();
	TexIndex result = nextTexIndex++;
	mappings.Emplace(filepath, Mapping{Type::TEXTURE, result});
	textures.Resize(max(result+1, textures.size));
	Texture &texture = textures[result];
	texture.file = fileManager.RequestDecode(std::move(buffer), filepath, priority, textureDecoder, TextureDecodeMetadata{result, &textures, &arrayMutex, linear});
	if (lock) arrayMutex.Unlock();
	return result;
}

FontIndex Manager::RequestFont(az::String filepath, i32 priority) {
	struct FontDecodeMetadata {
		FontIndex fontIndex;
		Array<Font> *dstArray;
		Mutex *dstArrayMutex;
	};
	if (auto node = mappings.Find(filepath)) {
		AzAssert(node->value.type == Type::FONT, Stringify("RequestFont for \"", filepath, "\" already exists as a ", typeStrings[(i32)node->value.type]));
		return node->value.index;
	}
	ScopedLock lock(arrayMutex);
	FontIndex result = nextFontIndex++;
	mappings.Emplace(filepath, Mapping{Type::FONT, result});
	fonts.Resize(max(result+1, fonts.size));
	Font &font = fonts[result];
	filepath = Stringify("fonts/", filepath);
	font.file = fileManager.RequestFile(filepath, priority, [](io::File *file, Any &any) -> bool {
		Font font;
		font.file = file;
		font.Decode();
		FontDecodeMetadata &metadata = any.Get<FontDecodeMetadata>();
		metadata.dstArrayMutex->Lock();
		metadata.dstArray->Resize(max(metadata.fontIndex+1, metadata.dstArray->size));
		(*metadata.dstArray)[metadata.fontIndex] = std::move(font);
		metadata.dstArrayMutex->Unlock();
		return false;
	}, FontDecodeMetadata{result, &fonts, &arrayMutex});
	return result;
}

SoundIndex Manager::RequestSound(az::String filepath, i32 priority) {
	struct SoundDecodeMetadata {
		SoundIndex soundIndex;
		Array<Sound> *dstArray;
		Mutex *dstArrayMutex;
	};
	if (auto node = mappings.Find(filepath)) {
		AzAssert(node->value.type == Type::SOUND, Stringify("RequestSound for \"", filepath, "\" already exists as a ", typeStrings[(i32)node->value.type]));
		return node->value.index;
	}
	ScopedLock lock(arrayMutex);
	SoundIndex result = nextSoundIndex++;
	mappings.Emplace(filepath, Mapping{Type::SOUND, result});
	sounds.Resize(max(result+1, sounds.size));
	Sound &sound = sounds[result];
	filepath = Stringify("sound/", filepath);
	sound.file = fileManager.RequestFile(filepath, priority, [](io::File *file, Any &any) -> bool {
		Sound sound;
		sound.file = file;
		sound.Decode();
		SoundDecodeMetadata &metadata = any.Get<SoundDecodeMetadata>();
		metadata.dstArrayMutex->Lock();
		metadata.dstArray->Resize(max(metadata.soundIndex+1, metadata.dstArray->size));
		(*metadata.dstArray)[metadata.soundIndex] = std::move(sound);
		metadata.dstArrayMutex->Unlock();
		return false;
	}, SoundDecodeMetadata{result, &sounds, &arrayMutex});
	return result;
}

StreamIndex Manager::RequestStream(az::String filepath, i32 priority) {
	struct StreamDecodeMetadata {
		StreamIndex streamIndex;
		Array<Stream> *dstArray;
		Mutex *dstArrayMutex;
	};
	if (auto node = mappings.Find(filepath)) {
		AzAssert(node->value.type == Type::STREAM, Stringify("RequestStream for \"", filepath, "\" already exists as a ", typeStrings[(i32)node->value.type]));
		return node->value.index;
	}
	ScopedLock lock(arrayMutex);
	StreamIndex result = nextStreamIndex++;
	mappings.Emplace(filepath, Mapping{Type::STREAM, result});
	streams.Resize(max(result+1, streams.size));
	Stream &stream = streams[result];
	filepath = Stringify("sound/", filepath);
	stream.file = fileManager.RequestFile(filepath, priority, [](io::File *file, Any &any) -> bool {
		Stream stream;
		stream.file = file;
		stream.Decode();
		StreamDecodeMetadata &metadata = any.Get<StreamDecodeMetadata>();
		metadata.dstArrayMutex->Lock();
		metadata.dstArray->Resize(max(metadata.streamIndex+1, metadata.dstArray->size));
		(*metadata.dstArray)[metadata.streamIndex] = std::move(stream);
		metadata.dstArrayMutex->Unlock();
		return true;
	}, StreamDecodeMetadata{result, &streams, &arrayMutex});
	return result;
}

i32 Manager::FindMapping(SimpleRange<char> filename, Type type) {
	AZCORE_PROFILING_FUNC_TIMER()
	auto *node = mappings.Find(filename);
	if (node == nullptr) {
		cout.PrintLn("No mapping found for \"", filename, "\"");
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

LockedPtr<Texture> Manager::GetTexture(TexIndex index) {
	AzAssert(index >= 0 && index < nextTexIndex, Stringify("TexIndex (", index, ") is invalid (must be >= 0 and < ", nextTexIndex, ")"));
	ScopedLock lock(arrayMutex);
	textures.Resize(max(index+1, textures.size));
	return LockedPtr(&textures[index], std::move(lock));
}

LockedPtr<Font> Manager::GetFont(FontIndex index) {
	AzAssert(index >= 0 && index < nextFontIndex, Stringify("FontIndex (", index, ") is invalid (must be >= 0 and < ", nextFontIndex, ")"));
	ScopedLock lock(arrayMutex);
	fonts.Resize(max(index+1, fonts.size));
	return LockedPtr(&fonts[index], std::move(lock));
}

LockedPtr<Sound> Manager::GetSound(SoundIndex index) {
	AzAssert(index >= 0 && index < nextSoundIndex, Stringify("SoundIndex (", index, ") is invalid (must be >= 0 and < ", nextSoundIndex, ")"));
	ScopedLock lock(arrayMutex);
	sounds.Resize(max(index+1, sounds.size));
	return LockedPtr(&sounds[index], std::move(lock));
}

LockedPtr<Stream> Manager::GetStream(StreamIndex index) {
	AzAssert(index >= 0 && index < nextStreamIndex, Stringify("StreamIndex (", index, ") is invalid (must be >= 0 and < ", nextStreamIndex, ")"));
	ScopedLock lock(arrayMutex);
	streams.Resize(max(index+1, streams.size));
	return LockedPtr(&streams[index], std::move(lock));
}

bool Manager::IsTextureValid(TexIndex index, bool lock) {
	// NOTE: You might think this should be a return value, but any indices outside of these bounds probably indicate a bug elsewhere. This function is for checking whether an index returned by a Request___ function was successfully loaded, and they can only return indices within these bounds.
	AzAssert(index >= 0 && index < nextTexIndex, Stringify("TexIndex (", index, ") is invalid (must be >= 0 and < ", nextTexIndex, ")"));
	ScopedLock _lock(lock ? &arrayMutex : nullptr);
	if (textures.size <= index) return false;
	if (nullptr == textures[index].file || textures[index].file->stage == io::File::Stage::FILE_NOT_FOUND) return false;
	return true;
}

bool Manager::IsFontValid(FontIndex index, bool lock) {
	AzAssert(index >= 0 && index < nextFontIndex, Stringify("FontIndex (", index, ") is invalid (must be >= 0 and < ", nextFontIndex, ")"));
	ScopedLock _lock(lock ? &arrayMutex : nullptr);
	if (fonts.size <= index) return false;
	if (nullptr == fonts[index].file || fonts[index].file->stage == io::File::Stage::FILE_NOT_FOUND) return false;
	return true;
}

bool Manager::IsSoundValid(SoundIndex index, bool lock) {
	AzAssert(index >= 0 && index < nextSoundIndex, Stringify("SoundIndex (", index, ") is invalid (must be >= 0 and < ", nextSoundIndex, ")"));
	ScopedLock _lock(lock ? &arrayMutex : nullptr);
	if (sounds.size <= index) return false;
	return sounds[index].valid;
}

bool Manager::IsStreamValid(StreamIndex index, bool lock) {
	AzAssert(index >= 0 && index < nextStreamIndex, Stringify("StreamIndex (", index, ") is invalid (must be >= 0 and < ", nextStreamIndex, ")"));
	ScopedLock _lock(lock ? &arrayMutex : nullptr);
	if (streams.size <= index) return false;
	return streams[index].valid;
}

} // namespace Az2D::Assets
