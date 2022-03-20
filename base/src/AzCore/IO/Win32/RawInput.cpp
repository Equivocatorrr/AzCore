/*
	File: Win32/RawInput.cpp
	Author: Philip Haynes
*/

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dinput.h>
#include "../RawInput.hpp"
#include "../../io.hpp"
#include "../../basictypes.hpp"
#include "../../keycodes.hpp"
#include "../../Memory/String.hpp"

#define WS_WINDOWED (WS_OVERLAPPEDWINDOW | WS_VISIBLE)

namespace AzCore {

String ZeroPaddedString(String in, i32 minSize) {
	String out;
	out.Reserve(minSize);
	for (i32 i = minSize; i > in.size; i--) {
		out += '0';
	}
	out += in;
	return out;
}

String ToString(GUID guid) {
	String out;
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

RawInputDevice &RawInputDevice::operator=(RawInputDevice &&other) {
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
	String windowClassName;
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
	data->windowClassName += ToString(windowClassNum++);
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

	if (enableMask & RAW_INPUT_ENABLE_KEYBOARD_MOUSE) {
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
		//	 RAWINPUTDEVICE rid;
		//	 rid.usUsagePage = 0x01;
		//	 rid.usUsage = 0x05;
		//	 rid.dwFlags = 0;
		//	 rid.hwndTarget = data->window;
		//	 rids += rid;
		// }
		// if (enableMask & RAW_INPUT_ENABLE_JOYSTICK_BIT) {
		//	 RAWINPUTDEVICE rid;
		//	 rid.usUsagePage = 0x01;
		//	 rid.usUsage = 0x04;
		//	 rid.dwFlags = 0;
		//	 rid.hwndTarget = data->window;
		//	 rids += rid;
		// }

		if (!RegisterRawInputDevices(rids.data, rids.size, sizeof(RAWINPUTDEVICE))) {
			error = "Failed to RegisterRawInputDevices: " + ToString((u32)GetLastError());
			return false;
		}
	}

	// DirectInput

	if (enableMask & RAW_INPUT_ENABLE_GAMEPAD_JOYSTICK) {
		if (DirectInput8Create(data->instance, DIRECTINPUT_VERSION, IID_IDirectInput8A, (LPVOID *)&data->directInput, NULL) != DI_OK) {
			error = "Failed to DirectInput8Create: " + ToString((u32)GetLastError());
			return false;
		}
		cout.PrintLn("Created DirectInput8!");
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
			cout.PrintLn("Device has ", rid.data->numAxes, " axes, ", rid.data->numButtons, " buttons, and ", rid.data->numHats, " hats.");
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
	while (PeekMessage(&msg, data->window, 0, 0, PM_REMOVE)) {
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
		return 0.0f;
	} else {
		if (in >= 0.0f) {
			return (in - deadZone) / (maxRange - deadZone);
		} else {
			return (in + deadZone) / (-minRange - deadZone);
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
	for (u32 i = 0; i < IO_GAMEPAD_MAX_AXES * 2; i++) {
		axisPush[i].Tick(timestep);
	}
	for (u32 i = 0; i < 4; i++) {
		hat[i].Tick(timestep);
	}
	HRESULT result;
	DIJOYSTATE state;

	result = rawInputDevice->data->device->Poll();
	if (result != DI_OK && result != DI_NOEFFECT) {
		result = rawInputDevice->data->device->Acquire();
		while (result == DIERR_INPUTLOST) {
			cout.PrintLn("DIERR_INPUTLOST");
			result = rawInputDevice->data->device->Acquire();
		}
		cout.PrintLn("Poll failed: ", result);
		return;
	}

	if (rawInputDevice->data->device->GetDeviceState(sizeof(DIJOYSTATE), &state) != DI_OK) {
		cout.PrintLn("Failed to GetDeviceState");
		return;
	}

	const f32 maxRange = 32767.0f;
	const f32 minRange = -32768.0f;
	const f32 deadZoneTemp = maxRange * deadZone;

	f32 axisLX = (f32)state.lX;
	f32 axisLY = (f32)state.lY;
	f32 axisLZ = (f32)state.lZ;
	f32 axisRX = (f32)state.lRx;
	f32 axisRY = (f32)state.lRy;
	f32 axisRZ = (f32)state.lRz;

	if (rawInputDevice->data->numAxes == 5) {
		// Probably combined Z-axes because Microsoft is a dum dum
		axisRZ = map(axisLZ, 0.0f, maxRange, minRange, maxRange);
		axisLZ = max(axisRZ, 0.0f);
		axisRZ = max(-axisRZ, 0.0f);
	}

	axis.vec.LS.x = mapAxisWithDeadZone(axisLX, minRange, maxRange, deadZoneTemp);
	axis.vec.LS.y = mapAxisWithDeadZone(axisLY, minRange, maxRange, deadZoneTemp);
	axis.vec.LT = mapAxisWithDeadZone(axisLZ, minRange, maxRange, deadZoneTemp);
	axis.vec.RS.x = mapAxisWithDeadZone(axisRX, minRange, maxRange, deadZoneTemp);
	axis.vec.RS.y = mapAxisWithDeadZone(axisRY, minRange, maxRange, deadZoneTemp);
	axis.vec.RT = mapAxisWithDeadZone(axisRZ, minRange, maxRange, deadZoneTemp);

	// We only support 1 hat right now
	if (LOWORD(state.rgdwPOV[0]) == 0xFFFF) {
		axis.vec.H0 = vec2(0.0f);
	} else {
		f32 hatDirection = (f32)state.rgdwPOV[0] / 36000.0f * tau; // Radians clockwise from north
		axis.vec.H0.y = mapAxisWithDeadZone(-cos(hatDirection), -1.0f, 1.0f, 0.0000001f);
		axis.vec.H0.x = mapAxisWithDeadZone(sin(hatDirection), -1.0f, 1.0f, 0.0000001f);
		// cout.PrintLn("H0.x = ", axis.vec.H0.x, ", H0.y = ", axis.vec.H0.y);
	}

	for (u32 i = 0; i < IO_GAMEPAD_MAX_AXES; i++) {
		if (abs(axis.array[i]) > 0.1f) {
			rawInputDevice->rawInput->AnyGPCode = i + KC_GP_AXIS_LS_X;
			rawInputDevice->rawInput->AnyGP.state = BUTTON_PRESSED_BIT;
			rawInputDevice->rawInput->AnyGPIndex = index;
		}
		handleButton(axisPush[i * 2], axis.array[i] > 0.5f, i * 2 + KC_GP_AXIS_LS_RIGHT,
					 rawInputDevice->rawInput, index);
		handleButton(axisPush[i * 2 + 1], axis.array[i] < -0.5f, i * 2 + KC_GP_AXIS_LS_LEFT,
					 rawInputDevice->rawInput, index);
		if (axisCurve != 1.0f) {
			bool negative = axis.array[i] < 0.0f;
			axis.array[i] = pow(abs(axis.array[i]), axisCurve);
			if (negative) {
				axis.array[i] *= -1.0f;
			}
		}
		// if (axisPush[i*2].Pressed()) {
		//	 cout.PrintLn("Pressed ", KeyCodeName(i*2 + KC_GP_AXIS_LS_RIGHT));
		// }
		// if (axisPush[i*2+1].Pressed()) {
		//	 cout.PrintLn("Pressed ", KeyCodeName(i*2+1 + KC_GP_AXIS_LS_RIGHT));
		// }
		// if (axisPush[i*2].Released()) {
		//	 cout.PrintLn("Released ", KeyCodeName(i*2 + KC_GP_AXIS_LS_RIGHT));
		// }
		// if (axisPush[i*2+1].Released()) {
		//	 cout.PrintLn("Released ", KeyCodeName(i*2+1 + KC_GP_AXIS_LS_RIGHT));
		// }
	}
	handleButton(hat[0], axis.vec.H0.x > 0.0f && axis.vec.H0.y < 0.0f, KC_GP_AXIS_H0_UP_RIGHT,
				 rawInputDevice->rawInput, index);
	handleButton(hat[1], axis.vec.H0.x > 0.0f && axis.vec.H0.y > 0.0f, KC_GP_AXIS_H0_DOWN_RIGHT,
				 rawInputDevice->rawInput, index);
	handleButton(hat[2], axis.vec.H0.x < 0.0f && axis.vec.H0.y > 0.0f, KC_GP_AXIS_H0_DOWN_LEFT,
				 rawInputDevice->rawInput, index);
	handleButton(hat[3], axis.vec.H0.x < 0.0f && axis.vec.H0.y < 0.0f, KC_GP_AXIS_H0_UP_LEFT,
				 rawInputDevice->rawInput, index);

	// for (u32 i = 0; i < 4; i++) {
	//	 if (hat[i].Pressed()) {
	//		 cout.PrintLn("Pressed ", KeyCodeName(i + KC_GP_AXIS_H0_UP_RIGHT));
	//	 }
	//	 if (hat[i].Released()) {
	//		 cout.PrintLn("Released ", KeyCodeName(i + KC_GP_AXIS_H0_UP_RIGHT));
	//	 }
	// }

	// NOTE: The only mapping I've tested is for the Logitech Gamepad F310.
	//	   The other ones are more or less guesses based on some deduction and research.
	if (rawInputDevice->data->numButtons == 10) {
		// It would appear that some gamepads don't give you access to that middle button
		// In those cases, it would just be missing from the list.
		handleButton(button[0], state.rgbButtons[0], KC_GP_BTN_A, rawInputDevice->rawInput, index);
		handleButton(button[1], state.rgbButtons[1], KC_GP_BTN_B, rawInputDevice->rawInput, index);
		handleButton(button[3], state.rgbButtons[2], KC_GP_BTN_X, rawInputDevice->rawInput, index);
		handleButton(button[4], state.rgbButtons[3], KC_GP_BTN_Y, rawInputDevice->rawInput, index);
		handleButton(button[6], state.rgbButtons[4], KC_GP_BTN_TL, rawInputDevice->rawInput, index);
		handleButton(button[7], state.rgbButtons[5], KC_GP_BTN_TR, rawInputDevice->rawInput, index);
		handleButton(button[10], state.rgbButtons[6], KC_GP_BTN_SELECT, rawInputDevice->rawInput, index);
		handleButton(button[11], state.rgbButtons[7], KC_GP_BTN_START, rawInputDevice->rawInput, index);
		handleButton(button[13], state.rgbButtons[8], KC_GP_BTN_THUMBL, rawInputDevice->rawInput, index);
		handleButton(button[14], state.rgbButtons[9], KC_GP_BTN_THUMBR, rawInputDevice->rawInput, index);
	} else if (rawInputDevice->data->numButtons == 15) {
		// This should be a 1:1 mapping to the keycodes
		for (u32 i = 0; i < 15; i++) {
			handleButton(button[i], state.rgbButtons[i], KC_GP_BTN_A + i, rawInputDevice->rawInput, index);
		}
	} else if (rawInputDevice->data->numButtons == 14) {
		// This should be a 1:1 mapping to the keycodes except for the MODE button
		for (u32 i = 0; i < 12; i++) {
			handleButton(button[i], state.rgbButtons[i], KC_GP_BTN_A + i, rawInputDevice->rawInput, index);
		}
		handleButton(button[13], state.rgbButtons[12], KC_GP_BTN_THUMBL, rawInputDevice->rawInput, index);
		handleButton(button[14], state.rgbButtons[13], KC_GP_BTN_THUMBR, rawInputDevice->rawInput, index);
	} else /* if (rawInputDevice->data->numButtons == 11) */ {
		// These are the mappings for the Logitech Gamepad F310
		// This is our default for an unknown layout.
		// NOTE: Is this really necessary? I really don't know.
		handleButton(button[0], state.rgbButtons[0], KC_GP_BTN_A, rawInputDevice->rawInput, index);
		handleButton(button[1], state.rgbButtons[1], KC_GP_BTN_B, rawInputDevice->rawInput, index);
		handleButton(button[3], state.rgbButtons[2], KC_GP_BTN_X, rawInputDevice->rawInput, index);
		handleButton(button[4], state.rgbButtons[3], KC_GP_BTN_Y, rawInputDevice->rawInput, index);
		handleButton(button[6], state.rgbButtons[4], KC_GP_BTN_TL, rawInputDevice->rawInput, index);
		handleButton(button[7], state.rgbButtons[5], KC_GP_BTN_TR, rawInputDevice->rawInput, index);
		handleButton(button[10], state.rgbButtons[6], KC_GP_BTN_SELECT, rawInputDevice->rawInput, index);
		handleButton(button[11], state.rgbButtons[7], KC_GP_BTN_START, rawInputDevice->rawInput, index);
		handleButton(button[12], state.rgbButtons[8], KC_GP_BTN_MODE, rawInputDevice->rawInput, index);
		handleButton(button[13], state.rgbButtons[9], KC_GP_BTN_THUMBL, rawInputDevice->rawInput, index);
		handleButton(button[14], state.rgbButtons[10], KC_GP_BTN_THUMBR, rawInputDevice->rawInput, index);
	}

	// You can use this to determine what button maps from where.
	// If you want to improve one of the above mappings or add one, this is your best bet.
	// for (u32 i = 0; i < 32; i++) {
	//	 if (state.rgbButtons[i]) {
	//		 cout.PrintLn("Button[", i, "] is down!");
	//	 }
	// }
}

LRESULT CALLBACK RawInputProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_CREATE) {
		SetLastError(0);
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)((CREATESTRUCT *)lParam)->lpCreateParams);
		if (GetLastError()) {
			cout.PrintLn("Failed to SetWindowLongPtr: ", (u32)GetLastError());
		}
		return 0;
	}

	RawInput *rawInput = (RawInput *)GetWindowLongPtr(hWnd, 0);
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
			cout.PrintLn("GetRawInputData didn't return the correct size!");
		}

