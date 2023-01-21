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

namespace AzCore {

namespace io {

i32 GetWindowDpiX11(Window *window);

xcb_atom_t xcbGetAtom(xcb_connection_t *connection, bool onlyIfExists, const String &name) {
	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t *reply;

	// In order to access our close button event
	cookie = xcb_intern_atom(connection, (u8)onlyIfExists, name.size, name.data);
	reply = xcb_intern_atom_reply(connection, cookie, 0);

	if (reply == nullptr) {
		error = "Failed to get reply to a cookie for retrieving an XCB atom!";
		return 0;
	}
	xcb_atom_t atom = reply->atom;
	free(reply);

	return atom;
}

char* xcbGetProperty(xcb_connection_t *connection, xcb_window_t window, xcb_atom_t atom, xcb_atom_t type, i32 size) {
	xcb_get_property_cookie_t cookie;
	xcb_get_property_reply_t *reply;
	xcb_generic_error_t *err;
	i32 reply_len;

	cookie = xcb_get_property(connection, 0, window, atom, type, 0, size);
	reply = xcb_get_property_reply(connection, cookie, &err);
	if (err != nullptr) {
		free(err);
		return nullptr;
	}
	if (reply == nullptr) {
		return nullptr;
	}
	if(0 == (reply_len = xcb_get_property_value_length(reply))) {
		free(reply);
		return nullptr;
	}
	if (reply->bytes_after > 0) {
		i32 newSize = size + (i32)ceil(reply->bytes_after / 4.0f);
		free(reply);
		return xcbGetProperty(connection, window, atom, type, newSize);
	}
	char *content = new char[reply_len+1];
	memcpy(content, xcb_get_property_value(reply), reply_len);
	content[reply_len] = 0;
	free(reply);
	return content;
}

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

void xkbCleanupX11(xkb_keyboard *xkb) {
	if (xkb->keymap) {
		xkb_keymap_unref(xkb->keymap);
		xkb->keymap = nullptr;
	}
	if (xkb->state) {
		xkb_state_unref(xkb->state);
		xkb->state = nullptr;
	}
	if (xkb->stateNone) {
		xkb_state_unref(xkb->stateNone);
		xkb->stateNone = nullptr;
	}
	if (xkb->context) {
		xkb_context_unref(xkb->context);
		xkb->context = nullptr;
	}
}

bool xkbUpdateKeymapX11(xkb_keyboard *xkb) {
	// Cleanup first
	if (xkb->keymap) {
		xkb_keymap_unref(xkb->keymap);
	}
	if (xkb->state) {
		xkb_state_unref(xkb->state);
	}
	if (xkb->stateNone) {
		xkb_state_unref(xkb->stateNone);
	}
	// Then reload
	xkb->keymap = xkb_x11_keymap_new_from_device(xkb->context, xkb->connection, xkb->deviceId, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!xkb->keymap) {
		error = "Cannot get XKB keymap from device!";
		return false;
	}

	xkb->state = xkb_x11_state_new_from_device(xkb->keymap, xkb->connection, xkb->deviceId);
	if (!xkb->state) {
		xkb_keymap_unref(xkb->keymap);
		xkb->keymap = nullptr;
		error = "Cannot get XKB state from keymap!";
		return false;
	}

	xkb->stateNone = xkb_x11_state_new_from_device(xkb->keymap, xkb->connection, xkb->deviceId);
	if (!xkb->stateNone) {
		xkb_keymap_unref(xkb->keymap);
		xkb->keymap = nullptr;
		xkb_state_unref(xkb->state);
		xkb->state = nullptr;
		error = "Cannot get XKB stateNone from keymap!";
		return false;
	}
	xkb_layout_index_t layout = xkb_state_key_get_layout(xkb->stateNone, 0);
	xkb_state_update_mask(xkb->stateNone, 2, 2, 2, layout, layout, layout);
	return true;
}

bool xkbSetupKeyboardX11(xkb_keyboard *xkb, xcb_connection_t *connection) {
	xkb->connection = connection;
	if (!xkb_x11_setup_xkb_extension(xkb->connection,
									 XKB_X11_MIN_MAJOR_XKB_VERSION,
									 XKB_X11_MIN_MINOR_XKB_VERSION,
									 XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
									 NULL, NULL, &xkb->first_xkb_event, NULL)) {
		error = "Failed to connect xkb to x11!";
		return false;
	}

	xkb->context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!xkb->context) {
		error = "Cannot get XKB context!";
		return false;
	}

