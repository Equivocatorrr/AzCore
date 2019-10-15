/*
    File: keycodes.hpp
    Author: Philip Haynes
    An intermediate representation of various inputs, used for
    cross-platform referencing of keyboard, mouse, and gamepad inputs.
    Based on USB HID scancodes, and modified to represent the mouse and gamepad at the same time.
*/
#ifndef KEYCODES_HPP
#define KEYCODES_HPP

#include "basictypes.hpp"

const u8 KC_KEY_NONE                = 0x00; // No key pressed
const u8 KC_KEY_ERR                 = 0x01; // Keyboard Error Roll Over - used for all slots if too many keys are pressed ("Phantom key")
const u8 KC_KEY_A                   = 0x04; // Keyboard a and A
const u8 KC_KEY_B                   = 0x05; // Keyboard b and B
const u8 KC_KEY_C                   = 0x06; // Keyboard c and C
const u8 KC_KEY_D                   = 0x07; // Keyboard d and D
const u8 KC_KEY_E                   = 0x08; // Keyboard e and E
const u8 KC_KEY_F                   = 0x09; // Keyboard f and F
const u8 KC_KEY_G                   = 0x0a; // Keyboard g and G
const u8 KC_KEY_H                   = 0x0b; // Keyboard h and H
const u8 KC_KEY_I                   = 0x0c; // Keyboard i and I
const u8 KC_KEY_J                   = 0x0d; // Keyboard j and J
const u8 KC_KEY_K                   = 0x0e; // Keyboard k and K
const u8 KC_KEY_L                   = 0x0f; // Keyboard l and L
const u8 KC_KEY_M                   = 0x10; // Keyboard m and M
const u8 KC_KEY_N                   = 0x11; // Keyboard n and N
const u8 KC_KEY_O                   = 0x12; // Keyboard o and O
const u8 KC_KEY_P                   = 0x13; // Keyboard p and P
const u8 KC_KEY_Q                   = 0x14; // Keyboard q and Q
const u8 KC_KEY_R                   = 0x15; // Keyboard r and R
const u8 KC_KEY_S                   = 0x16; // Keyboard s and S
const u8 KC_KEY_T                   = 0x17; // Keyboard t and T
const u8 KC_KEY_U                   = 0x18; // Keyboard u and U
const u8 KC_KEY_V                   = 0x19; // Keyboard v and V
const u8 KC_KEY_W                   = 0x1a; // Keyboard w and W
const u8 KC_KEY_X                   = 0x1b; // Keyboard x and X
const u8 KC_KEY_Y                   = 0x1c; // Keyboard y and Y
const u8 KC_KEY_Z                   = 0x1d; // Keyboard z and Z

const u8 KC_KEY_1                   = 0x1e; // Keyboard 1 and !
const u8 KC_KEY_2                   = 0x1f; // Keyboard 2 and @
const u8 KC_KEY_3                   = 0x20; // Keyboard 3 and #
const u8 KC_KEY_4                   = 0x21; // Keyboard 4 and $
const u8 KC_KEY_5                   = 0x22; // Keyboard 5 and %
const u8 KC_KEY_6                   = 0x23; // Keyboard 6 and ^
const u8 KC_KEY_7                   = 0x24; // Keyboard 7 and &
const u8 KC_KEY_8                   = 0x25; // Keyboard 8 and *
const u8 KC_KEY_9                   = 0x26; // Keyboard 9 and (
const u8 KC_KEY_0                   = 0x27; // Keyboard 0 and )

const u8 KC_KEY_ENTER               = 0x28; // Keyboard Return (ENTER)
const u8 KC_KEY_ESC                 = 0x29; // Keyboard ESCAPE
const u8 KC_KEY_BACKSPACE           = 0x2a; // Keyboard DELETE (Backspace)
const u8 KC_KEY_TAB                 = 0x2b; // Keyboard Tab
const u8 KC_KEY_SPACE               = 0x2c; // Keyboard Spacebar
const u8 KC_KEY_MINUS               = 0x2d; // Keyboard - and _
const u8 KC_KEY_EQUAL               = 0x2e; // Keyboard = and +
const u8 KC_KEY_LEFTBRACE           = 0x2f; // Keyboard [ and {
const u8 KC_KEY_RIGHTBRACE          = 0x30; // Keyboard ] and }
const u8 KC_KEY_BACKSLASH           = 0x31; // Keyboard \ and |
const u8 KC_KEY_HASHTILDE           = 0x32; // Keyboard Non-US # and ~
const u8 KC_KEY_SEMICOLON           = 0x33; // Keyboard ; and :
const u8 KC_KEY_APOSTROPHE          = 0x34; // Keyboard ' and "
const u8 KC_KEY_GRAVE               = 0x35; // Keyboard ` and ~
const u8 KC_KEY_COMMA               = 0x36; // Keyboard , and <
const u8 KC_KEY_DOT                 = 0x37; // Keyboard . and >
const u8 KC_KEY_SLASH               = 0x38; // Keyboard / and ?
const u8 KC_KEY_CAPSLOCK            = 0x39; // Keyboard Caps Lock