		RAWINPUT *raw = (RAWINPUT *)lpb;
		RID_DEVICE_INFO deviceInfo;
		deviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
		UINT deviceInfoSize = sizeof(RID_DEVICE_INFO);
		GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICEINFO, &deviceInfo, &deviceInfoSize);
		if (raw->header.dwType == RIM_TYPEKEYBOARD) {
			// TODO: Implement keyboards
			// cout.PrintLn("Raw Input from KEYBOARD:\n"
			// "Vendor: ", deviceInfo.hid.dwVendorId,
			// "Product: ", deviceInfo.hid.dwProductId,
			// "Version: ", deviceInfo.hid.dwVersionNumber);
		} else if (raw->header.dwType == RIM_TYPEMOUSE) {
			// TODO: Implement mice
			// cout.PrintLn("Raw Input from MOUSE:\n"
			// "Vendor: ", deviceInfo.hid.dwVendorId,
			// "Product: ", deviceInfo.hid.dwProductId,
			// "Version: ", deviceInfo.hid.dwVersionNumber);
		} /* else if (raw->header.dwType == RIM_TYPEHID) {
				// HID means not keyboard or mouse
				// This is handled by DirectInput and XInput
			} */

		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK RawInputDeviceEnumeration(const DIDEVICEINSTANCE *devInst, VOID *userdata) {
	RawInput *rawInput = (RawInput *)userdata;
	cout.PrintLn("Enumerating Joystick\n\tInstance(", ToString(devInst->guidInstance), ") Name: ", devInst->tszInstanceName, "\n\tProduct(", ToString(devInst->guidProduct), ") Name: ", devInst->tszProductName);
	if (devInst->wUsagePage != 0x01) {
		cout.PrintLn("Device is not HID!");
		return DIENUM_CONTINUE;
	}
	RawInputDevice rid;
	if (devInst->wUsage == 0x05) {
		cout.PrintLn("Device is a gamepad");
		if (!(rawInput->data->enableMask & RAW_INPUT_ENABLE_GAMEPAD_BIT)) {
			return DIENUM_CONTINUE;
		}
		rid.type = GAMEPAD;
	} else if (devInst->wUsage == 0x04) {
		cout.PrintLn("Device is a joystick proper");
		if (!(rawInput->data->enableMask & RAW_INPUT_ENABLE_JOYSTICK_BIT)) {
			return DIENUM_CONTINUE;
		}
		rid.type = JOYSTICK;
	} else {
		cout.PrintLn("Unsupported wUsage 0x", ToString(devInst->wUsage, 16));
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
			gamepad.rawInputDevice = rawInput->devices.GetPtr(rawInput->devices.size - 1);
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
	RawInputDevice *rid = (RawInputDevice *)userdata;

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

} // namespace io

} // namespace AzCore
