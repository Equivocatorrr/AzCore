/*
	File: WindowsHeaderPredefines.h
	Author: Philip Haynes
	Just common definitions that go before including Windows.h
*/

#ifndef NOMINMAX
	#define NOMINMAX
#endif
#ifndef NODRAWTEXT
	#define NODRAWTEXT
#endif
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef WINVER
	#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0A00
#endif