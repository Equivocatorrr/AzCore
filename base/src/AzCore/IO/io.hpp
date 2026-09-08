/*
	File: io.hpp
	Author: Philip Haynes
	Description: Handles keyboard, mouse, gamepads/joysticks, windows, ect.
*/
#ifndef AZCORE_IO_HPP
#define AZCORE_IO_HPP

#include "../Memory/String.hpp"
#include "Log.hpp"

namespace AzCore {

namespace io {

	extern String error;
	extern Log ioLog;

} // namespace io

} // namespace AzCore

#include "ButtonState.hpp"
#include "Input.hpp"
#include "KeyCodes.hpp"
#include "RawInput.hpp"
#include "Gamepad.hpp"
#include "Window.hpp"

#endif // AZCORE_IO_HPP
