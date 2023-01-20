/*
	File: Input.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_INPUT_HPP
#define AZCORE_INPUT_HPP

#include "ButtonState.hpp"
#include "../Memory/String.hpp"
#include "../math.hpp"

namespace AzCore {

namespace io {

/*  struct: Input
	Author: Philip Haynes
	Holds the state for the entire Keyboard and Mouse. Gamepads sold separately.   */
struct Input {
	ButtonState Any, AnyKey, AnyMB;
	u8 codeAny, codeAnyKey, codeAnyMB;
	char charAny; // This can be associated with AnyKey only
	String typingString;
	ButtonState inputs[256];
	ButtonState inputsChar[128];
	vec2i cursor, cursorPrevious;
	vec2 scroll;
	f32 charRepeatsPerSecond = 15.0f;
	f32 charRepeatDelay = 0.4f;
	Input();
	void Press(u8 keyCode);
	void Release(u8 keyCode);
	void PressChar(char character);
	void ReleaseChar(char character);
	void ReleaseAll();
	void Tick(f32 timestep);
	// These are keyboard-layout agnostic.
	bool Pressed(u8 keyCode) const;
	bool Down(u8 keyCode) const;
	bool Released(u8 keyCode) const;
	// These are keyboard-layout dependent.
	bool PressedChar(char character) const;
	bool DownChar(char character) const;
	bool ReleasedChar(char character) const;

	inline ButtonState& GetButtonState(u8 keyCode) {
		return inputs[keyCode];
	}
	inline ButtonState& GetButtonStateChar(char character) {
		return inputsChar[(u8)character];
	}
};

} // namespace io

} // namespace AzCore

#endif // AZCORE_INPUT_HPP
