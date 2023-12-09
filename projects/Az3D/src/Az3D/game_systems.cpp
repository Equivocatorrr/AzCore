/*
	File: game_systems.cpp
	Author: Philip Haynes
*/

#include "game_systems.hpp"
// #include "gui_basics.hpp"
#include "settings.hpp"
#include "AzCore/Profiling.hpp"
#include "AzCore/Thread.hpp"

#include <clocale>

namespace Az3D::GameSystems {

using namespace AzCore;

Manager *sys = nullptr;

void System::EventAssetsRequest() {}
void System::EventSync() {}
void System::EventUpdate() {}
void System::EventDraw(Array<Rendering::DrawingContext> &contexts) {}
void System::EventInitialize() {}
void System::EventClose() {}

bool Init(SimpleRange<char> windowTitle, Array<System*> systemsToRegister, bool enableVulkanValidation) {
	AZCORE_PROFILING_FUNC_TIMER()
	sys = new Manager();
	for (System *system : systemsToRegister) {
		sys->systems.Append(system);
	}
	sys->window.name = windowTitle;
	sys->sound.name = windowTitle;
	GPU::SetAppName(windowTitle.str);
	sys->enableVulkanValidation = enableVulkanValidation;
	return sys->Init();
}

void UpdateProc(Manager *manager) {
	manager->mutexUpdate.Lock();
	while (true) {
		while (!manager->doUpdate) {
			manager->condUpdate.Wait(manager->mutexUpdate);
			if (manager->abort || manager->stopThreads) goto break_full;
		}
		manager->doUpdate = false;
		manager->Update();
		manager->doneUpdate = true;
		manager->condControl.WakeAll();
	}
break_full:
	manager->mutexUpdate.Unlock();
	manager->condControl.WakeAll();
}

void DrawProc(Manager *manager) {
	manager->mutexDraw.Lock();
	while (true) {
		while (!manager->doDraw) {
			manager->condDraw.Wait(manager->mutexDraw);
			if (manager->abort || manager->stopThreads) goto break_full;
		}
		manager->doDraw = false;
		if (!manager->rendering.Draw() || !manager->rendering.Present()) {
			io::cerr.Lock().PrintLn("Error in Rendering::Manager::Draw or Present: ", Rendering::error).Unlock();
			manager->abort = true;
			goto break_full;
		};
		manager->doneDraw = true;
		manager->condControl.WakeAll();
	}
break_full:
	manager->mutexDraw.Unlock();
	manager->condControl.WakeAll();
}

void UpdateLoop() {
	ClockTime frameStart, frameNext;
	frameNext = Clock::now();
	bool soundProblem = false;

	f32 exitDelay = 0.1f;
	bool exit = false;
	i32 frame = 0;

	while (exitDelay > 0.0f && !sys->abort) {
		sys->window.Fullscreen(Settings::ReadBool(Settings::sFullscreen));
		if ((!sys->window.Update() || sys->exit) && !exit) {
			exit = true;
			sys->sound.FadeoutAll(0.2f);
		}
		if (sys->input.Pressed(KC_KEY_F11)) {
			Settings::SetBool(Settings::sFullscreen, !Settings::ReadBool(Settings::sFullscreen));
		}
		if (sys->input.Pressed(KC_KEY_F12)) {
			Settings::SetBool(Settings::sVSync, !Settings::ReadBool(Settings::sVSync));
		}
		if (exit) {
			exitDelay -= sys->timestep;
		}
		bool vsync = Settings::ReadBool(Settings::sVSync);
		if (frame == 0) {
			sys->frametimes.Update();
			f32 measuredFramerate = 1000.0f / sys->frametimes.AverageWithoutOutliers();
			f32 targetFramerate;
			if (vsync) {
				targetFramerate = clamp((f32)sys->window.refreshRate / 1000.0f, 30.0f, 300.0f);
			} else {
				targetFramerate = 1000.0f;
				if (Settings::ReadBool(Settings::sFramerateLimitEnabled)) {
					targetFramerate = Settings::ReadReal(Settings::sFramerateLimit);
				}
			}
			sys->SetFramerate(targetFramerate, measuredFramerate);
		}
		if (abs(Nanoseconds(frameNext - Clock::now()).count()) >= sys->frameDuration.count()*4) {
			// Something must have hung the program. Start fresh.
			frameStart = Clock::now();
		} else {
			frameStart = frameNext;
		}
		frameNext = frameStart + sys->frameDuration;
		// {
		// 	i32 dpi = sys->window.GetDPI();
		// 	f32 scale = (f32)dpi / 96.0f;
		// 	Gui::guiBasic->scale = scale;
		// }
		sys->rawInput.Update(sys->timestep);
		sys->Sync();
		
		sys->mutexUpdate.Lock();
		sys->doUpdate = true;
		sys->doneUpdate = false;
		sys->mutexUpdate.Unlock();
		sys->condUpdate.WakeAll();
		sys->mutexDraw.Lock();
		sys->doDraw = true;
		sys->doneDraw = false;
		sys->mutexDraw.Unlock();
		sys->condDraw.WakeAll();
		
		sys->mutexControl.Lock();
		while (!(sys->doneUpdate && sys->doneDraw)) {
			sys->condControl.Wait(sys->mutexControl);
			if (sys->abort) break;
		}
		sys->mutexControl.Unlock();
		
		if (sys->abort) break;
		
		if (!soundProblem) {
			if (!sys->sound.Update(sys->timestep)) {
				io::cerr.PrintLn(Sound::error);
				if (!sys->sound.DeleteSources()) {
					io::cerr.PrintLn("Failed to delete sound sources: ", Sound::error);
				}
				// Sound problems probably shouldn't crash the whole game
				soundProblem = true;
			}
		}
		sys->input.Tick(sys->timestep);
		{
			Nanoseconds frameSleep = Nanoseconds(frameNext - Clock::now()) - sys->frameDuration;
			if (frameSleep.count() >= 1000000) {
				Thread::Sleep(frameSleep);
			}
		}
		frame = (frame + 1) % sys->updateIterations;
	}
	
	for (System* system : sys->systems) {
		system->EventClose();
	}
}

void Deinit() {
	{
		AZCORE_PROFILING_FUNC_TIMER()
		sys->Deinit();
		delete sys;
	}
	az::Profiling::Report();
}

bool Manager::Init() {
	AZCORE_PROFILING_FUNC_TIMER()
	window.input = &input;
	rawInput.window = &window;
	LoadLocale();
	Settings::Load();
	if (!rawInput.Init(io::RAW_INPUT_ENABLE_GAMEPAD_BIT)) {
		error = Stringify("Failed to initialize RawInput: ", io::error);
		return false;
	}
	if (!sound.Initialize()) {
		error = Stringify("Failed to initialize sound: ", Sound::error);
		return false;
	}
	assets.Init();
	RequestAssets();
	CallInitialize();
	
	if (enableVulkanValidation) {
		GPU::EnableValidationLayers();
	}
	rendering.data.concurrency = 4;
	
	if (!window.Open()) {
		error = Stringify("Failed to open window: ", io::error);
		return false;
	}
	{
		i32 dpi = window.GetDPI();
		f32 scale = (f32)dpi / 96.0f;
		// Gui::guiBasic->scale = scale;
		window.Resize(u32((f32)window.width * scale), u32((u32)window.height * scale));
	}
	
	if (!rendering.Init()) {
		error = Stringify("Failed to init Rendering::Manager: ", Rendering::error);
		return false;
	}

	if (!window.Show()) {
		error = Stringify("Failed to show window: ", io::error);
		return false;
	}

	window.Fullscreen(Settings::ReadBool(Settings::sFullscreen));
	
	doUpdate = doDraw = doneUpdate = doneDraw = stopThreads = false;
	threadUpdate = Thread(UpdateProc, this);
	threadDraw = Thread(DrawProc, this);
	
	return true;
}

void Manager::Deinit() {
	if (!rendering.Deinit()) {
		io::cerr.PrintLn("Error deinitializing Rendering: ", Rendering::error);
	}
	window.Close();
	Settings::Save();
	if (!sound.DeleteSources()) {
		io::cerr.PrintLn("Failed to delete sound sources: ", Sound::error);
	}
	assets.Deinit();
	if (!sound.Deinitialize()) {
		io::cerr.PrintLn("Failed to deinitialize sound: ", Sound::error);
	}
	stopThreads = true;
	condUpdate.WakeAll();
	condDraw.WakeAll();
	threadUpdate.Join();
	threadDraw.Join();
	// NOTE: There appears to be a bug on shutdown where the last second or so of audio gets repeated for a split second before being cut off. (Confirmed on Windows, may be an OpenAL bug)
}

void Manager::LoadLocale() {
	AZCORE_PROFILING_FUNC_TIMER()
	String localeName;
	localeName.Reserve(21);
	localeName = "data/locale/";

	// if (localeOverride[0] != 0) {
	// 	localeName += localeOverride[0];
	// 	localeName += localeOverride[1];
	// } else {
		std::setlocale(LC_ALL, "");
		char *localeString = std::setlocale(LC_CTYPE, NULL);

		io::cout.PrintLn("localeString = ", localeString);

		localeName += localeString[0];
		localeName += localeString[1];
	// }

	localeName += ".locale";

	Array<char> buffer = FileContents(localeName);
	if (buffer.size == 0) {
		buffer = FileContents("data/locale/en.locale");
	}
	if (buffer.size == 0) return;

	bool skipToNewline = buffer[0] == '#';

	for (i32 i = 0; i < buffer.size;)
	{
		if (buffer[i] == '\n') {
			i++;
			if (i < buffer.size) {
				skipToNewline = buffer[i] == '#';
			}
			continue;
		}
		if (skipToNewline) {
			i += CharLen(buffer[i]);
			continue;
		}
		String name;
		String text;
		for (i32 j = i; j < buffer.size; j += CharLen(buffer[j])) {
			if (buffer[j] == '=')
			{
				name.Resize(j - i);
				memcpy(name.data, &buffer[i], name.size);
				i += name.size + 1;
				break;
			}
		}
		for (; i < buffer.size; i += CharLen(buffer[i])) {
			if (buffer[i] == '"') {
				i++;
				break;
			}
		}
		i32 start = i;
		for (; i < buffer.size; i += CharLen(buffer[i])) {
			if (buffer[i] == '"')
				break;
		}
		text.Resize(i - start);
		if (text.size) {
			memcpy(text.data, &buffer[start], text.size);
		}
		locale[name] = ToWString(text);
		i++;
	}
}

void Manager::SetFramerate(f32 framerateTarget, f32 framerateMeasured) {
	if (abs(framerateTarget - framerateMeasured) / framerateTarget < 0.02f) {
		// If we're consistent enough, cut all measured jitter to zero
		framerateMeasured = framerateTarget;
	}
	timestep = 1.0f / framerateMeasured;
	updateIterations = min((i32)ceil(minUpdateFrequency * timestep), 10);
	timestep /= updateIterations;
	framerateTarget *= updateIterations;
	frameDuration = Nanoseconds(1000000000/(i64)framerateTarget);
}

void Manager::RequestAssets() {
	AZCORE_PROFILING_FUNC_TIMER()
	for (System* system : systems) {
		system->EventAssetsRequest();
	}
}

void Manager::CallInitialize() {
	AZCORE_PROFILING_FUNC_TIMER()
	for (System* system : systems) {
		system->EventInitialize();
	}
}

void Manager::Sync() {
	AZCORE_PROFILING_FUNC_TIMER()
	if (!paused) {
		sys->simulationRate = min(1.0f, sys->simulationRate + sys->timestep * 5.0f);
	} else {
		sys->simulationRate = max(0.0f, sys->simulationRate - sys->timestep * 5.0f);
	}
	if (rawInput.AnyGP.Pressed()) {
		gamepad = &rawInput.gamepads[rawInput.AnyGPIndex];
	}
	for (System* system : systems) {
		system->EventSync();
	}
}

void Manager::Update() {
	AZCORE_PROFILING_FUNC_TIMER()
	for (System* system : systems) {
		system->EventUpdate();
	}
}

void Manager::Draw(Array<Rendering::DrawingContext>& contexts) {
	AZCORE_PROFILING_FUNC_TIMER()
	for (System *system : systems) {
		system->EventDraw(contexts);
	}
}

io::ButtonState* Manager::GetButtonState(u8 keyCode) {
	if (KeyCodeIsGamepad(keyCode)) {
		if (gamepad == nullptr) {
			return nullptr;
		}
		return gamepad->GetButtonState(keyCode);
	} else {
		return &input.GetButtonState(keyCode);
	}
}

bool Manager::Repeated(u8 keyCode) {
	io::ButtonState* state = GetButtonState(keyCode);
	if (!state) return false;
	return state->Repeated();
}

bool Manager::Pressed(u8 keyCode) {
	io::ButtonState* state = GetButtonState(keyCode);
	if (!state) return false;
	return state->Pressed();
}

bool Manager::Down(u8 keyCode) {
	io::ButtonState* state = GetButtonState(keyCode);
	if (!state) return false;
	return state->Down();
}

bool Manager::Released(u8 keyCode) {
	io::ButtonState* state = GetButtonState(keyCode);
	if (!state) return false;
	return state->Released();
}

void Manager::ConsumeInput(u8 keyCode) {
	io::ButtonState* state = GetButtonState(keyCode);
	if (!state) return;
	state->Set(false, false, false);
}

}
