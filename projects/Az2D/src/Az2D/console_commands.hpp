/*
	File: console_commands.hpp
	Author: Philip Haynes
	Defines the behavior of the dev console and its commands.
*/

#ifndef CONSOLE_COMMANDS_HPP
#define CONSOLE_COMMANDS_HPP

#include "AzCore/memory.hpp"

namespace Dev {

// The return value is what's printed in the console
typedef az::String (*fp_Command)(az::Array<az::Str> arguments);

// Parses a bool from input which is allowed to be "yes", "no", "on", "off", "true", "false"
// Returns whether parsing was successful
// Places the result in dst
bool ParseBool(az::String input, bool &dst);

// Does command parsing and dispatch and returns what to print to the console
az::String HandleCommand(az::String input);

// Returns false if the command already exists
bool AddCommand(az::String name, az::String helpMessage, fp_Command command);

// Type used to define getters for global variables
// The first parameter is the pointer to the original data
// The second parameter is the name of the variable
typedef az::String (*fp_global_getter_t)(void* userdata, az::String name);
// Type used to define setters for global variables
// The first parameter is the pointer to the original data
// The second parameter is the name of the variable
// The third parameter is the argument that needs to be parsed
typedef az::String (*fp_global_setter_t)(void* userdata, az::String name, az::String argument);

az::String defaultBoolGetter(void *userdata, az::String name);
az::String defaultBoolSetter(void *userdata, az::String name, az::String argument);
az::String defaultBoolSettingsGetter(void *userdata, az::String name);
az::String defaultBoolSettingsSetter(void *userdata, az::String name, az::String argument);

az::String defaultRealGetter(void *userdata, az::String name);
az::String defaultRealSetter(void *userdata, az::String name, az::String argument);
az::String defaultRealSettingsGetter(void *userdata, az::String name);
az::String defaultRealSettingsSetter(void *userdata, az::String name, az::String argument);

az::String defaultIntSettingsGetter(void *userdata, az::String name);
az::String defaultIntSettingsSetter(void *userdata, az::String name, az::String argument);

az::String defaultStringGetter(void *userdata, az::String name);
az::String defaultStringSetter(void *userdata, az::String name, az::String argument);
az::String defaultStringSettingsGetter(void *userdata, az::String name);
az::String defaultStringSettingsSetter(void *userdata, az::String name, az::String argument);

void AddGlobalVariable(az::String name, az::String description, void *value, fp_global_getter_t fp_getter, fp_global_setter_t fp_setter);

} // namespace Dev

#endif // CONSOLE_COMMANDS_HPP