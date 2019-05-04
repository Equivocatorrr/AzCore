/*
    File: io_windows.cpp
    Author: Philip Haynes
*/

#include "io.hpp"
#ifdef IO_FOR_VULKAN
    #include "vk.hpp"
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dinput.h>

#define WS_FULLSCREEN (WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE)
#define WS_WINDOWED (WS_OVERLAPPEDWINDOW | WS_VISIBLE)

#ifndef WM_MOUSEHWHEEL
    #define WM_MOUSEHWHEEL 0x020E
#endif

String ZeroPaddedString(String in, i32 minSize) {
    String out(false);
    out.Reserve(minSize);
    for (i32 i = minSize; i > in.size; i--) {
        out += '0';
    }
    out += in;
    return out;
}

String ToString(GUID guid) {
    String out(false);
    out.Reserve(36);
    out += ZeroPaddedString(ToString((u32)guid.Data1, 16), 8) + '-'
         + ZeroPaddedString(ToString((u32)guid.Data2, 16), 4) + '-'
         + ZeroPaddedString(ToString((u32)guid.Data3, 16), 4) + '-'
         + ZeroPaddedString(ToString((u32)guid.Data4[0], 16) + ToString((u32)guid.Data4[1], 16), 4) + '-'
         + ZeroPaddedString(
             ToString((u32)guid.Data4[2], 16) + ToString((u32)guid.Data4[3], 16)
           + ToString((u32)guid.Data4[4], 16) + ToString((u32)guid.Data4[5], 16)
           + ToString((u32)guid.Data4[6], 16) + ToString((u32)guid.Data4[7], 16),
           12);
    return out;
}

namespace io {

    u32 classNum = 0; // Prevent identical windowClasses

    struct RawInputDeviceData {
        IDirectInputDevice8 *device = nullptr;
        u32 numAxes = 0;
        u32 numButtons = 0;
        u32 numHats = 0;
    };

