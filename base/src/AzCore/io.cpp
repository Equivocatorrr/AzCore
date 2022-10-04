/*
	File: io.cpp
	Author: Philip Haynes
*/

#include "io.hpp"

namespace AzCore {

namespace io {

	String error = "No Error";
	vec2 screenSize;
	Log cout("io.log", true, true);

} // namespace io

} // namespace AzCore

#include "IO/Gamepad.cpp"
#include "IO/RawInput.cpp"
#include "IO/ButtonState.cpp"
#include "IO/Input.cpp"

#ifdef __unix
#include "IO/Linux/RawInput.cpp"
#include "IO/Linux/Window.cpp"
#elif defined(_WIN32)
#include "IO/Win32/RawInput.cpp"
#include "IO/Win32/Window.cpp"
#endif