	xkb->deviceId = xkb_x11_get_core_keyboard_device_id(xkb->connection);
	if (xkb->deviceId == -1) {
		xkb_context_unref(xkb->context);
		xkb->context = nullptr;
		error = "Cannot get XKB keyboard device id!";
		return false;
	}

	return xkbUpdateKeymapX11(xkb);
}

struct xkb_generic_event_t {
	u8 response_type;
	u8 xkb_type;
	u16 sequence;
	xcb_timestamp_t time;
	u8 deviceId;
};

bool xkbProcessEvent(xkb_keyboard *xkb, xkb_generic_event_t *event) {
	if (event->deviceId != xkb->deviceId) {
		return true;
	}

	switch (event->xkb_type) {
	case XCB_XKB_NEW_KEYBOARD_NOTIFY: {
		// cout << "XCB_XKB_NEW_KEYBOARD_NOTIFY" << std::endl;
		xcb_xkb_new_keyboard_notify_event_t *ev = (xcb_xkb_new_keyboard_notify_event_t *)event;
		if (ev->changed) {
			if (!xkbUpdateKeymapX11(xkb))
				return false;
		}
		break;
	}
	case XCB_XKB_MAP_NOTIFY: {
		// cout << "XCB_XKB_MAP_NOTIFY" << std::endl;
		if (!xkbUpdateKeymapX11(xkb))
			return false;
		break;
	}
	case XCB_XKB_STATE_NOTIFY: {
		// cout << "XCB_XKB_STATE_NOTIFY" << std::endl;
		xcb_xkb_state_notify_event_t *ev = (xcb_xkb_state_notify_event_t *)event;
		xkb_state_update_mask(xkb->state,
							  ev->baseMods,
							  ev->latchedMods,
							  ev->lockedMods,
							  ev->baseGroup,
							  ev->latchedGroup,
							  ev->lockedGroup);
		break;
	}
	default: {
		break;
	}
	}
	return true;
}

bool xkbSelectEventsForDevice(xkb_keyboard *xkb) {
	enum {
		required_events =
			(XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
			 XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
			 XCB_XKB_EVENT_TYPE_STATE_NOTIFY),

		required_nkn_details =
			(XCB_XKB_NKN_DETAIL_KEYCODES),

		required_map_parts =
			(XCB_XKB_MAP_PART_KEY_TYPES |
			 XCB_XKB_MAP_PART_KEY_SYMS |
			 XCB_XKB_MAP_PART_MODIFIER_MAP |
			 XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
			 XCB_XKB_MAP_PART_KEY_ACTIONS |
			 XCB_XKB_MAP_PART_VIRTUAL_MODS |
			 XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP),

		required_state_details =
			(XCB_XKB_STATE_PART_MODIFIER_BASE |
			 XCB_XKB_STATE_PART_MODIFIER_LATCH |
			 XCB_XKB_STATE_PART_MODIFIER_LOCK |
			 XCB_XKB_STATE_PART_GROUP_BASE |
			 XCB_XKB_STATE_PART_GROUP_LATCH |
			 XCB_XKB_STATE_PART_GROUP_LOCK),
	};

	static const xcb_xkb_select_events_details_t details = {
		.affectNewKeyboard = required_nkn_details,
		.newKeyboardDetails = required_nkn_details,
		.affectState = required_state_details,
		.stateDetails = required_state_details,
	};

	xcb_void_cookie_t cookie =
		xcb_xkb_select_events_aux_checked(xkb->connection,
										  xkb->deviceId,
										  required_events,    /* affectWhich */
										  0,                  /* clear */
										  0,                  /* selectAll */
										  required_map_parts, /* affectMap */
										  required_map_parts, /* map */
										  &details);          /* details */

	xcb_generic_error_t *err = xcb_request_check(xkb->connection, cookie);
	if (err) {
		free(err);
		error = "Failed to select xkb events for device";
		return false;
	}

	return true;
}

Window::Window() {
	char *wayland = getenv("WAYLAND_DISPLAY");
	data = new WindowData(wayland != nullptr);
	if (!data->useWayland) {
		data->x11.windowDepth = 24;
	}
	cout.PrintLn("Wayland is ", data->useWayland ? "enabled" : "disabled");
	data->frameCount = 0;
}

Window::~Window() {
	if (open) {
		Close();
	}
	delete data;
}

#ifndef AZCORE_IO_NO_XLIB
#define CLOSE_CONNECTION(data) XCloseDisplay(data->x11.display)
#else
#define CLOSE_CONNECTION(data) xcb_disconnect(data->x11.connection)
#endif

bool windowOpenX11(Window *window) {
	WindowData *data = window->data;
	i16 &x = window->x;
	i16 &y = window->y;
	u16 &width = window->width;
	u16 &height = window->height;
	String &name = window->name;
	bool &open = window->open;
	u16 &dpi = window->dpi;
	i32 defaultScreen = 0;
#ifndef AZCORE_IO_NO_XLIB
	data->x11.display = XOpenDisplay(0);
	if (!data->x11.display) {
		error = "Can't open X display";
		return false;
	}

	defaultScreen = DefaultScreen(data->x11.display);

	data->x11.connection = XGetXCBConnection(data->x11.display);
	if (!data->x11.connection) {
		XCloseDisplay(data->x11.display);
		error = "Can't get xcb connection from display";
		return false;
	}

	XSetEventQueueOwner(data->x11.display, XCBOwnsEventQueue);
#else
	data->x11.connection = xcb_connect(NULL, NULL);

	if (xcb_connection_has_error(data->x11.connection) > 0) {
		error = "Cannot open display";
		return false;
	}
#endif

	/* Find XCB screen */
	data->x11.screen = 0;
	xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(xcb_get_setup(data->x11.connection));
	for (i32 screen_num = defaultScreen;
		 screen_iter.rem && screen_num > 0;
		 --screen_num, xcb_screen_next(&screen_iter))
		;
	data->x11.screen = screen_iter.data;

	xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(data->x11.screen);
	xcb_depth_t *depth = nullptr;

	while (depth_iter.rem) {
		if (depth_iter.data->depth == data->x11.windowDepth && depth_iter.data->visuals_len) {
			depth = depth_iter.data;
			break;
		}
		xcb_depth_next(&depth_iter);
	}

	if (!depth) {
		CLOSE_CONNECTION(data);
		error = "Screen doesn't support " + ToString(data->x11.windowDepth) + "-bit depth!";
		return false;
	}

	xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth);
	xcb_visualtype_t *visual = nullptr;

