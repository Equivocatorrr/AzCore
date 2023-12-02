/*
	File: assets.cpp
	Author: Philip Haynes
*/

#include "assets.hpp"
#include "game_systems.hpp"
#include "AzCore/Profiling.hpp"

#include "AzCore/IO/Log.hpp"
#include "AzCore/Simd.hpp"

#include <stb_image.h>
#include <stb_vorbis.c>

namespace Az3D::Assets {

using namespace AzCore;

io::Log cout("assets.log");

String error = "No error.";

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
	AZCORE_PROFILING_FUNC_TIMER()
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

void Texture::Decode() {
	AZCORE_PROFILING_FUNC_TIMER()
	image.LoadFromBuffer(file->data);
	if (image.channels == 4) {
		// Only multiply alpha if we actually had an alpha channel in the first place
		PremultiplyAlpha();
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
		cout.PrintLn("Sound::Decode: Failed to create buffer: ", Az3D::Sound::error);
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
		cout.PrintLn("Sound::Load: Failed to load buffer: ", Az3D::Sound::error, " channels=",channels, " length=", length, " samplerate=", samplerate, " bufferid=", buffer.buffer, " &decoded=0x", FormatInt((i64)decoded, 16));
		free(decoded);
		return;
	}
	valid = true;
	free(decoded);
}

Sound::~Sound() {
	if (valid) {
		if (!buffer.Clean()) {
			cout.PrintLn("Failed to clean Sound buffer: ", Az3D::Sound::error);
		}
	}
}

void Stream::Open() {
	AZCORE_PROFILING_FUNC_TIMER()
	valid = false;
	for (i32 i = 0; i < numStreamBuffers; i++) {
		if (!buffers[i].Create()) {
			cout.PrintLn("Stream::Open: Failed to create buffer: ", Az3D::Sound::error);
			return;
		}
	}
	i32 iError = 0;
	vorbis = stb_vorbis_open_memory((u8*)file->data.data, file->data.size, &iError, nullptr);
	if (!vorbis) {
		cout.PrintLn("Stream::Open: Failed to decode \"", file->filepath, "\", error code ", iError);
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

i32 Stream::Decode(i32 sampleCount) {
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

void Mesh::Decode(Manager *manager) {
	AZCORE_PROFILING_FUNC_TIMER()
	Az3DObj::File az3dFile;
	Array<Az3DObj::File::ImageData> imageData;
	if (!az3dFile.LoadFromBuffer(file->data, &imageData)) return;
	// Hold the lock until we add all the images
	manager->arrayMutex.Lock();
	// Use this to offset material texture indices
	// Offset by -1 because mesh tex indices are 1-indexed, since 0 means no texture.
	i32 texOffset = manager->nextTexIndex-1;
	manager->textures.Resize(max(manager->nextTexIndex + az3dFile.images.size, manager->textures.size));
	// for (az::Image &image : az3dFile.images) {
	// 	TexIndex texIndex = manager->nextTexIndex++;
	// 	Texture &texture = manager->textures[texIndex];
	// 	texture.image = std::move(image);
	// 	if (texture.image.channels == 4) {
	// 		texture.PremultiplyAlpha();
	// 	}
	// 	if (texture.image.channels == 3) {
	// 		texture.image.SetChannels(4);
	// 	}
	// }
	for (auto &image : imageData) {
		manager->RequestTextureDecode(Array<char>(image.data), Stringify(file->filepath, "/", image.filename), image.isLinear, file->priority);
	}
	manager->arrayMutex.Unlock();
	for (Az3DObj::Mesh &meshData : az3dFile.meshes) {
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
		f32 sqrRadius = 0.0f;
		for (Az3DObj::Vertex &vert : meshPart->vertices) {
			f32 mySqrRadius = normSqr(vert.pos);
			if (mySqrRadius > sqrRadius) sqrRadius = mySqrRadius;
		}
		meshPart->boundingSphereRadius = sqrt(sqrRadius);
		manager->arrayMutex.Lock();
		parts.Append(manager->meshParts.Append(std::move(meshPart)).RawPtr());
		manager->arrayMutex.Unlock();
	}
}

void Manager::Init() {
	fileManager.Init();
	fileManager.searchDirectories = {
		"data/",
		"data/Az3D/",
	};
	mappings.Clear();
	textures.Clear();
	fonts.Clear();
	sounds.Clear();
	streams.Clear();
	meshes.Clear();
	meshParts.Clear();
	nextTexIndex = 0;
	nextFontIndex = 0;
	nextSoundIndex = 0;
	nextStreamIndex = 0;
	nextMeshIndex = 0;
	
	RequestTexture("TextureMissing.png", false);
	RequestTexture("blank.tga", false);
	RequestTexture("blank_n.tga", true);
	RequestFont("DroidSansFallback.ttf");
}

void Manager::Deinit() {
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
	filepath = Stringify("textures/", filepath);
	
	if (auto node = mappings.Find(filepath)) {
		AzAssert(node->value.type == Type::TEXTURE, Stringify("RequestTexture for \"", filepath, "\" already exists as a ", typeStrings[(i32)node->value.type]));
		return node->value.index;
	}
	arrayMutex.Lock();
	TexIndex result = nextTexIndex++;
	arrayMutex.Unlock();
	fileManager.RequestFile(filepath, priority, textureDecoder, TextureDecodeMetadata{result, &textures, &arrayMutex, linear});
	return result;
}

TexIndex Manager::RequestTextureDecode(Array<char> &&buffer, az::String filepath, bool linear, i32 priority) {
	arrayMutex.Lock();
	TexIndex result = nextTexIndex++;
	arrayMutex.Unlock();
	fileManager.RequestDecode(std::move(buffer), filepath, priority, textureDecoder, TextureDecodeMetadata{result, &textures, &arrayMutex, linear});
	return result;
}

FontIndex Manager::RequestFont(az::String filepath, i32 priority) {
	struct FontDecodeMetadata {
		FontIndex fontIndex;
		Array<Font> *dstArray;
		Mutex *dstArrayMutex;
	};
	filepath = Stringify("fonts/", filepath);
	
	if (auto node = mappings.Find(filepath)) {
		AzAssert(node->value.type == Type::FONT, Stringify("RequestFont for \"", filepath, "\" already exists as a ", typeStrings[(i32)node->value.type]));
		return node->value.index;
	}
	arrayMutex.Lock();
	FontIndex result = nextFontIndex++;
	arrayMutex.Unlock();
	fileManager.RequestFile(filepath, priority, [](io::File *file, Any &any) -> bool {
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
	filepath = Stringify("sounds/", filepath);
	
	if (auto node = mappings.Find(filepath)) {
		AzAssert(node->value.type == Type::SOUND, Stringify("RequestSound for \"", filepath, "\" already exists as a ", typeStrings[(i32)node->value.type]));
		return node->value.index;
	}
	arrayMutex.Lock();
	SoundIndex result = nextSoundIndex++;
	arrayMutex.Unlock();
	fileManager.RequestFile(filepath, priority, [](io::File *file, Any &any) -> bool {
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
	filepath = Stringify("sounds/", filepath);
	
	if (auto node = mappings.Find(filepath)) {
		AzAssert(node->value.type == Type::STREAM, Stringify("RequestStream for \"", filepath, "\" already exists as a ", typeStrings[(i32)node->value.type]));
		return node->value.index;
	}
	arrayMutex.Lock();
	StreamIndex result = nextStreamIndex++;
	arrayMutex.Unlock();
	fileManager.RequestFile(filepath, priority, [](io::File *file, Any &any) -> bool {
		Stream stream;
		stream.file = file;
		stream.Open();
		StreamDecodeMetadata &metadata = any.Get<StreamDecodeMetadata>();
		metadata.dstArrayMutex->Lock();
		metadata.dstArray->Resize(max(metadata.streamIndex+1, metadata.dstArray->size));
		(*metadata.dstArray)[metadata.streamIndex] = std::move(stream);
		metadata.dstArrayMutex->Unlock();
		return false;
	}, StreamDecodeMetadata{result, &streams, &arrayMutex});
	return result;
}

MeshIndex Manager::RequestMesh(az::String filepath, i32 priority) {
	struct MeshDecodeMetadata {
		MeshIndex meshIndex;
		Manager *manager;
	};
	filepath = Stringify("models/", filepath);
	
	if (auto node = mappings.Find(filepath)) {
		AzAssert(node->value.type == Type::MESH, Stringify("RequestMesh for \"", filepath, "\" already exists as a ", typeStrings[(i32)node->value.type]));
		return node->value.index;
	}
	arrayMutex.Lock();
	MeshIndex result = nextMeshIndex++;
	arrayMutex.Unlock();
	fileManager.RequestFile(filepath, priority, [](io::File *file, Any &any) -> bool {
		Mesh mesh;
		mesh.file = file;
		MeshDecodeMetadata &metadata = any.Get<MeshDecodeMetadata>();
		mesh.Decode(metadata.manager);
		metadata.manager->arrayMutex.Lock();
		metadata.manager->meshes.Resize(max(metadata.meshIndex+1, metadata.manager->meshes.size));
		metadata.manager->meshes[metadata.meshIndex] = std::move(mesh);
		metadata.manager->arrayMutex.Unlock();
		return false;
	}, MeshDecodeMetadata{result, this});
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

}
