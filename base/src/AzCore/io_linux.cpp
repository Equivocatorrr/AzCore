/*
    File: io_linux.cpp
    Author: Philip Haynes
*/

#include "io.hpp"
#ifdef IO_FOR_VULKAN
    #include "vk.hpp"
#endif

// To use GLX, you need Xlib, but for Vulkan you can just use xcb
#ifdef IO_FOR_VULKAN
    #define IO_NO_XLIB
#endif

#include <xcb/xcb.h>
#ifndef IO_NO_XLIB
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
#include <fcntl.h> // For evdev
#include <libevdev-1.0/libevdev/libevdev.h>
#include <unistd.h>

namespace io {

    struct RawInputDeviceData {
        libevdev *dev;
        i32 fd;
        String path;
        Array<input_event> syncBuffer;
    };

    RawInputDevice::~RawInputDevice() {
        if (data != nullptr) {
            if (data->dev != nullptr) {
                libevdev_free(data->dev);
            }
            if (data->fd > -1) {
                close(data->fd);
            }
            delete data;
        }
    }

    RawInputDevice& RawInputDevice::operator=(RawInputDevice&& other) {
        if (data != nullptr) {
            delete data;
        }
        data = other.data;
        other.data = nullptr;
        type = other.type;
        rawInput = other.rawInput;
        return *this;
    }

    bool RawInputDeviceInit(RawInputDevice *rid, i32 fd, String&& path, RawInputFeatureBits enableMask) {
        libevdev *dev = libevdev_new();
        if (dev == nullptr) {
            return false;
        }
        i32 rc = libevdev_set_fd(dev, fd);
        if (rc < 0) {
            libevdev_free(dev);
            return false;
        }
        if (libevdev_has_event_type(dev, EV_REL) &&
            libevdev_has_event_code(dev, EV_REL, REL_X) &&
            libevdev_has_event_code(dev, EV_REL, REL_Y) &&
            libevdev_has_event_code(dev, EV_KEY, BTN_MOUSE) &&
            libevdev_has_event_code(dev, EV_KEY, BTN_LEFT)) {
            if (!(enableMask & RAW_INPUT_ENABLE_MOUSE_BIT)) {
                libevdev_free(dev);
                return false;
            }
            rid->type = MOUSE;
        } else if (libevdev_has_event_code(dev, EV_KEY, BTN_JOYSTICK)) {
            if (!(enableMask & RAW_INPUT_ENABLE_JOYSTICK_BIT)) {
                libevdev_free(dev);
                return false;
            }
            rid->type = JOYSTICK;
        } else if (libevdev_has_event_code(dev, EV_KEY, BTN_GAMEPAD)) {
            if (!(enableMask & RAW_INPUT_ENABLE_GAMEPAD_BIT)) {
                libevdev_free(dev);
                return false;
            }
            rid->type = GAMEPAD;
        } else if (libevdev_has_event_code(dev, EV_KEY, KEY_KEYBOARD)) {
            if (!(enableMask & RAW_INPUT_ENABLE_KEYBOARD_BIT)) {
                libevdev_free(dev);
                return false;
            }
            rid->type = KEYBOARD;
        } else {
            rid->type = UNSUPPORTED;
            libevdev_free(dev);
            return false;
        }
        if (rid->data != nullptr) {
            delete rid->data;
        }
        rid->data = new RawInputDeviceData;
        rid->data->fd = fd;
        rid->data->path = std::move(path);
        rid->data->dev = dev;
        cout << "RawInputDevice from path \"" << rid->data->path << "\":\n"
        "\tType: " << RawInputDeviceTypeString[rid->type] << "\n"
        "\tName: " << libevdev_get_name(dev) << "\n"
        "\tID: bus " << libevdev_get_id_bustype(dev)
        << " vendor " << libevdev_get_id_vendor(dev)
        << " product " << libevdev_get_id_product(dev) << std::endl;
        return true;
    }

