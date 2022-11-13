/*
	File: keycodes.hpp
	Author: Philip Haynes
	An intermediate representation of various inputs, used for
	cross-platform referencing of keyboard, mouse, and gamepad inputs.
	Based on USB HID scancodes, and modified to represent the mouse and gamepad at the same time.
*/
#ifndef AZCORE_KEYCODES_HPP
#define AZCORE_KEYCODES_HPP

#include "basictypes.hpp"
#include "IO/Log.hpp"

namespace AzCore {

enum {
	KC_KEY_NONE                = 0x00, // No key pressed
	KC_KEY_ERR                 = 0x01, // Keyboard Error Roll Over - used for all slots if too many keys are pressed ("Phantom key")
	KC_KEY_A                   = 0x04, // Keyboard a and A
	KC_KEY_B                   = 0x05, // Keyboard b and B
	KC_KEY_C                   = 0x06, // Keyboard c and C
	KC_KEY_D                   = 0x07, // Keyboard d and D
	KC_KEY_E                   = 0x08, // Keyboard e and E
	KC_KEY_F                   = 0x09, // Keyboard f and F
	KC_KEY_G                   = 0x0a, // Keyboard g and G
	KC_KEY_H                   = 0x0b, // Keyboard h and H
	KC_KEY_I                   = 0x0c, // Keyboard i and I
	KC_KEY_J                   = 0x0d, // Keyboard j and J
	KC_KEY_K                   = 0x0e, // Keyboard k and K
	KC_KEY_L                   = 0x0f, // Keyboard l and L
	KC_KEY_M                   = 0x10, // Keyboard m and M
	KC_KEY_N                   = 0x11, // Keyboard n and N
	KC_KEY_O                   = 0x12, // Keyboard o and O
	KC_KEY_P                   = 0x13, // Keyboard p and P
	KC_KEY_Q                   = 0x14, // Keyboard q and Q
	KC_KEY_R                   = 0x15, // Keyboard r and R
	KC_KEY_S                   = 0x16, // Keyboard s and S
	KC_KEY_T                   = 0x17, // Keyboard t and T
	KC_KEY_U                   = 0x18, // Keyboard u and U
	KC_KEY_V                   = 0x19, // Keyboard v and V
	KC_KEY_W                   = 0x1a, // Keyboard w and W
	KC_KEY_X                   = 0x1b, // Keyboard x and X
	KC_KEY_Y                   = 0x1c, // Keyboard y and Y
	KC_KEY_Z                   = 0x1d, // Keyboard z and Z

	KC_KEY_1                   = 0x1e, // Keyboard 1 and !
	KC_KEY_2                   = 0x1f, // Keyboard 2 and @
	KC_KEY_3                   = 0x20, // Keyboard 3 and #
	KC_KEY_4                   = 0x21, // Keyboard 4 and $
	KC_KEY_5                   = 0x22, // Keyboard 5 and %
	KC_KEY_6                   = 0x23, // Keyboard 6 and ^
	KC_KEY_7                   = 0x24, // Keyboard 7 and &
	KC_KEY_8                   = 0x25, // Keyboard 8 and *
	KC_KEY_9                   = 0x26, // Keyboard 9 and (
	KC_KEY_0                   = 0x27, // Keyboard 0 and )

