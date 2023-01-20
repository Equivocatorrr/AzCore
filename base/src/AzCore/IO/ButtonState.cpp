/*
	File: ButtonState.cpp
	Author: Philip Haynes
*/

#include "ButtonState.hpp"

namespace AzCore {

namespace io {

ButtonState::ButtonState() : state(0), canRepeat(false), repeatTimer(0.4f) {}

void ButtonState::Set(bool pressed, bool down, bool released) {
	state = 0;
	if (pressed)
		state |= BUTTON_PRESSED_BIT;
	if (down)
		state |= BUTTON_DOWN_BIT;
	if (released)
		state |= BUTTON_RELEASED_BIT;
}

void ButtonState::Tick(f32 timestep, f32 charRepeatsPerSecond, f32 charRepeatDelay) {
	state &= BUTTON_DOWN_BIT;
	if (state && canRepeat) {
		if (repeatTimer > 0.0f) {
			repeatTimer -= timestep;
			if (repeatTimer <= 0.0f) {
				state |= BUTTON_PRESSED_BIT;
				repeatTimer += 1.0f / charRepeatsPerSecond;
			}
		}
	} else {
		repeatTimer = charRepeatDelay;
	}
}

void ButtonState::Press() {
	state |= BUTTON_PRESSED_BIT | BUTTON_DOWN_BIT;
}

void ButtonState::Release() {
	state |= BUTTON_RELEASED_BIT;
	state &= ~BUTTON_DOWN_BIT;
}

bool ButtonState::Pressed() const {
	return (state & BUTTON_PRESSED_BIT) != 0;
}

bool ButtonState::Down() const {
	return (state & BUTTON_DOWN_BIT) != 0;
}

bool ButtonState::Released() const {
	return (state & BUTTON_RELEASED_BIT) != 0;
}

} // namespace io

} // namespace AzCore
