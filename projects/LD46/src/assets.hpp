/*
	File: assets.hpp
	Author: Philip Haynes
	Manages loading of file assets.
*/

#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "AzCore/memory.hpp"
#include "AzCore/font.hpp"
#include "sound.hpp"

struct stb_vorbis;

namespace Rendering {
	struct Manager;
}

namespace Assets {

using namespace AzCore;

extern String error;

enum Type {
	NONE,
	TEXTURE,
	FONT,
	SOUND,
	STREAM
};

// Used to retrieve indices to actual assets
// Should be consistent with indices in the Rendering Manager
struct Mapping {
	u32 checkSum; // Used as a simple hash value for filenames
	String filename; // Actual filename to be loaded
	Type type; // Determines what arrays contain our asset
	i32 index;
	// Sets both the filename and the checksum.
	void SetFilename(String name);
	bool FilenameEquals(String name, u32 sum);

	static u32 CheckSum(String name);
};

struct Texture {
	Array<u8> pixels;
	i32 width, height, channels;

	bool Load(String filename);
};

struct Font {
	font::Font font;
	font::FontBuilder fontBuilder;

	bool Load(String filename);
};

struct Sound {
	bool valid;
	::Sound::Buffer buffer;
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

	bool Load(String filename);
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
	::Sound::Buffer buffers[numStreamBuffers];
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

	bool Open(String filename);
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
		String filename;
		Type type;
		inline FileToLoad() : filename(), type(NONE) {}
		inline FileToLoad(String fn) : filename(fn), type(NONE) {}
		inline FileToLoad(String fn, Type t) : filename(fn), type(t) {}
	};
	// Everything we want to actually load.
	Array<FileToLoad> filesToLoad{
		FileToLoad("TextureMissing.png"),
		FileToLoad("blank.bmp"),
		FileToLoad("DroidSansFallback.ttf")
	};
	Array<Mapping> mappings{};
	Array<Texture> textures{};
	Array<Font> fonts{};
	Array<Sound> sounds{};
	Array<Stream> streams{};

	inline void QueueFile(String filename) {
		filesToLoad.Append(FileToLoad(filename));
	}
	inline void QueueFile(String filename, Type type) {
		filesToLoad.Append({filename, type});
	}

	bool LoadAll();
	i32 FindMapping(String filename);
	f32 CharacterWidth(char32 c, i32 fontIndex) const;
};

} // namespace Assets

#endif // ASSETS_HPP
