/* Copyright 2011-2012 Dietrich Epp <depp@zdome.net>
   See LICENSE.txt for details.  */
#ifndef KEYCODE_KEYTABLE_HPP
#define KEYCODE_KEYTABLE_HPP

extern const unsigned char MAC_HID_TO_NATIVE[256];
extern const unsigned char MAC_NATIVE_TO_HID[128];

extern const unsigned char EVDEV_HID_TO_NATIVE[256];
extern const unsigned char EVDEV_NATIVE_TO_HID[256];

extern const unsigned char WIN_HID_TO_NATIVE[256];
extern const unsigned char WIN_NATIVE_TO_HID[256];
extern const unsigned char WIN_HID_TO_SCAN[256];
extern const unsigned char WIN_SCAN_TO_HID[256];

extern const char *HID_KEYCODE_NAMES[256];

#endif