	KC_KEY_ENTER               = 0x28, // Keyboard Return (ENTER)
	KC_KEY_ESC                 = 0x29, // Keyboard ESCAPE
	KC_KEY_BACKSPACE           = 0x2a, // Keyboard DELETE (Backspace)
	KC_KEY_TAB                 = 0x2b, // Keyboard Tab
	KC_KEY_SPACE               = 0x2c, // Keyboard Spacebar
	KC_KEY_MINUS               = 0x2d, // Keyboard - and _
	KC_KEY_EQUAL               = 0x2e, // Keyboard = and +
	KC_KEY_LEFTBRACE           = 0x2f, // Keyboard [ and {
	KC_KEY_RIGHTBRACE          = 0x30, // Keyboard ] and }
	KC_KEY_BACKSLASH           = 0x31, // Keyboard \ and |
	KC_KEY_HASHTILDE           = 0x32, // Keyboard Non-US # and ~
	KC_KEY_SEMICOLON           = 0x33, // Keyboard ; and :
	KC_KEY_APOSTROPHE          = 0x34, // Keyboard ' and "
	KC_KEY_GRAVE               = 0x35, // Keyboard ` and ~
	KC_KEY_COMMA               = 0x36, // Keyboard , and <
	KC_KEY_DOT                 = 0x37, // Keyboard . and >
	KC_KEY_SLASH               = 0x38, // Keyboard / and ?
	KC_KEY_CAPSLOCK            = 0x39, // Keyboard Caps Lock

	KC_KEY_F1                  = 0x3a, // Keyboard F1
	KC_KEY_F2                  = 0x3b, // Keyboard F2
	KC_KEY_F3                  = 0x3c, // Keyboard F3
	KC_KEY_F4                  = 0x3d, // Keyboard F4
	KC_KEY_F5                  = 0x3e, // Keyboard F5
	KC_KEY_F6                  = 0x3f, // Keyboard F6
	KC_KEY_F7                  = 0x40, // Keyboard F7
	KC_KEY_F8                  = 0x41, // Keyboard F8
	KC_KEY_F9                  = 0x42, // Keyboard F9
	KC_KEY_F10                 = 0x43, // Keyboard F10
	KC_KEY_F11                 = 0x44, // Keyboard F11
	KC_KEY_F12                 = 0x45, // Keyboard F12

	KC_KEY_SYSRQ               = 0x46, // Keyboard Print Screen
	KC_KEY_SCROLLLOCK          = 0x47, // Keyboard Scroll Lock
	KC_KEY_PAUSE               = 0x48, // Keyboard Pause
	KC_KEY_INSERT              = 0x49, // Keyboard Insert
	KC_KEY_HOME                = 0x4a, // Keyboard Home
	KC_KEY_PAGEUP              = 0x4b, // Keyboard Page Up
	KC_KEY_DELETE              = 0x4c, // Keyboard Delete Forward
	KC_KEY_END                 = 0x4d, // Keyboard End
	KC_KEY_PAGEDOWN            = 0x4e, // Keyboard Page Down
	KC_KEY_RIGHT               = 0x4f, // Keyboard Right Arrow
	KC_KEY_LEFT                = 0x50, // Keyboard Left Arrow
	KC_KEY_DOWN                = 0x51, // Keyboard Down Arrow
	KC_KEY_UP                  = 0x52, // Keyboard Up Arrow

	KC_KEY_NUMLOCK             = 0x53, // Keyboard Num Lock and Clear
	KC_KEY_KPSLASH             = 0x54, // Keypad /
	KC_KEY_KPASTERISK          = 0x55, // Keypad *
	KC_KEY_KPMINUS             = 0x56, // Keypad -
	KC_KEY_KPPLUS              = 0x57, // Keypad +
	KC_KEY_KPENTER             = 0x58, // Keypad ENTER
	KC_KEY_KP1                 = 0x59, // Keypad 1 and End
	KC_KEY_KP2                 = 0x5a, // Keypad 2 and Down Arrow
	KC_KEY_KP3                 = 0x5b, // Keypad 3 and PageDn
	KC_KEY_KP4                 = 0x5c, // Keypad 4 and Left Arrow
	KC_KEY_KP5                 = 0x5d, // Keypad 5
	KC_KEY_KP6                 = 0x5e, // Keypad 6 and Right Arrow
	KC_KEY_KP7                 = 0x5f, // Keypad 7 and Home
	KC_KEY_KP8                 = 0x60, // Keypad 8 and Up Arrow
	KC_KEY_KP9                 = 0x61, // Keypad 9 and Page Up
	KC_KEY_KP0                 = 0x62, // Keypad 0 and Insert
	KC_KEY_KPDOT               = 0x63, // Keypad . and Delete