    RawInputDevice::~RawInputDevice() {
        if (data != nullptr) {
            if (data->device != nullptr) {
                data->device->Release();
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

    void RawInputDeviceInit(RawInputDevice *rid) {
        if (rid->data != nullptr) {
            *rid->data = RawInputDeviceData();
        } else {
            rid->data = new RawInputDeviceData;
        }
    }

    struct RawInputData {
        HINSTANCE instance;
        String windowClassName{false};
        WNDCLASS windowClass;
        HWND window;
        IDirectInput8 *directInput = nullptr;
        RawInputFeatureBits enableMask;
    };

    RawInput::~RawInput() {
        if (data != nullptr) {
            DestroyWindow(data->window);
            UnregisterClass(data->windowClass.lpszClassName, data->instance);
            // DirectInput
            devices.Clear();
            if (data->directInput != nullptr) {
                data->directInput->Release();
            }
            delete data;
        }
    }

    LRESULT CALLBACK RawInputProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL CALLBACK RawInputDeviceEnumeration(const DIDEVICEINSTANCE *devInst, VOID *userdata);

    BOOL CALLBACK RawInputEnumObjects(const DIDEVICEOBJECTINSTANCE *devInst, VOID *userdata);

    bool RawInput::Init(RawInputFeatureBits enableMask) {
        devices.Reserve(4);
        data = new RawInputData;
        data->enableMask = enableMask;
        // Use a hidden message-only window to receive keyboard/mouse input messages
        data->instance = GetModuleHandle(NULL);
        data->windowClass.style = CS_OWNDC;
        data->windowClass.lpfnWndProc = RawInputProcedure;
        data->windowClass.cbClsExtra = 0;
        data->windowClass.cbWndExtra = sizeof(LONG_PTR);
        data->windowClass.hInstance = data->instance;
        data->windowClass.hIcon = NULL;
        data->windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        data->windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        data->windowClass.lpszMenuName = NULL;

        data->windowClassName = "AzCore";
        data->windowClassName += ToString(classNum++);
        data->windowClass.lpszClassName = data->windowClassName.data;
        if (!RegisterClass(&data->windowClass)) {
            error = "Failed to register RawInput window class: " + ToString((u32)GetLastError());
            return false;
        }
        data->window = CreateWindow(data->windowClassName.data,
                "You shouldn't be able to see this.",
                WS_WINDOWED, CW_USEDEFAULT, CW_USEDEFAULT,
                0, 0, HWND_MESSAGE, NULL, data->instance, (LPVOID)this);
        if (data->window == NULL) {
            error = "Failed to create window: " + ToString((u32)GetLastError());
            return false;
        }

        Array<RAWINPUTDEVICE> rids;
        rids.Reserve(2);
        if (enableMask & RAW_INPUT_ENABLE_KEYBOARD_BIT) {
            RAWINPUTDEVICE rid;
            rid.usUsagePage = 0x01;
            rid.usUsage = 0x06;
            rid.dwFlags = 0;
            rid.hwndTarget = data->window;
            rids += rid;
        }
        if (enableMask & RAW_INPUT_ENABLE_MOUSE_BIT) {
            RAWINPUTDEVICE rid;
            rid.usUsagePage = 0x01;
            rid.usUsage = 0x02;
            rid.dwFlags = 0;
            rid.hwndTarget = data->window;
            rids += rid;
        }
        //
        // We Probably don't need these at all since these are handled by DirectInput and XInput
        //
        // if (enableMask & RAW_INPUT_ENABLE_GAMEPAD_BIT) {
        //     RAWINPUTDEVICE rid;
        //     rid.usUsagePage = 0x01;
        //     rid.usUsage = 0x05;
        //     rid.dwFlags = 0;
        //     rid.hwndTarget = data->window;
        //     rids += rid;
        // }
        // if (enableMask & RAW_INPUT_ENABLE_JOYSTICK_BIT) {
        //     RAWINPUTDEVICE rid;
        //     rid.usUsagePage = 0x01;
        //     rid.usUsage = 0x04;
        //     rid.dwFlags = 0;
        //     rid.hwndTarget = data->window;
        //     rids += rid;
        // }

        if (!RegisterRawInputDevices(rids.data, rids.size, sizeof(RAWINPUTDEVICE))) {
            error = "Failed to RegisterRawInputDevices: " + ToString((u32)GetLastError());
            return false;
        }

        // DirectInput

        if (enableMask & RAW_INPUT_ENABLE_GAMEPAD_JOYSTICK) {
            if (DirectInput8Create(data->instance, DIRECTINPUT_VERSION, IID_IDirectInput8A, (LPVOID*)&data->directInput, NULL) != DI_OK) {
                error = "Failed to DirectInput8Create: " + ToString((u32)GetLastError());
                return false;
            }
            cout << "Created DirectInput8!" << std::endl;
            {
                // Enumerate DirectInput devices
                if (data->directInput->EnumDevices(DI8DEVCLASS_GAMECTRL,
                    RawInputDeviceEnumeration, this, DIEDFL_ATTACHEDONLY) != DI_OK) {
                    error = "Failed to EnumDevices: " + ToString((u32)GetLastError());
                    return false;
                }
            }

            // Now that we know what devices are available, we need to configure them for use.
            for (RawInputDevice &rid : devices) {
                if (rid.data->device->SetDataFormat(&c_dfDIJoystick) != DI_OK) {
                    error = "Failed to SetDataFormat: " + ToString((u32)GetLastError());
                    return false;
                }
                if (rid.data->device->SetCooperativeLevel(data->window, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE) != DI_OK) {
                    error = "Failed to SetCooperativeLevel: " + ToString((u32)GetLastError());
                    return false;
                }
                if (rid.data->device->EnumObjects(RawInputEnumObjects, &rid, DIDFT_ALL) != DI_OK) {
                    error = "Failed to EnumObjects: " + ToString((u32)GetLastError());
                    return false;
                }
                cout << "Device has " << rid.data->numAxes << " axes, " << rid.data->numButtons << " buttons, and " << rid.data->numHats << " hats." << std::endl;
                if (rid.data->device->Acquire() != DI_OK) {
                    error = "Failed to Acquire: " + ToString((u32)GetLastError());
                    return false;
                }
            }

            // TODO: Support XInput
        }

        return true;
    }

    void RawInput::Update(f32 timestep) {
        // TODO: The rest of the raw input device types.
        AnyGP.Tick(timestep);
        if (window != nullptr) {
            if (!window->focused)
                return;
        }
        MSG msg;
        while (PeekMessage(&msg, data->window, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // DirectInput

        for (i32 i = 0; i < gamepads.size; i++) {
            gamepads[i].Update(timestep, i);
        }
    }

    f32 mapAxisWithDeadZone(f32 in, f32 minRange, f32 maxRange, f32 deadZone) {
        if (abs(in) < deadZone) {
            return 0.0;
        } else {
            if (in >= 0.0) {
                return (in-deadZone) / (maxRange-deadZone);
            } else {
                return (in+deadZone) / (-minRange-deadZone);
            }
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
        HRESULT result;
        DIJOYSTATE state;

        result = rawInputDevice->data->device->Poll();
        if (result != DI_OK) {
            result = rawInputDevice->data->device->Acquire();
            while (result == DIERR_INPUTLOST) {
                cout << "DIERR_INPUTLOST" << std::endl;
                result = rawInputDevice->data->device->Acquire();
            }

            return;
        }

        if (rawInputDevice->data->device->GetDeviceState(sizeof(DIJOYSTATE), &state) != DI_OK) {
            cout << "Failed to GetDeviceState" << std::endl;
            return;
        }

        const f32 maxRange = 32767.0;
        const f32 minRange = -32768.0;
        const f32 deadZoneTemp = maxRange * deadZone;

        f32 axisLX = (f32)state.lX;
        f32 axisLY = (f32)state.lY;
        f32 axisLZ = (f32)state.lZ;
        f32 axisRX = (f32)state.lRx;
        f32 axisRY = (f32)state.lRy;
        f32 axisRZ = (f32)state.lRz;

        axis.vec.LS.x = mapAxisWithDeadZone(axisLX, minRange, maxRange, deadZoneTemp);
        axis.vec.LS.y = mapAxisWithDeadZone(axisLY, minRange, maxRange, deadZoneTemp);
        axis.vec.LT   = mapAxisWithDeadZone(axisLZ, minRange, maxRange, deadZoneTemp);
        axis.vec.RS.x = mapAxisWithDeadZone(axisRX, minRange, maxRange, deadZoneTemp);
        axis.vec.RS.y = mapAxisWithDeadZone(axisRY, minRange, maxRange, deadZoneTemp);
        axis.vec.RT   = mapAxisWithDeadZone(axisRZ, minRange, maxRange, deadZoneTemp);

        // We only support 1 hat right now
        if (LOWORD(state.rgdwPOV[0]) == 0xFFFF) {
            axis.vec.H0 = vec2(0.0);
        } else {
            f32 hatDirection = (f32)state.rgdwPOV[0] / 36000.0 * tau; // Radians clockwise from north
            axis.vec.H0.y = mapAxisWithDeadZone(-cos(hatDirection), -1.0, 1.0, 0.0000001);
            axis.vec.H0.x = mapAxisWithDeadZone(sin(hatDirection), -1.0, 1.0, 0.0000001);
            // cout << "H0.x = " << axis.vec.H0.x << ", H0.y = " << axis.vec.H0.y << std::endl;
        }

        for (u32 i = 0; i < IO_GAMEPAD_MAX_AXES; i++) {
            if (abs(axis.array[i]) > 0.1) {
                rawInputDevice->rawInput->AnyGPCode = i + KC_GP_AXIS_LS_X;
                rawInputDevice->rawInput->AnyGP.state = BUTTON_PRESSED_BIT;
                rawInputDevice->rawInput->AnyGPIndex = index;
            }
            handleButton(axisPush[i*2], axis.array[i] > 0.5,    i*2 + KC_GP_AXIS_LS_RIGHT,
                         rawInputDevice->rawInput, index);
            handleButton(axisPush[i*2+1], axis.array[i] < -0.5, i*2 + KC_GP_AXIS_LS_LEFT,
                         rawInputDevice->rawInput, index);
            // if (axisPush[i*2].Pressed()) {
            //     cout << "Pressed " << KeyCodeName(i*2 + KC_GP_AXIS_LS_RIGHT) << std::endl;
            // }
            // if (axisPush[i*2+1].Pressed()) {
            //     cout << "Pressed " << KeyCodeName(i*2+1 + KC_GP_AXIS_LS_RIGHT) << std::endl;
            // }
            // if (axisPush[i*2].Released()) {
            //     cout << "Released " << KeyCodeName(i*2 + KC_GP_AXIS_LS_RIGHT) << std::endl;
            // }
            // if (axisPush[i*2+1].Released()) {
            //     cout << "Released " << KeyCodeName(i*2+1 + KC_GP_AXIS_LS_RIGHT) << std::endl;
            // }
        }
        handleButton(hat[0], axis.vec.H0.x > 0.0 && axis.vec.H0.y < 0.0, KC_GP_AXIS_H0_UP_RIGHT,
                     rawInputDevice->rawInput, index);
        handleButton(hat[1], axis.vec.H0.x > 0.0 && axis.vec.H0.y > 0.0, KC_GP_AXIS_H0_DOWN_RIGHT,
                     rawInputDevice->rawInput, index);
        handleButton(hat[2], axis.vec.H0.x < 0.0 && axis.vec.H0.y > 0.0, KC_GP_AXIS_H0_DOWN_LEFT,
                     rawInputDevice->rawInput, index);
        handleButton(hat[3], axis.vec.H0.x < 0.0 && axis.vec.H0.y < 0.0, KC_GP_AXIS_H0_UP_LEFT,
                     rawInputDevice->rawInput, index);

        // for (u32 i = 0; i < 4; i++) {
        //     if (hat[i].Pressed()) {
        //         cout << "Pressed " << KeyCodeName(i + KC_GP_AXIS_H0_UP_RIGHT) << std::endl;
        //     }
        //     if (hat[i].Released()) {
        //         cout << "Released " << KeyCodeName(i + KC_GP_AXIS_H0_UP_RIGHT) << std::endl;
        //     }
        // }

        // NOTE: The only mapping I've tested is for the Logitech Gamepad F310.
        //       The other ones are more or less guesses based on some deduction and research.
        if (rawInputDevice->data->numButtons == 10) {
            // It would appear that some gamepads don't give you access to that middle button
            // In those cases, it would just be missing from the list.
            handleButton(button[ 0], state.rgbButtons[ 0], KC_GP_BTN_A,         rawInputDevice->rawInput, index);
            handleButton(button[ 1], state.rgbButtons[ 1], KC_GP_BTN_B,         rawInputDevice->rawInput, index);
            handleButton(button[ 3], state.rgbButtons[ 2], KC_GP_BTN_X,         rawInputDevice->rawInput, index);
            handleButton(button[ 4], state.rgbButtons[ 3], KC_GP_BTN_Y,         rawInputDevice->rawInput, index);
            handleButton(button[ 6], state.rgbButtons[ 4], KC_GP_BTN_TL,        rawInputDevice->rawInput, index);
            handleButton(button[ 7], state.rgbButtons[ 5], KC_GP_BTN_TR,        rawInputDevice->rawInput, index);
            handleButton(button[10], state.rgbButtons[ 6], KC_GP_BTN_SELECT,    rawInputDevice->rawInput, index);
            handleButton(button[11], state.rgbButtons[ 7], KC_GP_BTN_START,     rawInputDevice->rawInput, index);
            handleButton(button[13], state.rgbButtons[ 8], KC_GP_BTN_THUMBL,    rawInputDevice->rawInput, index);
            handleButton(button[14], state.rgbButtons[ 9], KC_GP_BTN_THUMBR,    rawInputDevice->rawInput, index);
        } else if (rawInputDevice->data->numButtons == 15) {
            // This should be a 1:1 mapping to the keycodes
            for (u32 i = 0; i < 15; i++) {
                handleButton(button[i], state.rgbButtons[i], KC_GP_BTN_A + i,   rawInputDevice->rawInput, index);
            }
        } else if (rawInputDevice->data->numButtons == 14) {
            // This should be a 1:1 mapping to the keycodes except for the MODE button
            for (u32 i = 0; i < 12; i++) {
                handleButton(button[i], state.rgbButtons[i], KC_GP_BTN_A + i,   rawInputDevice->rawInput, index);
            }
            handleButton(button[13], state.rgbButtons[12], KC_GP_BTN_THUMBL,    rawInputDevice->rawInput, index);
            handleButton(button[14], state.rgbButtons[13], KC_GP_BTN_THUMBR,    rawInputDevice->rawInput, index);
        } else /* if (rawInputDevice->data->numButtons == 11) */ {
            // These are the mappings for the Logitech Gamepad F310
            // This is our default for an unknown layout.
            // NOTE: Is this really necessary? I really don't know.
            handleButton(button[ 0], state.rgbButtons[ 0], KC_GP_BTN_A,         rawInputDevice->rawInput, index);
            handleButton(button[ 1], state.rgbButtons[ 1], KC_GP_BTN_B,         rawInputDevice->rawInput, index);
            handleButton(button[ 3], state.rgbButtons[ 2], KC_GP_BTN_X,         rawInputDevice->rawInput, index);
            handleButton(button[ 4], state.rgbButtons[ 3], KC_GP_BTN_Y,         rawInputDevice->rawInput, index);
            handleButton(button[ 6], state.rgbButtons[ 4], KC_GP_BTN_TL,        rawInputDevice->rawInput, index);
            handleButton(button[ 7], state.rgbButtons[ 5], KC_GP_BTN_TR,        rawInputDevice->rawInput, index);
            handleButton(button[10], state.rgbButtons[ 6], KC_GP_BTN_SELECT,    rawInputDevice->rawInput, index);
            handleButton(button[11], state.rgbButtons[ 7], KC_GP_BTN_START,     rawInputDevice->rawInput, index);
            handleButton(button[12], state.rgbButtons[ 8], KC_GP_BTN_MODE,      rawInputDevice->rawInput, index);
            handleButton(button[13], state.rgbButtons[ 9], KC_GP_BTN_THUMBL,    rawInputDevice->rawInput, index);
            handleButton(button[14], state.rgbButtons[10], KC_GP_BTN_THUMBR,    rawInputDevice->rawInput, index);
        }

        // You can use this to determine what button maps from where.
        // If you want to improve one of the above mappings or add one, this is your best bet.
        // for (u32 i = 0; i < 32; i++) {
        //     if (state.rgbButtons[i]) {
        //         cout << "Button[" << i << "] is down!" << std::endl;
        //     }
        // }
    }

    LRESULT CALLBACK RawInputProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (uMsg == WM_CREATE) {
            SetLastError(0);
            SetWindowLongPtr(hWnd, 0, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
            if (GetLastError()) {
                cout << "Failed to SetWindowLongPtr: " + ToString((u32)GetLastError());
            }
            return 0;
        }

        RawInput *rawInput = (RawInput*)GetWindowLongPtr(hWnd, 0);
        if (rawInput == nullptr) {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }

        if (uMsg == WM_INPUT) {
            UINT dwSize;

            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            if (dwSize == 0) {
                return 0;
            }
            LPBYTE lpb = new BYTE[dwSize];

            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
                cout << "GetRawInputData didn't return the correct size!" << std::endl;
            }

            RAWINPUT *raw = (RAWINPUT*)lpb;
            RID_DEVICE_INFO deviceInfo;
            deviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
            UINT deviceInfoSize = sizeof(RID_DEVICE_INFO);
            GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICEINFO, &deviceInfo, &deviceInfoSize);
            if (raw->header.dwType == RIM_TYPEKEYBOARD) {
                // TODO: Implement keyboards
                // cout << "Raw Input from KEYBOARD:\n"
                // "Vendor: " << deviceInfo.hid.dwVendorId <<
                // "Product: " << deviceInfo.hid.dwProductId <<
                // "Version: " << deviceInfo.hid.dwVersionNumber << std::endl;
            } else if (raw->header.dwType == RIM_TYPEMOUSE) {
                // TODO: Implement mice
                // cout << "Raw Input from MOUSE:\n"
                // "Vendor: " << deviceInfo.hid.dwVendorId <<
                // "Product: " << deviceInfo.hid.dwProductId <<
                // "Version: " << deviceInfo.hid.dwVersionNumber << std::endl;
            } /* else if (raw->header.dwType == RIM_TYPEHID) {
                // HID means not keyboard or mouse
                // This is handled by DirectInput and XInput
            } */

            return 0;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    BOOL CALLBACK RawInputDeviceEnumeration(const DIDEVICEINSTANCE *devInst, VOID *userdata) {
        RawInput *rawInput = (RawInput*)userdata;
        cout << "Enumerating Joystick\n\tInstance(" << ToString(devInst->guidInstance) << ") Name: " << devInst->tszInstanceName << "\n\tProduct(" << ToString(devInst->guidProduct) << ") Name: " << devInst->tszProductName << std::endl;
        if (devInst->wUsagePage != 0x01) {
            cout << "Device is not HID!" << std::endl;
            return DIENUM_CONTINUE;
        }
        RawInputDevice rid;
        if (devInst->wUsage == 0x05) {
            cout << "Device is a gamepad" << std::endl;
            if (!(rawInput->data->enableMask & RAW_INPUT_ENABLE_GAMEPAD_BIT)) {
                return DIENUM_CONTINUE;
            }
            rid.type = GAMEPAD;
        } else if (devInst->wUsage == 0x04) {
            cout << "Device is a joystick proper" << std::endl;
            if (!(rawInput->data->enableMask & RAW_INPUT_ENABLE_JOYSTICK_BIT)) {
                return DIENUM_CONTINUE;
            }
            rid.type = JOYSTICK;
        } else {
            cout << "Unsupported wUsage 0x" << std::hex << devInst->wUsage << std::endl;
            return DIENUM_CONTINUE;
        }
        RawInputDeviceInit(&rid);
        rid.rawInput = rawInput;
        if (rawInput->data->directInput->CreateDevice(devInst->guidInstance, &rid.data->device, NULL) == DI_OK) {
            rawInput->devices.Append(std::move(rid));
            switch (rid.type) {
            case KEYBOARD:
            case MOUSE:
                break;
            case GAMEPAD: {
                Gamepad gamepad;
                gamepad.rawInputDevice = rawInput->devices.GetPtr(rawInput->devices.size-1);
                rawInput->gamepads.Append(gamepad);
                break;
            }
            case JOYSTICK:
                // TODO: Implement joysticks
                break;
            case UNSUPPORTED:
                break;
            }
        }
        return DIENUM_CONTINUE;
    }

    BOOL CALLBACK RawInputEnumObjects(const DIDEVICEOBJECTINSTANCE *devInst, VOID *userdata) {
        RawInputDevice *rid = (RawInputDevice*)userdata;


        if (devInst->dwType & DIDFT_AXIS) {
            rid->data->numAxes++;
            DIPROPRANGE range;
            range.diph.dwSize = sizeof(DIPROPRANGE);
            range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
            range.diph.dwHow = DIPH_BYID;
            range.diph.dwObj = devInst->dwType; // Specify the enumerated axis
            if (devInst->guidType != GUID_ZAxis && devInst->guidType != GUID_RzAxis) {
                range.lMin = -32767;
            } else {
                range.lMin = 0.0;
            }
            range.lMax = +32768;

            if (rid->data->device->SetProperty(DIPROP_RANGE, &range.diph) != DI_OK) {
                return DIENUM_STOP;
            }
        } else if (devInst->dwType & DIDFT_BUTTON) {
            rid->data->numButtons++;
        } else if (devInst->dwType & DIDFT_POV) {
            rid->data->numHats++;
        }
        return DIENUM_CONTINUE;
    }

    String winGetInputName (u8 hid) {
        if (hid == 255) {
            return "Null";
        }
        // First make sure we're not anything that doesn't move
        if (hid < 0x04 || (hid >= 0x28 && hid <= 0x2c) || (hid >= 0x39 && hid <= 0x58) || hid >= 0x64) {
            return KeyCodeName(hid);
        }
        // Check if we even have a mapping at all
        u8 keyCode = KeyCodeToWinScan(hid);
        if (keyCode == 255) {
            return "None";
        }
        // If layout-dependent, update the label based on the layout
        char label[2] = {
            (char)MapVirtualKey(MapVirtualKey((u32)keyCode, MAPVK_VSC_TO_VK), MAPVK_VK_TO_CHAR),
            0
        };
        return String(label);
    }

    Window *focusedWindow=nullptr;

    struct WindowData {
        HINSTANCE instance;
        HWND window;
        WNDCLASSEX windowClass;
        HICON windowIcon, windowIconSmall;
        String windowClassName{false};
    };

    Window::Window() {
        data = new WindowData;
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
        VkWin32SurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        createInfo.hinstance = data->instance;
        createInfo.hwnd = data->window;
        VkResult result = vkCreateWin32SurfaceKHR(instance->data.instance, &createInfo, nullptr, surface);
        if (result != VK_SUCCESS) {
            error = "Failed to create Win32 Surface!";
            return false;
        }
        return true;
    }
#endif

    LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        Window* thisWindow = focusedWindow;
        if (focusedWindow == nullptr) {
            PostQuitMessage(0);
            return 0;
        }
        u8 keyCode = 0;
        char character = '\0';
        bool press=false, release=false;
        switch (uMsg)
        {
        case WM_INPUTLANGCHANGE: case WM_INPUTLANGCHANGEREQUEST: {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
        // Dealing with the close button
        case WM_CLOSE: {
            PostQuitMessage(0);
            return 0;
            break;
        }
        case WM_DESTROY: {
            return 0;
        }
        // Keyboard Controls
        case WM_KEYDOWN: {
            // keyCode = KeyCodeFromWinVK((u8)wParam);
            keyCode = KeyCodeFromWinScan((u8)(lParam>>16));
            if (wParam >= VK_NUMPAD1 && wParam <= VK_NUMPAD9) {
                keyCode = KC_KEY_KP1 + wParam - VK_NUMPAD1;
            } else if (wParam == VK_NUMPAD0) {
                keyCode = KC_KEY_KP0;
            } else if (wParam == VK_NUMLOCK) {
                keyCode = KC_KEY_NUMLOCK;
            } else if (wParam == VK_DECIMAL) {
                keyCode = KC_KEY_KPDOT;
            } else if (wParam == VK_MULTIPLY) {
                keyCode = KC_KEY_KPASTERISK;
            } else if (wParam == VK_DIVIDE) {
                keyCode = KC_KEY_KPSLASH;
            } else if (wParam == VK_MULTIPLY) {
                keyCode = KC_KEY_KPASTERISK;
            }
            // cout << "KeyCode down: " << KeyCodeName(keyCode) << std::endl;
            // cout << "WM_KEYDOWN scancode: 0x" << std::hex << (u32)((u8)(lParam>>16)) << " vk_code: 0x" << (u32)((u8)wParam) << std::endl;
            character = (char)MapVirtualKey(wParam, MAPVK_VK_TO_CHAR);
            press = true;
            break;
        }
        case WM_KEYUP: {
            // keyCode = KeyCodeFromWinVK((u8)wParam);
            keyCode = KeyCodeFromWinScan((u8)(lParam>>16));
            if (wParam >= VK_NUMPAD1 && wParam <= VK_NUMPAD9) {
                keyCode = KC_KEY_KP1 + wParam - VK_NUMPAD1;
            } else if (wParam == VK_NUMPAD0) {
                keyCode = KC_KEY_KP0;
            } else if (wParam == VK_NUMLOCK) {
                keyCode = KC_KEY_NUMLOCK;
            } else if (wParam == VK_DECIMAL) {
                keyCode = KC_KEY_KPDOT;
            } else if (wParam == VK_MULTIPLY) {
                keyCode = KC_KEY_KPASTERISK;
            } else if (wParam == VK_DIVIDE) {
                keyCode = KC_KEY_KPSLASH;
            } else if (wParam == VK_MULTIPLY) {
                keyCode = KC_KEY_KPASTERISK;
            }
            // cout << "KeyCode up: " << KeyCodeName(keyCode) << std::endl;
            // cout << "WM_KEYUP scancode: 0x" << std::hex << (u32)((u8)(lParam>>16)) << " vk_code: 0x" << (u32)((u8)wParam) << std::endl;
            // character = (char)MapVirtualKey(wParam, MAPVK_VK_TO_CHAR);
            release = true;
            break;
        }
        // Mouse Controls
        case WM_MOUSEMOVE: {
            if (thisWindow->input != nullptr) {
                thisWindow->input->cursor.x = i32(i16(lParam));
                thisWindow->input->cursor.y = i32(lParam>>16);
            }
            break;
        }
        case WM_MOUSEWHEEL: {
            // Input::scrollY = float(HIWORD(wParam))/120.0;
            // if (Input::scrollY > 0)
            //     keyCode = KC_MOUSE_ScrollUp;
            // else
            //     keyCode = KC_MOUSE_ScrollDown;
            press = true;
            break;
        }
        case WM_MOUSEHWHEEL: {
            // Input::scrollX = float(HIWORD(wParam))/120.0;
            // if (Input::scrollX > 0)
            //     keyCode = KC_MOUSE_ScrollRight;
            // else
            //     keyCode = KC_MOUSE_ScrollLeft;
            press = true;
            break;
        }
        case WM_LBUTTONDOWN: {
            keyCode = KC_MOUSE_LEFT;
            press = true;
            break;
        }
        case WM_LBUTTONUP: {
            keyCode = KC_MOUSE_LEFT;
            release = true;
            break;
        }
        case WM_MBUTTONDOWN: {
            keyCode = KC_MOUSE_MIDDLE;
            press = true;
            break;
        }
        case WM_MBUTTONUP: {
            keyCode = KC_MOUSE_MIDDLE;
            release = true;
            break;
        }
        case WM_RBUTTONDOWN: {
            keyCode = KC_MOUSE_RIGHT;
            press = true;
            break;
        }
        case WM_RBUTTONUP: {
            keyCode = KC_MOUSE_RIGHT;
            release = true;
            break;
        }
        case WM_XBUTTONDOWN: {
            i16 i = HIWORD(wParam);
            if (i == XBUTTON1)
                keyCode = KC_MOUSE_XONE;
            else
                keyCode = KC_MOUSE_XTWO;
            press = true;
            break;
        }
        case WM_XBUTTONUP: {
            i16 i = HIWORD(wParam); // XBUTTON1 = 1, XBUTTON2 = 2
            if (i == XBUTTON1)
                keyCode = KC_MOUSE_XONE;
            else
                keyCode = KC_MOUSE_XTWO;
            release = true;
            break;
        }
        case WM_CHAR: {
            // handleCharInput((char)wParam);
            break;
        }
        case WM_MOVE: {
            if (!thisWindow->resized) {
                if (!thisWindow->fullscreen) {
                    thisWindow->windowedX = LOWORD(lParam);
                    thisWindow->windowedY = HIWORD(lParam);
                }
                thisWindow->x = LOWORD(lParam);
                thisWindow->y = HIWORD(lParam);
            }
            break;
        }
        case WM_SIZE: {
            if (!thisWindow->resized) {
                // Workaround because Windows sucks
                thisWindow->width = LOWORD(lParam);
                thisWindow->height = HIWORD(lParam);
                if (!thisWindow->fullscreen) {
                    thisWindow->windowedWidth = LOWORD(lParam);
                    thisWindow->windowedHeight = HIWORD(lParam);
                }
            } else {
                thisWindow->resized = false;
            }
            break;
        }
        case WM_SETFOCUS: {
            thisWindow->focused = true;
            break;
        }
        case WM_KILLFOCUS: {
            thisWindow->focused = false;
            thisWindow->input->ReleaseAll();
            break;
        }
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }

        if (thisWindow->input != nullptr && thisWindow->focused) {
            if (press) {
                if (keyCode != 0) {
                    thisWindow->input->Press(keyCode);
                }
                if (character != '\0') {
                    thisWindow->input->PressChar(character);
                }
            }
            if (release) {
                if (keyCode != 0) {
                    thisWindow->input->Release(keyCode);
                }
                if (character != '\0') {
                    thisWindow->input->ReleaseChar(character);
                }
            }
        }

        if (keyCode == KC_MOUSE_XTWO || keyCode == KC_MOUSE_XONE)
            return TRUE;

        return 0;
    }

    bool Window::Open() {
        data->instance = GetModuleHandle(NULL);
        data->windowIcon = LoadIcon(data->instance,"icon.ico");
        data->windowIconSmall = data->windowIcon;
        data->windowClass.cbSize = sizeof(WNDCLASSEX);
        data->windowClass.style = CS_OWNDC;
        data->windowClass.lpfnWndProc = WindowProcedure;
        data->windowClass.cbClsExtra = 0;
        data->windowClass.cbWndExtra = 0;
        data->windowClass.hInstance = data->instance;
        data->windowClass.hIcon = data->windowIcon;
        data->windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        data->windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        data->windowClass.lpszMenuName = NULL;

        data->windowClassName = "AzCore";
        data->windowClassName += ToString(classNum++);
        data->windowClass.lpszClassName = data->windowClassName.data;
        data->windowClass.hIconSm = data->windowIconSmall;
        if (!RegisterClassEx(&data->windowClass)) {
            error = "Failed to register window class: ";
            error += ToString((u32)GetLastError());
            return false;
        }

        RECT rect;
        rect.left = 0;
        rect.right = width;
        rect.top = 0;
        rect.bottom = height;
        AdjustWindowRect(&rect,WS_WINDOWED,FALSE);
        focusedWindow = this;
        data->window = CreateWindowEx(0,data->windowClassName.data,name.data,WS_WINDOWED, CW_USEDEFAULT, CW_USEDEFAULT,
                rect.right-rect.left, rect.bottom-rect.top, NULL, NULL, data->instance, 0);
        if (data->window==NULL) {
            error = "Failed to create window: ";
            error += ToString((u32)GetLastError());
            return false;
        }
        open = true;
        return true;
    }

    bool Window::Show() {
        if (!open) {
            error = "Window hasn't been created yet";
            return false;
        }
        ShowWindow(data->window, SW_SHOWNORMAL);

        return true;
    }

    bool Window::Close() {
        if (!open) {
            error = "Window hasn't been created yet";
            return false;
        }
        DestroyWindow(data->window);
        UnregisterClass(data->windowClass.lpszClassName, data->instance);

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
        resized = true; // Prevent WM_SIZE from lying to us, the ungrateful bastard

        if (fullscreen) {
            HMONITOR monitor = MonitorFromWindow(data->window,MONITOR_DEFAULTTONEAREST);
            if(monitor!=NULL)
            {
                MONITORINFO minfo;
                minfo.cbSize = sizeof(MONITORINFO);
                GetMonitorInfo(monitor,&minfo);
                width = minfo.rcMonitor.right-minfo.rcMonitor.left;
                height = minfo.rcMonitor.bottom-minfo.rcMonitor.top;
                x = minfo.rcMonitor.left;
                y = minfo.rcMonitor.top;
                // if(minfo.dwFlags==MONITORINFOF_PRIMARY)
                //     Sys::cout << "Fullscreened on the primary monitor." << std::endl;
                // else
                //     Sys::cout << "Fullscreened on a non-primary monitor." << std::endl;
            }
            SetWindowLongPtr(data->window, GWL_STYLE, WS_FULLSCREEN);
            MoveWindow(data->window, x, y, width, height, TRUE);
        } else {
            width = windowedWidth;
            height = windowedHeight;
            RECT rect;
            rect.left = 0;
            rect.top = 0;
            rect.right = width;
            rect.bottom = height;
            SetWindowLongPtr(data->window, GWL_STYLE, WS_WINDOWED);
            AdjustWindowRect(&rect,WS_WINDOWED, FALSE);
            MoveWindow(data->window, windowedX, windowedY, rect.right-rect.left, rect.bottom-rect.top, TRUE);
            x = windowedX;
            y = windowedY;
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
        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = w;
        rect.bottom = h;
        AdjustWindowRect(&rect, WS_WINDOWED, FALSE);
        SetWindowPos(data->window, 0, 0, 0, rect.right-rect.left, rect.bottom-rect.top, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
        return true;
    }

    bool Window::Update() {
        MSG msg;
        while (PeekMessage(&msg, data->window, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                return false;
            }
            else
            {
                if (msg.message == WM_SETFOCUS) {
                    focusedWindow = this;
                }
                if (msg.message == WM_KEYDOWN) {
                    if (KC_KEY_F11 == KeyCodeFromWinScan((u8)(msg.lParam>>16)))
                        Fullscreen(!fullscreen);
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        return true;
    }

    String Window::InputName(u8 keyCode) const {
        return winGetInputName(keyCode);
    }

}
