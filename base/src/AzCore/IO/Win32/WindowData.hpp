/*
	File: WindowData.hpp
	Author: Philip Haynes
	For anyone who needs to know about WindowData
*/

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace AzCore {

namespace io {

struct WindowData {
	HINSTANCE instance;
	HWND window;
	WNDCLASSEX windowClass;
	HICON windowIcon, windowIconSmall;
	String windowClassName;
	bool resizeHack;
	bool moveHack;
};

} // namespace io

} // namespace AzCore
