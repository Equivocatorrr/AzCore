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

	inline bool ArgumentIsFlag(char *argument) {
		if (!argument) return false;
		if (argument[0] != '-') return false;
		if (argument[1] != '-') return false;
		return true;
	}

	inline SimpleRange<char> ArgumentFlag(char *argument) {
		return SimpleRange<char>(argument+2);
	}

	struct Argument {
		// Whether it starts with '--'
		bool isFlag;
		// The text part minus the beginning '--' if it's a flag
		SimpleRange<char> str;
	};

	inline Array<Argument> GetArguments(i32 argc, char *argv[]) {
		// NOTE: Do we want to omit the first one since it's just the app name on the system?
		Array<Argument> out(argc-1);
		for (i32 i = 1; i < argc; i++) {
			if (ArgumentIsFlag(argv[i])) {
				out.Append({true, ArgumentFlag(argv[i])});
			} else {
				out.Append({false, argv[i]});
			}
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
