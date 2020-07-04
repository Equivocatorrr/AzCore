/*
    File: Window.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_WINDOW_HPP
#define AZCORE_WINDOW_HPP

#ifdef AZCORE_IO_FOR_VULKAN
    #ifdef __unix
        #define VK_USE_PLATFORM_XCB_KHR
    #elif defined(_WIN32)
        #define VK_USE_PLATFORM_WIN32_KHR
    #endif
    #include <vulkan/vulkan.h>
#endif

#include "../basictypes.hpp"
#include "../Memory/String.hpp"

namespace AzCore {

namespace vk {
    class Instance;
}

namespace io {

#ifdef _WIN32
extern u32 windowClassNum; // Prevent identical windowClasses
#endif

struct Input;

/*  class: Window
    Author: Philip Haynes
    Generic window class that can receive events and display an image.  */
class Window {
    friend vk::Instance;
    // Opaque type for clean cross-platform implementation
    struct WindowData *data = nullptr;
#ifdef AZCORE_IO_FOR_VULKAN
    bool CreateVkSurface(vk::Instance *instance, VkSurfaceKHR *surface);
#endif
public:
    bool open = false;
    bool resized = false;
    bool focused = true;
    bool fullscreen = false;
    bool quit = false;
    bool cursorHidden = false;
    u16 width = 1280;
    u16 height = 720;
    u16 windowedWidth = 1280;
    u16 windowedHeight = 720;
    i16 x = 0;
    i16 y = 0;
    i16 windowedX = 0;
    i16 windowedY = 0;
    String name = "AzCore";
    Input *input = nullptr;
    Window();
    ~Window();
    bool Open();
    bool Show();
    bool Fullscreen(bool fullscreen);
    bool Resize(u32 w, u32 h);
    bool Update();
    bool Close();
    void HideCursor(bool hide = true);
    String InputName(u8 keyCode) const;
    u8 KeyCodeFromChar(char character) const;
};

} // namespace io

} // namespace AzCore

#endif // AZCORE_WINDOW_HPP
