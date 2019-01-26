/*
    File: io_linux.cpp
    Author: Philip Haynes
*/

#include "io.hpp"

// To use GLX, you need Xlib, but for Vulkan you can just use xcb
#define IO_NO_XLIB

#include <xcb/xcb.h>
#ifndef IO_NO_XLIB
    #include <X11/Xlib.h>
    #include <X11/Xlib-xcb.h>
#endif
#include <xcb/xcb_errors.h>
#include <xcb/xproto.h>
#include <linux/input.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>

#define explicit extern_explicit // Preventing C++ keyword bug
#include <xcb/xkb.h>
#undef explicit
#include <fcntl.h> // For evdev
#include <libevdev-1.0/libevdev/libevdev.h>
#include <unistd.h>

namespace io {

    xcb_atom_t xcbGetAtom(xcb_connection_t* connection, bool onlyIfExists, const std::string& name) {
        xcb_intern_atom_cookie_t cookie;
        xcb_intern_atom_reply_t *reply;

        // In order to access our close button event
        cookie = xcb_intern_atom(connection, (u8)onlyIfExists, name.length(), name.c_str());
        reply = xcb_intern_atom_reply(connection, cookie, 0);

        if (reply == nullptr) {
            error = "Failed to get reply to a cookie for retrieving an XCB atom!";
            return 0;
        }
        xcb_atom_t atom = reply->atom;
        free(reply);

        return atom;
    }

    struct xkb_keyboard {
        xcb_connection_t *connection;
        u8 first_xkb_event;
        struct xkb_context *context{nullptr};
        struct xkb_keymap *keymap{nullptr};
        i32 deviceId;
        struct xkb_state *state{nullptr};
        struct xkb_state *stateNone{nullptr};
    };

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