	KC_KEY_102ND               = 0x64, // Keyboard Non-US \ and |
	KC_KEY_COMPOSE             = 0x65, // Keyboard Application
	KC_KEY_POWER               = 0x66, // Keyboard Power
	KC_KEY_KPEQUAL             = 0x67, // Keypad =

	KC_KEY_F13                 = 0x68, // Keyboard F13
	KC_KEY_F14                 = 0x69, // Keyboard F14
	KC_KEY_F15                 = 0x6a, // Keyboard F15
	KC_KEY_F16                 = 0x6b, // Keyboard F16
	KC_KEY_F17                 = 0x6c, // Keyboard F17
	KC_KEY_F18                 = 0x6d, // Keyboard F18
	KC_KEY_F19                 = 0x6e, // Keyboard F19
	KC_KEY_F20                 = 0x6f, // Keyboard F20
	KC_KEY_F21                 = 0x70, // Keyboard F21
	KC_KEY_F22                 = 0x71, // Keyboard F22
	KC_KEY_F23                 = 0x72, // Keyboard F23
	KC_KEY_F24                 = 0x73, // Keyboard F24

	KC_KEY_OPEN                = 0x74, // Keyboard Execute
	KC_KEY_HELP                = 0x75, // Keyboard Help
	KC_KEY_PROPS               = 0x76, // Keyboard Menu
	KC_KEY_FRONT               = 0x77, // Keyboard Select
	KC_KEY_STOP                = 0x78, // Keyboard Stop
	KC_KEY_AGAIN               = 0x79, // Keyboard Again
	KC_KEY_UNDO                = 0x7a, // Keyboard Undo
	KC_KEY_CUT                 = 0x7b, // Keyboard Cut
	KC_KEY_COPY                = 0x7c, // Keyboard Copy
	KC_KEY_PASTE               = 0x7d, // Keyboard Paste
	KC_KEY_FIND                = 0x7e, // Keyboard Find
	KC_KEY_MUTE                = 0x7f, // Keyboard Mute
	KC_KEY_VOLUMEUP            = 0x80, // Keyboard Volume Up
	KC_KEY_VOLUMEDOWN          = 0x81, // Keyboard Volume Down
// 0x82  Keyboard Locking Caps Lock
// 0x83  Keyboard Locking Num Lock
// 0x84  Keyboard Locking Scroll Lock
	KC_KEY_KPCOMMA             = 0x85,
// 0x86  Keypad Equal Sign
	KC_KEY_RO                  = 0x87, // Keyboard International1
	KC_KEY_KATAKANAHIRAGANA    = 0x88, // Keyboard International2
	KC_KEY_YEN                 = 0x89, // Keyboard International3
	KC_KEY_HENKAN              = 0x8a, // Keyboard International4
	KC_KEY_MUHENKAN            = 0x8b, // Keyboard International5
	KC_KEY_KPJPCOMMA           = 0x8c, // Keyboard International6
// 0x8d  Keyboard International7
// 0x8e  Keyboard International8
// 0x8f  Keyboard International9
	KC_KEY_HANGEUL             = 0x90, // Keyboard LANG1
	KC_KEY_HANJA               = 0x91, // Keyboard LANG2
	KC_KEY_KATAKANA            = 0x92, // Keyboard LANG3
	KC_KEY_HIRAGANA            = 0x93, // Keyboard LANG4
	KC_KEY_ZENKAKUHANKAKU      = 0x94, // Keyboard LANG5
// 0x95  Keyboard LANG6
// 0x96  Keyboard LANG7
// 0x97  Keyboard LANG8
// 0x98  Keyboard LANG9
// 0x99  Keyboard Alternate Erase
// 0x9a  Keyboard SysReq/Attention
// 0x9b  Keyboard Cancel
// 0x9c  Keyboard Clear
// 0x9d  Keyboard Prior
// 0x9e  Keyboard Return
// 0x9f  Keyboard Separator
// 0xa0  Keyboard Out
// 0xa1  Keyboard Oper
// 0xa2  Keyboard Clear/Again
// 0xa3  Keyboard CrSel/Props
// 0xa4  Keyboard ExSel

// Special keycodes for mouse input
	KC_MOUSE_LEFT              = 0xa6,
	KC_MOUSE_RIGHT             = 0xa7,
	KC_MOUSE_MIDDLE            = 0xa8,
	KC_MOUSE_XONE              = 0xa9,
	KC_MOUSE_XTWO              = 0xaa,
	KC_MOUSE_SCROLLUP          = 0xab,
	KC_MOUSE_SCROLLDOWN        = 0xac,
	KC_MOUSE_SCROLLLEFT        = 0xad,
	KC_MOUSE_SCROLLRIGHT       = 0xae,

// 0xb0  Keypad 00
// 0xb1  Keypad 000
// 0xb2  Thousands Separator
// 0xb3  Decimal Separator
// 0xb4  Currency Unit
// 0xb5  Currency Sub-unit
	KC_KEY_KPLEFTPAREN         = 0xb6, // Keypad (
	KC_KEY_KPRIGHTPAREN        = 0xb7, // Keypad )
// 0xb8  Keypad {
// 0xb9  Keypad }
// 0xba  Keypad Tab
// 0xbb  Keypad Backspace
// 0xbc  Keypad A
// 0xbd  Keypad B
// 0xbe  Keypad C
// 0xbf  Keypad D
// 0xc0  Keypad E
// 0xc1  Keypad F
// 0xc2  Keypad XOR
// 0xc3  Keypad ^
// 0xc4  Keypad %
// 0xc5  Keypad <
// 0xc6  Keypad >
// 0xc7  Keypad &
// 0xc8  Keypad &&
// 0xc9  Keypad |
// 0xca  Keypad ||
// 0xcb  Keypad :
// 0xcc  Keypad #
// 0xcd  Keypad Space
// 0xce  Keypad @
// 0xcf  Keypad !
// 0xd0  Keypad Memory Store
// 0xd1  Keypad Memory Recall
// 0xd2  Keypad Memory Clear
// 0xd3  Keypad Memory Add
// 0xd4  Keypad Memory Subtract
// 0xd5  Keypad Memory Multiply
// 0xd6  Keypad Memory Divide
// 0xd7  Keypad +/-
// 0xd8  Keypad Clear
// 0xd9  Keypad Clear Entry
// 0xda  Keypad Binary
// 0xdb  Keypad Octal
// 0xdc  Keypad Decimal
// 0xdd  Keypad Hexadecimal

// Special keycodes for GamePads, replacing unused Keypad definitions
	KC_GP_AXIS_H0_UP_RIGHT     = 0xb2, // GamePad Hat (D-pad) Northeast
	KC_GP_AXIS_H0_DOWN_RIGHT   = 0xb3, // GamePad Hat (D-pad) Southeast
	KC_GP_AXIS_H0_DOWN_LEFT    = 0xb4, // GamePad Hat (D-pad) Southwest
	KC_GP_AXIS_H0_UP_LEFT      = 0xb5, // GamePad Hat (D-pad) Northwest