	while (visual_iter.rem) {
		if (visual_iter.data->_class == XCB_VISUAL_CLASS_TRUE_COLOR) {
			visual = visual_iter.data;
			break;
		}
		xcb_visualtype_next(&visual_iter);
	}

	if (!visual) {
		CLOSE_CONNECTION(data);
		error = "Screen doesn't support True Color";
		return false;
	}
	data->x11.visualID = visual->visual_id;

	xcb_void_cookie_t cookie;

	data->x11.colormap = xcb_generate_id(data->x11.connection);
	cookie = xcb_create_colormap_checked(data->x11.connection, XCB_COLORMAP_ALLOC_NONE,
										 data->x11.colormap, data->x11.screen->root, data->x11.visualID);

	if (xcb_generic_error_t *err = xcb_request_check(data->x11.connection, cookie)) {
		error = "Failed to create colormap: " + ToString(err->error_code);
		CLOSE_CONNECTION(data);
		return false;
	}

	u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
	u32 values[] = {data->x11.screen->black_pixel, data->x11.screen->black_pixel,
					XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
						XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
						XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE,
					data->x11.colormap,
					0};

	data->x11.window = xcb_generate_id(data->x11.connection);
	cookie = xcb_create_window_checked(data->x11.connection, data->x11.windowDepth, data->x11.window, data->x11.screen->root,
									   x, y, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, data->x11.visualID, mask, values);
	if (xcb_generic_error_t *err = xcb_request_check(data->x11.connection, cookie)) {
		error = "Error creating xcb window: " + ToString(err->error_code);
		CLOSE_CONNECTION(data);
		return false;
	}

