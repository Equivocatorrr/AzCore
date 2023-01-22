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
#include "WindowData.hpp"
#include <sys/mman.h>
// #include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#if defined(NDEBUG) && 1
#define DEBUG_PRINTLN(...)
#else
#define DEBUG_PRINTLN(...) az::io::cout.PrintLn(__VA_ARGS__)
#endif

namespace AzCore {

namespace io {
	
void windowResizeWayland(Window *window, i32 width, i32 height) {
	window->width = width;
	window->height = height;
	window->resized = true;
}

// Do this instead of wl_display_dispatch to avoid waiting for events
bool waylandDispatch(Window *window) {
	wl_display *display = window->data->wayland.display;
	pollfd fd = {
		.fd = window->data->wayland.displayFD,
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

i32 createAnonymousFile(i32 size) {
	RandomNumberGenerator rng;
	String path = getenv("XDG_RUNTIME_DIR");
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

bool CreateShmImageWayland(i32 width, i32 height, i32 *dstFD, u32 **dstShmData, i32 *dstSize, wl_buffer **dstBuffer, String &dstError, io::Window *window) {
	wl_shm_pool *pool;
	i32 stride = width * 4;
	*dstSize = stride * height;
	*dstFD = createAnonymousFile(*dstSize);
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

void DestroyShmImageWayland(i32 fd, u32 *shmData, i32 size, wl_buffer *buffer) {
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
	// This is done in windowUpdateWayland
	// windowResizeWayland(window);
	window->data->wayland.incomplete = false;
}

static const xdg_surface_listener xdgSurfaceListener = {
	.configure = xdgSurfaceConfigure
};

#undef wl_array_for_each
#define wl_array_for_each(pos,array,type) for (pos = (type*)(array)->data; (const char *) pos < ((const char *) (array)->data + (array)->size); (pos)++)

static void xdgToplevelConfigure(void *data, xdg_toplevel *xdgToplevel, i32 width, i32 height, wl_array *states) {
	Window *window = (Window*)data;
	DEBUG_PRINTLN("xdgToplevelConfigure with width ", width, " and height ", height);
	if (width != 0 && height != 0) {
		windowResizeWayland(window, width * window->data->wayland.scale, height * window->data->wayland.scale);
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

float wl_fixed_to_float(wl_fixed_t fixed) {
	float result = (float)fixed / 256.0f;
	return result;
}

static void pointerEnter(void *data, wl_pointer *wl_pointer, u32 serial, wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	// TODO: Use serial to change pointer image
	Window *window = (Window*)data;
	if (window->input) {
		window->input->cursor = vec2i(wl_fixed_to_int(surface_x), wl_fixed_to_int(surface_y));
	}
	DEBUG_PRINTLN("pointerEnter x = ", wl_fixed_to_float(surface_x), ", y = ", wl_fixed_to_float(surface_y));
}

static void pointerLeave(void *data, wl_pointer *wl_pointer, u32 serial, wl_surface *surface) {
	DEBUG_PRINTLN("pointerLeave");
}

static void pointerMotion(void *data, wl_pointer *wl_pointer, u32 time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	Window *window = (Window*)data;
	if (window->input) {
		window->input->cursor = vec2i(wl_fixed_to_int(surface_x), wl_fixed_to_int(surface_y)) * window->data->wayland.scale;
	}
	// this is spammy af
	// DEBUG_PRINTLN("pointerMotion x = ", wl_fixed_to_float(surface_x), ", y = ", wl_fixed_to_float(surface_y));
}

void HandleKCState(Input *input, u8 keycode, u32 state) {
	if (state) input->Press(keycode);
	else input->Release(keycode);
}

void HandleCharState(Input *input, char character, u32 state) {
	if (state)
		input->typingString += character;
	if (character >= 'a' && character <= 'z') {
		character += 'A' - 'a';
	}
	if (state) input->PressChar(character);
	else input->ReleaseChar(character);
}

static void pointerButton(void *data, wl_pointer *wl_pointer, u32 serial, u32 time, u32 button, u32 state) {
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

// Can someone explain to me why we need 9 SEPARATE EVENTS!!!
// Especially when you're supposed to COMBINE THEM ANYWAY???
static const wl_pointer_listener pointerListener = {
	.enter = pointerEnter,
	.leave = pointerLeave,
	.motion = pointerMotion,
	.button = pointerButton,
	.axis = pointerAxis,
	.frame = pointerFrame,
	.axis_source = pointerAxisSource,
	.axis_stop = pointerAxisStop,
	.axis_discrete = pointerAxisDiscrete
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
	} else {
		window->data->wayland.pointer = nullptr;
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
		// TODO: Listen to touch stuff
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
	Window *window = (Window*)data;
	AzAssert(window->data->wayland.outputs.Exists(output), "got an invalid wl_output");
	wlOutputInfo &info = window->data->wayland.outputs[output];
	// Commit some changes I guess
	i32 dpiX = info.width * 254 / 10 / info.phys_w;
	i32 dpiY = info.height * 254 / 10 / info.phys_h;
	DEBUG_PRINTLN("outputDone dpi = ", dpiX, ", ", dpiY);
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

void xkbSetupKeyboardWayland(xkb_keyboard *xkb) {
	xkb->context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
}

bool windowOpenWayland(Window *window) {
	WindowData *data = window->data;
	u16 &width = window->width;
	u16 &height = window->height;
	String &name = window->name;
	bool &open = window->open;
	window->data->wayland.scale = 1;
	// u16 &dpi = window->dpi;
	// Connect to the display named by $WAYLAND_DISPLAY if it's defined
	// Or "wayland-0" if $WAYLAND_DISPLAY is not defined
	if (nullptr == (data->wayland.display = wl_display_connect(nullptr))) {
		error = "Failed to open Wayland display";
		return false;
	}
	data->wayland.displayFD = wl_display_get_fd(data->wayland.display);
	wl_registry *registry = wl_display_get_registry(data->wayland.display);
	wl_registry_add_listener(registry, &wl::events::registryListener, (void*)window);
	wl_display_roundtrip(data->wayland.display);

	if (data->wayland.compositor == nullptr) {
		error = "Can't find compositor";
		return false;
	}

	if (nullptr == (data->wayland.surface = wl_compositor_create_surface(data->wayland.compositor))) {
		error = "Can't create surface";
		return false;
	}
	wl_surface_add_listener(data->wayland.surface, &wl::events::surfaceListener, window);
	

	if (data->wayland.wmBase == nullptr) {
		error = "We don't have an xdg_wm_base";
		return false;
	}

	if (nullptr == (data->wayland.xdgSurface = xdg_wm_base_get_xdg_surface(data->wayland.wmBase, data->wayland.surface))) {
		error = "Can't create an xdg_surface";
		return false;
	}
	
	xdg_surface_add_listener(data->wayland.xdgSurface, &wl::events::xdgSurfaceListener, window);
	
	if (nullptr == (data->wayland.xdgToplevel = xdg_surface_get_toplevel(data->wayland.xdgSurface))) {
		error = "Can't create an xdg_toplevel";
		return false;
	}
	
	xdg_toplevel_set_app_id(data->wayland.xdgToplevel, name.data);
	xdg_toplevel_set_title(data->wayland.xdgToplevel, name.data);
	xdg_toplevel_add_listener(data->wayland.xdgToplevel, &wl::events::xdgToplevelListener, window);

	if (data->wayland.seat == nullptr) {
		error = "We don't have a Wayland seat";
		return false;
	}
	if (!CreateShmImageWayland(width, height, &window->data->wayland.image.fd, &window->data->wayland.image.shmData, &window->data->wayland.image.size, &window->data->wayland.image.buffer, error, window)) return false;

	window->data->wayland.region = wl_compositor_create_region(window->data->wayland.compositor);
	wl_region_add(window->data->wayland.region, 0, 0, width, height);
	wl_surface_set_opaque_region(window->data->wayland.surface, window->data->wayland.region);
	wl_surface_commit(window->data->wayland.surface);

	xkbSetupKeyboardWayland(&data->xkb);

	window->data->wayland.hadError = false;
	open = true;
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

void windowResizeWaylandShm(Window *window) {
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

i32 GetWindowScaleWayland(Window *window) {
	i32 maxScale = 1;
	for (wl_output *output : window->data->wayland.outputsWeTouch) {
		wlOutputInfo &info = window->data->wayland.outputs[output];
		if (info.scale > maxScale) maxScale = info.scale;
	}
	window->data->wayland.scale = maxScale;
	return maxScale;
}

bool windowUpdateWayland(Window *window, bool &changeFullscreen) {
	WindowData *data = window->data;
	window->data->wayland.changeFullscreen = false;
	if (!waylandDispatch(window)) return false;
	while (window->data->wayland.incomplete) {
		// Do the blocking one until we get all the events we needed
		if (wl_display_dispatch(window->data->wayland.display) < 0) return false;
	}
	u16 newDpi = GetWindowScaleWayland(window) * 96;
	if (window->dpi != newDpi) {
		windowResizeWayland(window, window->width * newDpi / window->dpi, window->height * newDpi / window->dpi);
		window->dpi = newDpi;
	}
	if (window->resized) {
		windowResizeWaylandShm(window);
	}
	changeFullscreen = data->wayland.changeFullscreen;
	return !window->quit && !window->data->wayland.hadError;
}

} // namespace io

} // namespace AzCore

#endif // AZCORE_WAYLAND_CPP
