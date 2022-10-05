/*
	File: io.hpp
	Author: Philip Haynes
	Description: Handles keyboard, mouse, gamepads/joysticks, windows, ect.
	TODO:
		- Add raw input support (including gamepads/joysticks)
*/
#ifndef AZCORE_IO_HPP
#define AZCORE_IO_HPP

#include "Memory/String.hpp"
#include "math.hpp"
#include "IO/Log.hpp"
#include "Memory/Array.hpp"
#include "Memory/Range.hpp"

namespace AzCore {

namespace io {

	extern String error;
	extern vec2 screenSize;

	extern Log cout;

	inline Array<SimpleRange<char>> GetArguments(i32 argc, char *argv[]) {
		// NOTE: Do we want to omit the first one since it's just the app name on the system?
		Array<SimpleRange<char>> out;
		out.Reserve(argc-1);
		for (i32 i = 1; i < argc; i++) {
			out.Append(argv[i]);
		}
		return out;
	}

} // namespace io

} // namespace AzCore

#include "IO/ButtonState.hpp"
#include "IO/Input.hpp"
#include "keycodes.hpp"
#include "IO/RawInput.hpp"
#include "IO/Gamepad.hpp"
#include "IO/Window.hpp"

#endif // AZCORE_IO_HPP