    bool GetRawInputDeviceEvent(Ptr<RawInputDevice> rid, input_event *dst) {
        input_event ev;
        i32 rc;
        if (rid->data->syncBuffer.size > 0) {
            ev = rid->data->syncBuffer[0];
            rid->data->syncBuffer.Erase(0);
            *dst = ev;
            return true;
        }
        rc = libevdev_next_event(rid->data->dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc < 0) {
            if (rc == -EAGAIN) {
                return false;
            } else {
                error = "GetRawInputDeviceEvent error: libevdev_next_event returned errno "
                      + ToString(-rc) + " (" + strerror(-rc) + ")";
                return false;
            }
        } else if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            while (rc == LIBEVDEV_READ_STATUS_SYNC) {
                rc = libevdev_next_event(rid->data->dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
                if (rc < 0) {
                    if (rc != -EAGAIN) {
                        error = "GetRawInputDeviceEvent error: Sync error: "
                                "libevdev_next_event return errno " + ToString(-rc)
                              + " (" + strerror(-rc) + ")";
                        return false;
                    }
                    break;
                }
                if (!(ev.type == EV_SYN && ev.type == SYN_REPORT)) {
                    rid->data->syncBuffer.Append(ev);
                }
            }
            if (rid->data->syncBuffer.size > 0) {
                ev = rid->data->syncBuffer[0];
                rid->data->syncBuffer.Erase(0);
                *dst = ev;
                return true;
            } else {
                cout << "GetRawInputDeviceEvent warning: Sync was empty!" << std::endl;
                return false;
            }
        } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            *dst = ev;
            return true;
        }
        return false;
    }

    struct RawInputData {
        u32 frame;
    };

    RawInput::~RawInput() {
        if (data != nullptr) {
            delete data;
        }
    }

    bool RawInput::Init(RawInputFeatureBits enableMask) {
        devices.Reserve(4);
        data = new RawInputData;
        data->frame = 0;
        ClockTime start = Clock::now();
        char path[] = "/dev/input/eventXX";
        for (u32 i = 0; i < 64; i++) {
            if (i < 10) {
                path[16] = i+'0';
                path[17] = 0;
            } else {
                path[16] = i/10+'0';
                path[17] = i%10+'0';
            }
            // cout << path << std::endl;
            i32 fd = open(path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) {
                if (errno == EACCES) {
                    cout << "Permission denied opening device with path \"" << path << "\". Giving up." << std::endl;
                    return true;
                }
                continue;
            }
            // We got a live one boys!
            RawInputDevice device;
            if (RawInputDeviceInit(&device, fd, std::move(path), enableMask)) {
                device.rawInput = this;
                devices.Append(std::move(device));
                switch (device.type) {
                case KEYBOARD:
                    // TODO: Implement raw keyboards
                    break;
                case MOUSE:
                    // TODO: Implement raw mice
                    break;
                case GAMEPAD: {
                    Gamepad gamepad;
                    gamepad.rawInputDevice = devices.GetPtr(devices.size-1);
                    gamepads.Append(gamepad);
                    break;
                }
                case JOYSTICK:
                    // TODO: Implement raw joysticks
                    break;
                case UNSUPPORTED:
                    break;
                }
            }
        }
        cout << "Total time to check 64 raw input devices: "
        << (f64)std::chrono::duration_cast<Nanoseconds>(Clock::now()-start).count() / 1000000000.0d
        << " seconds" << std::endl;
        return true;
    }

    void RawInput::Update(f32 timestep) {
        // TODO: The rest of the raw input device types.
        AnyGP.Tick(timestep);
        if (window != nullptr) {
            if (!window->focused)
                return;
        }
        for (i32 i = 0; i < gamepads.size; i++) {
            gamepads[i].Update(timestep, i);
        }
    }

    void handleButton(ButtonState &dst, bool down, u8 keyCode, RawInput *rawInput, i32 index) {
        if (down && !dst.Down()) {
            rawInput->AnyGPCode = keyCode;
            rawInput->AnyGP.state = BUTTON_PRESSED_BIT;
            dst.Press();
            rawInput->AnyGPIndex = index;
        }
        if (!down && dst.Down()) {
            rawInput->AnyGPCode = keyCode;
            rawInput->AnyGP.state = BUTTON_RELEASED_BIT;
            dst.Release();
            rawInput->AnyGPIndex = index;
        }
    }

    void Gamepad::Update(f32 timestep, i32 index) {
        if (!rawInputDevice.Valid()) {
            return;
        }
        for (u32 i = 0; i < IO_GAMEPAD_MAX_BUTTONS; i++) {
            button[i].Tick(timestep);
        }
        for (u32 i = 0; i < IO_GAMEPAD_MAX_AXES*2; i++) {
            axisPush[i].Tick(timestep);
        }
        for (u32 i = 0; i < 4; i++) {
            hat[i].Tick(timestep);
        }
        input_event ev;
        while (GetRawInputDeviceEvent(rawInputDevice, &ev)) {
            if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
                continue;
            }
            if (ev.type == EV_KEY) {
                // Button presses
                i32 bIndex = ev.code - 0x130;
                if (bIndex >= IO_GAMEPAD_MAX_BUTTONS || bIndex < 0) {
                    continue; // Unsupported
                }
                rawInputDevice->rawInput->AnyGPCode = bIndex + KC_GP_BTN_A;
                if (ev.value) {
                    rawInputDevice->rawInput->AnyGP.state = BUTTON_PRESSED_BIT;
                    button[bIndex].Press();
                } else {
                    rawInputDevice->rawInput->AnyGP.state = BUTTON_RELEASED_BIT;
                    button[bIndex].Release();
                }
                rawInputDevice->rawInput->AnyGPIndex = index;
            } else if(ev.type == EV_ABS) {
                // Axis movements
                f32 maxRange = 32767.0;
                f32 minRange = -32768.0;
                f32 deadZoneTemp = maxRange * deadZone;
                i32 aIndex = ev.code;
                if (aIndex > 5) {
                    // A hat perhaps?
                    aIndex -= 10;
                    maxRange = 1.0;
                    minRange = -1.0;
                    deadZoneTemp = 0.0;
                }
                if (aIndex == GP_AXIS_LT || aIndex == GP_AXIS_RT) {
                    maxRange = 255.0;
                    minRange = -255.0;
                    deadZoneTemp = 0.0;
                }
                if (aIndex >= IO_GAMEPAD_MAX_AXES || aIndex < 0)
                    continue; // Unsupported
                f32 val = (f32)ev.value;
                if (abs(val) < deadZoneTemp) {
                    axis.array[aIndex] = 0.0;
                } else {
                    if (val >= 0.0) {
                        axis.array[aIndex] = (val-deadZoneTemp) / (maxRange-deadZoneTemp);
                    } else {
                        axis.array[aIndex] = (val+deadZoneTemp) / (-minRange-deadZoneTemp);
                    }
                    if (abs(axis.array[aIndex]) > 0.1) {
                        rawInputDevice->rawInput->AnyGPCode = aIndex + KC_GP_AXIS_LS_X;
                        rawInputDevice->rawInput->AnyGP.state = BUTTON_PRESSED_BIT;
                        rawInputDevice->rawInput->AnyGPIndex = index;
                    }
                }
                if (axisCurve != 1.0) {
                    bool negative = axis.array[aIndex] < 0.0;
                    axis.array[aIndex] = pow(abs(axis.array[aIndex]), axisCurve);
                    if (negative) {
                        axis.array[aIndex] *= -1.0;
                    }
                }
                handleButton(axisPush[aIndex*2], axis.array[aIndex] > 0.5,    aIndex*2 + KC_GP_AXIS_LS_RIGHT,
                             rawInputDevice->rawInput, index);
                handleButton(axisPush[aIndex*2+1], axis.array[aIndex] < -0.5, aIndex*2 + KC_GP_AXIS_LS_LEFT,
                             rawInputDevice->rawInput, index);
            }
        }
        if (axis.vec.H0.x != 0.0 && axis.vec.H0.y != 0.0) {
            axis.vec.H0 = normalize(axis.vec.H0);
            // cout << "H0.x = " << axis.vec.H0.x << ", H0.y = " << axis.vec.H0.y << std::endl;
        }
        handleButton(hat[0], axis.vec.H0.x > 0.0 && axis.vec.H0.y < 0.0, KC_GP_AXIS_H0_UP_RIGHT,
                     rawInputDevice->rawInput, index);
        handleButton(hat[1], axis.vec.H0.x > 0.0 && axis.vec.H0.y > 0.0, KC_GP_AXIS_H0_DOWN_RIGHT,
                     rawInputDevice->rawInput, index);
        handleButton(hat[2], axis.vec.H0.x < 0.0 && axis.vec.H0.y > 0.0, KC_GP_AXIS_H0_DOWN_LEFT,
                     rawInputDevice->rawInput, index);
        handleButton(hat[3], axis.vec.H0.x < 0.0 && axis.vec.H0.y < 0.0, KC_GP_AXIS_H0_UP_LEFT,
                     rawInputDevice->rawInput, index);
        // for (u32 i = 0; i < IO_GAMEPAD_MAX_AXES; i++) {
        //     if (axisPush[i*2].Pressed()) {
        //         cout << "Pressed " << KeyCodeName(i*2 + KC_GP_AXIS_LS_RIGHT) << std::endl;
        //     }
        //     if (axisPush[i*2+1].Pressed()) {
        //         cout << "Pressed " << KeyCodeName(i*2+1 + KC_GP_AXIS_LS_RIGHT) << std::endl;
        //     }
        //     if (axisPush[i*2].Released()) {
        //         cout << "Released " << KeyCodeName(i*2 + KC_GP_AXIS_LS_RIGHT) << std::endl;
        //     }
        //     if (axisPush[i*2+1].Released()) {
        //         cout << "Released " << KeyCodeName(i*2+1 + KC_GP_AXIS_LS_RIGHT) << std::endl;
        //     }
        // }
        // for (u32 i = 0; i < 4; i++) {
        //     if (hat[i].Pressed()) {
        //         cout << "Pressed " << KeyCodeName(i + KC_GP_AXIS_H0_UP_RIGHT) << std::endl;
        //     }
        //     if (hat[i].Released()) {
        //         cout << "Released " << KeyCodeName(i + KC_GP_AXIS_H0_UP_RIGHT) << std::endl;
        //     }
        // }
    }

    xcb_atom_t xcbGetAtom(xcb_connection_t* connection, bool onlyIfExists, const String& name) {
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

#ifdef IO_FOR_VULKAN
    bool Window::CreateVkSurface(vk::Instance *instance, VkSurfaceKHR *surface) {
        if (!open) {
            error = "CreateVkSurface was called before the window was created!";
            return false;
        }
        VkXcbSurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
        createInfo.connection = data->connection;
        createInfo.window = data->window;
        VkResult result = vkCreateXcbSurfaceKHR(instance->data.instance, &createInfo, nullptr, surface);
        if (result != VK_SUCCESS) {
            error = "Failed to create XCB surface!";
            return false;
        }
        return true;
    }
#endif

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
            CLOSE_CONNECTION(data);
            error = "Screen doesn't support " + ToString(data->windowDepth) + "-bit depth!";
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
        data->visualID = visual->visual_id;

        xcb_void_cookie_t cookie;

        data->colormap = xcb_generate_id(data->connection);
        cookie = xcb_create_colormap_checked(data->connection, XCB_COLORMAP_ALLOC_NONE,
                    data->colormap, data->screen->root, data->visualID);


        if (xcb_generic_error_t *err = xcb_request_check(data->connection, cookie)) {
            error = "Failed to create colormap: " + ToString(err->error_code);
            CLOSE_CONNECTION(data);
            return false;
        }

        u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
        u32 values[] = {data->screen->black_pixel, data->screen->black_pixel,
            XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
            XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE,
            data->colormap,
            0
        };

        data->window = xcb_generate_id(data->connection);
        cookie = xcb_create_window_checked(data->connection, data->windowDepth, data->window, data->screen->root,
                    x, y, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, data->visualID, mask, values);
        if (xcb_generic_error_t *err = xcb_request_check(data->connection, cookie)) {
            error = "Error creating xcb window: " + ToString(err->error_code);
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
                XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, name.size, name.data);
        xcb_change_property (data->connection, XCB_PROP_MODE_REPLACE, data->window,
                XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8, name.size, name.data);

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

        xcb_change_property(data->connection, XCB_PROP_MODE_REPLACE,
            data->window, data->atoms[0], 4, 32, 1, &data->atoms[1]);

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
        open = false;
        return true;
    }

    #define _NET_WM_STATE_REMOVE        0    // remove/unset property
    #define _NET_WM_STATE_ADD           1    // add/set property
    #define _NET_WM_STATE_TOGGLE        2    // toggle property

    bool Window::Fullscreen(bool fs) {
        if (!open) {
            error = "Window hasn't been created yet";
            return false;
        }
        if (fullscreen == fs)
            return true;

        fullscreen = fs;

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

    bool Window::Resize(u32 w, u32 h) {
        if (!open) {
            error = "Window hasn't been created yet";
            return false;
        }
        if (fullscreen) {
            error = "Fullscreen windows can't be resized";
            return false;
        }
        const u32 values[2] = {w, h};
        xcb_configure_window(data->connection, data->window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
        xcb_flush(data->connection);
        return true;
    }

    bool Window::Update() {
        bool changeFullscreen = false;
        resized = false;
        while ((data->event = xcb_poll_for_event(data->connection))) {
            if (!xkbProcessEvent(&data->xkb, (xkb_generic_event_t*)data->event)) {
                free(data->event);
                return false;
            }
            u8 keyCode = 0;
            char character = '\0';
            bool press=false, release=false;
            switch (data->event->response_type & ~0x80) {
                case XCB_CLIENT_MESSAGE: {
                    if (((xcb_client_message_event_t*)data->event)->data.data32[0] == data->atoms[1]) {
                        free(data->event);
                        return false; // Because this atom was bound to the close button
                    }
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
                    break;
                }
                case XCB_KEY_RELEASE: {
                    xcb_key_release_event_t* ev = (xcb_key_release_event_t*)data->event;
                    keyCode = KeyCodeFromEvdev(ev->detail);
                    char buffer[4] = {0};
                    xkb_state_key_get_utf8(data->xkb.state, (xkb_keycode_t)ev->detail, buffer, 4);
                    if (buffer[1] == '\0') {
                        if (!(buffer[0] & 0x80)) {
                            character = buffer[0];
                        }
                    }
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
                if (press) input->typingString += character;
                if (character >= 'a' && character <= 'z') {
                    character += 'A'-'a';
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