	if (!xkbSetupKeyboardX11(&data->xkb, data->x11.connection)) {
		xcb_destroy_window(data->x11.connection, data->x11.window);
		CLOSE_CONNECTION(data);
		return false;
	}

	if (!xkbSelectEventsForDevice(&data->xkb)) {
		xcb_destroy_window(data->x11.connection, data->x11.window);
		CLOSE_CONNECTION(data);
		return false;
	}

	xcb_change_property(data->x11.connection, XCB_PROP_MODE_REPLACE, data->x11.window,
						XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, name.size, name.data);
	xcb_change_property(data->x11.connection, XCB_PROP_MODE_REPLACE, data->x11.window,
						XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8, name.size, name.data);

	if ((data->x11.atoms[0] = xcbGetAtom(data->x11.connection, true, "WM_PROTOCOLS")) == 0) {
		error = "Couldn't get WM_PROTOCOLS atom";
		xcb_destroy_window(data->x11.connection, data->x11.window);
		CLOSE_CONNECTION(data);
		return false;
	}
	if ((data->x11.atoms[1] = xcbGetAtom(data->x11.connection, false, "WM_DELETE_WINDOW")) == 0) {
		error = "Couldn't get WM_DELETE_WINDOW atom";
		xcb_destroy_window(data->x11.connection, data->x11.window);
		CLOSE_CONNECTION(data);
		return false;
	}
	if ((data->x11.atoms[2] = xcbGetAtom(data->x11.connection, false, "_NET_WM_STATE")) == 0) {
		error = "Couldn't get _NET_WM_STATE atom";
		xcb_destroy_window(data->x11.connection, data->x11.window);
		CLOSE_CONNECTION(data);
		return false;
	}
	if ((data->x11.atoms[3] = xcbGetAtom(data->x11.connection, false, "_NET_WM_STATE_FULLSCREEN")) == 0) {
		error = "Couldn't get _NET_WM_STATE_FULLSCREEN atom";
		xcb_destroy_window(data->x11.connection, data->x11.window);
		CLOSE_CONNECTION(data);
		return false;
	}

	xcb_change_property(data->x11.connection, XCB_PROP_MODE_REPLACE,
						data->x11.window, data->x11.atoms[0], 4, 32, 1, &data->x11.atoms[1]);

	xcb_pixmap_t pixmap_source, pixmap_mask;
	pixmap_source = xcb_generate_id(data->x11.connection);
	xcb_create_pixmap(data->x11.connection, 1, pixmap_source, data->x11.window, 1, 1);
	pixmap_mask = xcb_generate_id(data->x11.connection);
	xcb_create_pixmap(data->x11.connection, 1, pixmap_mask, data->x11.window, 1, 1);

	xcb_gcontext_t gc = xcb_generate_id(data->x11.connection);
	u8 black[1] = {0};
	xcb_create_gc(data->x11.connection, gc, pixmap_mask, 0, nullptr);
	xcb_put_image(data->x11.connection, XCB_IMAGE_FORMAT_XY_PIXMAP, pixmap_mask, gc, 1, 1, 0, 0, 0, 1, 1, black);

	xcb_free_gc(data->x11.connection, gc);

	data->x11.cursorHidden = xcb_generate_id(data->x11.connection);
	xcb_create_cursor(data->x11.connection, data->x11.cursorHidden, pixmap_source, pixmap_mask, 0, 0, 0, 0, 0, 0, 0, 0);

	xcb_free_pixmap(data->x11.connection, pixmap_source);
	xcb_free_pixmap(data->x11.connection, pixmap_mask);

	open = true;
	dpi = GetWindowDpiX11(window);
	return true;
}

bool Window::Open() {
	if (data->useWayland) {
		if (!windowOpenWayland(this)) return false;
	} else {
		if (!windowOpenX11(this)) return false;
	}
	data->frameCount = 0;
	return true;
}

