/*
	File: WindowData.hpp
	Author: Philip Haynes
	For anyone who needs to know about WindowData
*/

// To use GLX, you need Xlib, but for Vulkan you can just use xcb
#if defined(AZCORE_IO_FOR_VULKAN) && !defined(AZCORE_IO_NO_XLIB)
	#define AZCORE_IO_NO_XLIB
#endif

#include "../../basictypes.hpp"
#include "../../Thread.hpp"

#include <xcb/xcb.h>
#ifndef AZCORE_IO_NO_XLIB
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#endif
#include <xcb/xproto.h>

#include <linux/input.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>

#define explicit extern_explicit // Preventing C++ keyword bug
#include <xcb/xkb.h>
#undef explicit

namespace AzCore {

namespace io {

struct xkb_keyboard {
	xcb_connection_t *connection;
	u8 first_xkb_event;
	struct xkb_context *context{nullptr};
	struct xkb_keymap *keymap{nullptr};
	i32 deviceId;
	struct xkb_state *state{nullptr};
	struct xkb_state *stateNone{nullptr};
};

struct WindowData {
	xcb_connection_t *connection;
	xcb_colormap_t colormap;
	i32 visualID;
	xcb_window_t window;
	xcb_screen_t *screen;
	xcb_generic_event_t *event;
	xcb_atom_t atoms[4];
	xcb_cursor_t cursorHidden;
	xcb_cursor_t cursorVisible;
	i32 windowDepth;
	xkb_keyboard xkb;
	i32 frameCount;
	Thread dpiThread;
#ifndef AZCORE_IO_NO_XLIB
	Display *display;
#endif
};

} // namespace io

} // namespace AzCore
