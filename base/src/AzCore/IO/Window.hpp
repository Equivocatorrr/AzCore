/*
	File: Window.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_WINDOW_HPP
#define AZCORE_WINDOW_HPP

#include "../basictypes.hpp"
#include "../Memory/String.hpp"

namespace AzCore {

namespace vk {
	struct Window;
}

namespace io {

#ifdef _WIN32
extern u32 windowClassNum; // Prevent identical windowClasses
#endif

struct Input;

/*  class: Window
	Author: Philip Haynes
	Generic window class that can receive events and display an image.  */
struct Window {
	// Opaque type for clean cross-platform implementation
	struct WindowData *data = nullptr;
	bool open = false;
	bool resized = false;
	bool focused = true;
	bool fullscreen = false;
	bool quit = false;
	bool cursorHidden = false;
	u16 dpi = 96;
	// current monitor refresh rate in mHz
	u32 refreshRate = 60000;
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

	i32 GetDPI();
};

} // namespace io

} // namespace AzCore

#endif // AZCORE_WINDOW_HPP
