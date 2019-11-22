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
    String error = "No error";
    Objects::Manager objects;
    io::Input input;
    io::Window window;
    io::RawInput rawInput;
    io::Gamepad *gamepad = nullptr;
    Assets::Manager assets;
    Rendering::Manager rendering;
    Entities::Manager entities;
    Int::Gui gui;
    Sound::Manager sound;
    RandomNumberGenerator rng;
    bool exit = false;
    Nanoseconds frameDuration;
    // Settings
    bool fullscreen = false;
    f32 framerate = 60;
    f32 volumeMain = 1.0;
    f32 volumeMusic = 1.0;
    f32 volumeEffects = 1.0;

    bool LoadSettings();
    bool SaveSettings();
    inline void Framerate(const f32 &fr) {
        framerate = fr;
        objects.timestep = 1.0 / framerate;
        frameDuration = Nanoseconds(1000000000/(i32)fr);
    }
};

extern Globals *globals;

#endif
