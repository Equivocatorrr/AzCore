/*
	File: ButtonState.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_BUTTONSTATE_HPP
#define AZCORE_BUTTONSTATE_HPP

#include "../basictypes.hpp"

namespace AzCore {

namespace io {

enum ButtonStateBits {
	BUTTON_PRESSED_BIT = 0x01,
	BUTTON_DOWN_BIT = 0x02,
	BUTTON_RELEASED_BIT = 0x04
};
/*  struct: ButtonState
		Author: Philip Haynes   */
struct ButtonState {
	i16 state;
	bool canRepeat;
	f32 repeatTimer;
	ButtonState();
	void Set(bool pressed, bool down, bool released);
	void Tick(f32 timestep); // Resets pressed and released, leaving down
	void Press();            // Sets pressed and down, leaving released
	void Release();          // Sets released and down, leaving pressed
	bool Pressed() const;
	bool Down() const;
	bool Released() const;
};

} // namespace io

} // namespace AzCore

#endif // AZCORE_BUTTONSTATE_HPP
