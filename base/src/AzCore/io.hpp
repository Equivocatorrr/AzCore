/*
    File: io.hpp
    Author: Philip Haynes
    Description: Handles keyboard, mouse, gamepads/joysticks, windows, ect.
    TODO:
        - Add raw input support (including gamepads/joysticks)
*/
#ifndef IO_HPP
#define IO_HPP

#ifdef IO_FOR_VULKAN
    #ifdef __unix
        #define VK_USE_PLATFORM_XCB_KHR
    #elif defined(_WIN32)
        #define VK_USE_PLATFORM_WIN32_KHR
    #endif
    #include <vulkan/vulkan.h>
#endif

// For most gamepads, these values should be sufficient.
#define IO_GAMEPAD_MAX_BUTTONS 15
#define IO_GAMEPAD_MAX_AXES 8

#include "common.hpp"
#include "log_stream.hpp"

namespace vk {
    class Instance;
}

namespace io {

    extern String error;
    extern vec2 screenSize;

    extern logStream cout;

    enum ButtonStateBits {
        BUTTON_PRESSED_BIT  = 0x01,
        BUTTON_DOWN_BIT     = 0x02,
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
        void Press(); // Sets pressed and down, leaving released
        void Release(); // Sets released and down, leaving pressed
        bool Pressed() const;
        bool Down() const;
        bool Released() const;
    };

    enum RawInputDeviceType {
        UNSUPPORTED = 0,
        KEYBOARD    = 1,
        MOUSE       = 2,
        GAMEPAD     = 3,
        JOYSTICK    = 4
    };

    extern const char *RawInputDeviceTypeString[5];

    enum RawInputFeatureBits {
        RAW_INPUT_ENABLE_KEYBOARD_BIT        = 0x01,
        RAW_INPUT_ENABLE_MOUSE_BIT           = 0x02,
        RAW_INPUT_ENABLE_GAMEPAD_BIT         = 0x04,
        RAW_INPUT_ENABLE_JOYSTICK_BIT        = 0x08,
        // Some aggregate values
        RAW_INPUT_ENABLE_KEYBOARD_MOUSE      = 0x03,
        RAW_INPUT_ENABLE_GAMEPAD_JOYSTICK    = 0x0c,
        RAW_INPUT_ENABLE_ALL                 = 0x0f
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
        RawInputDevice(RawInputDevice&& other);
        RawInputDevice& operator=(RawInputDevice&& other);
    };

    enum GamepadAxisArrayIndices {
        GP_AXIS_LS_X    = 0x00,
        GP_AXIS_LS_Y    = 0x01,
        GP_AXIS_RS_X    = 0x03,
        GP_AXIS_RS_Y    = 0x04,
        GP_AXIS_LT      = 0x02,
        GP_AXIS_RT      = 0x05,
        GP_AXIS_H0_X    = 0x06,
        GP_AXIS_H0_Y    = 0x07
    };
    /*  struct: Gamepad
        Author: Philip Haynes
        Utilities to use a RawInputDevice as a gamepad.     */
    struct Gamepad {
        Ptr<RawInputDevice> rawInputDevice;
        f32 deadZone = 0.05; // A value between 0.0 and 1.0, should probably not be very high.
        // Basic button interface
        ButtonState button[IO_GAMEPAD_MAX_BUTTONS];
        // An axis has moved beyond 50% in its direction.
        // Even indices are all positive directions while odd indices are negative.
        // Positive push is AXIS_INDEX*2
        // Negative push is AXIS_INDEX*2 + 1
        ButtonState axisPush[IO_GAMEPAD_MAX_AXES*2];
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
            } vec{{0.0, 0.0},0.0,{0.0, 0.0,},0.0,{0.0,0.0}};
            f32 array[IO_GAMEPAD_MAX_AXES];
        } axis;

        void Update(f32 timestep, i32 index);
        bool Pressed(u8 keyCode) const;
        bool Down(u8 keyCode) const;
        bool Released(u8 keyCode) const;
    };

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
        u8 AnyGPCode; // Which button/axisPush was pressed
        i32 AnyGPIndex; // Index into gamepads of the device that pressed a button/axisPush

        ~RawInput();
        bool Init(RawInputFeatureBits enableMask);
        void Update(f32 timestep);
    };

    /*  struct: Input
        Author: Philip Haynes
        Holds the state for the entire Keyboard and Mouse. Gamepads sold separately.   */
    struct Input {
        ButtonState Any, AnyKey, AnyMB;
        u8 codeAny, codeAnyKey, codeAnyMB;
        char charAny; // This can be associated with AnyKey only
        ButtonState inputs[256];
        ButtonState inputsChar[128];
        vec2i cursor;
        vec2 scroll;
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
    };

    /*  class: Window
        Author: Philip Haynes
        Generic window class that can receive events and display an image.  */
    class Window {
        friend vk::Instance;
        // Opaque type for clean cross-platform implementation
        struct WindowData *data = nullptr;
#ifdef IO_FOR_VULKAN
        bool CreateVkSurface(vk::Instance *instance, VkSurfaceKHR *surface);
#endif
    public:
        bool    open            = false;
        bool    resized         = false;
        bool    focused         = true;
        bool    fullscreen      = false;
        u16     width           = 1280;
        u16     height          =  720;
        u16     windowedWidth   = 1280;
        u16     windowedHeight  =  720;
        i16     x               = 0;
        i16     y               = 0;
        i16     windowedX       = 0;
        i16     windowedY       = 0;
        String  name            = "AzCore";
        Input  *input           = nullptr;
        Window();
        ~Window();
        bool Open();
        bool Show();
        bool Fullscreen(bool fullscreen);
        bool Resize(u32 w, u32 h);
        bool Update();
        bool Close();
        String InputName(u8 keyCode) const;
        u8 KeyCodeFromChar(char character) const;
    };
}


#endif
