/*
	File: vt_strings.hpp
	Author: Philip Haynes
	Commonly used virtual table codes for coloring and styling console output.
*/

#ifndef AZCORE_VT_STRINGS_HPP
#define AZCORE_VT_STRINGS_HPP

#include <AzCore/Memory/String.hpp>

namespace AzCore {

enum vt_code {
	VT_RESET=0,
	VT_FG_BLACK,
	VT_FG_DK_RED,
	VT_FG_DK_GREEN,
	VT_FG_DK_YELLOW,
	VT_FG_DK_BLUE,
	VT_FG_DK_MAGENTA,
	VT_FG_DK_CYAN,
	VT_FG_LT_GRAY,
	VT_FG_DK_GRAY,
	VT_FG_RED,
	VT_FG_GREEN,
	VT_FG_YELLOW,
	VT_FG_BLUE,
	VT_FG_MAGENTA,
	VT_FG_CYAN,
	VT_FG_WHITE,
	VT_BG_BLACK,
	VT_BG_DK_RED,
	VT_BG_DK_GREEN,
	VT_BG_DK_YELLOW,
	VT_BG_DK_BLUE,
	VT_BG_DK_MAGENTA,
	VT_BG_DK_CYAN,
	VT_BG_LT_GRAY,
	VT_BG_DK_GRAY,
	VT_BG_RED,
	VT_BG_GREEN,
	VT_BG_YELLOW,
	VT_BG_BLUE,
	VT_BG_MAGENTA,
	VT_BG_CYAN,
	VT_BG_WHITE,
};

static const Str vt[] = {
	/*[VT_RESET]          = */ "\033[0m",
	/*[VT_FG_BLACK]       = */ "\033[30m",
	/*[VT_FG_DK_RED]      = */ "\033[31m",
	/*[VT_FG_DK_GREEN]    = */ "\033[32m",
	/*[VT_FG_DK_YELLOW]   = */ "\033[33m",
	/*[VT_FG_DK_BLUE]     = */ "\033[34m",
	/*[VT_FG_DK_MAGENTA]  = */ "\033[35m",
	/*[VT_FG_DK_CYAN]     = */ "\033[36m",
	/*[VT_FG_LT_GRAY]     = */ "\033[37m",
	/*[VT_FG_DK_GRAY]     = */ "\033[90m",
	/*[VT_FG_RED]         = */ "\033[91m",
	/*[VT_FG_GREEN]       = */ "\033[92m",
	/*[VT_FG_YELLOW]      = */ "\033[93m",
	/*[VT_FG_BLUE]        = */ "\033[94m",
	/*[VT_FG_MAGENTA]     = */ "\033[95m",
	/*[VT_FG_CYAN]        = */ "\033[96m",
	/*[VT_FG_WHITE]       = */ "\033[97m",
	/*[VT_BG_BLACK]       = */ "\033[40m",
	/*[VT_BG_DK_RED]      = */ "\033[41m",
	/*[VT_BG_DK_GREEN]    = */ "\033[42m",
	/*[VT_BG_DK_YELLOW]   = */ "\033[43m",
	/*[VT_BG_DK_BLUE]     = */ "\033[44m",
	/*[VT_BG_DK_MAGENTA]  = */ "\033[45m",
	/*[VT_BG_DK_CYAN]     = */ "\033[46m",
	/*[VT_BG_LT_GRAY]     = */ "\033[47m",
	/*[VT_BG_DK_GRAY]     = */ "\033[100m",
	/*[VT_BG_RED]         = */ "\033[101m",
	/*[VT_BG_GREEN]       = */ "\033[102m",
	/*[VT_BG_YELLOW]      = */ "\033[103m",
	/*[VT_BG_BLUE]        = */ "\033[104m",
	/*[VT_BG_MAGENTA]     = */ "\033[105m",
	/*[VT_BG_CYAN]        = */ "\033[106m",
	/*[VT_BG_WHITE]       = */ "\033[107m",
};

inline Str vt_str(vt_code code) {
	return vt[code];
}

// Be sure r, g, and b are between 0 and 5
inline String vt_fg_rgb6(u8 r, u8 g, u8 b) {
	AzAssert(r < 6 && g < 6 && b < 6, "vt_fg_rgb6 value out of range.");
	i32 code = 16 + 36 * r + 6 * g + b;
	return Stringify("\033[38:5:", code, "m");
}

// Be sure r, g, and b are between 0 and 5
inline String vt_bg_rgb6(u8 r, u8 g, u8 b) {
	AzAssert(r < 6 && g < 6 && b < 6, "vt_bg_rgb6 value out of range.");
	i32 code = 16 + 36 * r + 6 * g + b;
	return Stringify("\033[48:5:", code, "m");
}

// 24-bit true color
inline String vt_fg_rgb(u8 r, u8 g, u8 b) {
	return Stringify("\033[38;2;", r, ";", g, ";", b, "m");
}
// 24-bit true color
inline String vt_bg_rgb(u8 r, u8 g, u8 b) {
	return Stringify("\033[48;2;", r, ";", g, ";", b, "m");
}

// inline String vt_span(String string, vt_code code) {
// 	String out = vt[code];
// 	out += string;
// 	out += vt[VT_RESET];
// 	return out;
// }
//
// inline String vt_span(String string, String code) {
// 	String out = code;
// 	out += string;
// 	out += vt[VT_RESET];
// 	return out;
// }
#define vt_span(code, ...) vt[code], __VA_ARGS__, vt[VT_RESET]

} // namespace AzCore

#endif // AZCORE_VT_STRINGS_HPP
