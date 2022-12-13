/*
	File: Input.cpp
	Author: Philip Haynes
*/

#include "Input.hpp"
#include "../keycodes.hpp"

namespace AzCore {

namespace io {

Input::Input() {
	cursor = vec2i(0, 0);
	cursorPrevious = vec2i(0, 0);
	scroll = vec2(0.0);
}

void Input::Press(u8 keyCode) {
	if (!KeyCodeIsGamepad(keyCode)) {
		Any.Press();
		codeAny = keyCode;
	}
	if (KeyCodeIsKeyboard(keyCode)) {
		AnyKey.Press();
		codeAnyKey = keyCode;
	}
	if (KeyCodeIsMouse(keyCode)) {
		AnyMB.Press();
		codeAnyMB = keyCode;
	}
	if (!inputs[keyCode].Down()) // De-duplicate
		inputs[keyCode].Press();
}

void Input::Release(u8 keyCode) {
	if (!KeyCodeIsGamepad(keyCode)) {
		Any.Release();
		codeAny = keyCode;
	}
	if (KeyCodeIsKeyboard(keyCode)) {
		AnyKey.Release();
		codeAnyKey = keyCode;
	}
	if (KeyCodeIsMouse(keyCode)) {
		AnyMB.Release();
		codeAnyMB = keyCode;
	}
	inputs[keyCode].Release();
}

void Input::PressChar(char character) {
	AnyKey.Press();
	charAny = character;
	inputsChar[(u8)character].Press();
}

void Input::ReleaseChar(char character) {
	AnyKey.Release();
	charAny = character;
	inputsChar[(u8)character].Release();
}

void Input::ReleaseAll() {
	if (Any.Down())
		Any.Release();
	if (AnyKey.Down())
		AnyKey.Release();
	if (AnyMB.Down())
		AnyMB.Release();
	for (u16 i = 0; i < 256; i++) {
		if (inputs[i].Down()) {
			inputs[i].Release();
		}
	}
	for (u16 i = 0; i < 128; i++) {
		if (inputsChar[i].Down()) {
			inputsChar[i].Release();
		}
	}
}

void Input::Tick(f32 timestep) {
	Any.Tick(timestep);
	AnyKey.Tick(timestep);
	AnyMB.Tick(timestep);
	scroll = vec2(0.0);
	for (u16 i = 0; i < 256; i++) {
		inputs[i].Tick(timestep);
	}
	for (u16 i = 0; i < 128; i++) {
		inputsChar[i].Tick(timestep);
	}
	cursorPrevious = cursor;
}

bool Input::Pressed(u8 keyCode) const {
	return inputs[keyCode].Pressed();
}

bool Input::Down(u8 keyCode) const {
	return inputs[keyCode].Down();
}

bool Input::Released(u8 keyCode) const {
	return inputs[keyCode].Released();
}

bool Input::PressedChar(char character) const {
	if (character & 0x80) {
		return false;
	}
	return inputsChar[(u8)character].Pressed();
}

bool Input::DownChar(char character) const {
	if (character & 0x80) {
		return false;
	}
	return inputsChar[(u8)character].Down();
}

bool Input::ReleasedChar(char character) const {
	if (character & 0x80) {
		return false;
	}
	return inputsChar[(u8)character].Released();
}

} // namespace io

} // namespace AzCore
