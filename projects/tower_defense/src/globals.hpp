/*
	File: globals.hpp
	Author: Philip Haynes
	Objects and data we want to be accessible anywhere in code.
*/

#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include "rendering.hpp"
#include "assets.hpp"
#include "objects.hpp"
#include "gui.hpp"
#include "entities.hpp"

#include "AzCore/io.hpp"

struct Globals {
	AzCore::String error = "No error";
	Objects::Manager objects;
	AzCore::io::Input input;
	AzCore::io::Window window;
	AzCore::io::RawInput rawInput;
	AzCore::io::Gamepad *gamepad = nullptr;
	Sound::Manager sound;
	Assets::Manager assets;
	Rendering::Manager rendering;
	Entities::Manager entities;
	Int::Gui gui;
	AzCore::RandomNumberGenerator rng;
	bool exit = false;
	AzCore::Nanoseconds frameDuration = AzCore::Nanoseconds(1000000000/60);
	AzCore::BinaryMap<AzCore::String, AzCore::WString> locale;
	AzCore::FrametimeCounter frametimes;
	// Settings
	bool fullscreen = false;
	bool vsync = true;
	bool debugInfo = false;
	f32 framerate = 60.0f;
	f32 volumeMain = 1.0f;
	f32 volumeMusic = 1.0f;
	f32 volumeEffects = 1.0f;
	char localeOverride[2] = {0};

	void LoadLocale();
	inline AzCore::WString ReadLocale(AzCore::String name) {
		if (!locale.Exists(name))
			return AzCore::ToWString(name);
		else
			return locale[name];
	}
	bool LoadSettings();
	bool SaveSettings();
	inline void Framerate(const f32 &fr) {
		framerate = fr;
		objects.timestep = 1.0f / framerate;
		frameDuration = AzCore::Nanoseconds(1000000000/(i32)fr);
	}
};

extern Globals *globals;

#endif