    void xkbCleanup(xkb_keyboard *xkb) {
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

    bool xkbUpdateKeymap(xkb_keyboard *xkb) {
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

    bool xkbSetupKeyboard(xkb_keyboard *xkb, xcb_connection_t *connection) {
        xkb->connection = connection;
        if (!xkb_x11_setup_xkb_extension(xkb->connection,
                                         XKB_X11_MIN_MAJOR_XKB_VERSION,
                                         XKB_X11_MIN_MINOR_XKB_VERSION,
                                         XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
                                         NULL, NULL, &xkb->first_xkb_event, NULL)) {
            error  = "Failed to connect xkb to x11!";
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

        return xkbUpdateKeymap(xkb);
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

        switch(event->xkb_type) {
            case XCB_XKB_NEW_KEYBOARD_NOTIFY: {
                // cout << "XCB_XKB_NEW_KEYBOARD_NOTIFY" << std::endl;
                xcb_xkb_new_keyboard_notify_event_t *ev = (xcb_xkb_new_keyboard_notify_event_t*)event;
                if (ev->changed) {
                    if (!xkbUpdateKeymap(xkb))
                        return false;
                }
                break;
            }
            case XCB_XKB_MAP_NOTIFY: {
                // cout << "XCB_XKB_MAP_NOTIFY" << std::endl;
                if (!xkbUpdateKeymap(xkb))
                    return false;
                break;
            }
            case XCB_XKB_STATE_NOTIFY: {
                // cout << "XCB_XKB_STATE_NOTIFY" << std::endl;
                xcb_xkb_state_notify_event_t *ev = (xcb_xkb_state_notify_event_t*)event;
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

    struct WindowData {
#ifndef IO_NO_XLIB
        Display *display;
#endif
        xcb_connection_t* connection;
        xcb_errors_context_t *xcb_errors;
        xcb_colormap_t colormap;
        i32 visualID;
        xcb_window_t window;
        xcb_screen_t* screen;
        xcb_generic_event_t* event;
        xcb_atom_t atoms[4];
        i32 windowDepth;
        xkb_keyboard xkb;
    };

    Window::Window() {
        data = new WindowData;
        data->windowDepth = 24;
    }

    Window::~Window() {
        if (open) {
            Close();
        }
        delete data;
    }

#ifndef IO_NO_XLIB
    #define CLOSE_CONNECTION(data) XCloseDisplay(data->display)
#else
    #define CLOSE_CONNECTION(data) xcb_disconnect(data->connection)
#endif

    bool Window::Open() {
        i32 defaultScreen=0;
#ifndef IO_NO_XLIB
        data->display = XOpenDisplay(0);
        if (!data->display) {
            error = "Can't open X display";
            return false;
        }

        defaultScreen = DefaultScreen(data->display);

        data->connection = XGetXCBConnection(data->display);
        if (!data->connection) {
            XCloseDisplay(data->display);
            error = "Can't get xcb connection from display";
            return false;
        }

        XSetEventQueueOwner(data->display, XCBOwnsEventQueue);
#else
        data->connection = xcb_connect(NULL, NULL);

        if (xcb_connection_has_error(data->connection) > 0) {
            error = "Cannot open display";
            return false;
        }
#endif
        if (xcb_errors_context_new(data->connection, &data->xcb_errors) == -1) {
            CLOSE_CONNECTION(data);
            error = "Can't get xcb_errors context";
            return false;
        }

        /* Find XCB screen */
        data->screen = 0;
        xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(xcb_get_setup(data->connection));
        for(i32 screen_num = defaultScreen;
            screen_iter.rem && screen_num > 0;
            --screen_num, xcb_screen_next(&screen_iter));
        data->screen = screen_iter.data;

        xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(data->screen);
        xcb_depth_t *depth = nullptr;

        while (depth_iter.rem) {
            if (depth_iter.data->depth == data->windowDepth && depth_iter.data->visuals_len) {
                depth = depth_iter.data;
                break;
            }
            xcb_depth_next(&depth_iter);
        }

        if (!depth) {
            xcb_errors_context_free(data->xcb_errors);
            CLOSE_CONNECTION(data);
            error = "Screen doesn't support ";
            error += std::to_string(data->windowDepth);
            error += "-bit depth!";
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
            xcb_errors_context_free(data->xcb_errors);
            CLOSE_CONNECTION(data);
            error = "Screen doesn't support True Color";
            return false;
        }
        data->visualID = visual->visual_id;

        xcb_void_cookie_t cookie;

        data->colormap = xcb_generate_id(data->connection);
        cookie = xcb_create_colormap_checked(data->connection, XCB_COLORMAP_ALLOC_NONE, data->colormap, data->screen->root, data->visualID);


        if (xcb_generic_error_t *err = xcb_request_check(data->connection, cookie)) {
            const char *extension;
            error = "Failed to create colormap: ";
            error += xcb_errors_get_name_for_error(data->xcb_errors, err->error_code, &extension);
            xcb_errors_context_free(data->xcb_errors);
            CLOSE_CONNECTION(data);
            return false;
        }

        u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
        u32 values[] = {data->screen->black_pixel, data->screen->black_pixel,
            XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE,
            data->colormap,
            0
        };

        data->window = xcb_generate_id(data->connection);
        cookie = xcb_create_window_checked(data->connection, data->windowDepth, data->window, data->screen->root, x, y, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, data->visualID, mask, values);
        if (xcb_generic_error_t *err = xcb_request_check(data->connection, cookie)) {
            const char *extension;
            error = "Error creating xcb window: ";
            error += xcb_errors_get_name_for_error(data->xcb_errors, err->error_code, &extension);
            xcb_errors_context_free(data->xcb_errors);
            CLOSE_CONNECTION(data);
            return false;
        }

        if (!xkbSetupKeyboard(&data->xkb, data->connection)) {
            xcb_destroy_window(data->connection, data->window);
            CLOSE_CONNECTION(data);
            return false;
        }

        if (!xkbSelectEventsForDevice(&data->xkb)) {
            xcb_destroy_window(data->connection, data->window);
            CLOSE_CONNECTION(data);
            return false;
        }

        xcb_change_property(data->connection, XCB_PROP_MODE_REPLACE, data->window,
                XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, name.length(), name.c_str());
        xcb_change_property (data->connection, XCB_PROP_MODE_REPLACE, data->window,
                XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8, name.length(), name.c_str());

        if ((data->atoms[0] = xcbGetAtom(data->connection, true, "WM_PROTOCOLS")) == 0) {
            error = "Couldn't get WM_PROTOCOLS atom";
            xcb_destroy_window(data->connection, data->window);
            CLOSE_CONNECTION(data);
            return false;
        }
        if ((data->atoms[1] = xcbGetAtom(data->connection, false, "WM_DELETE_WINDOW")) == 0) {
            error = "Couldn't get WM_DELETE_WINDOW atom";
            xcb_destroy_window(data->connection, data->window);
            CLOSE_CONNECTION(data);
            return false;
        }
        if ((data->atoms[2] = xcbGetAtom(data->connection, false, "_NET_WM_STATE")) == 0) {
            error = "Couldn't get _NET_WM_STATE atom";
            xcb_destroy_window(data->connection, data->window);
            CLOSE_CONNECTION(data);
            return false;
        }
        if ((data->atoms[3] = xcbGetAtom(data->connection, false, "_NET_WM_STATE_FULLSCREEN")) == 0) {
            error = "Couldn't get _NET_WM_STATE_FULLSCREEN atom";
            xcb_destroy_window(data->connection, data->window);
            CLOSE_CONNECTION(data);
            return false;
        }

        xcb_change_property(data->connection, XCB_PROP_MODE_REPLACE, data->window, data->atoms[0], 4, 32, 1, &data->atoms[1]);

        open = true;
        return true;
    }

    bool Window::Show() {
        if (!open) {
            error = "Window hasn't been created yet";
            return false;
        }
        xcb_map_window(data->connection, data->window);
        xcb_flush(data->connection);
        return true;
    }

    bool Window::Close() {
        if (!open) {
            error = "Window hasn't been created yet";
            return false;
        }
        xkbCleanup(&data->xkb);
        xcb_destroy_window(data->connection, data->window);
        CLOSE_CONNECTION(data);
        xcb_errors_context_free(data->xcb_errors);
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

        #define _NET_WM_STATE_REMOVE        0    // remove/unset property
        #define _NET_WM_STATE_ADD           1    // add/set property
        #define _NET_WM_STATE_TOGGLE        2    // toggle property

        xcb_client_message_event_t ev;
        ev.response_type = XCB_CLIENT_MESSAGE;
        ev.type = data->atoms[2];
        ev.format = 32;
        ev.window = data->window;
        ev.data.data32[0] = _NET_WM_STATE_TOGGLE;
        ev.data.data32[1] = data->atoms[3];
        ev.data.data32[2] = XCB_ATOM_NONE;
        ev.data.data32[3] = 0;
        ev.data.data32[4] = 0;

        xcb_send_event(data->connection, 1, data->window, XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
                | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY, (const char*)(&ev));
        xcb_flush(data->connection);
        return true;
    }

    bool Window::Update() {
        bool changeFullscreen = false;
        resized = false;
        while ((data->event = xcb_poll_for_event(data->connection))) {
            if (!xkbProcessEvent(&data->xkb, (xkb_generic_event_t*)data->event)) {
                return false;
            }
            u8 keyCode = 0;
            bool press=false, release=false;
            switch (data->event->response_type & ~0x80) {
                case XCB_CLIENT_MESSAGE: {
                    if (((xcb_client_message_event_t*)data->event)->data.data32[0] == data->atoms[1])
                        return false; // Because this atom was bound to the close button
                    break;
                }
                case XCB_CONFIGURE_NOTIFY: {
                    xcb_configure_notify_event_t* ev = (xcb_configure_notify_event_t*)data->event;
                    if (width != ev->width || height != ev->height) {
                        width = ev->width;
                        height = ev->height;
                        screenSize = vec2((float)width, (float)height);
                        resized = true;
                    }
                    break;
                }
                case XCB_KEY_PRESS: {
                    xcb_key_press_event_t* ev = (xcb_key_press_event_t*)data->event;
                    // cout << "XCB_KEY_PRESS scancode: " << std::hex << (u32)ev->detail << " evdev: " << std::dec << ev->detail-8 << std::endl;
                    keyCode = KeyCodeFromEvdev(ev->detail);
                    char buffer[4] = {0};
                    xkb_state_key_get_utf8(data->xkb.state, (xkb_keycode_t)ev->detail, buffer, 4);
                    // if (buffer[1] == '\0')
                    //     handleCharInput(buffer[0]);
                    if (keyCode == KC_KEY_F11)
                        changeFullscreen = true;
                    press = true;
                    break;
                }
                case XCB_KEY_RELEASE: {
                    xcb_key_release_event_t* ev = (xcb_key_release_event_t*)data->event;
                    keyCode = KeyCodeFromEvdev(ev->detail);
                    release = true;
                    break;
                }
                case XCB_BUTTON_PRESS: {
                    xcb_button_press_event_t* ev = (xcb_button_press_event_t*)data->event;
                    switch(ev->detail) {
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
                                input->scroll.y += 1.0;
                            }
                            break;
                        case 5:
                            keyCode = KC_MOUSE_SCROLLDOWN;
                            if (input != nullptr) {
                                input->scroll.y -= 1.0;
                            }
                            break;
                        case 6: // Sideways scrolling
                            keyCode = KC_MOUSE_SCROLLLEFT;
                            if (input != nullptr) {
                                input->scroll.x -= 1.0;
                            }
                            break;
                        case 7:
                            keyCode = KC_MOUSE_SCROLLRIGHT;
                            if (input != nullptr) {
                                input->scroll.x += 1.0;
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
                    break;
                }
                case XCB_BUTTON_RELEASE: {
                    xcb_button_release_event_t* ev = (xcb_button_release_event_t*)data->event;
                    switch(ev->detail) {
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
                    break;
                }
                case XCB_FOCUS_IN: {
                    focused = true;
                    break;
                }
                case XCB_FOCUS_OUT: {
                    focused = false;
                    if (input != nullptr) {
                        input->ReleaseAll();
                    }
                    break;
                }
                case XCB_MOTION_NOTIFY: {
                    xcb_motion_notify_event_t* ev = (xcb_motion_notify_event_t*)data->event;
                    if (input != nullptr) {
                        input->cursor.x = ev->event_x;
                        input->cursor.y = ev->event_y;
                    }
                    break;
                }
                case XCB_EXPOSE: {
                    // Repaint?
                    break;
                }
                default: {
                    break;
                }
            }
            free(data->event);

            if (input != nullptr && focused) {
                if (press)
                    input->Press(keyCode);
                if (release)
                    input->Release(keyCode);
            }
        }

        if (changeFullscreen) {
            Fullscreen(!fullscreen);
        }

        return true;
    }

    String Window::InputName(u8 keyCode) const {
        if (!open) {
            return "Error";
        }
        return xkbGetInputName(&data->xkb, keyCode);
    }

}
