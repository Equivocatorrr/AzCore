/*
	File: WindowData.hpp
	Author: Philip Haynes
	For anyone who needs to know about WindowData
*/

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
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
};

} // namespace io

} // namespace AzCore
