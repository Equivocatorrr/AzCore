/*
	File: Wayland.cpp
	Author: Philip Haynes
	Separating out all our Wayland code, because there is a LOT of it.
*/

#ifndef AZCORE_WAYLAND_CPP
#define AZCORE_WAYLAND_CPP

#include "../Window.hpp"
#include "../../io.hpp"
#include "../../keycodes.hpp"
#include "WaylandProtocols/pointer-constraints-unstable-v1.h"
#include "WaylandProtocols/relative-pointer-manager-unstable-v1.h"
#include "WindowData.hpp"
#include <sys/mman.h>
#include <poll.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#ifndef NDEBUG
	#define AZCORE_WAYLAND_VERBOSE 0
#else
	#define AZCORE_WAYLAND_VERBOSE 0
#endif

#if AZCORE_WAYLAND_VERBOSE
#define DEBUG_PRINTLN(...) az::io::cout.PrintLn(__VA_ARGS__)
#else
#define DEBUG_PRINTLN(...)
#endif

namespace AzCore {

namespace io {

static inline float wl_fixed_to_float(wl_fixed_t fixed) {
	float result = (float)fixed / 256.0f;
	return result;
}

static inline wl_fixed_t wl_fixed_from_float(float value) {
	wl_fixed_t result = value * 256.0f;
	return result;
}

static wlCursor* GetSystemCursorWayland(Window *window, i32 scale) {
	i32 cursorSize = 0;
	// Try to read theme from XCURSOR_THEME
	// This might not work 100%, but even if themeName is null, we get a valid theme
	// TODO: use dbus to read system configuration
	const char *themeName = getenv("XCURSOR_THEME");
	const char *xCursorSize = getenv("XCURSOR_SIZE");
	DEBUG_PRINTLN("XCURSOR_THEME=", themeName ? themeName : "NULL", "\nXCURSOR_SIZE=", xCursorSize ? xCursorSize : "NULL");
	if (xCursorSize) {
		StringToI32(xCursorSize, &cursorSize);
	}
	if (cursorSize <= 0) cursorSize = 24;
	cursorSize *= scale;

	if (auto *node = window->data->wayland.cursors.Find(cursorSize)) {
		return &node->value;
	}
	wlCursor cursors;
	cursors.theme = wl_cursor_theme_load(themeName, cursorSize, window->data->wayland.shm);
	// TODO: Different cursors for different contexts
	cursors.cursor = wl_cursor_theme_get_cursor(cursors.theme, "left_ptr");
	DEBUG_PRINTLN("Getting new cursor with hotspot_x = ", cursors.cursor->images[0]->hotspot_x, ", hotspot_y = ", cursors.cursor->images[0]->hotspot_y);
	// TODO: Animated cursors
	cursors.buffer = wl_cursor_image_get_buffer(cursors.cursor->images[0]);
	cursors.surface = wl_compositor_create_surface(window->data->wayland.compositor);
	wl_surface_attach(cursors.surface, cursors.buffer, 0, 0);
	wl_surface_set_buffer_scale(cursors.surface, scale);
	wl_surface_commit(cursors.surface);
	return &window->data->wayland.cursors.Emplace(cursorSize, cursors);
}

void SetCursorWayland(Window *window) {
	if (!window->data->wayland.pointerFocus) return;
	if (window->cursorHidden) {
		wl_pointer_set_cursor(window->data->wayland.pointer, window->data->wayland.pointerEnterSerial, nullptr, 0, 0);
	} else {
		int scale = window->data->wayland.scale;
		wlCursor *cursor = GetSystemCursorWayland(window, scale);
		wl_cursor_image *image = cursor->cursor->images[0];
		wl_pointer_set_cursor(window->data->wayland.pointer, window->data->wayland.pointerEnterSerial, cursor->surface, image->hotspot_x / scale, image->hotspot_y / scale);
	}
}

void MoveCursorWayland(Window *window, i32 x, i32 y) {
	if (window->data->wayland.pointerConstraints == nullptr) {
		// Extension unavailable, bail out :(
		return;
	}
	if (window->input) {
		window->input->cursor = vec2i(x, y);
	}
	zwp_locked_pointer_v1 *lockedPointer = zwp_pointer_constraints_v1_lock_pointer(window->data->wayland.pointerConstraints, window->data->wayland.surface, window->data->wayland.pointer, window->data->wayland.region, ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT);
	i32 scale = window->data->wayland.scale;
	wl_fixed_t surfaceX = wl_fixed_from_int(x) / scale;
	wl_fixed_t surfaceY = wl_fixed_from_int(y) / scale;
	// DEBUG_PRINTLN("Locking cursor to ", surfaceX, ", ", surfaceY);
	zwp_locked_pointer_v1_set_cursor_position_hint(lockedPointer, surfaceX, surfaceY);
	wl_surface_commit(window->data->wayland.surface);
	zwp_locked_pointer_v1_destroy(lockedPointer);
}

// Sets width, height, and resized so the buffer will be resized at the end of an update
// Rather than updating the buffer here, since we can get many xdgToplevelConfigure events in one frame.
static void windowResizeLater(Window *window, i32 width, i32 height) {
	if (window->width != width || window->height != height) {
		window->width = width;
		window->height = height;
		window->resized = true;
	}
}

// Do this instead of wl_display_dispatch to avoid waiting for events if there are none
static bool wlDisplayDispatchNonblocking(wl_display *display, i32 displayFD) {
	pollfd fd = {
		.fd = displayFD,
		.events = POLLIN,
		.revents = 0,
	};
	if (wl_display_flush(display) < 0) {
		cerr.PrintLn("failed to flush display");
		return false;
	}
	while (wl_display_prepare_read(display) != 0) {
		if (wl_display_dispatch_pending(display) < 0) {
			cerr.PrintLn("wl_display_dispatch_pending failed when preparing for read");
			return false;
		}
	}
	i32 ready = poll(&fd, 1, 0);
	if (ready < 0) {
		cerr.PrintLn("poll failed: ", strerror(errno));
		return false;
	}
	if (ready > 0) {
		if (wl_display_read_events(display) < 0) {
			cerr.PrintLn("wl_display_read_events failed");
			return false;
		}
	} else {
		wl_display_cancel_read(display);
	}
	if (wl_display_dispatch_pending(display) < 0) {
		cerr.PrintLn("wl_display_dispatch_pending failed post read");
		return false;
	}
	return true;
}

static i32 CreateAnonymousFile(i32 size) {
	RandomNumberGenerator rng;
	const char *path = getenv("XDG_RUNTIME_DIR");
	String shmName;
	i32 tries = 0;
	i32 fd;
	do {
		shmName = Stringify(path, "/wayland-shm-", random(100000, 999999, &rng));
		fd = memfd_create(shmName.data, MFD_CLOEXEC);
		if (fd >= 0) {
			unlink(shmName.data);
			break;
		}
		tries++;
	} while (tries < 100 && errno == EEXIST);
	if (tries == 100) {
		return -1;
	}
	i32 ret;
	do {
		ret = ftruncate(fd, size);
	} while (ret < 0 && errno == EINTR);
	if (ret < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

static bool CreateShmImageWayland(i32 width, i32 height, i32 *dstFD, u32 **dstShmData, i32 *dstSize, wl_buffer **dstBuffer, String &dstError, io::Window *window) {
	wl_shm_pool *pool;
	i32 stride = width * 4;
	*dstSize = stride * height;
	*dstFD = CreateAnonymousFile(*dstSize);
	if (*dstFD < 0) {
		dstError = "Failed to create fd for shm";
		return false;
	}
	*dstShmData = (u32*)mmap(nullptr, *dstSize, PROT_READ | PROT_WRITE, MAP_SHARED, *dstFD, 0);
	if (*dstShmData == MAP_FAILED) {
		close(*dstFD);
		dstError = "Failed to map shm_data";
		return false;
	}
	for (i32 i = 0; i < width*height; i++) {
		(*dstShmData)[i] = 0xff000000;
	}

	pool = wl_shm_create_pool(window->data->wayland.shm, *dstFD, *dstSize);
	*dstBuffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	
	wl_surface_attach(window->data->wayland.surface, *dstBuffer, 0, 0);
	return true;
}

static void DestroyShmImageWayland(i32 fd, u32 *shmData, i32 size, wl_buffer *buffer) {
	munmap(shmData, size);
	close(fd);
	wl_buffer_destroy(buffer);
}

namespace wl::events {

static void surfaceEnter(void *data, wl_surface *surface, wl_output *output) {
	Window *window = (Window*)data;
	window->data->wayland.outputsWeTouch.Append(output);
	DEBUG_PRINTLN("surfaceEnter");
}

static void surfaceLeave(void *data, wl_surface *surface, wl_output *output) {
	Window *window = (Window*)data;
	for (i32 i = 0; i < window->data->wayland.outputsWeTouch.size; i++) {
		if (window->data->wayland.outputsWeTouch[i] == output) {
			window->data->wayland.outputsWeTouch.Erase(i);
			break;
		}
	}
	DEBUG_PRINTLN("surfaceLeave");
}

static const wl_surface_listener surfaceListener = {
	.enter = surfaceEnter,
	.leave = surfaceLeave,
};

static void xdgWMBasePing(void *data, xdg_wm_base *xdgWMBase, u32 serial) {
	xdg_wm_base_pong(xdgWMBase, serial);
}

static const xdg_wm_base_listener xdgWMBaseListener = {
	.ping = xdgWMBasePing
};

static void xdgSurfaceConfigure(void *data, xdg_surface *xdgSurface, u32 serial) {
	Window *window = (Window*)data;
	DEBUG_PRINTLN("xdgSurfaceConfigure");
	xdg_surface_ack_configure(xdgSurface, serial);
	// Don't do the actual resize here.
	// Instead, wait until we've received all the configure events and then do it.
	window->data->wayland.incomplete = false;
}

static const xdg_surface_listener xdgSurfaceListener = {
	.configure = xdgSurfaceConfigure
};

// wl_array_for_each breaks aliasing rules, so modify it a bit
#undef wl_array_for_each
#define wl_array_for_each(pos,array,type) for (pos = (type*)(array)->data; (const char *) pos < ((const char *) (array)->data + (array)->size); (pos)++)

static void xdgToplevelConfigure(void *data, xdg_toplevel *xdgToplevel, i32 width, i32 height, wl_array *states) {
	Window *window = (Window*)data;
	DEBUG_PRINTLN("xdgToplevelConfigure with width ", width, " and height ", height);
	if (width != 0 && height != 0) {
		windowResizeLater(window, width * window->data->wayland.scale, height * window->data->wayland.scale);
		xdg_toplevel_state *state;
		bool fullscreened = false;
		wl_array_for_each(state, states, xdg_toplevel_state) {
			switch (*state) {
				case XDG_TOPLEVEL_STATE_FULLSCREEN: {
					fullscreened = true;
				} break;
				default: break;
			}
		}
		DEBUG_PRINTLN("fullscreened = ", fullscreened ? "true" : "false");
		if (!fullscreened) {
			window->windowedWidth = window->width;
			window->windowedHeight = window->height;
		}
		window->data->wayland.incomplete = true;
	}
}

static void xdgToplevelClose(void *data, xdg_toplevel *xdgToplevel) {
	Window *window = (Window*)data;
	window->quit = true;
}

static void xdgToplevelConfigureBounds(void *data, xdg_toplevel *xdgToplevel, i32 width, i32 height) {
	Window *window = (Window*)data;
	window->data->wayland.widthMax = width;
	window->data->wayland.heightMax = height;
	DEBUG_PRINTLN("Max window bounds: ", width, ", ", height);
}

static void xdgToplevelWMCapabilities(void *data, xdg_toplevel *xdgToplevel, wl_array *capabilities) {
	// Do something
	DEBUG_PRINTLN("ToplevelWMCapabilities");
}

static const xdg_toplevel_listener xdgToplevelListener = {
	.configure = xdgToplevelConfigure,
	.close = xdgToplevelClose,
	.configure_bounds = xdgToplevelConfigureBounds,
	.wm_capabilities = xdgToplevelWMCapabilities
};

static void pointerEnter(void *data, wl_pointer *pointer, u32 serial, wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	Window *window = (Window*)data;
	window->data->wayland.pointerEnterSerial = serial;
	window->data->wayland.pointerFocus = true;
	if (window->input) {
		window->input->cursor = vec2i(wl_fixed_to_int(surface_x), wl_fixed_to_int(surface_y));
	}
	SetCursorWayland(window);
	DEBUG_PRINTLN("pointerEnter x = ", wl_fixed_to_float(surface_x), ", y = ", wl_fixed_to_float(surface_y));
}

static void pointerLeave(void *data, wl_pointer *pointer, u32 serial, wl_surface *surface) {
	Window *window = (Window*)data;
	window->data->wayland.pointerFocus = false;
	DEBUG_PRINTLN("pointerLeave");
}

static void pointerMotion(void *data, wl_pointer *pointer, u32 time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	Window *window = (Window*)data;
	if (window->_setCursor) return; // Ignore pointer motion if we set the cursor position this frame
	if (window->input) {
		float scale = window->data->wayland.scale;
		window->input->cursor = vec2i(
			round(wl_fixed_to_float(surface_x) * scale),
			round(wl_fixed_to_float(surface_y) * scale)
		);
	}
	// this is spammy af
	// DEBUG_PRINTLN("pointerMotion x = ", wl_fixed_to_float(surface_x), ", y = ", wl_fixed_to_float(surface_y), ", cursor.x = ", window->input->cursor.x, ", cursor.y = ", window->input->cursor.y);
}

static void HandleKCState(Input *input, u8 keycode, u32 state) {
	if (state) input->Press(keycode);
	else input->Release(keycode);
}

static void HandleCharState(Input *input, char character, u32 state) {
	if (state)
		input->typingString += character;
	if (character >= 'a' && character <= 'z') {
		character += 'A' - 'a';
	}
	if (state) input->PressChar(character);
	else input->ReleaseChar(character);
}

static void pointerButton(void *data, wl_pointer *pointer, u32 serial, u32 time, u32 button, u32 state) {
	Window *window = (Window*)data;
	if (window->input) {
		switch (button) {
			case BTN_LEFT: {
				HandleKCState(window->input, KC_MOUSE_LEFT, state);
			} break;
			case BTN_RIGHT: {
				HandleKCState(window->input, KC_MOUSE_RIGHT, state);
			} break;
			case BTN_MIDDLE: {
				HandleKCState(window->input, KC_MOUSE_MIDDLE, state);
			} break;
			case BTN_SIDE: {
				HandleKCState(window->input, KC_MOUSE_XONE, state);
			} break;
			case BTN_EXTRA: {
				HandleKCState(window->input, KC_MOUSE_XTWO, state);
			} break;
			case BTN_FORWARD: {
				cerr.PrintLn("Unhandled BTN_FORWARD");
			} break;
			case BTN_BACK: {
				cerr.PrintLn("Unhandled BTN_BACK");
			} break;
			case BTN_TASK: {
				cerr.PrintLn("Unhandled BTN_TASK");
			} break;
			default: {
				cerr.PrintLn("Unhandled mouse button ", button);
			} break;
		}
	}
	DEBUG_PRINTLN("pointerButton button = ", button, ", state = ", state);
}

// Not sure where this comes from, but this is what 1 tick of a scroll wheel amounts to on Gnome
// According to the spec, that means gnome is trying to scroll 10 pixels per tick.
// Whether this is a good baseline or not is yet to be seen.
static constexpr f32 magicScrollValue = 10.0f;

static void pointerAxis(void *data, wl_pointer *pointer, u32 time, u32 axis, wl_fixed_t value) {
	Window *window = (Window*)data;
	Input *input = window->input;
	float scroll = wl_fixed_to_float(value);
	if (input) {
		switch (axis) {
			case WL_POINTER_AXIS_VERTICAL_SCROLL: {
				input->scroll.y -= scroll / magicScrollValue;
				if (scroll > 0.0f) {
					HandleKCState(input, KC_MOUSE_SCROLLDOWN, 1);
					HandleKCState(input, KC_MOUSE_SCROLLDOWN, 0);
				} else {
					HandleKCState(input, KC_MOUSE_SCROLLUP, 1);
					HandleKCState(input, KC_MOUSE_SCROLLUP, 0);
				}
			} break;
			case WL_POINTER_AXIS_HORIZONTAL_SCROLL: {
				input->scroll.x += scroll / magicScrollValue;
				if (scroll > 0.0f) {
					HandleKCState(input, KC_MOUSE_SCROLLRIGHT, 1);
					HandleKCState(input, KC_MOUSE_SCROLLRIGHT, 0);
				} else {
					HandleKCState(input, KC_MOUSE_SCROLLLEFT, 1);
					HandleKCState(input, KC_MOUSE_SCROLLLEFT, 0);
				}
			} break;
		}
	}
	DEBUG_PRINTLN("pointerAxis axis = ", axis, ", scroll = ", scroll);
}

// Looks like we can ignore most of these

static void pointerFrame(void *data, wl_pointer *pointer) {
	// This is spammy
	// DEBUG_PRINTLN("pointerFrame");
}

static void pointerAxisSource(void *data, wl_pointer *pointer, u32 source) {
	DEBUG_PRINTLN("pointerAxisSource source = ", source);
}

static void pointerAxisStop(void *data, wl_pointer *pointer, u32 time, u32 axis) {
	DEBUG_PRINTLN("pointerAxisStop axis = ", axis);
}

static void pointerAxisDiscrete(void *data, wl_pointer *pointer, u32 axis, i32 discrete) {
	DEBUG_PRINTLN("pointerAxisDiscrete axis = ", axis, ", discrete = ", discrete);
}

static const wl_pointer_listener pointerListener = {
	.enter = pointerEnter,
	.leave = pointerLeave,
	.motion = pointerMotion,
	.button = pointerButton,
	.axis = pointerAxis,
	.frame = pointerFrame,
	.axis_source = pointerAxisSource,
	.axis_stop = pointerAxisStop,
	.axis_discrete = pointerAxisDiscrete,
};

void relativePointerMotion(void *data, struct zwp_relative_pointer_v1 *zwp_relative_pointer_v1, uint32_t utime_hi, uint32_t utime_lo, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t dx_unaccel, wl_fixed_t dy_unaccel) {
	Window *window = (Window*)data;
	if (!window->input) return;
	if (!window->_setCursor) return; // Ignore relative events unless we set the cursor this frame
	i32 scale = window->data->wayland.scale;
	vec2i totalMotion = vec2i(dx * scale, dy * scale) + window->data->wayland.relativePointerAccum;
	vec2i scaledMotion = totalMotion / 256;
	window->input->cursor += scaledMotion;
	window->data->wayland.relativePointerAccum = totalMotion - scaledMotion * 256;
	// DEBUG_PRINTLN("relativePointerMotion: dx = ", dx, ", dy = ", dy, "\naccum: x = ", window->data->wayland.relativePointerAccum.x, ", y = ", window->data->wayland.relativePointerAccum.y);
}

static const zwp_relative_pointer_v1_listener relativePointerListener = {
	.relative_motion = relativePointerMotion,
};

// NOTE: I don't actually have a touch device to test this on so fingers crossed I'm not doing something dum.
// In any case, making any attempt at touch support is better than none I think

static void touchDown(void *data, wl_touch *touch, u32 serial, u32 time, wl_surface *surface, i32 id, wl_fixed_t x, wl_fixed_t y) {
	// And the crowd goes wild!
	Window *window = (Window*)data;
	Input *input = window->input;
	// Support exactly one touch point
	if (input && window->data->wayland.touchId == TOUCH_ID_NONE) {
		// Only use this event if we don't already have an active id
		HandleKCState(input, KC_MOUSE_LEFT, 1);
		input->cursor = vec2i(x, y) * window->data->wayland.scale / 256;
		window->data->wayland.touchId = id;
	}
	DEBUG_PRINTLN("touchDown id = ", id);
}

static void touchUp(void *data, wl_touch *toutche, u32 serial, u32 time, i32 id) {
	Window *window = (Window*)data;
	Input *input = window->input;
	if (input && window->data->wayland.touchId == id) {
		// Ignore events not pertaining to the first id we got
		HandleKCState(input, KC_MOUSE_LEFT, 0);
		window->data->wayland.touchId = TOUCH_ID_NONE;
	}
	DEBUG_PRINTLN("touchUp id = ", id);
}

static void touchMotion(void *data, wl_touch *touch, u32 time, i32 id, wl_fixed_t x, wl_fixed_t y) {
	Window *window = (Window*)data;
	Input *input = window->input;
	if (input && window->data->wayland.touchId == id) {
		// Ignore events not pertaining to the first id we got
		input->cursor = vec2i(x, y) * window->data->wayland.scale / 256;
	}
	DEBUG_PRINTLN("touchMotion id = ", id);
}

static void touchFrame(void *data, wl_touch *touch) {
	// We're supposed to accumulate the above events and commit here but I'm not bovered
	DEBUG_PRINTLN("touchFrame");
}

static void touchCancel(void *data, wl_touch *touch) {
	// I guess we can get events and then bail
	Window *window = (Window*)data;
	Input *input = window->input;
	if (input && window->data->wayland.touchId != TOUCH_ID_NONE) {
		// If we have an active touch point, stop it, get help
		// Release without setting release because the press event wasn't meant for us
		input->inputs[KC_MOUSE_LEFT].Set(false, false, false);
		if (input->codeAny == KC_MOUSE_LEFT) input->Any.Set(false, false, false);
		if (input->codeAnyMB == KC_MOUSE_LEFT) input->AnyMB.Set(false, false, false);
		window->data->wayland.touchId = TOUCH_ID_NONE;
	}
	DEBUG_PRINTLN("touchCancel");
}

// I don't think we need these with seat version 5, but just in case, I'd rather not segfault

static void touchShape(void *data, wl_touch *touch, i32 id, wl_fixed_t major, wl_fixed_t minor) {
	DEBUG_PRINTLN("touchShape id = ", id);
}

static void touchOrientation(void *data, wl_touch *touch, i32 id, wl_fixed_t orientation) {
	DEBUG_PRINTLN("touchOrientation id = ", id);
}

static const wl_touch_listener touchListener = {
	.down = touchDown,
	.up = touchUp,
	.motion = touchMotion,
	.frame = touchFrame,
	.cancel = touchCancel,
	.shape = touchShape,
	.orientation = touchOrientation,
};

static void keyboardKeymap(void *data, wl_keyboard *wl_keyboard, u32 format, i32 fd, u32 size) {
	Window *window = (Window*)data;
	AzAssert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, "Unsupported wayland keymap");
	char *map_shm = (char*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
	AzAssert(map_shm != MAP_FAILED, "Failed to map the keymap shm");
	window->data->xkb.keymap = xkb_keymap_new_from_string(window->data->xkb.context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
	window->data->xkb.state = xkb_state_new(window->data->xkb.keymap);
	window->data->xkb.stateNone = xkb_state_new(window->data->xkb.keymap);
	munmap(map_shm, size);
	close(fd);
	DEBUG_PRINTLN("keyboardKeymap");
}

static void keyboardEnter(void *data, wl_keyboard *wl_keyboard, u32 serial, wl_surface *surface, wl_array *keys) {
	Window *window = (Window*)data;
	window->focused = true;
	// TODO: Handle the array of pressed keys
	DEBUG_PRINTLN("keyboardEnter");
}

static void keyboardLeave(void *data, wl_keyboard *wl_keyboard, u32 serial, wl_surface *surface) {
	Window *window = (Window*)data;
	window->focused = false;
	if (window->input != nullptr) {
		window->input->ReleaseAll();
	}
	DEBUG_PRINTLN("keyboardLeave");
}

static void keyboardKey(void *data, wl_keyboard *wl_keyboard, u32 serial, u32 time, u32 key, u32 state) {
	Window *window = (Window*)data;
	// Convert from evdev to xkb, which is apparently what we've been using this whole time while calling it evdev
	key += 8;
	if (key > 256) {
		cerr.PrintLn("xkb key code is too big (", key, ")");
		return;
	}
	u8 keycode = KeyCodeFromEvdev(key);
	if (state && keycode == KC_KEY_F11) {
		window->data->wayland.changeFullscreen = true;
		window->data->wayland.fullscreenSerial = serial;
	}
	if (!window->input) return;
	char character = '\0';
	char buffer[4] = {0};
	xkb_state_key_get_utf8(window->data->xkb.state, (xkb_keycode_t)key, buffer, 4);
	if (buffer[1] == '\0') {
		if (!(buffer[0] & 0x80)) {
			character = buffer[0];
		}
	}
	HandleKCState(window->input, keycode, state);
	if (character != '\0') {
		HandleCharState(window->input, character, state);
		// DEBUG_PRINTLN("char event '", character, "'");
	}
}

static void keyboardModifiers(void *data, wl_keyboard *wl_keyboard, u32 serial, u32 mods_depressed, u32 mods_latched, u32 mods_locked, u32 group) {
	Window *window = (Window*)data;
	xkb_state_update_mask(window->data->xkb.state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
	DEBUG_PRINTLN("keyboardModifiers");
}

static void keyboardRepeatInfo(void *data, wl_keyboard *wl_keyboard, i32 rate, i32 delay) {
	Window *window = (Window*)data;
	if (window->input) {
		if (rate == 0) {
			// Disable repeating by making the delay super long
			window->input->charRepeatDelay = 1000000.0f;
		} else {
			window->input->charRepeatsPerSecond = (f32)rate;
			window->input->charRepeatDelay = (f32)delay / 1000.0f;
		}
	}
	DEBUG_PRINTLN("keyboardRepeatInfo rate = ", rate, "Hz, delay = ", delay, "ms");
}

static const wl_keyboard_listener keyboardListener = {
	.keymap = keyboardKeymap,
	.enter = keyboardEnter,
	.leave = keyboardLeave,
	.key = keyboardKey,
	.modifiers = keyboardModifiers,
	.repeat_info = keyboardRepeatInfo
};

static void seatCapabilities(void *data, wl_seat *seat, u32 caps) {
	Window *window = (Window*)data;
	if (window->data->wayland.pointer) {
		wl_pointer_destroy(window->data->wayland.pointer);
		if (window->data->wayland.relativePointerManager && window->data->wayland.relativePointer) {
			zwp_relative_pointer_v1_destroy(window->data->wayland.relativePointer);
		}
	}
	if (window->data->wayland.keyboard) {
		wl_keyboard_destroy(window->data->wayland.keyboard);
	}
	if (window->data->wayland.touch) {
		wl_touch_destroy(window->data->wayland.touch);
	}
	if (caps & WL_SEAT_CAPABILITY_POINTER) {
		window->data->wayland.pointer = wl_seat_get_pointer(window->data->wayland.seat);
		wl_pointer_add_listener(window->data->wayland.pointer, &pointerListener, window);
		DEBUG_PRINTLN("Display has a pointer.");
		if (window->data->wayland.relativePointerManager) {
			window->data->wayland.relativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(window->data->wayland.relativePointerManager, window->data->wayland.pointer);
			zwp_relative_pointer_v1_add_listener(window->data->wayland.relativePointer, &relativePointerListener, window);
		}
	} else {
		window->data->wayland.pointer = nullptr;
		window->data->wayland.relativePointer = nullptr;
	}
	if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
		window->data->wayland.keyboard = wl_seat_get_keyboard(window->data->wayland.seat);
		wl_keyboard_add_listener(window->data->wayland.keyboard, &keyboardListener, window);
		DEBUG_PRINTLN("Display has a keyboard.");
	} else {
		window->data->wayland.keyboard = nullptr;
	}
	if (caps & WL_SEAT_CAPABILITY_TOUCH) {
		window->data->wayland.touch = wl_seat_get_touch(window->data->wayland.seat);
		wl_touch_add_listener(window->data->wayland.touch, &touchListener, window);
		DEBUG_PRINTLN("Display has a touch screen.");
	} else {
		window->data->wayland.touch = nullptr;
	}
}

static void seatName(void *data, wl_seat *seat, const char *name) {
	DEBUG_PRINTLN("seatName name = \"", name, "\"");
}

static const wl_seat_listener seatListener = {
	.capabilities = seatCapabilities,
	.name = seatName,
};

static void outputGeometry(void *data, wl_output *output, i32 x, i32 y, i32 physical_width, i32 physical_height, i32 subpixel, const char *make, const char *model, i32 transform) {
	Window *window = (Window*)data;
	AzAssert(window->data->wayland.outputs.Exists(output), "got an invalid wl_output");
	wlOutputInfo &info = window->data->wayland.outputs[output];
	info.x = x;
	info.y = y;
	info.phys_w = physical_width;
	info.phys_h = physical_height;
	info.make = make;
	info.model = model;
	DEBUG_PRINTLN("outputGeometry x = ", x, ", y = ", y, ", phys_w = ", physical_width, "mm, phys_h = ", physical_height, "mm, subpixel = ", subpixel, ", make = \"", make, "\", model = \"", model, "\", transform = ", transform);
}

static void outputMode(void *data, wl_output *output, u32 flags, i32 width, i32 height, i32 refresh) {
	Window *window = (Window*)data;
	AzAssert(window->data->wayland.outputs.Exists(output), "got an invalid wl_output");
	wlOutputInfo &info = window->data->wayland.outputs[output];
	info.width = width;
	info.height = height;
	info.refresh = refresh;
	DEBUG_PRINTLN("outputMode width = ", width, "px, height = ", height, "px, refresh = ", refresh, "mHz");
}

static void outputDone(void *data, wl_output *output) {
#if AZCORE_WAYLAND_VERBOSE
	Window *window = (Window*)data;
	AzAssert(window->data->wayland.outputs.Exists(output), "got an invalid wl_output");
	wlOutputInfo &info = window->data->wayland.outputs[output];
	i32 dpiX = info.width * 254 / 10 / info.phys_w;
	i32 dpiY = info.height * 254 / 10 / info.phys_h;
	cout.PrintLn("outputDone dpi = ", dpiX, ", ", dpiY);
#endif
}

static void outputScale(void *data, wl_output *output, i32 factor) {
	Window *window = (Window*)data;
	AzAssert(window->data->wayland.outputs.Exists(output), "got an invalid wl_output");
	wlOutputInfo &info = window->data->wayland.outputs[output];
	info.scale = factor;
	DEBUG_PRINTLN("outputScale factor = ", factor);
}

static void outputName(void *data, wl_output *output, const char *name) {
	Window *window = (Window*)data;
	AzAssert(window->data->wayland.outputs.Exists(output), "got an invalid wl_output");
	wlOutputInfo &info = window->data->wayland.outputs[output];
	info.name = name;
	DEBUG_PRINTLN("outputName name = \"", name, "\"");
}

static void outputDescription(void *data, wl_output *output, const char *description) {
	Window *window = (Window*)data;
	AzAssert(window->data->wayland.outputs.Exists(output), "got an invalid wl_output");
	wlOutputInfo &info = window->data->wayland.outputs[output];
	info.description = description;
	DEBUG_PRINTLN("outputDescription description = \"", description, "\"");
}

static const wl_output_listener outputListener = {
	.geometry = outputGeometry,
	.mode = outputMode,
	.done = outputDone,
	.scale = outputScale,
	.name = outputName,
	.description = outputDescription,
};

static constexpr u32 compositorInterfaceVersion = 4;
static constexpr u32 outputInterfaceVersion = 2;
static constexpr u32 xdgWMBaseInterfaceVersion = 4;
static constexpr u32 seatInterfaceVersion = 5;
static constexpr u32 shmInterfaceVersion = 1;
static constexpr u32 pointerConstrainstInterfaceVersion = 1;
static constexpr u32 relativePointerManagerInterfaceVersion = 1;

static void globalRegistryAdd(void *data, wl_registry *registry, u32 id, const char *interface, u32 version) {
	Window *window = (Window*)data;
	DEBUG_PRINTLN("Got a registry add event for ", interface, " id ", id);
	if (equals(interface, wl_compositor_interface.name)) {
		window->data->wayland.compositor = (wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, compositorInterfaceVersion);
	} else if (equals(interface, xdg_wm_base_interface.name)) {
		window->data->wayland.wmBase = (xdg_wm_base*)wl_registry_bind(registry, id, &xdg_wm_base_interface, xdgWMBaseInterfaceVersion);
		xdg_wm_base_add_listener(window->data->wayland.wmBase, &xdgWMBaseListener, window);
	} else if (equals(interface, wl_seat_interface.name)) {
		window->data->wayland.seat = (wl_seat*)wl_registry_bind(registry, id, &wl_seat_interface, seatInterfaceVersion);
		wl_seat_add_listener(window->data->wayland.seat, &seatListener, window);
	} else if (equals(interface, wl_shm_interface.name)) {
		window->data->wayland.shm = (wl_shm*)wl_registry_bind(registry, id, &wl_shm_interface, shmInterfaceVersion);
	} else if (equals(interface, wl_output_interface.name)) {
		wl_output *output = (wl_output*)wl_registry_bind(registry, id, &wl_output_interface, outputInterfaceVersion);
		window->data->wayland.outputs.Emplace(output, wlOutputInfo());
		wl_output_add_listener(output, &outputListener, window);
	} else if (equals(interface, zwp_pointer_constraints_v1_interface.name)) {
		window->data->wayland.pointerConstraints = (zwp_pointer_constraints_v1*)wl_registry_bind(registry, id, &zwp_pointer_constraints_v1_interface, pointerConstrainstInterfaceVersion);
	} else if (equals(interface, zwp_relative_pointer_manager_v1_interface.name)) {
		window->data->wayland.relativePointerManager = (zwp_relative_pointer_manager_v1*)wl_registry_bind(registry, id, &zwp_relative_pointer_manager_v1_interface, relativePointerManagerInterfaceVersion);
	}
}

static void globalRegistryRemove(void *data, wl_registry *registry, u32 id) {
	DEBUG_PRINTLN("Got a registry remove event for ", id);
}

static const wl_registry_listener registryListener = {
	globalRegistryAdd,
	globalRegistryRemove
};

} // namespace wl::events

static void xkbSetupKeyboardWayland(xkb_keyboard *xkb) {
	xkb->context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
}

static i32 GetWindowScaleWayland(Window *window) {
	i32 maxScale = 1;
	for (wl_output *output : window->data->wayland.outputsWeTouch) {
		wlOutputInfo &info = window->data->wayland.outputs[output];
		if (info.scale > maxScale) maxScale = info.scale;
	}
	window->data->wayland.scale = maxScale;
	return maxScale;
}

static u32 GetWindowRefreshWayland(Window *window) {
	u32 maxRefresh = 0;
	for (wl_output *output : window->data->wayland.outputsWeTouch) {
		wlOutputInfo &info = window->data->wayland.outputs[output];
		if (info.refresh > (i32)maxRefresh) maxRefresh = max(info.refresh, 0);
	}
	if (maxRefresh == 0) maxRefresh = 60000;
	return maxRefresh;
}

bool windowOpenWayland(Window *window) {
	window->data->wayland.scale = 1;
	// Connect to the display named by $WAYLAND_DISPLAY if it's defined
	// Or "wayland-0" if $WAYLAND_DISPLAY is not defined
	if (nullptr == (window->data->wayland.display = wl_display_connect(nullptr))) {
		error = "Failed to open Wayland display";
		return false;
	}
	window->data->wayland.displayFD = wl_display_get_fd(window->data->wayland.display);
	wl_registry *registry = wl_display_get_registry(window->data->wayland.display);
	wl_registry_add_listener(registry, &wl::events::registryListener, (void*)window);
	wl_display_roundtrip(window->data->wayland.display);

	if (window->data->wayland.compositor == nullptr) {
		error = "Can't find compositor";
		return false;
	}

	if (nullptr == (window->data->wayland.surface = wl_compositor_create_surface(window->data->wayland.compositor))) {
		error = "Can't create surface";
		return false;
	}
	wl_surface_add_listener(window->data->wayland.surface, &wl::events::surfaceListener, window);
	

	if (window->data->wayland.wmBase == nullptr) {
		error = "We don't have an xdg_wm_base";
		return false;
	}

	if (nullptr == (window->data->wayland.xdgSurface = xdg_wm_base_get_xdg_surface(window->data->wayland.wmBase, window->data->wayland.surface))) {
		error = "Can't create an xdg_surface";
		return false;
	}
	
	xdg_surface_add_listener(window->data->wayland.xdgSurface, &wl::events::xdgSurfaceListener, window);
	
	if (nullptr == (window->data->wayland.xdgToplevel = xdg_surface_get_toplevel(window->data->wayland.xdgSurface))) {
		error = "Can't create an xdg_toplevel";
		return false;
	}
	
	xdg_toplevel_set_app_id(window->data->wayland.xdgToplevel, window->name.data);
	xdg_toplevel_set_title(window->data->wayland.xdgToplevel, window->name.data);
	xdg_toplevel_add_listener(window->data->wayland.xdgToplevel, &wl::events::xdgToplevelListener, window);

	if (window->data->wayland.seat == nullptr) {
		error = "We don't have a Wayland seat";
		return false;
	}
	if (!CreateShmImageWayland(window->width, window->height, &window->data->wayland.image.fd, &window->data->wayland.image.shmData, &window->data->wayland.image.size, &window->data->wayland.image.buffer, error, window)) return false;

	window->data->wayland.region = wl_compositor_create_region(window->data->wayland.compositor);
	wl_region_add(window->data->wayland.region, 0, 0, window->width, window->height);
	wl_surface_set_opaque_region(window->data->wayland.surface, window->data->wayland.region);
	wl_surface_commit(window->data->wayland.surface);

	xkbSetupKeyboardWayland(&window->data->xkb);
	
	// We need to know which surface we're on to get the DPI
	i32 tries = 0;
	while (window->data->wayland.outputsWeTouch.size == 0) {
		wl_display_dispatch(window->data->wayland.display);
		tries++;
		if (tries > 10) break; // This shouldn't happen but you never know
	}

	u16 newDpi = GetWindowScaleWayland(window) * 96;
	if (window->dpi != newDpi) {
		window->dpi = newDpi;
	}
	i32 newRefresh = GetWindowRefreshWayland(window);
	window->refreshRate = newRefresh;

	window->data->wayland.touchId = TOUCH_ID_NONE;
	window->data->wayland.hadError = false;
	window->open = true;
	return true;
}

void windowFullscreenWayland(Window *window) {
	if (window->fullscreen) {
		xdg_toplevel_set_max_size(window->data->wayland.xdgToplevel, 0, 0);
		wl_surface_commit(window->data->wayland.surface);
		xdg_toplevel_set_fullscreen(window->data->wayland.xdgToplevel, nullptr);
	} else {
		xdg_toplevel_set_max_size(window->data->wayland.xdgToplevel, window->data->wayland.widthMax, window->data->wayland.heightMax);
		wl_surface_commit(window->data->wayland.surface);
		xdg_toplevel_unset_fullscreen(window->data->wayland.xdgToplevel);
	}
}

// Resizes the actual buffer
void windowResizeWayland(Window *window) {
	window->resized = true;
	i32 width = window->width;
	i32 height = window->height;
	AzAssert(width != 0 && height != 0, "window size is invalid");
	if (window->data->wayland.image.buffer) {
		DestroyShmImageWayland(window->data->wayland.image.fd, window->data->wayland.image.shmData, window->data->wayland.image.size, window->data->wayland.image.buffer);
	}
	if (!CreateShmImageWayland(width, height, &window->data->wayland.image.fd, &window->data->wayland.image.shmData, &window->data->wayland.image.size, &window->data->wayland.image.buffer, error, window)) {
		window->data->wayland.hadError = true;
		return;
	}
	if (window->data->wayland.region) {
		wl_region_destroy(window->data->wayland.region);
	}
	window->data->wayland.region = wl_compositor_create_region(window->data->wayland.compositor);
	wl_region_add(window->data->wayland.region, 0, 0, width, height);
	wl_surface_set_opaque_region(window->data->wayland.surface, window->data->wayland.region);
	wl_surface_set_buffer_scale(window->data->wayland.surface, window->data->wayland.scale);
	wl_surface_commit(window->data->wayland.surface);
}

bool windowUpdateWayland(Window *window, bool &changeFullscreen) {
	window->data->wayland.changeFullscreen = false;
	if (!wlDisplayDispatchNonblocking(window->data->wayland.display, window->data->wayland.displayFD)) return false;
	while (window->data->wayland.incomplete) {
		// Do the blocking one until we get all the events we needed
		if (wl_display_dispatch(window->data->wayland.display) < 0) return false;
	}
	u16 newDpi = GetWindowScaleWayland(window) * 96;
	if (window->dpi != newDpi) {
		windowResizeLater(window, window->width * newDpi / window->dpi, window->height * newDpi / window->dpi);
		window->dpi = newDpi;
	}
	u32 newRefresh = GetWindowRefreshWayland(window);
	window->refreshRate = newRefresh;
	if (window->resized) {
		windowResizeWayland(window);
	}
	changeFullscreen = window->data->wayland.changeFullscreen;
	return !window->quit && !window->data->wayland.hadError;
}

void windowCloseWayland(Window *window) {
	wl_display_disconnect(window->data->wayland.display);
}

} // namespace io

} // namespace AzCore

#endif // AZCORE_WAYLAND_CPP