	KC_GP_BTN_SOUTH            = 0xb8, // GamePad A
	KC_GP_BTN_A = KC_GP_BTN_SOUTH,
	KC_GP_BTN_EAST             = 0xb9, // GamePad B
	KC_GP_BTN_B = KC_GP_BTN_EAST,
	KC_GP_BTN_C                = 0xba, // GamePad C
	KC_GP_BTN_NORTH            = 0xbb, // GamePad X
	KC_GP_BTN_X = KC_GP_BTN_NORTH,
	KC_GP_BTN_WEST             = 0xbc, // GamePad Y
	KC_GP_BTN_Y = KC_GP_BTN_WEST,
	KC_GP_BTN_Z                = 0xbd, // GamePad Z
	KC_GP_BTN_TL               = 0xbe, // GamePad Left Trigger
	KC_GP_BTN_TR               = 0xbf, // GamePad Right Trigger
	KC_GP_BTN_TL2              = 0xc0, // GamePad Left Trigger 2
	KC_GP_BTN_TR2              = 0xc1, // GamePad Right Trigger 2
	KC_GP_BTN_SELECT           = 0xc2, // GamePad Select/Back
	KC_GP_BTN_START            = 0xc3, // GamePad Start
	KC_GP_BTN_MODE             = 0xc4, // GamePad Mode
	KC_GP_BTN_THUMBL           = 0xc5, // GamePad Left Thumbstick Press
	KC_GP_BTN_THUMBR           = 0xc6, // GamePad Right Thumbstick Press
	KC_GP_AXIS_LS_RIGHT        = 0xc7, // GamePad Left Thumbstick Right
	KC_GP_AXIS_LS_LEFT         = 0xc8, // GamePad Left Thumbstick Left
	KC_GP_AXIS_LS_DOWN         = 0xc9, // GamePad Left Thumbstick Down
	KC_GP_AXIS_LS_UP           = 0xca, // GamePad Left Thumbstick Up
	KC_GP_AXIS_LT_IN           = 0xcb, // GamePad Left Trigger Pull
// 0xcc  Gamepad Axis Left Trigger Out
	KC_GP_AXIS_RS_RIGHT        = 0xcd, // GamePad Right Thumbstick Right
	KC_GP_AXIS_RS_LEFT         = 0xce, // GamePad Right Thumbstick Left
	KC_GP_AXIS_RS_DOWN         = 0xcf, // GamePad Right Thumbstick Down
	KC_GP_AXIS_RS_UP           = 0xd0, // GamePad Right Thumbstick Up
	KC_GP_AXIS_RT_IN           = 0xd1, // GamePad Right Trigger Pull
// 0xd2  Gamepad Axis Right Trigger Out
	KC_GP_AXIS_H0_RIGHT        = 0xd3, // GamePad Hat (D-pad) Right
	KC_GP_AXIS_H0_LEFT         = 0xd4, // GamePad Hat (D-pad) Left
	KC_GP_AXIS_H0_DOWN         = 0xd5, // GamePad Hat (D-pad) Down
	KC_GP_AXIS_H0_UP           = 0xd6, // GamePad Hat (D-pad) Up
	KC_GP_AXIS_LS_X            = 0xd7, // GamePad Left Thumbstick X-axis
	KC_GP_AXIS_LS_Y            = 0xd8, // GamePad Left Thumbstick Y-axis
	KC_GP_AXIS_LT              = 0xd9, // GamePad Left Trigger Axis
	KC_GP_AXIS_RS_X            = 0xda, // GamePad Right Thumbstick X-axis
	KC_GP_AXIS_RS_Y            = 0xdb, // GamePad Right Thumbstick Y-axis
	KC_GP_AXIS_RT              = 0xdc, // GamePad Right Trigger Axis
	KC_GP_AXIS_H0_X            = 0xdd, // GamePad Hat (D-pad) X-axis
	KC_GP_AXIS_H0_Y            = 0xde, // GamePad Hat (D-pad) Y-axis
// 0xdf  Unused
	KC_KEY_LEFTCTRL            = 0xe0, // Keyboard Left Control
	KC_KEY_LEFTSHIFT           = 0xe1, // Keyboard Left Shift
	KC_KEY_LEFTALT             = 0xe2, // Keyboard Left Alt
	KC_KEY_LEFTMETA            = 0xe3, // Keyboard Left GUI
	KC_KEY_RIGHTCTRL           = 0xe4, // Keyboard Right Control
	KC_KEY_RIGHTSHIFT          = 0xe5, // Keyboard Right Shift
	KC_KEY_RIGHTALT            = 0xe6, // Keyboard Right Alt
	KC_KEY_RIGHTMETA           = 0xe7, // Keyboard Right GUI

