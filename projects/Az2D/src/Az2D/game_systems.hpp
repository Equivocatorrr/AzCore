/*
	File: game_systems.hpp
	Author: Philip Haynes
	Defines an abstract interface and manager for event-driven systems.
	Helps define interaction between said systems in 
*/

#ifndef AZ2D_GAME_SYSTEMS_HPP
#define AZ2D_GAME_SYSTEMS_HPP

#include "AzCore/memory.hpp"
#include "AzCore/io.hpp"
#include "AzCore/vk.hpp"
#include "rendering.hpp"
#include "sound.hpp"
#include "assets.hpp"
#include <atomic>

namespace Az2D::Assets {
struct Manager;
}
namespace Az2D::Rendering {
struct Manager;
}

namespace Az2D::GameSystems {

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

	// Queue all asset files in this event
	virtual void EventAssetsQueue();
	// Get all your asset mappings in this event
	virtual void EventAssetsAcquire();
	virtual void EventInitialize();
	// Called once per frame synchronously
	virtual void EventSync();
	virtual void EventUpdate();
	virtual void EventDraw(az::Array<Rendering::DrawingContext> &contexts);
	// Called on application shutdown.
	virtual void EventClose();
};

struct Manager {
	// buffer swaps every frame. Used for lockless multithreading.
	az::Array<System*> systems;
	bool buffer = false;
	f32 timestep = 1.0f/60.0f;
	i32 updateIterations = 1;
	f32 simulationRate = 1.0f;
	f32 minUpdateFrequency = 59.0f;
	az::Nanoseconds frameDuration = az::Nanoseconds(1000000000/60);
	void SetFramerate(f32 framerate, bool tryCatchup=false);
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
	
	Sound::Manager sound;
	Assets::Manager assets;
	Rendering::Manager rendering;
	bool enableVulkanValidation;
	
	bool Init();
	void Deinit();
	
	static void RenderCallback(void *userdata, Rendering::Manager *rendering, az::Array<Rendering::DrawingContext>& drawingContexts);

	// Registers the rendering callbacks
	void RegisterDrawing();
	// Calls EventAssetInit for every type of object.
	void GetAssets();
	// Calls EventAssetAcquire for every type of object.
	void UseAssets();
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

} // namespace Az2D::GameSystems

#endif // AZ2D_GAME_SYSTEMS_HPP