const u8 KC_KEY_F1                  = 0x3a; // Keyboard F1
const u8 KC_KEY_F2                  = 0x3b; // Keyboard F2
const u8 KC_KEY_F3                  = 0x3c; // Keyboard F3
const u8 KC_KEY_F4                  = 0x3d; // Keyboard F4
const u8 KC_KEY_F5                  = 0x3e; // Keyboard F5
const u8 KC_KEY_F6                  = 0x3f; // Keyboard F6
const u8 KC_KEY_F7                  = 0x40; // Keyboard F7
const u8 KC_KEY_F8                  = 0x41; // Keyboard F8
const u8 KC_KEY_F9                  = 0x42; // Keyboard F9
const u8 KC_KEY_F10                 = 0x43; // Keyboard F10
const u8 KC_KEY_F11                 = 0x44; // Keyboard F11
const u8 KC_KEY_F12                 = 0x45; // Keyboard F12

const u8 KC_KEY_SYSRQ               = 0x46; // Keyboard Print Screen
const u8 KC_KEY_SCROLLLOCK          = 0x47; // Keyboard Scroll Lock
const u8 KC_KEY_PAUSE               = 0x48; // Keyboard Pause
const u8 KC_KEY_INSERT              = 0x49; // Keyboard Insert
const u8 KC_KEY_HOME                = 0x4a; // Keyboard Home
const u8 KC_KEY_PAGEUP              = 0x4b; // Keyboard Page Up
const u8 KC_KEY_DELETE              = 0x4c; // Keyboard Delete Forward
const u8 KC_KEY_END                 = 0x4d; // Keyboard End
const u8 KC_KEY_PAGEDOWN            = 0x4e; // Keyboard Page Down
const u8 KC_KEY_RIGHT               = 0x4f; // Keyboard Right Arrow
const u8 KC_KEY_LEFT                = 0x50; // Keyboard Left Arrow
const u8 KC_KEY_DOWN                = 0x51; // Keyboard Down Arrow
const u8 KC_KEY_UP                  = 0x52; // Keyboard Up Arrow

const u8 KC_KEY_NUMLOCK             = 0x53; // Keyboard Num Lock and Clear
const u8 KC_KEY_KPSLASH             = 0x54; // Keypad /
const u8 KC_KEY_KPASTERISK          = 0x55; // Keypad *
const u8 KC_KEY_KPMINUS             = 0x56; // Keypad -
const u8 KC_KEY_KPPLUS              = 0x57; // Keypad +
const u8 KC_KEY_KPENTER             = 0x58; // Keypad ENTER
const u8 KC_KEY_KP1                 = 0x59; // Keypad 1 and End
const u8 KC_KEY_KP2                 = 0x5a; // Keypad 2 and Down Arrow
const u8 KC_KEY_KP3                 = 0x5b; // Keypad 3 and PageDn
const u8 KC_KEY_KP4                 = 0x5c; // Keypad 4 and Left Arrow
const u8 KC_KEY_KP5                 = 0x5d; // Keypad 5
const u8 KC_KEY_KP6                 = 0x5e; // Keypad 6 and Right Arrow
const u8 KC_KEY_KP7                 = 0x5f; // Keypad 7 and Home
const u8 KC_KEY_KP8                 = 0x60; // Keypad 8 and Up Arrow
const u8 KC_KEY_KP9                 = 0x61; // Keypad 9 and Page Up
const u8 KC_KEY_KP0                 = 0x62; // Keypad 0 and Insert
const u8 KC_KEY_KPDOT               = 0x63; // Keypad . and Delete

const u8 KC_KEY_102ND               = 0x64; // Keyboard Non-US \ and |
const u8 KC_KEY_COMPOSE             = 0x65; // Keyboard Application
const u8 KC_KEY_POWER               = 0x66; // Keyboard Power
const u8 KC_KEY_KPEQUAL             = 0x67; // Keypad =

