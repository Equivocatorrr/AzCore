/*
    File: sound.cpp
    Author: Philip Haynes
*/

#include "sound.hpp"
#include "globals.hpp"
// #include "AzCore/log_stream.hpp"

namespace Sound {

String error = "No error";

// logStream cout("sound.log");

String openAlErrorToString(ALenum err) {
    switch (err) {
    case AL_NO_ERROR:
        return "AL_NO_ERROR";
    case AL_INVALID_ENUM:
        return "AL_INVALID_ENUM";
    case AL_INVALID_VALUE:
        return "AL_INVALID_VALUE";
    case AL_OUT_OF_MEMORY:
        return "AL_OUT_OF_MEMORY";
    case AL_INVALID_OPERATION:
        return "AL_INVALID_OPERATION";
    /* ... */
    default:
        return ToString((u32)err);
    }
}

bool ErrorCheck(const char* info) {
    ALCenum errorCode;
    if ((errorCode = alGetError()) != ALC_NO_ERROR) {
        error = String("OpenAL error from [") + info + "]: " + openAlErrorToString(errorCode);
        return false;
    }
    return true;
}

bool Manager::Initialize() {
    device = alcOpenDevice(nullptr);
    if (!device) {
        error = "Failed to alcOpenDevice: " + openAlErrorToString(alGetError());
        return false;
    }
    context = alcCreateContext(device, nullptr);
    if (!context) {
        error = "Failed to alcCreateContext: " + openAlErrorToString(alGetError());
        return false;
    }
    alGetError(); // Clear the error code
    alcMakeContextCurrent(context);
    if (!ErrorCheck("alcMakeContextCurrent")) return false;
    // alDopplerVelocity(34.3f); // Scale with our units (decimeters)
    // ErrorCheck("alDopplerFactor");
    // alDistanceModel(AL_EXPONENT_DISTANCE);
    // ErrorCheck("alDistanceModel");

    alGenSources((ALuint)maxSources, sources);
    if (!ErrorCheck("alGenSources")) return false;

    for (uint32_t i = 0; i < maxSources; i++) {
        sourcesFree[i] = true;
        // alSourcef(sources[i], AL_REFERENCE_DISTANCE, 5.0f);
        // ErrorCheck("alSourcef(AL_REFERENCE_DISTANCE)");
        // alSourcef(sources[i], AL_ROLLOFF_FACTOR, 1.0f);
        // ErrorCheck("alSourcef(AL_ROLLOFF_FACTOR)");
    }
    return true;
}

bool Manager::DeleteSources() {
    alDeleteSources((ALuint)maxSources, sources);
    return ErrorCheck("alDeleteSources");
}

bool Manager::Deinitialize() {
    if (!ErrorCheck("Unknown")) return false;
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
    return true;
}

bool Manager::Update() {
    Array<PriorityIndex> priorities(0);
    for (i32 i = 0; i < sounds.size; i++) {
        PriorityIndex index;
        index.sound = sounds[i];
        if (!sounds[i]->playing && !sounds[i]->play) {
            // No sense in prioritizing a sound that isn't playing
            index.priority = 0.0;
        } else {
            if (sounds[i]->channel == MUSIC) {
                // Music takes priority over everything and doesn't work spacially anyway
                index.priority = 10000000.0 * index.sound->gain;
            } else /* if (sounds[i]->stereo) */ {
                // Stereo sounds aren't spacially attenuated
                index.priority = index.sound->gain;
            } /* else {
                // Looks like our sound is just a normal monophonic 3D sound
                index.priority = sounds[i]->gain / glm::length(sounds[i]->position - listener.position);
            } */
        }

        bool foundSpot = false;
        for (i32 ii = 0; ii < priorities.size; ii++) {
            if (index.priority > priorities[ii].priority) {
                priorities.Insert(ii, index);
                foundSpot = true;
                break;
            }
        }
        if (!foundSpot)
            priorities.Append(index);
    }

    for (i32 i = maxSources; i < priorities.size; i++) {
        if (priorities[i].sound->active) {
            // We have an active sound that was pushed out of our priority limit so deactivate it
            alSourceStop(priorities[i].sound->source);
            if (!ErrorCheck("alSourceStop")) return false;
            priorities[i].sound->active = false;
            priorities[i].sound->play = priorities[i].sound->playing;
            for (i32 ii = 0; ii < maxSources; ii++) {
                if (priorities[i].sound->source == sources[ii]) {
                    sourcesFree[ii] = true;
                    break;
                }
            }
        }
    }

    for (i32 i = 0; i < min(maxSources, priorities.size); i++) {
        SourceBase *sound = priorities[i].sound;
        if (!sound->active) {
            // We have a sound that's within our priority limit that isn't active so activate it
            for (i32 ii = 0; ii < maxSources; ii++) {
                if (sourcesFree[ii]) {
                    sourcesFree[ii] = false;
                    sound->source = sources[ii];
                    sound->active = true;

                    if (!sound->stream) {
                        alSourcei(sources[ii], AL_BUFFER, ((Source*)sound)->buffer);
                        if (!ErrorCheck("Manager::Update alSourcei(AL_BUFFER)")) return false;
                    }
                    break;
                }
            }
        }
        if (sound->active) {
            // Updates for every active sound
            // alSource3f(sound->source, AL_POSITION, sound->position.x, sound->position.y, sound->position.z);
            // ErrorCheck("alSource3f(AL_POSITION)");
            // alSource3f(sound->source, AL_VELOCITY, sound->velocity.x, sound->velocity.y, sound->velocity.z);
            // ErrorCheck("alSource3f(AL_VELOCITY)");
            // alSource3f(sound->source, AL_DIRECTION, sound->direction.x, sound->direction.y, sound->direction.z);
            // ErrorCheck("alSource3f(AL_DIRECTION)");
            alSourcef(sound->source, AL_PITCH, sound->pitch);
            if (!ErrorCheck("alSourcef(AL_PITCH)")) return false;
            alSourcef(sound->source, AL_GAIN, sound->gain);
            if (!ErrorCheck("alSourcef(AL_GAIN)")) return false;
            alSourcei(sound->source, AL_LOOPING, sound->loop);
            if (!ErrorCheck("alSourcei(AL_LOOPING)")) return false;
            // Handle changing play states
            ALint state;
            alGetSourcei(sound->source, AL_SOURCE_STATE, &state);
            if (!ErrorCheck("SourceBase::Playing alGetSourcei(AL_SOURCE_STATE)")) return false;
            bool stopped = false;
            if (state == AL_PLAYING) {
                if (sound->pause) {
                    alSourcePause(sound->source);
                    if (!ErrorCheck("alSourcePause")) return false;
                    sound->pause = false;
                    sound->playing = false;
                }
                if (sound->stop && !sound->stream) {
                    alSourceStop(sound->source);
                    if (!ErrorCheck("alSourceStop")) return false;
                    sound->stop = false;
                    sound->playing = false;
                    stopped = true;
                }
            }
            if (state != AL_PLAYING || stopped) {
                // Not playing
                if (sound->play) {
                    //Sys::cout << "We're telling it to play" << std::endl;
                    if (!sound->stream) {
                        alSourcei(sound->source, AL_BUFFER, ((Source*)sound)->buffer);
                        if (!ErrorCheck("DC::Update alSourcei(AL_BUFFER)")) return false;
                    }
                    alSourcePlay(sound->source);
                    if (!ErrorCheck("alSourcePlay")) return false;
                    sound->play = false;
                    sound->stop = false;
                    sound->playing = true;
                }
            }
        }
    }
    return true;
}

void Manager::Unregister(SourceBase *sound) {
    for (i32 i = 0; i < sounds.size; i++) {
        if (sound == sounds[i]) {
            if (sounds[i]->active) {
                for (i32 ii = 0; ii < maxSources; ii++) {
                    if (sounds[i]->source == sources[ii]) {
                        sourcesFree[ii] = true;
                        alSourceStop(sources[ii]);
                        ErrorCheck("alSourceStop");
                    }
                }
            }
            sounds.Erase(i);
            return;
        }
    }
}

bool Buffer::Create() {
    alGenBuffers((ALuint)1, &buffer);
    return ErrorCheck("alGenBuffers");
}

bool Buffer::Load(ALvoid *data, ALenum format, ALsizei size, ALsizei freq) {
    alBufferData(buffer, format, data, size, freq);
    return ErrorCheck("alBufferData");
}

bool Buffer::Clean() {
    alDeleteBuffers(1, &buffer);
    return ErrorCheck("alDeleteBuffers");
}

SourceBase::SourceBase() : source(0), /* position(0.0), velocity(0.0), direction(0.0), */ pitch(1.0), gain(1.0), loop(false), playing(false), play(false), pause(false), stop(false), active(false), stereo(false), stream(false), channel(FX) {}

// void SourceBase::SetPosition(vec3f pos) {
//     position = pos;
// }
//
// void SourceBase::SetVelocity(vec3f vel) {
//     velocity = vel;
// }
//
// void SourceBase::SetDirection(vec3f dir) {
//     direction = dir;
// }

void SourceBase::SetPitch(f32 p) {
    pitch = p * globals->objects.simulationRate;
}

void SourceBase::SetGain(f32 g) {
    gain = g * globals->volumeMain;
    switch (channel) {
        case MUSIC:
            g *= globals->volumeMusic;
            break;
        case FX:
            g *= globals->volumeEffects;
            break;
    }
}

void SourceBase::Play() {
    play = true;
}

void SourceBase::Pause() {
    pause = true;
}

void SourceBase::SetLoop(bool on) {
    loop = on;
}

void Source::Create(Buffer *buf) {
    stereo = buf->stereo;
    buffer = buf->buffer;
    channel = FX;
    stream = false;
}

void Source::Create(String filename) {
    i32 soundIndex = globals->assets.FindMapping(filename);
    Create(&globals->assets.sounds[soundIndex].buffer);
    globals->sound.Register(this);
}

void Source::Stop() {
    stop = true;
}

void MultiSource::Play(f32 gain, f32 pitch) {
    Stop();
    current = random(0, sources.size, globals->rng);
    sources[current]->SetGain(gain);
    sources[current]->SetPitch(pitch);
    sources[current]->Play();
}

void MultiSource::Play() {
    Stop();
    current = random(0, sources.size, globals->rng);
    sources[current]->Play();
}

void MultiSource::Pause() {
    if (current >= 0) {
        sources[current]->Pause();
    }
}
void MultiSource::Stop() {
    if (current >= 0) {
        sources[current]->Stop();
    }
}

void Stream::Create() {
    playing = false;
    channel = MUSIC;
    stream = true;
}

void Stream::Stop() {
    playing = false;
    if (!active)
        return;
    // alSourceStop(source);
    // ErrorCheck("alSourceStop");
}

bool Stream::Queue(ALuint buffer) {
    if (!active)
        return true;
    alSourceQueueBuffers(source, 1, &buffer);
    return ErrorCheck("Stream::Queue alSourceQueueBuffers");
}

i32 Stream::BuffersDone() {
    if (!active)
        return false;
    ALint buffersProcessed;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &buffersProcessed);
    if (!ErrorCheck("Stream::BufferDone alGetSourcei(AL_BUFFERS_PROCESSED)")) return -1;
    return buffersProcessed;
}

bool Stream::Unqueue(ALuint buffer) {
    if (!active)
        return true;
    alSourceUnqueueBuffers(source, 1, &buffer);
    return ErrorCheck("Stream::Unqueue alSourceUnqueueBuffers");
}


} // namespace Sound
