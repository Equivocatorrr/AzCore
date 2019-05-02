/*
    File: io.cpp
    Author: Philip Haynes
*/

#include "io.hpp"

namespace io {

    String error = "No Error";
    vec2 screenSize;
    logStream cout("io.log");

    const char *RawInputDeviceTypeString[5] = {
        "Unsupported",
        "Keyboard",
        "Mouse",
        "Gamepad",
        "Joystick"
    };

    RawInputDevice::RawInputDevice(RawInputDevice&& other) {
        data = other.data;
        other.data = nullptr;
        rawInput = other.rawInput;
        type = other.type;
    }

    bool Gamepad::Pressed(u8 keyCode) const {
        if (keyCode >= KC_GP_AXIS_LS_RIGHT && keyCode <= KC_GP_AXIS_RT_IN) {
            return axisPush[ keyCode-KC_GP_AXIS_LS_RIGHT ].Pressed();
        } else if (keyCode >= KC_GP_BTN_A && keyCode <= KC_GP_BTN_THUMBR) {
            return button[ keyCode-KC_GP_BTN_A ].Pressed();
        } else {
            return false;
        }
    }

    bool Gamepad::Down(u8 keyCode) const {
        if (keyCode >= KC_GP_AXIS_LS_RIGHT && keyCode <= KC_GP_AXIS_RT_IN) {
            return axisPush[ keyCode-KC_GP_AXIS_LS_RIGHT ].Down();
        } else if (keyCode >= KC_GP_BTN_A && keyCode <= KC_GP_BTN_THUMBR) {
            return button[ keyCode-KC_GP_BTN_A ].Down();
        } else {
            return false;
        }
    }

    bool Gamepad::Released(u8 keyCode) const {
        if (keyCode >= KC_GP_AXIS_LS_RIGHT && keyCode <= KC_GP_AXIS_RT_IN) {
            return axisPush[ keyCode-KC_GP_AXIS_LS_RIGHT ].Released();
        } else if (keyCode >= KC_GP_BTN_A && keyCode <= KC_GP_BTN_THUMBR) {
            return button[ keyCode-KC_GP_BTN_A ].Released();
        } else {
            return false;
        }
    }

    ButtonState::ButtonState() : state(0) , canRepeat(false) , repeatTimer(0.4) {}

    void ButtonState::Set(bool pressed, bool down, bool released) {
        state = 0;
        if (pressed)
            state |= BUTTON_PRESSED_BIT;
        if (down)
            state |= BUTTON_DOWN_BIT;
        if (released)
            state |= BUTTON_RELEASED_BIT;
    }

    void ButtonState::Tick(f32 timestep) {
        state &= BUTTON_DOWN_BIT;
        if (state && canRepeat) {
            if (repeatTimer > 0.0) {
                repeatTimer -= timestep;
                if (repeatTimer <= 0.0) {
                    state |= BUTTON_PRESSED_BIT;
                    repeatTimer += 1.0 / 15.0;
                }
            }
        } else {
            repeatTimer = 0.4;
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
        return inputsChar[ (u8)character ].Pressed();
    }

    bool Input::DownChar(char character) const {
        if (character & 0x80) {
            return false;
        }
        return inputsChar[ (u8)character ].Down();
    }

    bool Input::ReleasedChar(char character) const {
        if (character & 0x80) {
            return false;
        }
        return inputsChar[ (u8)character ].Released();
    }

}
