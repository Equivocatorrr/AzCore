/*
    File: main.cpp
    Author: Philip Haynes
    Description: High-level definition of the structure of our program.
*/

#include "rendering.hpp"
#include "assets.hpp"
#include "objects.hpp"
#include "gui.hpp"

#include "AzCore/io.hpp"

const char *title = "AzCore Tower Defense";

io::logStream cout("main.log");

i32 main(i32 argumentCount, char** argumentValues) {

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

    Objects::Manager objects;
    objects.Register(new Gui());

    io::Input input;
    objects.input = &input;
    io::Window window;
    objects.window = &window;
    window.name = title;
    window.input = &input;

    Assets::Manager assets;
    objects.GetAssets(&assets);
    if (!assets.LoadAll()) {
        cout << "Failed to load assets: " << Assets::error << std::endl;
        return 1;
    }
    objects.UseAssets(&assets);

    Rendering::Manager rendering;
    rendering.textures = &assets.textures;
    rendering.fonts = &assets.fonts;
    rendering.data.instance.AppInfo(title, 1, 0, 0);
    objects.RegisterDrawing(&rendering);

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
        rendering.data.instance.AddLayers(layers);
    }

    rendering.window = &window;

    if (!window.Open()) {
        cout << "Failed to open window: " << io::error << std::endl;
        return 1;
    }

    if (!rendering.Init()) {
        cout << "Failed to init Rendering::Manager: " << Rendering::error << std::endl;
        return 1;
    }

    if (!window.Show()) {
        cout << "Failed to show window: " << io::error << std::endl;
        return 1;
    }

    ClockTime frameStart;
    const Nanoseconds frameDuration = Nanoseconds(1000000000/144);

    while (window.Update()) {
        if (input.Released(KC_KEY_ESC)) {
            break;
        }
        input.Tick(1.0/144.0);
        frameStart = Clock::now();
        objects.Update(1.0/144.0);
        if (!rendering.Draw()) {
            cout << "Error in Rendering::Manager::Draw: " << Rendering::error << std::endl;
            return 1;
        }
        Nanoseconds frameDelta = Nanoseconds(Clock::now() - frameStart);
        Nanoseconds frameSleep = frameDuration - frameDelta;
        if (frameSleep.count() >= 100) {
            std::this_thread::sleep_for(frameSleep);
        }
    }

    return 0;
}
