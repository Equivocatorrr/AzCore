/*
	File: Environment.hpp
	Author: Philip Haynes
	Utilities for dealing with filesystem paths.
*/

#ifndef AZCORE_ENVIRONMENT_HPP
#define AZCORE_ENVIRONMENT_HPP

#include "Memory/String.hpp"

namespace AzCore {

String ConfigDir();

String DataDir();

void CleanFilePath(String *path);

} // namespace AzCore

#endif // AZCORE_ENVIRONMENT_HPP
