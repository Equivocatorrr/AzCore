/*
	File: Environment.cpp
	Author: Philip Haynes
*/

#include "Environment.hpp"
#include <cstdlib>

namespace AzCore {

String ConfigDir() {
	String out;
	#ifdef __unix
		char *var = getenv("HOME");
		if (var) {
			out = var;
			out += "/.config/";
		}
	#elif defined(_WIN32)
		char *var = getenv("APPDATA");
		if (var) {
			out = var;
			out += "/";
		}
	#endif
	return out;
}

String DataDir() {
	String out;
	#ifdef __unix
		char *var = getenv("HOME");
		if (var) {
			out = var;
			out += "/.local/share/";
		}
	#elif defined(_WIN32)
		char *var = getenv("LOCALAPPDATA");
		if (var) {
			out = var;
			out += "/";
		}
	#endif
	return out;
}

bool isSlash(char c) {
	return c == '\\' || c == '/';
}

void CleanFilePath(String *path) {
	i32 lastDir = -1;
	for (i32 i = 0; i < path->size-3; i++) {
		char c1 = (*path)[i];
		char c2 = (*path)[i+1];
		char c3 = (*path)[i+2];
		char c4 = (*path)[i+3];
		if (isSlash(c1) && c2 == '.') {
			if (c3 == '.' && isSlash(c4)) {
				// dir/../dir
				// ^^^^^^^
				path->Erase(lastDir+1, i+3 - lastDir);
			} else if (isSlash(c3)) {
				// dir/./dir
				//	^^
				path->Erase(i, 2);
			}
		} else {
			if (isSlash(c1)) {
				lastDir = i;
			}
		}
	}
}

} // namespace AzCore
