/*
	File: Linux/Window.cpp
	Author: Philip Haynes
*/

// #include "../Window.hpp"
// #include "../../io.hpp"
// #include "../../keycodes.hpp"
// #include "WindowData.hpp"
// ^these are included by Wayland.cpp
#include "Wayland.cpp"
#include "XCB.cpp"

namespace AzCore::io {

String xkbGetInputName(xkb_keyboard *xkb, u8 hid) {
	if (hid == 255) {
		return "Null";
	}
	char utf8[16];
	// First make sure we're not anything that doesn't move
	if (hid < 0x04 || (hid >= 0x28 && hid <= 0x2c) || (hid >= 0x39 && hid <= 0x58) || hid >= 0x64) {
		return KeyCodeName(hid);
	}
	// Check if we even have a mapping at all
	u8 keyCode = KeyCodeToEvdev(hid);
	if (keyCode == 255) {
		return "None";
	}
	// If layout-dependent, update the label based on the layout
	if (hid >= 0x64 || hid <= 0x58) { // Non-keypad
		xkb_state_key_get_utf8(xkb->stateNone, (xkb_keycode_t)keyCode, utf8, 16);
	} else { // Keypad
		xkb_state_key_get_utf8(xkb->state, (xkb_keycode_t)keyCode, utf8, 16);
		if (utf8[0] != '\0' && utf8[1] == '\0') { // Single-character from the keypad
			// This is if numlock is on.
			return KeyCodeName(hid);
		}
	}
	if (utf8[0] != '\0') {
		return utf8;
	}
	// If we don't have a proper utf-8 label find the keySym name
	xkb_keysym_t keySym = xkb_state_key_get_one_sym(xkb->stateNone, (xkb_keycode_t)keyCode);
	xkb_keysym_get_name(keySym, utf8, 16);
	if (utf8[0] != '\0') {
		return utf8;
	} else {
		// If all else fails we don't know what to do.
		return "Error";
	}
}

Window::Window() {
	bool useWayland = false;

	char *waylandDisplay = getenv("WAYLAND_DISPLAY");
	if (waylandDisplay) useWayland = true;

	char *azEnableWayland = getenv("AZCORE_ENABLE_WAYLAND");
	if (azEnableWayland) {
		if (equals(azEnableWayland, "1")) {
			useWayland = true;
		} else if (equals(azEnableWayland, "0")) {
			useWayland = false;
		}
	}

	data = new WindowData(useWayland);
	cout.PrintLn("Wayland is ", data->useWayland ? "enabled" : "disabled");
}

Window::~Window() {
	if (open) {
		Close();
	}
	delete data;
}

bool Window::Open() {
	if (data->useWayland) {
		if (!windowOpenWayland(this)) return false;
	} else {
		if (!windowOpenX11(this)) return false;
	}
	return true;
}

bool Window::Show() {
	if (!open) {
		error = "Window hasn't been created yet";
		return false;
	}
	if (data->useWayland) {
		// We show in Open because otherwise we can't get DPI
	} else {
		windowShowXCB(this);
	}
	return true;
}

bool Window::Close() {
	if (!open) {
		error = "Window hasn't been created yet";
		return false;
	}
	if (data->useWayland) {
		windowCloseWayland(this);
	} else {
		windowCloseXCB(this);
	}
	open = false;
	return true;
}

bool Window::Fullscreen(bool fs) {
	if (!open) {
		error = "Window hasn't been created yet";
		return false;
	}
	if (fullscreen == fs)
		return true;

	fullscreen = fs;

	if (data->useWayland) {
		windowFullscreenWayland(this);
	} else {
		windowFullscreenX11(this);
	}

	return true;
}

bool Window::Resize(u32 w, u32 h) {
	if (!open) {
		error = "Window hasn't been created yet";
		return false;
	}
	if (fullscreen) {
		error = "Fullscreen windows can't be resized";
		return false;
	}
	if (w == width && h == height) return true;
	width = w;
	height = h;
	windowedWidth = w;
	windowedHeight = h;
	if (data->useWayland) {
		windowResizeWayland(this);
	} else {
		windowResizeX11(this);
	}
	resized = true;
	return true;
}

bool Window::Update() {
	bool changeFullscreen = false;
	resized = false;
	if (data->useWayland) {
		if (!windowUpdateWayland(this, changeFullscreen)) return false;
	} else {
		if (!windowUpdateXCB(this, changeFullscreen)) return false;
	}

	if (changeFullscreen) {
		Fullscreen(!fullscreen);
	}

	return true;
}

void Window::HideCursor(bool hide) {
	cursorHidden = hide;
	if (data->useWayland) {
		SetCursorWayland(this);
	} else {
		SetCursorXCB(this);
	}
}

void Window::MoveCursor(i32 x, i32 y) {
	if (data->useWayland) {
		AzAssert(false, "Unimplemented");
	} else {
		MoveCursorXCB(this, x, y);
	}
}

String Window::InputName(u8 keyCode) const {
	if (!open) {
		return "Error";
	}
	return xkbGetInputName(&data->xkb, keyCode);
}

i32 Window::GetDPI() {
	return dpi > 0 ? dpi : 96;
}

} // namespace AzCore::io