const u8 KC_KEY_F13                 = 0x68; // Keyboard F13
const u8 KC_KEY_F14                 = 0x69; // Keyboard F14
const u8 KC_KEY_F15                 = 0x6a; // Keyboard F15
const u8 KC_KEY_F16                 = 0x6b; // Keyboard F16
const u8 KC_KEY_F17                 = 0x6c; // Keyboard F17
const u8 KC_KEY_F18                 = 0x6d; // Keyboard F18
const u8 KC_KEY_F19                 = 0x6e; // Keyboard F19
const u8 KC_KEY_F20                 = 0x6f; // Keyboard F20
const u8 KC_KEY_F21                 = 0x70; // Keyboard F21
const u8 KC_KEY_F22                 = 0x71; // Keyboard F22
const u8 KC_KEY_F23                 = 0x72; // Keyboard F23
const u8 KC_KEY_F24                 = 0x73; // Keyboard F24

const u8 KC_KEY_OPEN                = 0x74; // Keyboard Execute
const u8 KC_KEY_HELP                = 0x75; // Keyboard Help
const u8 KC_KEY_PROPS               = 0x76; // Keyboard Menu
const u8 KC_KEY_FRONT               = 0x77; // Keyboard Select
const u8 KC_KEY_STOP                = 0x78; // Keyboard Stop
const u8 KC_KEY_AGAIN               = 0x79; // Keyboard Again
const u8 KC_KEY_UNDO                = 0x7a; // Keyboard Undo
const u8 KC_KEY_CUT                 = 0x7b; // Keyboard Cut
const u8 KC_KEY_COPY                = 0x7c; // Keyboard Copy
const u8 KC_KEY_PASTE               = 0x7d; // Keyboard Paste
const u8 KC_KEY_FIND                = 0x7e; // Keyboard Find
const u8 KC_KEY_MUTE                = 0x7f; // Keyboard Mute
const u8 KC_KEY_VOLUMEUP            = 0x80; // Keyboard Volume Up
const u8 KC_KEY_VOLUMEDOWN          = 0x81; // Keyboard Volume Down
// 0x82  Keyboard Locking Caps Lock
// 0x83  Keyboard Locking Num Lock
// 0x84  Keyboard Locking Scroll Lock
const u8 KC_KEY_KPCOMMA             = 0x85;
// 0x86  Keypad Equal Sign
const u8 KC_KEY_RO                  = 0x87; // Keyboard International1
const u8 KC_KEY_KATAKANAHIRAGANA    = 0x88; // Keyboard International2
const u8 KC_KEY_YEN                 = 0x89; // Keyboard International3
const u8 KC_KEY_HENKAN              = 0x8a; // Keyboard International4
const u8 KC_KEY_MUHENKAN            = 0x8b; // Keyboard International5
const u8 KC_KEY_KPJPCOMMA           = 0x8c; // Keyboard International6
// 0x8d  Keyboard International7
// 0x8e  Keyboard International8
// 0x8f  Keyboard International9
const u8 KC_KEY_HANGEUL             = 0x90; // Keyboard LANG1
const u8 KC_KEY_HANJA               = 0x91; // Keyboard LANG2
const u8 KC_KEY_KATAKANA            = 0x92; // Keyboard LANG3
const u8 KC_KEY_HIRAGANA            = 0x93; // Keyboard LANG4
const u8 KC_KEY_ZENKAKUHANKAKU      = 0x94; // Keyboard LANG5
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
const u8 KC_MOUSE_LEFT              = 0xa6;
const u8 KC_MOUSE_RIGHT             = 0xa7;
const u8 KC_MOUSE_MIDDLE            = 0xa8;
const u8 KC_MOUSE_XONE              = 0xa9;
const u8 KC_MOUSE_XTWO              = 0xaa;
const u8 KC_MOUSE_SCROLLUP          = 0xab;
const u8 KC_MOUSE_SCROLLDOWN        = 0xac;
const u8 KC_MOUSE_SCROLLLEFT        = 0xad;
const u8 KC_MOUSE_SCROLLRIGHT       = 0xae;

// 0xb0  Keypad 00
// 0xb1  Keypad 000
// 0xb2  Thousands Separator
// 0xb3  Decimal Separator
// 0xb4  Currency Unit
// 0xb5  Currency Sub-unit
const u8 KC_KEY_KPLEFTPAREN         = 0xb6; // Keypad (
const u8 KC_KEY_KPRIGHTPAREN        = 0xb7; // Keypad )
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
const u8 KC_GP_AXIS_H0_UP_RIGHT     = 0xb2; // GamePad Hat (D-pad) Northeast
const u8 KC_GP_AXIS_H0_DOWN_RIGHT   = 0xb3; // GamePad Hat (D-pad) Southeast
const u8 KC_GP_AXIS_H0_DOWN_LEFT    = 0xb4; // GamePad Hat (D-pad) Southwest
const u8 KC_GP_AXIS_H0_UP_LEFT      = 0xb5; // GamePad Hat (D-pad) Northwest

