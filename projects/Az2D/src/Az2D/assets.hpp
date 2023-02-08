/*
	File: assets.hpp
	Author: Philip Haynes
	Manages loading of file assets.
*/

#ifndef AZ2D_ASSETS_HPP
#define AZ2D_ASSETS_HPP

#include "AzCore/memory.hpp"
#include "AzCore/font.hpp"
#include "sound.hpp"

struct stb_vorbis;

namespace Az2D::Rendering {
	struct Manager;
}

namespace Az2D::Assets {

extern az::String error;

typedef i32 TexIndex;
typedef i32 FontIndex;
typedef i32 SoundIndex;
typedef i32 StreamIndex;

enum class Type {
	NONE=0,
	TEXTURE,
	FONT,
	SOUND,
	STREAM
};
extern const char* typeStrings[5];

// Used to retrieve indices to actual assets
// Should be consistent with indices in the Rendering Manager
struct Mapping {
	Type type; // Determines what arrays contain our asset
	i32 index;
};

struct Texture {
	u8 *pixels = nullptr;
	i32 width = 0, height = 0, channels = 0;
	// if this is false, then the image is sRGB
	bool linear = false;

	bool Load(az::String filename);
	Texture() = default;
	Texture(const Texture &other) = delete;
	Texture(Texture &&other);
	Texture& operator=(Texture &&other);
	Texture& operator=(const Texture &other) = delete;
	~Texture();
};

struct Font {
	az::font::Font font;
	az::font::FontBuilder fontBuilder;

	bool Load(az::String filename);
};

struct Sound {
	bool valid;
	Az2D::Sound::Buffer buffer;
	inline Sound() : valid(false), buffer({UINT32_MAX, false}) {}
	inline Sound(const Sound &a) : valid(false), buffer(a.buffer) {}
	inline Sound(Sound &&a) : valid(a.valid), buffer(a.buffer) { a.valid = false; }
	~Sound();
	inline Sound& operator=(const Sound &a) {
		valid = false;
		buffer = a.buffer;
		return *this;
	}
	inline Sound& operator=(Sound &&a) {
		valid = a.valid;
		buffer = a.buffer;
		a.valid = false;
		return *this;
	}

	bool Load(az::String filename);
};

constexpr i8 numStreamBuffers = 2;

struct Stream {
	stb_vorbis *vorbis;
	bool valid;
	struct {
		i8 channels = 0;
		i8 lastBuffer = 0;
		i8 currentBuffer = 0;
		i32 samplerate = 0;
		// The total number of samples in the audio file.
		i32 totalSamples = 0;
		// The location in samples that we want to decode next.
		i32 cursorSample = 0;
		// Where we should seek to if we reach loopEndSample
		i32 loopBeginSample = 0;
		// Where we should stop before looping back to loopBeginSample
		i32 loopEndSample = -1;
		// How long a fadeout should be.
		i32 fadeoutSamples = -1;
		// How many samples have been done in the fadeout.
		i32 fadeoutCompleted = -1;
	} data;
	Az2D::Sound::Buffer buffers[numStreamBuffers];
	inline Stream() : valid(false), data(), buffers{{UINT32_MAX, false}} {}
	inline Stream(const Stream &a) :
		vorbis(a.vorbis), valid(false), data(a.data) {
		for (i32 i = 0; i < numStreamBuffers; i++) {
			buffers[i] = a.buffers[i];
		}
	}
	inline Stream(Stream &&a) :
		vorbis(a.vorbis), valid(a.valid), data(a.data)
	{
		for (i32 i = 0; i < numStreamBuffers; i++) {
			buffers[i] = a.buffers[i];
		}
		a.valid = false;
	}
	~Stream();
	inline Stream& operator=(const Stream &a) {
		vorbis = a.vorbis;
		valid = false;
		data = a.data;
		for (i32 i = 0; i < numStreamBuffers; i++) {
			buffers[i] = a.buffers[i];
		}
		return *this;
	}
	inline Stream& operator=(Stream &&a) {
		vorbis = a.vorbis;
		valid = a.valid;
		data = a.data;
		for (i32 i = 0; i < numStreamBuffers; i++) {
			buffers[i] = a.buffers[i];
		}
		a.valid = false;
		return *this;
	}

	bool Open(az::String filename);
	// Returns the number of samples decoded or -1 on error
	i32 Decode(i32 sampleCount);
	void SeekStart();
	ALuint LastBuffer();
	inline void BeginFadeout(f32 duration) {
		data.fadeoutSamples = i32((f32)data.samplerate * duration);
		data.fadeoutCompleted = 0;
	}
	bool Close();
};

constexpr i32 textureIndexBlank = 1;

struct Manager {
	struct FileToLoad {
		az::String filename;
		Type type;
		bool isLinearTexture = false;
		inline FileToLoad() : filename(), type(Type::NONE) {}
		explicit inline FileToLoad(az::String fn) : filename(fn), type(Type::NONE) {}
		inline FileToLoad(az::String fn, Type t) : filename(fn), type(t) {}
		inline FileToLoad(az::String fn, bool linTexture) : filename(fn), type(Type::TEXTURE), isLinearTexture(linTexture) {}
	};
	// Everything we want to actually load.
	az::Array<FileToLoad> filesToLoad{
		FileToLoad("TextureMissing.png"),
		FileToLoad("blank.bmp"),
		FileToLoad("blank_normal.bmp", true),
		FileToLoad("DroidSansFallback.ttf")
	};
	az::HashMap<az::SimpleRange<char>, Mapping> mappings;
	az::Array<Texture> textures;
	az::Array<Font> fonts;
	az::Array<Sound> sounds;
	az::Array<Stream> streams;

	inline void QueueFile(az::String filename) {
		filesToLoad.Append(FileToLoad(filename));
	}
	inline void QueueFile(az::String filename, Type type) {
		filesToLoad.Append(FileToLoad(filename, type));
	}
	inline void QueueLinearTexture(az::String filename) {
		filesToLoad.Append(FileToLoad(filename, true));
	}

	bool LoadAll();
	i32 FindMapping(az::SimpleRange<char> filename, Type type);
	inline TexIndex FindTexture(az::SimpleRange<char> filename) {
		return (TexIndex)FindMapping(filename, Type::TEXTURE);
	}
	inline FontIndex FindFont(az::SimpleRange<char> filename) {
		return (FontIndex)FindMapping(filename, Type::FONT);
	}
	inline SoundIndex FindSound(az::SimpleRange<char> filename) {
		return (SoundIndex)FindMapping(filename, Type::SOUND);
	}
	inline StreamIndex FindStream(az::SimpleRange<char> filename) {
		return (StreamIndex)FindMapping(filename, Type::STREAM);
	}
	f32 CharacterWidth(char32 c, i32 fontIndex) const;
};

} // namespace Az2D::Assets

#endif // AZ2D_ASSETS_HPP
