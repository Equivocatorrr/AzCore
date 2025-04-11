/*
	File: WindowData.hpp
	Author: Philip Haynes
	For anyone who needs to know about WindowData
*/

#include "../../Memory/String.hpp"
#include "../../Utility/Windows.h"

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
