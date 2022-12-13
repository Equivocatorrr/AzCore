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

using namespace AzCore;

struct Manager;
struct System;

extern Manager *sys;

// Initializes the engine
bool Init(SimpleRange<char> windowTitle, Array<System*> systemsToRegister, bool enableVulkanValidation);
// Does the loop internally
void UpdateLoop();
// Cleans up and saves stuff
void Deinit();

// The basis for any object registered with Manager
struct System {
	std::atomic<bool> readyForDraw = false;
	virtual ~System() = default;

	virtual void EventAssetInit() = 0;
	virtual void EventAssetAcquire() = 0;
	virtual void EventInitialize();
	virtual void EventSync();
	virtual void EventUpdate();
	virtual void EventDraw(Array<Rendering::DrawingContext> &contexts);
	// Called on application shutdown.
	virtual void EventClose();
};

struct Manager {
	// buffer swaps every frame. Used for lockless multithreading.
	Array<System*> systems;
	bool buffer = false;
	f32 timestep = 1.0f/60.0f;
	f32 simulationRate = 1.0f;
	Nanoseconds frameDuration = Nanoseconds(1000000000/60);
	void SetFramerate(f32 framerate);
	FrametimeCounter frametimes;
	bool paused = false;
	bool exit = false;
	String error;
	
	AzCore::BinaryMap<String, WString> locale;
	void LoadLocale();
	inline WString ReadLocale(SimpleRange<char> name) {
		if (!locale.Exists(name))
			return AzCore::ToWString(name);
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
	
	static void RenderCallback(void *userdata, Rendering::Manager *rendering, Array<Rendering::DrawingContext>& drawingContexts);

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
	void Draw(Array<Rendering::DrawingContext>& drawingContexts);

	// Input queries that support gamepads and regular input inline
	bool Pressed(u8 keyCode);
	bool Down(u8 keyCode);
	bool Released(u8 keyCode);
	io::ButtonState* GetButtonState(u8 keyCode);
	void ConsumeInput(u8 keyCode);
};

} // namespace Az2D::GameSystems

#endif // AZ2D_GAME_SYSTEMS_HPP
