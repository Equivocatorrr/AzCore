/*
	File: main.cpp
	Author: Philip Haynes
	Description: High-level definition of the structure of our program.
*/

#include "globals.hpp"
#include "AzCore/io.hpp"
#include "AzCore/Thread.hpp"
#include "AzCore/Time.hpp"

using namespace AzCore;

const char *title = "AzCore Tower Defense";

io::Log cout("main.log", true, true);

void UpdateProc() {
	globals->objects.Update();
}

void DrawProc() {
	if (!globals->rendering.Draw()) {
		cout.Lock().PrintLn("Error in Rendering::Manager::Draw: ", Rendering::error).Unlock();
		globals->exit = true;
	};
}

#define DEBUG_SLEEP 0

i32 main(i32 argumentCount, char** argumentValues) {
	ClockTime loadStart = Clock::now();
	Globals _globals;
	globals = &_globals;

	bool enableLayers = false, enableCoreValidation = false;

	cout.PrintLn("\nTest program received ", argumentCount, " arguments:");
	for (i32 i = 0; i < argumentCount; i++) {
		cout.PrintLn(i, ": ", argumentValues[i]);
		if (equals(argumentValues[i], "--enable-layers")) {
			enableLayers = true;
		} else if (equals(argumentValues[i], "--core-validation")) {
			enableCoreValidation = true;
		}
	}

	cout.PrintLn("Starting with layers ", (enableLayers ? "enabled" : "disabled"), " and core validation ", (enableCoreValidation ? "enabled" : "disabled"));
	
	if (enableLayers) {
		Array<const char*> layers = {
			"VK_LAYER_KHRONOS_validation"
		};
		globals->rendering.data.instance.AddLayers(layers);
	}

	if (!globals->LoadSettings()) {
		cout.PrintLn("No settings to load. Using defaults.");
	}

	globals->LoadLocale();

	globals->objects.Register(&globals->entities);
	globals->objects.Register(&globals->gui);

	globals->rendering.data.concurrency = 4;

	globals->window.name = title;
	globals->window.input = &globals->input;

	globals->rawInput.window = &globals->window;
	if (!globals->rawInput.Init(io::RAW_INPUT_ENABLE_GAMEPAD_BIT)) {
		cout.PrintLn("Failed to initialize RawInput: ", io::error);
		return 1;
	}

	globals->sound.name = "AzCore Tower Defense";
	if (!globals->sound.Initialize()) {
		cout.PrintLn("Failed to initialize sound: ", Sound::error);
		return 1;
	}

	globals->objects.GetAssets();
	if (!globals->assets.LoadAll()) {
		cout.PrintLn("Failed to load assets: ", Assets::error);
		return 1;
	}
	globals->objects.UseAssets();

	globals->rendering.data.instance.AppInfo(title, 1, 0, 0);
	globals->objects.RegisterDrawing(&globals->rendering);

	globals->objects.CallInitialize();

	if (enableLayers) {
		Array<const char*> layers = {
			"VK_LAYER_GOOGLE_threading",
			"VK_LAYER_LUNARG_parameter_validation",
			"VK_LAYER_LUNARG_object_tracker",
			"VK_LAYER_GOOGLE_unique_objects"
		};
		if (enableCoreValidation) {
			layers.Append("VK_LAYER_LUNARG_core_validation");
		}
		globals->rendering.data.instance.AddLayers(layers);
	}


	if (!globals->window.Open()) {
		cout.PrintLn("Failed to open window: ", io::error);
		return 1;
	}
	{
		i32 dpi = globals->window.GetDPI();
		f32 scale = (f32)dpi / 96.0f;
		globals->gui.scale = scale;
		globals->window.Resize(u32((f32)globals->window.width * scale), u32((u32)globals->window.height * scale));
	}
	globals->window.HideCursor();

	if (!globals->rendering.Init()) {
		cout.PrintLn("Failed to init Rendering::Manager: ", Rendering::error);
		return 1;
	}

	if (!globals->window.Show()) {
		cout.PrintLn("Failed to show window: ", io::error);
		return 1;
	}

	globals->window.Fullscreen(globals->fullscreen);

	cout.PrintLn("Initialization took ", FormatTime(Clock::now() - loadStart));

	ClockTime frameStart, frameNext;
	frameNext = Clock::now();

	while (globals->window.Update() && !globals->exit) {
		globals->frametimes.Update();
		if (globals->vsync) {
			globals->Framerate(1000.0f / globals->frametimes.Average());
		}
		if (abs(Nanoseconds(frameNext - Clock::now()).count()) >= 10000000) {
			// Something must have hung the program. Start fresh.
			frameStart = Clock::now();
#if DEBUG_SLEEP
			cout.PrintLn("Sync! Frame difference was ", Nanoseconds(frameNext - Clock::now()).count()/1000000, "ms");
			cout.PrintLn("frameDuration is ", globals->frameDuration.count() / 1000000, "ms");
#endif
		} else {
			frameStart = frameNext;
		}
		frameNext = frameStart + globals->frameDuration;
		{
			i32 dpi = globals->window.GetDPI();
			f32 scale = (f32)dpi / 96.0f;
			globals->gui.scale = scale;
		}
		globals->rawInput.Update(globals->objects.timestep);
		globals->objects.Sync();
		Thread threads[2];
		threads[0] = Thread(UpdateProc);
		threads[1] = Thread(DrawProc);
		for (i32 i = 0; i < 2; i++) {
			if (threads[i].Joinable()) threads[i].Join();
		}
		if (!globals->sound.Update()) {
			cout.PrintLn(Sound::error);
			return false;
		}
		globals->input.Tick(globals->objects.timestep);
		if (!globals->vsync) {
			Nanoseconds frameSleep = frameNext - Clock::now() - Nanoseconds(1000000);
			if (frameSleep.count() >= 1000000) {
#if DEBUG_SLEEP
				ClockTime sleepStart = Clock::now();
				cout.PrintLn("Sleeping for ", frameSleep.count()/1000, "us");
#endif
				Thread::Sleep(frameSleep);
#if DEBUG_SLEEP
				cout.PrintLn("Actually slept for ", Nanoseconds(Clock::now() - sleepStart).count() / 1000, "us");
#endif
			}
		}
	}
	if (!globals->SaveSettings()) {
		cout.PrintLn("Failed to save settings: ", globals->error);
	}
	if (!globals->rendering.Deinit()) {
		cout.PrintLn("Error deinitializing Rendering::Manager: ", Rendering::error);
		return 1;
	}
	Thread::Sleep(Milliseconds(80)); // Don't cut off the exit click sound
	if (!globals->sound.DeleteSources()) {
		cout.PrintLn("Failed to delete sound sources: ", Sound::error);
		return 1;
	}
	globals->assets.sounds.Clear(); // Deletes the OpenAL buffers
	globals->assets.streams.Clear(); // Deletes the OpenAL buffers
	globals->window.Close();
	if (!globals->sound.Deinitialize()) {
		cout.PrintLn("Failed to deinitialize sound: ", Sound::error);
		return 1;
	}

	return 0;
}