const u8 KC_GP_BTN_SOUTH            = 0xb8; // GamePad A
const u8 KC_GP_BTN_A = KC_GP_BTN_SOUTH;
const u8 KC_GP_BTN_EAST             = 0xb9; // GamePad B
const u8 KC_GP_BTN_B = KC_GP_BTN_EAST;
const u8 KC_GP_BTN_C                = 0xba; // GamePad C
// NOTE: I swapped X and Y to reflect the layout of an Xbox controller
//       and all that follow that same design.
const u8 KC_GP_BTN_NORTH            = 0xbb; // GamePad Y
const u8 KC_GP_BTN_Y = KC_GP_BTN_NORTH;
const u8 KC_GP_BTN_WEST             = 0xbc; // GamePad X
const u8 KC_GP_BTN_X = KC_GP_BTN_WEST;
const u8 KC_GP_BTN_Z                = 0xbd; // GamePad Z
const u8 KC_GP_BTN_TL               = 0xbe; // GamePad Left Trigger
const u8 KC_GP_BTN_TR               = 0xbf; // GamePad Right Trigger
const u8 KC_GP_BTN_TL2              = 0xc0; // GamePad Left Trigger 2
const u8 KC_GP_BTN_TR2              = 0xc1; // GamePad Right Trigger 2
const u8 KC_GP_BTN_SELECT           = 0xc2; // GamePad Select/Back
const u8 KC_GP_BTN_START            = 0xc3; // GamePad Start
const u8 KC_GP_BTN_MODE             = 0xc4; // GamePad Mode
const u8 KC_GP_BTN_THUMBL           = 0xc5; // GamePad Left Thumbstick Press
const u8 KC_GP_BTN_THUMBR           = 0xc6; // GamePad Right Thumbstick Press
const u8 KC_GP_AXIS_LS_RIGHT        = 0xc7; // GamePad Left Thumbstick Right
const u8 KC_GP_AXIS_LS_LEFT         = 0xc8; // GamePad Left Thumbstick Left
const u8 KC_GP_AXIS_LS_DOWN         = 0xc9; // GamePad Left Thumbstick Down
const u8 KC_GP_AXIS_LS_UP           = 0xca; // GamePad Left Thumbstick Up
const u8 KC_GP_AXIS_LT_IN           = 0xcb; // GamePad Left Trigger Pull
// 0xcc  Gamepad Axis Left Trigger Out
const u8 KC_GP_AXIS_RS_RIGHT        = 0xcd; // GamePad Right Thumbstick Right
const u8 KC_GP_AXIS_RS_LEFT         = 0xce; // GamePad Right Thumbstick Left
const u8 KC_GP_AXIS_RS_DOWN         = 0xcf; // GamePad Right Thumbstick Down
const u8 KC_GP_AXIS_RS_UP           = 0xd0; // GamePad Right Thumbstick Up
const u8 KC_GP_AXIS_RT_IN           = 0xd1; // GamePad Right Trigger Pull
// 0xd2  Gamepad Axis Right Trigger Out
const u8 KC_GP_AXIS_H0_RIGHT        = 0xd3; // GamePad Hat (D-pad) Right
const u8 KC_GP_AXIS_H0_LEFT         = 0xd4; // GamePad Hat (D-pad) Left
const u8 KC_GP_AXIS_H0_DOWN         = 0xd5; // GamePad Hat (D-pad) Down
const u8 KC_GP_AXIS_H0_UP           = 0xd6; // GamePad Hat (D-pad) Up
const u8 KC_GP_AXIS_LS_X            = 0xd7; // GamePad Left Thumbstick X-axis
const u8 KC_GP_AXIS_LS_Y            = 0xd8; // GamePad Left Thumbstick Y-axis
const u8 KC_GP_AXIS_LT              = 0xd9; // GamePad Left Trigger Axis
const u8 KC_GP_AXIS_RS_X            = 0xda; // GamePad Right Thumbstick X-axis
const u8 KC_GP_AXIS_RS_Y            = 0xdb; // GamePad Right Thumbstick Y-axis
const u8 KC_GP_AXIS_RT              = 0xdc; // GamePad Right Trigger Axis
const u8 KC_GP_AXIS_H0_X            = 0xdd; // GamePad Hat (D-pad) X-axis
const u8 KC_GP_AXIS_H0_Y            = 0xde; // GamePad Hat (D-pad) Y-axis
// 0xdf  Unused
const u8 KC_KEY_LEFTCTRL            = 0xe0; // Keyboard Left Control
const u8 KC_KEY_LEFTSHIFT           = 0xe1; // Keyboard Left Shift
const u8 KC_KEY_LEFTALT             = 0xe2; // Keyboard Left Alt
const u8 KC_KEY_LEFTMETA            = 0xe3; // Keyboard Left GUI
const u8 KC_KEY_RIGHTCTRL           = 0xe4; // Keyboard Right Control
const u8 KC_KEY_RIGHTSHIFT          = 0xe5; // Keyboard Right Shift
const u8 KC_KEY_RIGHTALT            = 0xe6; // Keyboard Right Alt
const u8 KC_KEY_RIGHTMETA           = 0xe7; // Keyboard Right GUI

