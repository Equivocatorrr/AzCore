/*
	File: Windows.h
	An attempt at making it more sane to include Windows.h
	If you define WINVER and _WIN32_WINNT before including this, we'll honor your choices, else defaults to Windows 10.
	Will try to #undef some of the more annoying #defines that Michaelsoft decided everyone needed unconditionally.
	Deliberately made easy for you to add more Windows headers by just using Predefines and Cleanup directly.
*/

#include "WindowsHeaderPredefines.h"
#include <windows.h>
#include "WindowsHeaderCleanup.h"