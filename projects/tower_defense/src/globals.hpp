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

struct Globals {
    Objects::Manager objects;
    io::Input input;
    io::Window window;
    io::RawInput rawInput;
    io::Gamepad *gamepad = nullptr;
    Assets::Manager assets;
    Rendering::Manager rendering;
    Entities::Manager entities;
    Int::Gui gui;
    RandomNumberGenerator rng;
};

extern Globals *globals;

#endif
