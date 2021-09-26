/*
	File: Linux/RawInput.cpp
	Author: Philip Haynes
*/

#include "../../io.hpp"
#include "../../keycodes.hpp"
#include "../../math.hpp"

#include <fcntl.h>
#include <linux/joystick.h>
#include <unistd.h>

namespace AzCore {

namespace io {

const i32 GAMEPAD_MAPPING_MAX_AXES = 12;
const i32 GAMEPAD_MAPPING_MAX_BUTTONS = 20;

struct GamepadMapping {
	// Just some overkill numbers to prevent segfaults in the case of a really wacky controller
	u8 axes[GAMEPAD_MAPPING_MAX_AXES];
	u8 buttons[GAMEPAD_MAPPING_MAX_BUTTONS];
	void FromDevice(int fd);
};

struct jsMapping {
	__u8 axes[ABS_CNT];
	__u16 buttons[(KEY_MAX - BTN_MISC + 1)];
};

void GamepadMapping::FromDevice(int fd) {
	jsMapping driverMap;
	char numAxes, numButtons;
	ioctl(fd, JSIOCGAXES, &numAxes);
	ioctl(fd, JSIOCGBUTTONS, &numButtons);
	ioctl(fd, JSIOCGAXMAP, driverMap.axes);
	ioctl(fd, JSIOCGBTNMAP, driverMap.buttons);
	bool hasLTAxis = false, hasRTAxis = false;
	for (i32 i = 0; i < numAxes; i++) {
		if (driverMap.axes[i] > ABS_HAT0Y) {
			if (driverMap.axes[i] >= BTN_GAMEPAD && driverMap.axes[i] <= BTN_DPAD_RIGHT) {
				// Axis-to-button mapping
				axes[i] = 255; // Disabled for now
			} else {
				axes[i] = 255;
			}
		} else if (driverMap.axes[i] < ABS_THROTTLE) {
			axes[i] = driverMap.axes[i];
		} else if (driverMap.axes[i] < ABS_HAT0X) {
			axes[i] = 255;
		} else {
			axes[i] = driverMap.axes[i] - 10;
		}
		if (axes[i] == GP_AXIS_LT)
			hasLTAxis = true;
		if (axes[i] == GP_AXIS_RT)
			hasRTAxis = true;
	}
	for (i32 i = 0; i < numButtons; i++) {
		if (driverMap.buttons[i] < BTN_GAMEPAD) {
			buttons[i] = 0;
		} else if (driverMap.buttons[i] <= BTN_THUMBR) {
			buttons[i] = driverMap.buttons[i] - BTN_A + KC_GP_BTN_A;
		} else if (driverMap.buttons[i] < BTN_DPAD_UP) {
			buttons[i] = 0;
		} else if (driverMap.buttons[i] <= BTN_DPAD_RIGHT) {
			buttons[i] = KC_GP_AXIS_H0_UP - (driverMap.buttons[i] - BTN_DPAD_UP);
		} else {
			buttons[i] = 0;
		}
		if (buttons[i] == KC_GP_BTN_TL2) {
			if (hasLTAxis)
				buttons[i] = 0;
			else
				buttons[i] = KC_GP_AXIS_LT_IN;
		}
		if (buttons[i] == KC_GP_BTN_TR2) {
			if (hasRTAxis)
				buttons[i] = 0;
			else
				buttons[i] = KC_GP_AXIS_RT_IN;
		}
	}
}

struct RawInputDeviceData {
	GamepadMapping mapping;
	String name;
	String path;
	i32 fd;
	u32 version;
	f32 retryTimer;
};

RawInputDevice::~RawInputDevice() {
	if (data != nullptr) {
		if (data->fd > -1) {
			close(data->fd);
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

void RawInputDeviceInit(RawInputDevice *rid, i32 fd, String &&path, RawInputFeatureBits enableMask) {
	if (rid->data == nullptr) {
		rid->data = new RawInputDeviceData;
	}
	rid->data->fd = fd;
	rid->data->retryTimer = -1.0;
	rid->data->path = std::move(path);
	rid->data->name.Resize(128);
	if (-1 == ioctl(fd, JSIOCGNAME(rid->data->name.size), rid->data->name.data)) {
		rid->data->name = "Error Retrieving Name";
	}
	if (-1 == ioctl(fd, JSIOCGVERSION, &rid->data->version)) {
		rid->data->version = UINT32_MAX;
	}
	// TODO: Recognize Joysticks separately
	rid->type = GAMEPAD;
	cout.PrintLn("RawInputDevice from path \"", rid->data->path, "\":\n"
		 "\t   Type: ", RawInputDeviceTypeString[rid->type], "\n"
		 "\t   Name: ", rid->data->name, "\n"
		 "\tVersion: ", rid->data->version);
	u8 axes, buttons;
	if (-1 == ioctl(fd, JSIOCGAXES, &axes)) {
		cout.PrintLn("\tFailed to get axes...");
	} else {
		cout.PrintLn("\tJoystick has ",  (u32)axes,  " axes.");
	}
	if (-1 == ioctl(fd, JSIOCGBUTTONS, &buttons)) {
		cout.PrintLn("\tFailed to get buttons...");
	} else {
		cout.PrintLn("\tJoystick has ",  (u32)buttons,  " buttons.");
	}
	rid->data->mapping.FromDevice(rid->data->fd);
}

bool GetRawInputDeviceEvent(Ptr<RawInputDevice> rid, js_event *dst) {
	ssize_t rc = read(rid->data->fd, dst, sizeof(js_event));
	if (rc == -1 && errno != EAGAIN) {
		cout.PrintLn("Lost raw input device ",  rid->data->path);
		close(rid->data->fd);
		rid->data->retryTimer = 1.0;
		return false;
	}
	return rc == sizeof(js_event);
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
#ifdef IO_GAMEPAD_LOGGING_VERBOSE
	ClockTime start = Clock::now();
#endif
	char path[] = "/dev/input/jsXX";
	for (u32 i = 0; i < 32; i++) {
		if (i < 10) {
			path[13] = i + '0';
			path[14] = 0;
		} else {
			path[13] = i / 10 + '0';
			path[14] = i % 10 + '0';
		}
		// cout.PrintLn(path);
		i32 fd = open(path, O_RDONLY | O_NONBLOCK);
		if (fd < 0) {
			if (errno == EACCES) {
				cout.PrintLn("Permission denied opening device with path \"",  path,  "\".");
			}
			continue;
		}
		// We got a live one boys!
		RawInputDevice device;
		RawInputDeviceInit(&device, fd, std::move(path), enableMask);
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
			gamepad.rawInputDevice = devices.GetPtr(devices.size - 1);
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
#ifdef IO_GAMEPAD_LOGGING_VERBOSE
	cout.PrintLn("Total time to check 32 raw input devices: ",  FormatTime(Clock::now() - start));
#endif
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
	if (rawInputDevice->data->retryTimer != -1.0f) {
		rawInputDevice->data->retryTimer -= timestep;
		if (rawInputDevice->data->retryTimer < 0.0f) {
			i32 fd = open(rawInputDevice->data->path.data, O_RDONLY | O_NONBLOCK);
			if (fd >= 0) {
				// String path(std::move(rawInputDevice->data->path)); // Kinda weird but if it moved in place it would self destruct lol
				RawInputDeviceInit(&(*rawInputDevice), fd, std::move(rawInputDevice->data->path), RAW_INPUT_ENABLE_GAMEPAD_BIT);
			} else {
				rawInputDevice->data->retryTimer = 1.0f;
			}
		}
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
	js_event ev;
	const GamepadMapping &mapping = rawInputDevice->data->mapping;
	while (GetRawInputDeviceEvent(rawInputDevice, &ev)) {
		switch (ev.type) {
		case JS_EVENT_INIT: {
			// Not sure what this is for... seems to not be triggered ever???
			cout.PrintLn("JS_EVENT_INIT has number ",  (u32)ev.number,  " and value ",  ev.value);
		}
		break;
		case JS_EVENT_AXIS: {
			f32 maxRange = 1.0f;
			f32 minRange = -1.0f;
			f32 deadZoneTemp = deadZone;
			if (ev.number >= GAMEPAD_MAPPING_MAX_AXES)
				continue; // Let's not make crazy things happen
			i32 aIndex = mapping.axes[ev.number];
			if (aIndex == GP_AXIS_LT || aIndex == GP_AXIS_RT) {
				// No such thing as an outward trigger pull AFAIK
				minRange = 0.0f;
				deadZoneTemp = 0.0f;
			}
			if (aIndex >= IO_GAMEPAD_MAX_AXES) {
				continue; // Unsupported
			}
			f32 val = map((f32)ev.value, -32767.0f, 32767.0f, minRange, maxRange);
			// cout.PrintLn("axis = ",  aIndex,  ", val = ",  val);
			if (abs(val) < deadZoneTemp) {
				axis.array[aIndex] = 0.0f;
			} else {
				if (val >= 0.0f) {
					axis.array[aIndex] = (val - deadZoneTemp) / (1.0f - deadZoneTemp);
				} else {
					axis.array[aIndex] = (val + deadZoneTemp) / (1.0f - deadZoneTemp);
				}
				if (abs(axis.array[aIndex]) > 0.1f) {
					rawInputDevice->rawInput->AnyGPCode = aIndex + KC_GP_AXIS_LS_X;
					rawInputDevice->rawInput->AnyGP.state = BUTTON_PRESSED_BIT;
					rawInputDevice->rawInput->AnyGPIndex = index;
				}
			}
			if (axisCurve != 1.0f) {
				bool negative = axis.array[aIndex] < 0.0f;
				axis.array[aIndex] = pow(abs(axis.array[aIndex]), axisCurve);
				if (negative) {
					axis.array[aIndex] *= -1.0f;
				}
			}
			handleButton(axisPush[aIndex * 2], axis.array[aIndex] > 0.5f, aIndex * 2 + KC_GP_AXIS_LS_RIGHT,
						 rawInputDevice->rawInput, index);
			handleButton(axisPush[aIndex * 2 + 1], axis.array[aIndex] < -0.5f, aIndex * 2 + KC_GP_AXIS_LS_LEFT,
						 rawInputDevice->rawInput, index);
		}
		break;
		case JS_EVENT_BUTTON: {
			if (ev.number >= GAMEPAD_MAPPING_MAX_BUTTONS)
				continue; // No tomfuckery here, oh no
			i32 bIndex = mapping.buttons[ev.number];
			if (bIndex >= KC_GP_AXIS_LS_RIGHT && bIndex <= KC_GP_AXIS_H0_UP) {
				// In the case where buttons have to be mapped to axes
				// Like how Sony's Playstation 3 controller uses buttons for the D-pad
				i32 aIndex = (bIndex - KC_GP_AXIS_LS_RIGHT);
				bool left = aIndex % 2 == 1;
				aIndex /= 2;
				handleButton(axisPush[bIndex - KC_GP_AXIS_LS_RIGHT], ev.value != 0, bIndex,
							 rawInputDevice->rawInput, index);
				if (ev.value) {
					axis.array[aIndex] = left ? -1.0f : 1.0f;
				} else {
					axis.array[aIndex] = 0.0f;
				}
			} else if (bIndex < KC_GP_BTN_A || bIndex > KC_GP_AXIS_H0_UP) {
				continue; // Unsupported
			}
			rawInputDevice->rawInput->AnyGPCode = bIndex;
			if (ev.value) {
				rawInputDevice->rawInput->AnyGP.state = BUTTON_PRESSED_BIT;
				button[bIndex - KC_GP_BTN_A].Press();
			} else {
				rawInputDevice->rawInput->AnyGP.state = BUTTON_RELEASED_BIT;
				button[bIndex - KC_GP_BTN_A].Release();
			}
			rawInputDevice->rawInput->AnyGPIndex = index;
		}
		break;
		}
	}
	if (axis.vec.H0.x != 0.0f && axis.vec.H0.y != 0.0f) {
		axis.vec.H0 = normalize(axis.vec.H0);
		// cout.PrintLn("H0.x = ",  axis.vec.H0.x,  ", H0.y = ",  axis.vec.H0.y);
	}
	handleButton(hat[0], axis.vec.H0.x > 0.0f && axis.vec.H0.y < 0.0f, KC_GP_AXIS_H0_UP_RIGHT,
				 rawInputDevice->rawInput, index);
	handleButton(hat[1], axis.vec.H0.x > 0.0f && axis.vec.H0.y > 0.0f, KC_GP_AXIS_H0_DOWN_RIGHT,
				 rawInputDevice->rawInput, index);
	handleButton(hat[2], axis.vec.H0.x < 0.0f && axis.vec.H0.y > 0.0f, KC_GP_AXIS_H0_DOWN_LEFT,
				 rawInputDevice->rawInput, index);
	handleButton(hat[3], axis.vec.H0.x < 0.0f && axis.vec.H0.y < 0.0f, KC_GP_AXIS_H0_UP_LEFT,
				 rawInputDevice->rawInput, index);
#ifdef IO_GAMEPAD_LOGGING_VERBOSE
	for (u32 i = 0; i < IO_GAMEPAD_MAX_AXES; i++) {
		if (axisPush[i * 2].Pressed()) {
			cout.PrintLn("Pressed ",  KeyCodeName(i * 2 + KC_GP_AXIS_LS_RIGHT));
		}
		if (axisPush[i * 2 + 1].Pressed()) {
			cout.PrintLn("Pressed ",  KeyCodeName(i * 2 + 1 + KC_GP_AXIS_LS_RIGHT));
		}
		if (axisPush[i * 2].Released()) {
			cout.PrintLn("Released ",  KeyCodeName(i * 2 + KC_GP_AXIS_LS_RIGHT));
		}
		if (axisPush[i * 2 + 1].Released()) {
			cout.PrintLn("Released ",  KeyCodeName(i * 2 + 1 + KC_GP_AXIS_LS_RIGHT));
		}
	}
	for (u32 i = 0; i < 4; i++) {
		if (hat[i].Pressed()) {
			cout.PrintLn("Pressed ",  KeyCodeName(i + KC_GP_AXIS_H0_UP_RIGHT));
		}
		if (hat[i].Released()) {
			cout.PrintLn("Released ",  KeyCodeName(i + KC_GP_AXIS_H0_UP_RIGHT));
		}
	}
	for (u32 i = 0; i < IO_GAMEPAD_MAX_BUTTONS; i++) {
		if (button[i].Pressed()) {
			cout.PrintLn("Pressed ",  KeyCodeName(i + KC_GP_BTN_A));
		}
		if (button[i].Released()) {
			cout.PrintLn("Released ",  KeyCodeName(i + KC_GP_BTN_A));
		}
	}
#endif
}

} // namespace io

} // namespace AzCore
