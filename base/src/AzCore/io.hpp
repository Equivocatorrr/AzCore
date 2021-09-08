/*
    File: io.hpp
    Author: Philip Haynes
    Description: Handles keyboard, mouse, gamepads/joysticks, windows, ect.
    TODO:
        - Add raw input support (including gamepads/joysticks)
*/
#ifndef AZCORE_IO_HPP
#define AZCORE_IO_HPP

#include "IO/LogStream.hpp"
#include "IO/Log.hpp"
#include "math.hpp"

namespace AzCore {

namespace io {

    extern String error;
    extern vec2 screenSize;

    extern Log cout;

} // namespace io

} // namespace AzCore

#include "IO/ButtonState.hpp"
#include "IO/Input.hpp"
#include "IO/RawInput.hpp"
#include "IO/Gamepad.hpp"
#include "IO/Window.hpp"

#endif // AZCORE_IO_HPP
