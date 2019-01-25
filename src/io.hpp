/*
    File: io.hpp
    Author: Philip Haynes
    Description: Handles keyboard, mouse, gamepads/joysticks, windows, logging, ect.
    TODO: Add raw input support (including gamepads/joysticks)
*/
#ifndef IO_HPP
#define IO_HPP

#include "common.hpp"

#include <iostream>
#include <fstream>

namespace io {

    extern String error;
    extern vec2 screenSize;

    /*  class: out
    Author: Philip Haynes
    Use this class to write any and all debugging/status text.
    Use it the same way you would std::cout.
    Ex: io::cout << "Say it ain't so!!" << std::endl;
    Entries in this class will be printed to terminal and a log file.   */
    class logStream {
        std::ofstream fstream;
        bool log; // Whether we're using a log file
        Mutex mutex;
    public:
        logStream();
        logStream(String logFilename);
        template<typename T> logStream& operator<<(const T& something) {
            std::cout << something;
            if (log)
                fstream << something;
            return *this;
        }
        typedef std::ostream& (*stream_function)(std::ostream&);
        logStream& operator<<(stream_function func);
        void MutexLock();
        void MutexUnlock();
    };

    extern logStream cout;

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
        ButtonState inputs[256];
        vec2i cursor;
        vec2 scroll;
        Input();
        void Press(u8 keyCode);
        void Release(u8 keyCode);
        void ReleaseAll();
        void Tick(f32 timestep);
        bool Pressed(u8 keyCode) const;
        bool Down(u8 keyCode) const;
        bool Released(u8 keyCode) const;
    };

    /*  class: Window
        Author: Philip Haynes
        Generic window class that can receive events and display an image.  */
    class Window {
        // Opaque type for clean cross-platform implementation
        struct WindowData *data = nullptr;
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
        bool Update();
        bool Close();
        String InputName(u8 keyCode) const;
    };
}


#endif
