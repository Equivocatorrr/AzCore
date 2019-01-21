/*
    File: io.cpp
    Author: Philip Haynes
*/

#include "io.hpp"

namespace io {

    #include "keycode/keytable_common.cpp"

    String error = "No Error";
    vec2 screenSize;

    logStream::logStream() : fstream("log.txt") , log(true) {
        if (!fstream.is_open()) {
            std::cout << "Failed to open log.txt for writing" << std::endl;
            log = false;
        }
    }

    logStream::logStream(String logFilename) : fstream(logFilename) , log(true) {
        if (!fstream.is_open()) {
            std::cout << "Failed to open " << logFilename << " for writing" << std::endl;
            log = false;
        }
    }

    logStream& logStream::operator<<(stream_function func) {
        func(std::cout);
        if (log)
            func(fstream);
        return *this;
    }

    void logStream::MutexLock() {
        mutex.lock();
    }

    void logStream::MutexUnlock() {
        mutex.unlock();
    }

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
        if (keyCode < 119 || keyCode > 157) {
            Any.Press();
            codeAny = keyCode;
        }
        if (keyCode < 119 || (keyCode >= 224 && keyCode < 232)) {
            AnyKey.Press();
            codeAnyKey = keyCode;
        }
        if (keyCode >= 232) {
            AnyMB.Press();
            codeAnyMB = keyCode;
        }
        inputs[keyCode].Press();
    }

    void Input::Release(u8 keyCode) {
        if (keyCode < 119 || keyCode > 157) {
            Any.Release();
            codeAny = keyCode;
        }
        if (keyCode < 119 || (keyCode >= 224 && keyCode < 232)) {
            AnyKey.Release();
            codeAnyKey = keyCode;
        }
        if (keyCode >= 232) {
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
