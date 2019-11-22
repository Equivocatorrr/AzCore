/*
    File: sound.hpp
    Author: Philip Haynes
    OpenAL sound effects and music.
*/

#ifndef SOUND_HPP
#define SOUND_HPP

#include "AzCore/common.hpp"
#include <AL/al.h>
#include <AL/alc.h>

using namespace AzCore;

namespace Sound {

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
    void Play(f32 gain, f32 pitch);
    void Play();
    void Pause();
    void Stop();
};

/*  struct: Stream
    Author: Philip Haynes
    Opens and maintains the buffers needed to stream long audio files       */
struct Stream : public SourceBase {
    void Create();
    bool Queue(ALuint buffer);
    i32 BuffersDone();
    bool Unqueue(ALuint buffer);
    void Stop();
};

/*
    struct: PriorityIndex
    Author: Philip Haynes
    Used for determining which sounds will get
    replaced should there be too many to play at once.         */
struct PriorityIndex {
    SourceBase* sound = nullptr;
    f32 priority = 0.0;
};

struct Manager {
    String name{};
    ALCdevice *device;
    ALCcontext *context;

    ALuint sources[maxSources];
    bool sourcesFree[maxSources];

    Array<SourceBase*> sounds;

    bool Initialize();
    bool DeleteSources();
    bool Deinitialize();

    bool Update();

    inline void Register(SourceBase *sound) { sounds.Append(sound); }
    void Unregister(SourceBase *sound);
};

} // namespace Sound

#endif
