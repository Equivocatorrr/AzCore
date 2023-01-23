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
#include "../../memory.hpp"
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
#include <wayland-cursor.h>
#include "WaylandProtocols/xdg-shell.h"

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

struct wlOutputInfo {
	wlOutputInfo *pNext = nullptr;
	i32 x=-1, y=-1; // position in global compositor space
	i32 width=-1, height=-1; // pixel dimensions
	i32 refresh=-1; // refresh rate in mHz
	i32 phys_w=-1, phys_h=-1; // physical dimensions in mm
	i32 scale = 1;
	const char *make="make N/A", *model="model N/A";
	const char *name="name N/A", *description="description N/A";
};

struct wlCursor {
	wl_cursor_theme *theme;
	wl_cursor *cursor;
	wl_buffer *buffer;
	wl_surface *surface;
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
			i32 displayFD;
			// These come from global registry
			wl_compositor *compositor;
			BinaryMap<wl_output*, wlOutputInfo> outputs;
			Array<wl_output*> outputsWeTouch;
			xdg_wm_base *wmBase;
			wl_seat *seat;
			wl_shm *shm;
			// These we created, in order
			BinaryMap<i32, wlCursor> cursors;
			wl_surface *surface;
			xdg_surface *xdgSurface;
			xdg_toplevel *xdgToplevel;
			wl_pointer *pointer;
			wl_keyboard *keyboard;
			wl_touch *touch;
			wl_region *region;
			i32 scale;
			struct {
				wl_buffer *buffer;
				i32 fd;
				i32 size;
				u32 *shmData;
			} image;
			bool changeFullscreen;
			bool hadError;
			bool incomplete;
			bool pointerFocus;
			u32 fullscreenSerial;
			u32 pointerEnterSerial;
			i32 widthMax;
			i32 heightMax;
		} wayland;
	};
	xkb_keyboard xkb;
	i32 frameCount;
	Thread dpiThread;
	WindowData(bool _useWayland) : useWayland(_useWayland) {
		xkb.useWayland = useWayland;
		if (useWayland) {
			AzPlacementNew(wayland.outputs);
			AzPlacementNew(wayland.outputsWeTouch);
			AzPlacementNew(wayland.cursors);
			wayland = {0};
		}
	}
	~WindowData() {
		if (useWayland) {
			wayland.outputs.~BinaryMap();
			wayland.outputsWeTouch.~Array();
			wayland.cursors.~BinaryMap();
		}
	}
}; // struct WindowData

} // namespace io

} // namespace AzCore

#endif // AZCORE_WINDOW_DATA_HPP
