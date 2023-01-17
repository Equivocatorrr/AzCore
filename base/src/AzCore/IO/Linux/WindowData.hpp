/*
	File: WindowData.hpp
	Author: Philip Haynes
	For anyone who needs to know about WindowData
*/

#ifndef AZCORE_WINDOW_DATA_HPP
#define AZCORE_WINDOW_DATA_HPP

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

#include <wayland-client.h>
#include "Wayland/xdg-shell.h"

namespace AzCore {

namespace io {

struct xkb_keyboard {
	bool useWayland;
	xcb_connection_t *connection;
	u8 first_xkb_event;
	struct xkb_context *context{nullptr};
	struct xkb_keymap *keymap{nullptr};
	i32 deviceId;
	struct xkb_state *state{nullptr};
	struct xkb_state *stateNone{nullptr};
};

struct WindowData {
	bool useWayland;
	union {
		struct { // X11
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
		#ifndef AZCORE_IO_NO_XLIB
			Display *display;
		#endif
		} x11;
		struct { // wayland
			wl_display *display;
			// These come from global registry
			wl_compositor *compositor;
			xdg_wm_base *wmBase;
			wl_seat *seat;
			wl_shm *shm;
			// These we created, in order
			wl_surface *surface;
			xdg_surface *xdgSurface;
			xdg_toplevel *xdgToplevel;
			wl_buffer *buffer;
			wl_region *region;
		} wayland;
	};
	xkb_keyboard xkb;
	i32 frameCount;
	Thread dpiThread;
	WindowData(bool _useWayland) : useWayland(_useWayland) {
		xkb.useWayland = useWayland;
	}
}; // struct WindowData

} // namespace io

} // namespace AzCore

#endif // AZCORE_WINDOW_DATA_HPP
