/*
	File: game_systems.hpp
	Author: Philip Haynes
	Defines an abstract interface and manager for event-driven systems.
	Helps define interaction between said systems in 
*/

#ifndef AZ3D_GAME_SYSTEMS_HPP
#define AZ3D_GAME_SYSTEMS_HPP

#include "AzCore/memory.hpp"
#include "AzCore/io.hpp"
#include "AzCore/vk.hpp"
#include "rendering.hpp"
#include "sound.hpp"
#include "assets.hpp"
#include "AzCore/Thread.hpp"
#include <atomic>

namespace Az3D::Assets {
struct Manager;
}
namespace Az3D::Rendering {
struct Manager;
}

namespace Az3D::GameSystems {

struct Manager;
struct System;

extern Manager *sys;

// Initializes the engine
bool Init(az::SimpleRange<char> windowTitle, az::Array<System*> systemsToRegister, bool enableVulkanValidation);
// Does the loop internally
void UpdateLoop();
// Cleans up and saves stuff
void Deinit();

// The basis for any object registered with Manager
struct System {
	virtual ~System() = default;

	// Called once at the beginning of the game. Assets requested here are guaranteed to be available by frame 0
	virtual void EventAssetsRequest();
	virtual void EventInitialize();
	// Called once per frame synchronously
	virtual void EventSync();
	virtual void EventUpdate();
	virtual void EventDraw(az::Array<Rendering::DrawingContext> &contexts);
	// Called on application shutdown.
	virtual void EventClose();
};

struct Manager {
	az::Array<System*> systems;
	f32 timestep = 1.0f/60.0f;
	i32 updateIterations = 1;
	f32 simulationRate = 1.0f;
	f32 minUpdateFrequency = 55.0f;
	// Used for frame limiting with vsync off, and update frames in-between rendering frames with vsync on
	az::Nanoseconds frameDuration = az::Nanoseconds(1000000000/60);
	void SetFramerate(f32 framerateTarget, f32 framerateActual);
	az::FrametimeCounter frametimes;
	bool paused = false;
	bool exit = false;
	bool abort = false;
	az::String error;
	
	az::BinaryMap<az::String, az::WString> locale;
	void LoadLocale();
	inline az::WString ReadLocale(az::SimpleRange<char> name) {
		if (!locale.Exists(name))
			return az::ToWString(name);
		else
			return locale[name];
	}
	
	AzCore::io::Input input;
	AzCore::io::Window window;
	AzCore::io::RawInput rawInput;
	AzCore::io::Gamepad *gamepad = nullptr;
	
	az::Thread threadUpdate;
	az::Mutex mutexUpdate;
	az::CondVar condUpdate;
	bool doUpdate, doneUpdate;
	az::Thread threadDraw;
	az::Mutex mutexDraw;
	az::CondVar condDraw;
	bool doDraw, doneDraw;
	az::Mutex mutexControl;
	az::CondVar condControl;
	bool stopThreads;
	
	Sound::Manager sound;
	Assets::Manager assets;
	Rendering::Manager rendering;
	bool enableVulkanValidation;
	
	bool Init();
	void Deinit();
	
	// Calls EventAssetsRequest for every type of object.
	void RequestAssets();
	// Calls EventInitialize
	void CallInitialize();
	// Calls different Sync events.
	void Sync();
	// Calls different Update events.
	void Update();
	// Calls different Draw events.
	void Draw(az::Array<Rendering::DrawingContext>& drawingContexts);

	// Input queries that support gamepads and regular input inline
	bool Repeated(u8 keyCode);
	bool Pressed(u8 keyCode);
	bool Down(u8 keyCode);
	bool Released(u8 keyCode);
	az::io::ButtonState* GetButtonState(u8 keyCode);
	void ConsumeInput(u8 keyCode);
};

} // namespace Az3D::GameSystems

#endif // AZ3D_GAME_SYSTEMS_HPP
