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

#include "common.hpp"
#include "log_stream.hpp"

namespace vk {
    class Instance;
}

namespace io {

    extern String error;
    extern vec2 screenSize;

    extern logStream cout;

    enum RawInputDeviceType {
        UNSUPPORTED=0,
        KEYBOARD=1,
        MOUSE=2,
        GAMEPAD=3,
        JOYSTICK=4
    };

    extern const char *RawInputDeviceTypeString[5];

    /*  struct: RawInputDevice
        Author: Philip Haynes
        A generic interface to raw input devices.   */
    struct RawInputDevice {
        struct RawInputDeviceData *data = nullptr;
        RawInputDeviceType type;

        RawInputDevice();
        ~RawInputDevice();
        RawInputDevice(RawInputDevice&& other);
        bool Init();
    };

    /*  struct: RawInput
        Author: Philip Haynes
        Manages all RawInputDevices */
    struct RawInput {
        struct RawInputData *data = nullptr;
        List<RawInputDevice> devices;

        ~RawInput();
        bool Init();
    };

    #define IO_BUTTON_PRESSED_BIT 0x01
    #define IO_BUTTON_DOWN_BIT 0x02
    #define IO_BUTTON_RELEASED_BIT 0x04

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
