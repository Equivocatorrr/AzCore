/*
	File: sound.cpp
	Author: Philip Haynes
*/

#include "sound.hpp"
#include "globals.hpp"

namespace Sound {

String error = "No error";

String openAlErrorToString(ALenum err) {
	switch (err) {
	case AL_NO_ERROR:
		return "AL_NO_ERROR";
	case AL_INVALID_NAME:
		return "AL_INVALID_NAME";
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
	if (initialized) return false;
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
	procStop = false;
	procFailure = false;
	streamUpdateProc = Thread(StreamUpdateProc, this);
	initialized = true;
	return true;
}

bool Manager::DeleteSources() {
	for (i32 i = 0; i < sounds.size; i++) {
		SourceBase *sound = sounds[i];
		if (sound->active && sound->playing) {
			if (!Deactivate(sound)) return false;
		}
	}
	alDeleteSources((ALuint)maxSources, sources);
	return ErrorCheck("alDeleteSources");
}

bool Manager::Deinitialize() {
	if (!initialized) return true;
	procStop = true;
	if (streamUpdateProc.Joinable()) {
		streamUpdateProc.Join();
	}
	if (!ErrorCheck("Unknown")) return false;
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
	alcCloseDevice(device);
	initialized = false;
	return true;
}

Manager::~Manager() {
	Deinitialize();
}

Array<PriorityIndex> Manager::GetPriorities() {
	Array<PriorityIndex> priorities;
	priorities.Reserve(sounds.size);
	for (i32 i = 0; i < sounds.size; i++) {
		PriorityIndex index;
		index.sound = sounds[i];
		if (!sounds[i]->playing && !sounds[i]->play) {
			// No sense in prioritizing a sound that isn't playing
			index.priority = 0.0f;
		} else {
			if (sounds[i]->channel == MUSIC) {
				// Music takes priority over everything and doesn't work spacially anyway
				index.priority = 10000000.0f * index.sound->gain;
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
	return priorities;
}

bool Manager::Deactivate(SourceBase *sound) {
	alSourceStop(sound->source);
	if (!ErrorCheck("alSourceStop")) return false;
	sound->active = false;
	sound->play = sound->playing;
	if (sound->stream) {
		Stream *stream = (Stream*)sound;
		for (i32 i = 0; i < Assets::numStreamBuffers; i++) {
			if (!stream->Unqueue(stream->file->buffers[i].buffer)) {
				error = "Manager::Deactivate: Failed to Unqueue: " + error;
				return false;
			}
		}
	} else {
		alSourcei(sound->source, AL_BUFFER, 0);
	}
	for (i32 i = 0; i < maxSources; i++) {
		if (sound->source == sources[i]) {
			sourcesFree[i] = true;
			return true;
		}
	}
	error = "Manager::Deactivate: source is not in sources???";
	return false;
}

bool Manager::Activate(SourceBase *sound) {
	for (i32 ii = 0; ii < maxSources; ii++) {
		if (sourcesFree[ii]) {
			sourcesFree[ii] = false;
			sound->source = sources[ii];
			sound->active = true;

			if (!sound->stream) {
				alSourcei(sources[ii], AL_BUFFER, ((Source*)sound)->buffer);
				if (!ErrorCheck("Manager::Activate alSourcei(AL_BUFFER)")) return false;
			} else {
				Stream *stream = (Stream*)sound;
				for (i32 i = 0; i < Assets::numStreamBuffers; i++) {
					if (!stream->file->Decode(stream->file->data.samplerate/8)) {
						error = "Manager::Activate: Failed to Decode: " + Assets::error;
						return false;
					}
					if (!stream->Queue(stream->file->LastBuffer())) {
						error = "Manager::Activate: Failed to Queue: " + error;
						return false;
					}
				}
			}
			return true;
		}
	}
	error = "Manager::Activate: didn't have a free source!";
	return false;
}

bool Manager::UpdateActiveSound(SourceBase *sound) {
	// alSource3f(sound->source, AL_POSITION, sound->position.x, sound->position.y, sound->position.z);
	// ErrorCheck("alSource3f(AL_POSITION)");
	// alSource3f(sound->source, AL_VELOCITY, sound->velocity.x, sound->velocity.y, sound->velocity.z);
	// ErrorCheck("alSource3f(AL_VELOCITY)");
	// alSource3f(sound->source, AL_DIRECTION, sound->direction.x, sound->direction.y, sound->direction.z);
	// ErrorCheck("alSource3f(AL_DIRECTION)");
	alSourcef(sound->source, AL_PITCH, sound->pitch * (sound->simulationPitch ? globals->objects.simulationRate : 1.0f));
	if (!ErrorCheck("alSourcef(AL_PITCH)")) return false;
	f32 gain = sound->gain * globals->volumeMain;
	switch(sound->channel) {
		case MUSIC:
			gain *= globals->volumeMusic;
			break;
		case FX:
			gain *= globals->volumeEffects;
			break;
	}
	alSourcef(sound->source, AL_GAIN, gain);
	if (!ErrorCheck("alSourcef(AL_GAIN)")) return false;
	alSourcei(sound->source, AL_LOOPING, sound->loop);
	if (!ErrorCheck("alSourcei(AL_LOOPING)")) return false;
	// Handle changing play states
	ALint state;
	alGetSourcei(sound->source, AL_SOURCE_STATE, &state);
	if (!ErrorCheck("SourceBase::Playing alGetSourcei(AL_SOURCE_STATE)")) return false;
	if (sound->stream) {
		// Handle fadeouts
		Stream *stream = (Stream*)sound;
		if (stream->fadeout) {
			if (stream->file->data.fadeoutSamples < 0) {
				stream->stop = true;
				stream->fadeout = false;
			}
		}
	}
	bool stopped = false;
	if (state == AL_PLAYING) {
		// We're playing. Should we keep doing that?
		if (sound->pause) {
			if (!Pause(sound)) return false;
		}
		if (sound->stop) {
			if (!Stop(sound)) return false;
			stopped = true;
		}
	}
	if (state != AL_PLAYING || stopped) {
		// Not playing
		if (sound->play) {
			if (!Play(sound)) return false;
		} else if (sound->playing) {
			sound->playing = false;
		}
	}
	return true;
}

bool Manager::Play(SourceBase *sound) {
	alSourcePlay(sound->source);
	if (!ErrorCheck("alSourcePlay")) return false;
	sound->play = false;
	sound->stop = false;
	sound->playing = true;
	return true;
}

bool Manager::Pause(SourceBase *sound) {
	alSourcePause(sound->source);
	if (!ErrorCheck("alSourcePause")) return false;
	sound->pause = false;
	sound->playing = false;
	return true;
}

bool Manager::Stop(SourceBase *sound) {
	alSourceStop(sound->source);
	if (!ErrorCheck("alSourceStop")) return false;
	if (sound->stream) {
		Stream *stream = (Stream*)sound;
		for (i32 i = 0; i < Assets::numStreamBuffers; i++) {
			if (!stream->Unqueue(stream->file->buffers[i].buffer)) {
				error = "Manager::Stop: Failed to Unqueue: " + error;
				return false;
			}
		}
		stream->file->SeekStart();
		for (i32 i = 0; i < Assets::numStreamBuffers; i++) {
			if (!stream->file->Decode(stream->file->data.samplerate/8)) {
				error = "Manager::Activate: Failed to Decode: " + Assets::error;
				return false;
			}
			if (!stream->Queue(stream->file->LastBuffer())) {
				error = "Manager::Activate: Failed to Queue: " + error;
				return false;
			}
		}

	}
	sound->stop = false;
	sound->playing = false;
	return true;
}

bool Manager::Update() {
	if (procFailure) {
		return false;
	}
	Array<PriorityIndex> priorities = GetPriorities();
	soundMutex.Lock();
	// Free up sources from the sounds that got pushed out of priorities
	for (i32 i = maxSources; i < priorities.size; i++) {
		SourceBase *sound = priorities[i].sound;
		if (sound->active) {
			if (!Deactivate(sound)) {
				goto failure;
			}
		}
	}

	for (i32 i = 0; i < min(maxSources, priorities.size); i++) {
		SourceBase *sound = priorities[i].sound;
		if (!sound->active) {
			// We have a sound that's within our priority limit that isn't active so activate it
			if (!Activate(sound)) {
				goto failure;
			}
		}
		if (sound->active) {
			if (!UpdateActiveSound(sound)) goto failure;
		}
	}
	soundMutex.Unlock();
	return true;
failure:
	soundMutex.Unlock();
	return false;
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

void Manager::StreamUpdateProc(Manager *theThisPointer) {
	Array<SourceBase*> &sounds = theThisPointer->sounds;
	while(!theThisPointer->procStop) {
		theThisPointer->soundMutex.Lock();
		for (i32 i = 0; i < sounds.size; i++) {
			if (!sounds[i]->stream || !sounds[i]->active || !sounds[i]->playing) continue;
			Stream *stream = (Stream*)sounds[i];
			if (stream->BuffersDone() > 0) {
				// io::cout.PrintLn("Decode and queue");
				if (!stream->Unqueue(stream->file->data.currentBuffer)) {
					goto failure;
				}
				i32 decoded = stream->file->Decode(stream->file->data.samplerate/8);
				if (decoded < 0) {
					goto failure;
				}
				if (decoded > 0) {
					if (!stream->Queue(stream->file->LastBuffer())) {
						goto failure;
					}
				}
				// stream->buffersToQueue.Append(stream->file->LastBuffer());
			}
		}
		theThisPointer->soundMutex.Unlock();
		Thread::Sleep(Milliseconds(25));
	}
	return;
failure:
	theThisPointer->procFailure = true;
	theThisPointer->soundMutex.Unlock();
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

SourceBase::SourceBase() :
source(0), /* position(0.0), velocity(0.0), direction(0.0), */
pitch(1.0f), gain(1.0f), loop(false), playing(false),
play(false), pause(false), stop(false), active(false),
stereo(false), stream(false), simulationPitch(false), channel(FX) {}

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

void MultiSource::Play(f32 gain, f32 pitch) {
	Stop();
	current = random(0, sources.size-1, &globals->rng);
	sources[current]->SetGain(gain);
	sources[current]->SetPitch(pitch);
	sources[current]->Play();
}

void MultiSource::Play() {
	Stop();
	current = random(0, sources.size-1, &globals->rng);
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

bool Stream::Create(Ptr<Assets::Stream> file_in) {
	if (!file_in.Valid()) {
		error = "Stream::Create: Ptr not valid!";
		return false;
	}
	playing = false;
	channel = MUSIC;
	stream = true;
	file = file_in;
	return true;
}

bool Stream::Create(String filename) {
	i32 streamIndex = globals->assets.FindMapping(filename);
	if (!Create(globals->assets.streams.GetPtr(streamIndex))) {
		return false;
	}
	globals->sound.Register(this);
	return true;
}

void Stream::Stop(f32 fadeoutDuration) {
	if (!active)
		return;
	if (fadeoutDuration > 0.0f) {
		file->BeginFadeout(fadeoutDuration);
		fadeout = true;
	} else {
		stop = true;
	}
}

bool Stream::SetLoopRange(i32 begin, i32 end) {
	if (!file.Valid()) {
		return false;
	}
	if (end < file->data.totalSamples) {
		file->data.loopBeginSample = begin;
		file->data.loopEndSample = end;
	} else if (loop) {
		file->data.loopBeginSample = 0;
		file->data.loopEndSample = file->data.totalSamples;
	} else {
		file->data.loopEndSample = -1;
	}
	return true;
}

bool Stream::Queue(ALuint buffer) {
	if (!active)
		return true;
	alSourceQueueBuffers(source, 1, &buffer);
	return ErrorCheck("Stream::Queue alSourceQueueBuffers");
}

i32 Stream::BuffersDone() {
	if (!active)
		return 0;
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
