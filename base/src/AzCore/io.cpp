/*
    File: io.cpp
    Author: Philip Haynes
*/

#include "io.hpp"

namespace io {

    String error = "No Error";
    vec2 screenSize;
    logStream cout("io.log");

    ButtonState::ButtonState() : state(0) , canRepeat(false) , repeatTimer(0.4) {}

    void ButtonState::Set(bool pressed, bool down, bool released) {
        state = 0;
        if (pressed)
            state |= IO_BUTTON_PRESSED_BIT;
        if (down)
            state |= IO_BUTTON_DOWN_BIT;
        if (released)
            state |= IO_BUTTON_RELEASED_BIT;
    }

    void ButtonState::Tick(f32 timestep) {
        state &= IO_BUTTON_DOWN_BIT;
        if (state && canRepeat) {
            if (repeatTimer > 0.0) {
                repeatTimer -= timestep;
                if (repeatTimer <= 0.0) {
                    state |= IO_BUTTON_PRESSED_BIT;
                    repeatTimer += 1.0 / 15.0;
                }
            }
        } else {
            repeatTimer = 0.4;
        }
    }

    void ButtonState::Press() {
        state |= IO_BUTTON_PRESSED_BIT | IO_BUTTON_DOWN_BIT;
    }

    void ButtonState::Release() {
        state |= IO_BUTTON_RELEASED_BIT;
        state &= ~IO_BUTTON_DOWN_BIT;
    }

    bool ButtonState::Pressed() const {
        return (state & IO_BUTTON_PRESSED_BIT) != 0;
    }

    bool ButtonState::Down() const {
        return (state & IO_BUTTON_DOWN_BIT) != 0;
    }

    bool ButtonState::Released() const {
        return (state & IO_BUTTON_RELEASED_BIT) != 0;
    }

    Input::Input() {
        cursor = vec2i(0,0);
        scroll = vec2(0.0);
    }

    void Input::Press(u8 keyCode) {
        if (keyCode < 0xb8 || keyCode > 0xde) {
            Any.Press();
            codeAny = keyCode;
        }
        if (keyCode < 0xa6 || (keyCode >= 0xb0 && keyCode < 0xb8) || keyCode >= 0xe0) {
            AnyKey.Press();
            codeAnyKey = keyCode;
        }
        if (keyCode >= 0xa6 && keyCode <= 0xae) {
            AnyMB.Press();
            codeAnyMB = keyCode;
        }
        inputs[keyCode].Press();
    }

    void Input::Release(u8 keyCode) {
        if (keyCode < 0xb8 || keyCode > 0xde) {
            Any.Release();
            codeAny = keyCode;
        }
        if (keyCode < 0xa6 || (keyCode >= 0xb0 && keyCode < 0xb8) || keyCode >= 0xe0) {
            AnyKey.Release();
            codeAnyKey = keyCode;
        }
        if (keyCode >= 0xa6 && keyCode <= 0xae) {
            AnyMB.Release();
            codeAnyMB = keyCode;
        }
        inputs[keyCode].Release();
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
    }

    void Input::Tick(f32 timestep) {
        Any.Tick(timestep);
        AnyKey.Tick(timestep);
        AnyMB.Tick(timestep);
        scroll = vec2(0.0);
        for (u16 i = 0; i < 256; i++) {
            inputs[i].Tick(timestep);
        }
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

}
