/*
	File: WindowsHeaderCleanup.h
	Author: Philip Haynes
	Goes after the last Windows header include. Can be included multiple times if need be.
*/

#ifdef near
	#undef near
#endif
#ifdef far
	#undef far
#endif
#ifdef min
	#undef min
#endif
#ifdef max
	#undef max
#endif
#ifdef OPAQUE
	#undef OPAQUE
#endif
#ifdef TRANSPARENT
	#undef TRANSPARENT
#endif
#ifdef TRUE
	#undef TRUE
#endif
#ifdef FALSE
	#undef FALSE
#endif
#ifdef DELETE
	#undef DELETE
#endif
#ifdef DrawText
	#undef DrawText
#endif
#ifdef Yield // This one is especially sinister... defined to nothing
	#undef Yield
#endif