/*
	File: io.cpp
	Author: Philip Haynes
*/

#include "io.hpp"

namespace AzCore {

namespace io {

	String error = "No Error";
	Log ioLog("io.log", true, true);

} // namespace io

} // namespace AzCore

#include "Gamepad.cpp"
#include "RawInput.cpp"
#include "ButtonState.cpp"
#include "Input.cpp"

#ifdef __unix
#include "Linux/RawInput.cpp"
#include "Linux/Window.cpp"
#elif defined(_WIN32)
#include "Win32/RawInput.cpp"
#include "Win32/Window.cpp"
#endif
