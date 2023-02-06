/*
	File: Gamepad.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_GAMEPAD_HPP
#define AZCORE_GAMEPAD_HPP

// For most gamepads, these values should be sufficient.
#define IO_GAMEPAD_MAX_BUTTONS 15
#define IO_GAMEPAD_MAX_AXES 8

#include "ButtonState.hpp"
#include "RawInput.hpp"
#include "../Memory/Ptr.hpp"
#include "../math.hpp"

namespace AzCore {

namespace io {

enum GamepadAxisArrayIndices {
	GP_AXIS_LS_X = 0x00,
	GP_AXIS_LS_Y = 0x01,
	GP_AXIS_RS_X = 0x03,
	GP_AXIS_RS_Y = 0x04,
	GP_AXIS_LT = 0x02,
	GP_AXIS_RT = 0x05,
	GP_AXIS_H0_X = 0x06,
	GP_AXIS_H0_Y = 0x07
};
/*  struct: Gamepad
		Author: Philip Haynes
		Utilities to use a RawInputDevice as a gamepad.     */
struct Gamepad {
	Ptr<RawInputDevice> rawInputDevice;
	f32 deadZone = 0.05f; // A value between 0.0 and 1.0, should probably not be very high.
	f32 axisCurve = 1.0f; // 1.0 is linear, 2.0 is squared, 0.5 is sqrt
	// Basic button interface
	ButtonState button[IO_GAMEPAD_MAX_BUTTONS];
	// An axis has moved beyond 50% in its direction.
	// Even indices are all positive directions while odd indices are negative.
	// Positive push is AXIS_INDEX*2
	// Negative push is AXIS_INDEX*2 + 1
	ButtonState axisPush[IO_GAMEPAD_MAX_AXES * 2];
	// For an 8-directional hat, these are the in-between directions
	// Since axisPush already contains the 4 main directions
	ButtonState hat[4];
	// Axis values are between -1.0 and 1.0
	union {
		struct {
			vec2 LS;
			f32 LT;
			vec2 RS;
			f32 RT;
			vec2 H0;
			// If you plan on expanding the axes, be sure to add them here too!
		} vec{{0.0f, 0.0f},
			  0.0f, {0.0f, 0.0f},
			  0.0f, {0.0f, 0.0f}};
		f32 array[IO_GAMEPAD_MAX_AXES];
	} axis;

	Gamepad() = default;
	void Update(f32 timestep, i32 index);
	ButtonState* GetButtonState(u8 keyCode);
	bool Repeated(u8 keyCode);
	bool Pressed(u8 keyCode);
	bool Down(u8 keyCode);
	bool Released(u8 keyCode);
};

} // namespace io

} // namespace AzCore

#endif // AZCORE_GAMEPAD_HPP
