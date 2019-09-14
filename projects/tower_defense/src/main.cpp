/*
    File: main.cpp
    Author: Philip Haynes
    Description: High-level definition of the structure of our program.
*/

#include "globals.hpp"
#include "AzCore/io.hpp"

const char *title = "AzCore Tower Defense";

io::logStream cout("main.log");

void UpdateProc() {
    globals->objects.Update();
}

void DrawProc() {
    if (!globals->rendering.Draw()) {
        cout.MutexLock();
        cout << "Error in Rendering::Manager::Draw: " << Rendering::error << std::endl;
        globals->exit = true;
        cout.MutexUnlock();
    };
}

i32 main(i32 argumentCount, char** argumentValues) {

    Globals _globals;
    globals = &_globals;

    bool enableLayers = false, enableCoreValidation = false;

    cout << "\nTest program received " << argumentCount << " arguments:\n";
    for (i32 i = 0; i < argumentCount; i++) {
        cout << i << ": " << argumentValues[i] << std::endl;
        if (equals(argumentValues[i], "--enable-layers")) {
            enableLayers = true;
        } else if (equals(argumentValues[i], "--core-validation")) {
            enableCoreValidation = true;
        }
    }

    cout << "Starting with layers " << (enableLayers ? "enabled" : "disabled")
         << " and core validation " << (enableCoreValidation ? "enabled" : "disabled") << std::endl;

    globals->objects.Register(&globals->entities);
    globals->objects.Register(&globals->gui);

    globals->rendering.data.concurrency = 4;

    globals->window.name = title;
    globals->window.input = &globals->input;

    globals->rawInput.window = &globals->window;
    if (!globals->rawInput.Init(io::RAW_INPUT_ENABLE_GAMEPAD_BIT)) {
        cout << "Failed to initialize RawInput: " << io::error << std::endl;
        return 1;
    }

    globals->objects.GetAssets();
    if (!globals->assets.LoadAll()) {
        cout << "Failed to load assets: " << Assets::error << std::endl;
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
        cout << "Failed to open window: " << io::error << std::endl;
        return 1;
    }

    if (!globals->rendering.Init()) {
        cout << "Failed to init Rendering::Manager: " << Rendering::error << std::endl;
        return 1;
    }

    if (!globals->window.Show()) {
        cout << "Failed to show window: " << io::error << std::endl;
        return 1;
    }

    ClockTime frameStart;
    const Nanoseconds frameDuration = Nanoseconds(1000000000/144);
    globals->objects.timestep = 1.0/144.0;

    while (globals->window.Update() && !globals->exit) {
        frameStart = Clock::now();
        globals->rawInput.Update(globals->objects.timestep);
        if (globals->input.Released(KC_KEY_ESC)) {
            break;
        }
        globals->objects.Sync();
        Thread threads[2];
        threads[0] = Thread(UpdateProc);
        threads[1] = Thread(DrawProc);
        for (i32 i = 0; i < 2; i++) {
            if (threads[i].joinable()) threads[i].join();
        }
        globals->input.Tick(globals->objects.timestep);
        Nanoseconds frameDelta = Nanoseconds(Clock::now() - frameStart);
        Nanoseconds frameSleep = frameDuration - frameDelta;
        if (frameSleep.count() >= 1000) {
            std::this_thread::sleep_for(frameSleep);
        }
    }

    if (!globals->rendering.Deinit()) {
        cout << "Error deinitializing Rendering::Manager: " << Rendering::error << std::endl;
        return 1;
    }
    globals->window.Close();

    return 0;
}
