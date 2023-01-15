/*
	File: WindowData.hpp
	Author: Philip Haynes
	For anyone who needs to know about WindowData
*/

#include "../../basictypes.hpp"

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
