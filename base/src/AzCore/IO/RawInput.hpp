/*
    File: RawInput.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_RAWINPUT_HPP
#define AZCORE_RAWINPUT_HPP

#include "ButtonState.hpp"
#include "../Memory/Array.hpp"

namespace AzCore {

namespace io {

enum RawInputDeviceType {
    UNSUPPORTED = 0,
    KEYBOARD = 1,
    MOUSE = 2,
    GAMEPAD = 3,
    JOYSTICK = 4
};

extern const char *RawInputDeviceTypeString[5];

enum RawInputFeatureBits {
    RAW_INPUT_ENABLE_KEYBOARD_BIT = 0x01,
    RAW_INPUT_ENABLE_MOUSE_BIT = 0x02,
    RAW_INPUT_ENABLE_GAMEPAD_BIT = 0x04,
    RAW_INPUT_ENABLE_JOYSTICK_BIT = 0x08,
    // Some aggregate values
    RAW_INPUT_ENABLE_KEYBOARD_MOUSE = 0x03,
    RAW_INPUT_ENABLE_GAMEPAD_JOYSTICK = 0x0c,
    RAW_INPUT_ENABLE_ALL = 0x0f
};

struct RawInput;

/*  struct: RawInputDevice
        Author: Philip Haynes
        A generic interface to raw input devices.   */
struct RawInputDevice {
    struct RawInputDeviceData *data = nullptr;
    RawInput *rawInput;
    RawInputDeviceType type;

    RawInputDevice() = default;
    ~RawInputDevice();
    RawInputDevice(RawInputDevice &&other);
    RawInputDevice &operator=(RawInputDevice &&other);
};

struct Gamepad;

/*  struct: RawInput
        Author: Philip Haynes
        Manages all RawInputDevices */
struct RawInput {
    struct RawInputData *data = nullptr;
    // Provide this pointer to automatically disable input when it's not the focused window.
    // Leave it null to always capture input.
    class Window *window = nullptr;
    Array<RawInputDevice> devices;
    Array<Gamepad> gamepads;
    ButtonState AnyGP;
    u8 AnyGPCode;   // Which button/axisPush was pressed
    i32 AnyGPIndex; // Index into gamepads of the device that pressed a button/axisPush

    ~RawInput();
    bool Init(RawInputFeatureBits enableMask);
    void Update(f32 timestep);
};

} // namespace io

} // namespace AzCore

#endif // AZCORE_RAWINPUT_HPP