const u8 KC_KEY_MEDIA_PLAYPAUSE     = 0xe8;
const u8 KC_KEY_MEDIA_STOPCD        = 0xe9;
const u8 KC_KEY_MEDIA_PREVIOUSSONG  = 0xea;
const u8 KC_KEY_MEDIA_NEXTSONG      = 0xeb;
const u8 KC_KEY_MEDIA_EJECTCD       = 0xec;
const u8 KC_KEY_MEDIA_VOLUMEUP      = 0xed;
const u8 KC_KEY_MEDIA_VOLUMEDOWN    = 0xee;
const u8 KC_KEY_MEDIA_MUTE          = 0xef;
const u8 KC_KEY_MEDIA_WWW           = 0xf0;
const u8 KC_KEY_MEDIA_BACK          = 0xf1;
const u8 KC_KEY_MEDIA_FORWARD       = 0xf2;
const u8 KC_KEY_MEDIA_STOP          = 0xf3;
const u8 KC_KEY_MEDIA_FIND          = 0xf4;
const u8 KC_KEY_MEDIA_SCROLLUP      = 0xf5;
const u8 KC_KEY_MEDIA_SCROLLDOWN    = 0xf6;
const u8 KC_KEY_MEDIA_EDIT          = 0xf7;
const u8 KC_KEY_MEDIA_SLEEP         = 0xf8;
const u8 KC_KEY_MEDIA_COFFEE        = 0xf9;
const u8 KC_KEY_MEDIA_REFRESH       = 0xfa;
const u8 KC_KEY_MEDIA_CALC          = 0xfb;
// These codes are actually way beyond the scope of a u8, but I'm moving them here.
const u8 KC_KEY_MEDIA_MAIL          = 0xfc;
const u8 KC_KEY_MEDIA_FILE          = 0xfd;

const char* KeyCodeName(u8 keyCode);

u8 KeyCodeToEvdev(u8 keyCode);
u8 KeyCodeFromEvdev(u8 keyCode);
u8 KeyCodeToWinVK(u8 keyCode);
u8 KeyCodeFromWinVK(u8 keyCode);
u8 KeyCodeToWinScan(u8 keyCode);
u8 KeyCodeFromWinScan(u8 keyCode);

namespace io {
    class logStream;
}

inline bool KeyCodeIsKeyboard(const u8 &keyCode) {
    return (keyCode >= KC_KEY_A && keyCode <= KC_KEY_ZENKAKUHANKAKU) || keyCode == KC_KEY_KPLEFTPAREN || keyCode == KC_KEY_KPRIGHTPAREN || (keyCode >= KC_KEY_LEFTCTRL && keyCode <= KC_KEY_MEDIA_FILE);
}

inline bool KeyCodeIsMouse(const u8 &keyCode) {
    return keyCode >= KC_MOUSE_LEFT && keyCode <= KC_MOUSE_SCROLLRIGHT;
}

inline bool KeyCodeIsGamepad(const u8 &keyCode) {
    return keyCode >= KC_GP_AXIS_H0_UP_RIGHT && keyCode <= KC_GP_AXIS_H0_Y;
}

// Utility functions so you don't have to write both to and from maps
// Creates the inverse based on the "To" map
// This might not work if there are multiple mappings to KeyCodes
// And with multiple mappings from KeyCodes, it will use the first.
void PrintKeyCodeMapsEvdev(io::logStream& cout);
void PrintKeyCodeMapsWinVK(io::logStream& cout);
void PrintKeyCodeMapsWinScan(io::logStream& cout);

#endif