	KC_KEY_MEDIA_PLAYPAUSE     = 0xe8,
	KC_KEY_MEDIA_STOPCD        = 0xe9,
	KC_KEY_MEDIA_PREVIOUSSONG  = 0xea,
	KC_KEY_MEDIA_NEXTSONG      = 0xeb,
	KC_KEY_MEDIA_EJECTCD       = 0xec,
	KC_KEY_MEDIA_VOLUMEUP      = 0xed,
	KC_KEY_MEDIA_VOLUMEDOWN    = 0xee,
	KC_KEY_MEDIA_MUTE          = 0xef,
	KC_KEY_MEDIA_WWW           = 0xf0,
	KC_KEY_MEDIA_BACK          = 0xf1,
	KC_KEY_MEDIA_FORWARD       = 0xf2,
	KC_KEY_MEDIA_STOP          = 0xf3,
	KC_KEY_MEDIA_FIND          = 0xf4,
	KC_KEY_MEDIA_SCROLLUP      = 0xf5,
	KC_KEY_MEDIA_SCROLLDOWN    = 0xf6,
	KC_KEY_MEDIA_EDIT          = 0xf7,
	KC_KEY_MEDIA_SLEEP         = 0xf8,
	KC_KEY_MEDIA_COFFEE        = 0xf9,
	KC_KEY_MEDIA_REFRESH       = 0xfa,
	KC_KEY_MEDIA_CALC          = 0xfb,
// These codes are actually way beyond the scope of a u8, but I'm moving them here.
	KC_KEY_MEDIA_MAIL          = 0xfc,
	KC_KEY_MEDIA_FILE          = 0xfd,
};

const char* KeyCodeName(u8 keyCode);

u8 KeyCodeToEvdev(u8 keyCode);
u8 KeyCodeFromEvdev(u8 keyCode);
u8 KeyCodeToWinVK(u8 keyCode);
u8 KeyCodeFromWinVK(u8 keyCode);
u8 KeyCodeToWinScan(u8 keyCode);
u8 KeyCodeFromWinScan(u8 keyCode);

namespace io {
	class Log;
}

inline bool KeyCodeIsKeyboard(u8 keyCode) {
	return (keyCode >= KC_KEY_A && keyCode <= KC_KEY_ZENKAKUHANKAKU) || keyCode == KC_KEY_KPLEFTPAREN || keyCode == KC_KEY_KPRIGHTPAREN || (keyCode >= KC_KEY_LEFTCTRL && keyCode <= KC_KEY_MEDIA_FILE);
}

inline bool KeyCodeIsMouse(u8 keyCode) {
	return keyCode >= KC_MOUSE_LEFT && keyCode <= KC_MOUSE_SCROLLRIGHT;
}

inline bool KeyCodeIsGamepad(u8 keyCode) {
	return (keyCode >= KC_GP_AXIS_H0_UP_RIGHT && keyCode <= KC_GP_AXIS_H0_UP_LEFT)
		|| (keyCode >= KC_GP_BTN_SOUTH && keyCode <= KC_GP_AXIS_H0_Y);
}

// Utility functions so you don't have to write both to and from maps
// Creates the inverse based on the "To" map
// This might not work if there are multiple mappings to KeyCodes
// And with multiple mappings from KeyCodes, it will use the first.
void PrintKeyCodeMapsEvdev(io::Log& cout);
void PrintKeyCodeMapsWinVK(io::Log& cout);
void PrintKeyCodeMapsWinScan(io::Log& cout);

} // namespace AzCore

#endif // AZCORE_KEYCODES_HPP