bool Window::Show() {
	if (!open) {
		error = "Window hasn't been created yet";
		return false;
	}
	if (data->useWayland) {
		// TODO: Implement this
	} else {
		xcb_map_window(data->x11.connection, data->x11.window);
		xcb_flush(data->x11.connection);
	}
	return true;
}

bool Window::Close() {
	if (!open) {
		error = "Window hasn't been created yet";
		return false;
	}
	if (data->dpiThread.Joinable()) data->dpiThread.Join();
	if (data->useWayland) {
		wl_display_disconnect(data->wayland.display);
	} else {
		xkbCleanupX11(&data->xkb);
		xcb_free_cursor(data->x11.connection, data->x11.cursorHidden);
		xcb_destroy_window(data->x11.connection, data->x11.window);
		CLOSE_CONNECTION(data);
	}
	open = false;
	return true;
}

#define _NET_WM_STATE_REMOVE 0 // remove/unset property
#define _NET_WM_STATE_ADD 1    // add/set property
#define _NET_WM_STATE_TOGGLE 2 // toggle property

void windowFullscreenX11(Window *window) {
	WindowData *data = window->data;
	xcb_client_message_event_t ev;
	ev.response_type = XCB_CLIENT_MESSAGE;
	ev.type = data->x11.atoms[2];
	ev.format = 32;
	ev.window = data->x11.window;
	ev.data.data32[0] = _NET_WM_STATE_TOGGLE;
	ev.data.data32[1] = data->x11.atoms[3];
	ev.data.data32[2] = XCB_ATOM_NONE;
	ev.data.data32[3] = 0;
	ev.data.data32[4] = 0;

	xcb_send_event(data->x11.connection, 1, data->x11.window, XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY, (const char *)(&ev));
	xcb_flush(data->x11.connection);
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

void windowResizeX11(Window *window) {
	WindowData *data = window->data;
	const u32 values[2] = {window->width, window->height};
	xcb_configure_window(data->x11.connection, data->x11.window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
	xcb_flush(data->x11.connection);
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
	width = w;
	height = h;
	windowedWidth = w;
	windowedHeight = h;
	if (data->useWayland) {
		windowResizeWayland(this, w, h);
	} else {
		windowResizeX11(this);
	}
	return true;
}

void UpdateWindowDpi(Window *window) {
	if (window->data->useWayland) {
		
	} else {
		window->dpi = GetWindowDpiX11(window);
	}
}

bool windowUpdateX11(Window *window, bool &changeFullscreen) {
	WindowData *data = window->data;
	Input *input = window->input;
	u16 &width = window->width;
	u16 &height = window->height;
	bool &resized = window->resized;
	bool &focused = window->focused;
	while ((data->x11.event = xcb_poll_for_event(data->x11.connection))) {
		if (!xkbProcessEvent(&data->xkb, (xkb_generic_event_t *)data->x11.event)) {
			free(data->x11.event);
			return false;
		}
		u8 keyCode = 0;
		char character = '\0';
		bool press = false, release = false;
		switch (data->x11.event->response_type & ~0x80) {
		case XCB_CLIENT_MESSAGE: {
			if (((xcb_client_message_event_t *)data->x11.event)->data.data32[0] == data->x11.atoms[1]) {
				free(data->x11.event);
				return false; // Because this atom was bound to the close button
			}
		} break;
		case XCB_CONFIGURE_NOTIFY: {
			xcb_configure_notify_event_t *ev = (xcb_configure_notify_event_t *)data->x11.event;
			if (width != ev->width || height != ev->height) {
				width = ev->width;
				height = ev->height;
				screenSize = vec2((float)width, (float)height);
				resized = true;
			}
		} break;
		case XCB_KEY_PRESS: {
			xcb_key_press_event_t *ev = (xcb_key_press_event_t *)data->x11.event;
			keyCode = KeyCodeFromEvdev(ev->detail);
			// cout << "KeyCode down: " << KeyCodeName(keyCode) << std::endl;
			// cout << "XCB_KEY_PRESS scancode: " << std::hex << (u32)ev->detail << " evdev: " << std::dec << ev->detail-8 << std::endl;
			char buffer[4] = {0};
			xkb_state_key_get_utf8(data->xkb.state, (xkb_keycode_t)ev->detail, buffer, 4);
			if (buffer[1] == '\0') {
				if (!(buffer[0] & 0x80)) {
					character = buffer[0];
				}
				// handleCharInput(buffer[0]);
			}
			if (keyCode == KC_KEY_F11)
				changeFullscreen = true;
			press = true;
		} break;
		case XCB_KEY_RELEASE: {
			xcb_key_release_event_t *ev = (xcb_key_release_event_t *)data->x11.event;
			keyCode = KeyCodeFromEvdev(ev->detail);
			char buffer[4] = {0};
			xkb_state_key_get_utf8(data->xkb.state, (xkb_keycode_t)ev->detail, buffer, 4);
			if (buffer[1] == '\0') {
				if (!(buffer[0] & 0x80)) {
					character = buffer[0];
				}
			}
			release = true;
		} break;
		case XCB_BUTTON_PRESS: {
			xcb_button_press_event_t *ev = (xcb_button_press_event_t *)data->x11.event;
			switch (ev->detail) {
			case 1:
				keyCode = KC_MOUSE_LEFT;
				break;
			case 2:
				keyCode = KC_MOUSE_MIDDLE;
				break;
			case 3:
				keyCode = KC_MOUSE_RIGHT;
				break;
			case 4:
				keyCode = KC_MOUSE_SCROLLUP;
				if (input != nullptr) {
					input->scroll.y += 1.0f;
				}
				break;
			case 5:
				keyCode = KC_MOUSE_SCROLLDOWN;
				if (input != nullptr) {
					input->scroll.y -= 1.0f;
				}
				break;
			case 6: // Sideways scrolling
				keyCode = KC_MOUSE_SCROLLLEFT;
				if (input != nullptr) {
					input->scroll.x -= 1.0f;
				}
				break;
			case 7:
				keyCode = KC_MOUSE_SCROLLRIGHT;
				if (input != nullptr) {
					input->scroll.x += 1.0f;
				}
				break;
			case 8:
				keyCode = KC_MOUSE_XONE;
				break;
			case 9:
				keyCode = KC_MOUSE_XTWO;
				break;
			default:
				keyCode = 0;
				break;
			}
			press = true;
		} break;
		case XCB_BUTTON_RELEASE: {
			xcb_button_release_event_t *ev = (xcb_button_release_event_t *)data->x11.event;
			switch (ev->detail) {
			case 1:
				keyCode = KC_MOUSE_LEFT;
				break;
			case 2:
				keyCode = KC_MOUSE_MIDDLE;
				break;
			case 3:
				keyCode = KC_MOUSE_RIGHT;
				break;
			case 4:
				keyCode = KC_MOUSE_SCROLLUP;
				break;
			case 5:
				keyCode = KC_MOUSE_SCROLLDOWN;
				break;
			case 6: // Sideways scrolling
				keyCode = KC_MOUSE_SCROLLLEFT;
				break;
			case 7:
				keyCode = KC_MOUSE_SCROLLRIGHT;
				break;
			case 8:
				keyCode = KC_MOUSE_XONE;
				break;
			case 9:
				keyCode = KC_MOUSE_XTWO;
				break;
			default:
				keyCode = 0;
				break;
			}
			release = true;
		} break;
		case XCB_FOCUS_IN: {
			focused = true;
		} break;
		case XCB_FOCUS_OUT: {
			focused = false;
			if (input != nullptr) {
				input->ReleaseAll();
			}
		} break;
		case XCB_MOTION_NOTIFY: {
			xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)data->x11.event;
			if (input != nullptr) {
				input->cursor.x = ev->event_x;
				input->cursor.y = ev->event_y;
			}
		} break;
		case XCB_EXPOSE: {
			// Repaint?
		} break;
		default: {
		} break;
		}
		free(data->x11.event);

		if (input != nullptr && focused) {
			if (press)
				input->typingString += character;
			if (character >= 'a' && character <= 'z') {
				character += 'A' - 'a';
			}
			if (press) {
				if (keyCode != 0) {
					input->Press(keyCode);
				}
				if (character != '\0') {
					input->PressChar(character);
				}
			}
			if (release) {
				if (keyCode != 0) {
					input->Release(keyCode);
				}
				if (character != '\0') {
					input->ReleaseChar(character);
				}
			}
		}
	}
	return true;
}

bool Window::Update() {
	bool changeFullscreen = false;
	resized = false;
	data->frameCount++;
	if (data->frameCount >= 15) {
		if (!data->dpiThread.Joinable()) {
			// Only try again if the last thread finished already
			data->dpiThread = Thread(UpdateWindowDpi, this);
			data->frameCount = 0;
		}
	}
	if (data->useWayland) {
		if (!windowUpdateWayland(this, changeFullscreen)) return false;
	} else {
		if (!windowUpdateX11(this, changeFullscreen)) return false;
	}

	if (changeFullscreen) {
		Fullscreen(!fullscreen);
	}

	return true;
}

void Window::HideCursor(bool hide) {
	cursorHidden = hide;
	if (data->useWayland) {
		// TODO: Implement this
	} else {
		if (hide) {
			u32 value = data->x11.cursorHidden;
			xcb_change_window_attributes(data->x11.connection, data->x11.window, XCB_CW_CURSOR, &value);
		} else {
			u32 value = XCB_CURSOR_NONE;
			xcb_change_window_attributes(data->x11.connection, data->x11.window, XCB_CW_CURSOR, &value);
		}
		xcb_flush(data->x11.connection);
	}
}

String Window::InputName(u8 keyCode) const {
	if (!open) {
		return "Error";
	}
	return xkbGetInputName(&data->xkb, keyCode);
}

i32 GetWindowDpiX11(Window *window) {
	// ClockTime start = Clock::now();
	char *res = xcbGetProperty(window->data->x11.connection, window->data->x11.screen->root, XCB_ATOM_RESOURCE_MANAGER, XCB_ATOM_STRING, 16*1024);
	Array<char> resources;
	resources.data = res;
	resources.size = StringLength(res);
	resources.allocated = resources.size;
	if (0 == resources.size) {
		error = "Couldn't get X Resource Manager property";
		return -1;
	}
	// ClockTime endGetProperty = Clock::now();
	// u32 widthPx, heightPx, widthMM, heightMM;
	// widthPx  = data->x11.screen->width_in_pixels;
	// widthMM  = data->x11.screen->width_in_millimeters;
	// heightPx = data->x11.screen->height_in_pixels;
	// heightMM = data->x11.screen->height_in_millimeters;
	// cout.PrintLn("Screen info:"
	// 	"\nwidthPx: ", widthPx,
	// 	"\nheightPx: ", heightPx,
	// 	"\nwidthMM: ", widthMM,
	// 	"\nheightMM: ", heightMM,
	// 	"\nhorizontal dpi: ", i32((f32)widthPx * 25.4f / (f32)widthMM),
	// 	"\nvertical dpi: ", i32((f32)heightPx * 25.4f / (f32)heightMM));
	// cout.PrintLn("Resource Manager: \n\n", resources, "\n");

	// This could be sped up, but the vast majority of our time is spent above.
	i32 dpi = 0;
	Array<Range<char>> ranges = SeparateByValues(resources, {'\n', ' ', ':', '\t'});
	for (i32 i = 0; i < ranges.size-1; i++) {
		if (ranges[i] == "Xft.dpi") {
			if (!StringToI32(ranges[i+1], &dpi)) {
				error = Stringify("Failed to parse DPI from Range '", ranges[i+1], "'");
				return 0;
			}
			break;
		}
	}
	// cout.PrintLn("DPI took ", FormatTime(Clock::now() - start), " total, where ", FormatTime(endGetProperty - start), " was xcbGetProperty.");
	return dpi > 0 ? dpi : 0;
}

i32 Window::GetDPI() {
	return dpi > 0 ? dpi : 96;
}

} // namespace io

} // namespace AzCore
