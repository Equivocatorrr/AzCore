/*
	File: RawInput.cpp
	Author: Philip Haynes
*/

#include "RawInput.hpp"

namespace AzCore {

namespace io {

const char *RawInputDeviceTypeString[5] = {
	"Unsupported",
	"Keyboard",
	"Mouse",
	"Gamepad",
	"Joystick"
};

RawInputDevice::RawInputDevice(RawInputDevice &&other) {
	data = other.data;
	other.data = nullptr;
	rawInput = other.rawInput;
	type = other.type;
}

} // namespace io

} // namespace AzCore
