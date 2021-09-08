/*
	File: Sockets.cpp
	Author: Philip Haynes
*/

#ifdef __unix
#include "Linux/Sockets.cpp"
#elif defined(_WIN32)
#include "Win32/Sockets.cpp"
#endif
