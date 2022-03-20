/*
	File: sound.hpp
	Author: Philip Haynes
	OpenAL sound effects and music.
*/

#ifndef SOUND_HPP
#define SOUND_HPP

#include "AzCore/common.hpp"
#include "AzCore/Thread.hpp"
#include <AL/al.h>
#include <AL/alc.h>

namespace Assets {
	struct Stream;
}

namespace Sound {

using namespace AzCore;

extern String error;

constexpr i32 maxSources = 32;

enum VolumeChannel {
	MUSIC,
	FX
};

/*  struct: Buffer
	Author: Philip Haynes
	Defines one chunk of PCM sound data     */
struct Buffer {
	ALuint buffer;
	bool stereo;
	bool Create();
	bool Load(ALvoid *data, ALenum format, ALsizei size, ALsizei freq);
	bool Clean();
};

/*  struct: SourceBase
	Author: Philip Haynes
	Defines a reference to one instance of any sound buffer(s)        */
struct SourceBase {
	ALuint source;
	// vec3f position, velocity, direction;
	f32 pitch, gain;
	bool loop, playing;
	bool play, pause, stop; // Whether or not we were told to play, pause or stop
	bool active; // Whether or not our sound made it into the priority limit
	bool stereo; // Whether or not our sound is stereo, and therefore whether its priority should be spacial
	bool stream; // Whether we're a stream or a source
	bool simulationPitch;
	VolumeChannel channel;

	SourceBase();

	inline void SetPitch(f32 p) {
		pitch = p;
	}
	inline void SetGain(f32 g) {
		gain = g;
	}

	inline void Play() {
		play = true;
	}
	inline void Pause() {
		pause = true;
	}
	inline void SetLoop(bool on) {
		loop = on;
	}
};

/*  struct: Source
	Author: Philip Haynes
	A single-buffer sound       */
struct Source : public SourceBase {
	ALuint buffer;
	void Create(Buffer *buf);
	void Create(String filename);

	inline void Stop() {
		stop = true;
	}
};

/*  struct: MultiSource
	Author: Philip Haynes
	Can choose randomly between a set number of Sources */
struct MultiSource {
	Array<Source*> sources;
	i32 current = -1;
	i32 shuffleId = genShuffleId();
	void Play(f32 gain, f32 pitch);
	void Play();
	void Pause();
	void Stop();
};

/*  struct: Stream
	Author: Philip Haynes
	Opens and maintains the buffers needed to stream long audio files       */
struct Stream : public SourceBase {
	Ptr<Assets::Stream> file = nullptr;
	bool fadeout = false;
	// Array<ALuint> buffersToQueue{};
	bool Create(Ptr<Assets::Stream> file_in);
	bool Create(String filename);
	bool Queue(ALuint buffer);
	i32 BuffersDone();
	bool Unqueue(ALuint buffer);
	void Stop(f32 fadeoutDuration = 0.0f);
	bool SetLoopRange(i32 begin, i32 end);
};

/*
	struct: PriorityIndex
	Author: Philip Haynes
	Used for determining which sounds will get
	replaced should there be too many to play at once.         */
struct PriorityIndex {
	SourceBase* sound = nullptr;
	f32 priority = 0.0f;
};

struct Manager {
	bool initialized = false;
	String name{};
	ALCdevice *device;
	ALCcontext *context;

	ALuint sources[maxSources];
	bool sourcesFree[maxSources];

	Array<SourceBase*> sounds;

	Thread streamUpdateProc;
	std::atomic<bool> procStop;
	std::atomic<bool> procFailure;
	Mutex soundMutex;

	bool Initialize();
	bool DeleteSources();
	bool Deinitialize();
	~Manager();


	bool Update();

	inline void Register(SourceBase *sound) { sounds.Append(sound); }
	void Unregister(SourceBase *sound);

private:
	Array<PriorityIndex> GetPriorities();
	bool Deactivate(SourceBase *sound);
	bool Activate(SourceBase *sound);
	bool UpdateActiveSound(SourceBase *sound);
	bool Play(SourceBase *sound);
	bool Pause(SourceBase *sound);
	bool Stop(SourceBase *sound);
	static void StreamUpdateProc(Manager *theThisPointer);
};

} // namespace Sound

#endif
