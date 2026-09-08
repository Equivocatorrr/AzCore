/*
	File: console_commands.cpp
	Author: Philip Haynes
*/

#include "console_commands.hpp"

#include "game_systems.hpp"
#include "settings.hpp"

namespace Dev {

using namespace AzCore;

static String sYes = "yes";
static String sNo = "no";
static String sOn = "on";
static String sOff = "off";
static String sTrue = "true";
static String sFalse = "false";

bool ParseBool(String input, bool &dst) {
	if (sYes == input) {
		dst = true;
		return true;
	}
	if (sOn == input) {
		dst = true;
		return true;
	}
	if (sTrue == input) {
		dst = true;
		return true;
	}
	if (sNo == input) {
		dst = false;
		return true;
	}
	if (sOff == input) {
		dst = false;
		return true;
	}
	if (sFalse == input) {
		dst = false;
		return true;
	}
	return false;
}

struct GlobalVar {
	String description;
	void *value;
	fp_global_getter_t fp_getter;
	fp_global_setter_t fp_setter;
};

static BinaryMap<String, GlobalVar> globalVariables;

void AddGlobalVariable(String name, String description, void *value, fp_global_getter_t fp_getter, fp_global_setter_t fp_setter) {
	AzAssert(!globalVariables.Exists(name), Stringify("Cannot have 2 variables with the same name \"", name, "\""));
	globalVariables.Emplace(name, GlobalVar{description, value, fp_getter, fp_setter});
}

String defaultBoolGetter(void *userdata, String name) {
	bool *toggle = (bool*)userdata;
	if (*toggle) {
		return Stringify(name, " is on");
	} else {
		return Stringify(name, " is off");
	}
}
String defaultBoolSetter(void *userdata, String name, String arg) {
	bool *toggle = (bool*)userdata;
	bool argVal;
	if (!Dev::ParseBool(arg, argVal)) {
		return Stringify(name, " expected a bool value");
	}
	*toggle = argVal;
	if (*toggle) {
		return Stringify("set ", name, " to on");
	} else {
		return Stringify("set ", name, " to off");
	}
}

String defaultBoolSettingsGetter(void *userdata, String name) {
	Az2D::Settings::Name settingName = name;
	bool toggle = Az2D::Settings::ReadBool(settingName);
	if (toggle) {
		return Stringify(name, " is on");
	} else {
		return Stringify(name, " is off");
	}
}

String defaultBoolSettingsSetter(void *userdata, String name, String arg) {
	bool toggle;
	if (!ParseBool(arg, toggle)) {
		return Stringify(name, " expected a bool value");
	}
	Az2D::Settings::Name settingName = name;
	Az2D::Settings::SetBool(settingName, toggle);
	if (toggle) {
		return Stringify("set ", name, " to on");
	} else {
		return Stringify("set ", name, " to off");
	}
}

String defaultRealGetter(void *userdata, String name) {
	f32 *real = (f32*)userdata;
	return Stringify(name, " is ", FormatFloat(*real, 10, 3));
}

String defaultRealSetter(void *userdata, String name, String arg) {
	f32 *real = (f32*)userdata;
	f32 argVal;
	if (!StringToF32(arg, &argVal)) {
		return Stringify(name, " expected a real number value");
	}
	*real = argVal;
	return Stringify("set ", name, " to ", FormatFloat(*real, 10, 3));
}

String defaultRealSettingsGetter(void *userdata, String name) {
	Az2D::Settings::Name settingName = name;
	f32 real = Az2D::Settings::ReadReal(settingName);
	return Stringify(name, " is ", FormatFloat(real, 10, 3));
}

String defaultRealSettingsSetter(void *userdata, String name, String arg) {
	f32 real;
	if (!StringToF32(arg, &real)) {
		return Stringify(name, " expected a real number value");
	}
	Az2D::Settings::Name settingName = name;
	Az2D::Settings::SetReal(settingName, real);
	return Stringify("set ", name, " to ", FormatFloat(real, 10, 3));
}

String defaultIntSettingsGetter(void *userdata, String name) {
	Az2D::Settings::Name settingName = name;
	i32 value = Az2D::Settings::ReadInt(settingName);
	return Stringify(name, " is ", value);
}

String defaultIntSettingsSetter(void *userdata, String name, String arg) {
	i32 value;
	if (!StringToI32(arg, &value)) {
		return Stringify(name, " expected an integer value");
	}
	Az2D::Settings::Name settingName = name;
	Az2D::Settings::SetInt(settingName, value);
	return Stringify("set ", name, " to ", value);
}

String defaultStringGetter(void *userdata, String name) {
	String *string = (String*)userdata;
	return Stringify(name, " is \"", *string, '"');
}

String defaultStringSetter(void *userdata, String name, String arg) {
	String *string = (String*)userdata;
	*string = std::move(arg);
	return Stringify("set ", name, " to \"", *string, '"');
}

String defaultStringSettingsGetter(void *userdata, String name) {
	Az2D::Settings::Name settingName = name;
	String string = Az2D::Settings::ReadString(settingName);
	return Stringify(name, " is \"", string, '"');
}

String defaultStringSettingsSetter(void *userdata, String name, String arg) {
	Az2D::Settings::Name settingName = name;
	Az2D::Settings::SetString(settingName, arg);
	return Stringify("set ", name, " to \"", arg, '"');
}

namespace Commands {

String set(Array<Str> args) {
	if (args.size < 3) {
		return "Expected 2 arguments: <name> <value>";
	}
	auto *node = globalVariables.Find(args[1]);
	if (nullptr == node) {
		return Stringify("variable \"", args[1], "\" does not exist.");
	}
	if (nullptr == node->value.fp_setter) {
		return Stringify("variable \"", args[1], "\" does not have a setter.");
	}
	return node->value.fp_setter(node->value.value, args[1], args[2]);
}

String get(Array<Str> args) {
	if (args.size < 2) {
		return "Expected 1 argument: <name>";
	}
	auto *node = globalVariables.Find(args[1]);
	if (nullptr == node) {
		return Stringify("variable \"", args[1], "\" does not exist.");
	}
	if (nullptr == node->value.fp_getter) {
		return Stringify("variable \"", args[1], "\" does not have a getter.");
	}
	return node->value.fp_getter(node->value.value, args[1]);
}

String whatis(Array<Str> args) {
	if (args.size < 2) {
		return "Expected 1 argument: <name>";
	}
	auto *node = globalVariables.Find(args[1]);
	if (nullptr == node) {
		return Stringify("variable \"", args[1], "\" does not exist.");
	}
	return Stringify(args[1], ": ", node->value.description);
}

String list(Array<Str> args) {
	Array<Str> names;
	for (auto &node : globalVariables) {
		names.Append(node.key);
	}
	return Stringify("Available variables: { ", Join(names, ", "), " }");
}

String echo(Array<Str> args) {
	String result;
	for (i32 i = 1; i < args.size; i++) {
		result.Append(args[i]);
		result.Append(' ');
	}
	if (result.size) result.size--;
	return result;
}

String greet(Array<Str> args) {
	return "Why hello there!";
}

String quit(Array<Str> args) {
	Az2D::GameSystems::sys->exit = true;
	return "Quitting...";
}

String help(Array<Str> args);

static BinaryMap<String, fp_Command> dispatch = {
	{"echo", echo},
	{"quit", quit},
	{"exit", quit},
	{"q", quit},
	{"hi", greet},
	{"hello", greet},
	{"help", help},
	{"set", set},
	{"get", get},
	{"whatis", whatis},
	{"list", list},
};

static BinaryMap<fp_Command, String> helpMessages = {
	{(fp_Command)echo,   "echo <text>\t\t\t\t\tPrint text in the console"},
	{(fp_Command)help,   "help\t\t\t\t\t\t\tDisplay this help"},
	{(fp_Command)exit,   "exit | quit | q\t\t\t\t\tExit the game"},
	{(fp_Command)set,    "set <name> <value>\t\t\tset global variable with name to value"},
	{(fp_Command)get,    "get <name>\t\t\t\t\tget current value of global variable with name"},
	{(fp_Command)whatis, "whatis <name>\t\t\t\tPrint the description of the global variable with name"},
	{(fp_Command)list,   "list\t\t\t\t\t\t\t\tList all global variable names"},
};

String help(Array<Str> args) {
	String result = "Available commands:";
	for (auto &node : helpMessages) {
		result += "\n\t";
		result += node.value;
	}
	return result;
}

} // namespace Commands


String HandleCommand(String input) {
	Array<Str> args;
	// SeparateByValues is very easy, but doesn't make exceptions, such as for quoted strings
	// ConvertArrayTo<Str, 0>(SeparateByValues(input, {(char32)' ', (char32)'\n', (char32)'\t'}));
	{
		bool quotes = false;
		i32 wordStart = 0;
		for (i32 i = 0; i < input.size; i++) {
			if (quotes) {
				if (input[i] == '"') {
					args.Append(Str(&input[wordStart], i-wordStart));
					quotes = false;
					wordStart = i+1;
				}
			} else {
				if (input[i] == '"') {
					quotes = true;
					wordStart = i+1;
				} else if (isWhitespace(input[i])) {
					if (i - wordStart > 0) {
						args.Append(Str(&input[wordStart], i-wordStart));
					}
					wordStart = i+1;
				}
			}
		}
		if (quotes) {
			return "Unterminated string";
		}
		if (wordStart < input.size) {
		args.Append(Str(&input[wordStart], input.size-wordStart));
	}
	}
	if (args.size == 0) {
		return String();
	}
	String command = args[0];
	StrToLower(command);
	if (auto node = Commands::dispatch.Find(command)) {
		return node->value(args);
	} else {
		return "Unknown command '" + command + "'";
	}
}

bool AddCommand(String name, String helpMessage, fp_Command command) {
	StrToLower(name);
	if (Commands::dispatch.ValueOf(name, command) != command) return false;
	Commands::helpMessages.Emplace(command, helpMessage);
	return true;
}

} // namespace Dev