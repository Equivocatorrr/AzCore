/*
	File: settings.cpp
	Author: Philip Haynes
*/

#include "settings.hpp"
#include "AzCore/memory.hpp"
#include "AzCore/IO/Log.hpp"
#include <cstdio>
#include <locale>

namespace Az3D {
namespace Settings {

AZCORE_CREATE_STRING_ARENA_CPP()

using namespace AzCore;

const char *Setting::typeStrings[5] = {
	"None",
	"Bool",
	"Int",
	"Real",
	"String",
};

Name sFullscreen = "fullscreen";
Name sVSync = "vsync";
Name sMSAA = "msaa";
Name sDebugInfo = "debugInfo";
Name sFramerate = "framerate";
Name sVolumeMain = "volumeMain";
Name sVolumeMusic = "volumeMusic";
Name sVolumeEffects = "volumeEffects";
Name sLocaleOverride = "localeOverride";

AStringMap<Setting> settings = {
	{sFullscreen, Setting(false)},
	{sVSync, Setting(true)},
	{sMSAA, Setting(false)},
	{sDebugInfo, Setting(false)},
	{sFramerate, Setting(60.0, 30.0, 600.0)},
	{sVolumeMain, Setting(1.0, 0.0, 1.0)},
	{sVolumeMusic, Setting(1.0, 0.0, 1.0)},
	{sVolumeEffects, Setting(1.0, 0.0, 1.0)},
	{sLocaleOverride, Setting(String())},
};

void Add(Name name, Setting &&defaultValue) {
	settings[name] = std::move(defaultValue);
}


bool ReadBool(Name name) {
	return settings[name].GetBool();
}

i64 ReadInt(Name name) {
	return settings[name].GetInt();
}

f64 ReadReal(Name name) {
	return settings[name].GetReal();
}

az::String ReadString(Name name) {
	return settings[name].GetString();
}


void SetBool(Name name, bool value) {
	settings[name] = value;
}

void SetInt(Name name, i64 value) {
	settings[name] = value;
}

void SetReal(Name name,  f64 value) {
	settings[name] = value;
}

void SetString(Name name, const az::String &value) {
	settings[name] = value;
}

void SetString(Name name, az::String &&value) {
	settings[name] = std::move(value);
}


bool ReadBoolFromStr(SimpleRange<char> val, bool def) {
	if (val == "true") {
		return true;
	} else if (val == "false") {
		return false;
	} else {
		return def;
	}
}

i64 ReadIntFromStr(SimpleRange<char> val, i64 def) {
	i64 result;
	if (!StringToI64(val, &result)) {
		result = def;
	}
	return result;
}

f64 ReadRealFromStr(SimpleRange<char> val, f64 def) {
	f64 result;
	if (!StringToF64(val, &result)) {
		result = def;
	}
	return result;
}

bool GetKeyValuePair(SimpleRange<char> line, SimpleRange<char> &outKey, SimpleRange<char> &outValue) {
	i64 space = -1;
	for (i64 i = 0; i < line.size; i++) {
		if (line[i] == ' ') {
			space = i;
			break;
		}
	}
	if (space == -1) {
		outKey = line;
		outValue = SimpleRange<char>();
		return false;
	} else {
		outKey = line.SubRange(0, space);
		outValue = line.SubRange(space+1, line.size-space-1);
		return true;
	}
}

bool Load() {
	Array<char> buffer = FileContents("settings.conf");
	if (buffer.size == 0) {
		az::io::cerr.PrintLn("Failed to load settings.conf");
		return false;
	}
	Array<SimpleRange<char>> lines = SeparateByNewlines(buffer);
	for (i32 i = 0; i < lines.size; i++) {
		SimpleRange<char> key, value;
		if (!GetKeyValuePair(lines[i], key, value)) continue;
		Name string = key;
		if (!settings.Exists(string)) continue;
		Setting &setting = settings[string];
		switch (setting.type) {
			case Setting::Type::BOOL: {
				setting = ReadBoolFromStr(value, setting.GetBool());
			} break;
			case Setting::Type::INT: {
				setting = ReadIntFromStr(value, setting.GetInt());
			} break;
			case Setting::Type::REAL: {
				setting = ReadRealFromStr(value, setting.GetReal());
			} break;
			case Setting::Type::STRING: {
				setting = String(value);
			} break;
			default: continue;
		}
	}
	return true;
}

bool Save() {
	FILE *file = fopen("settings.conf", "w");
	if (!file) {
		io::cerr.PrintLn("Failed to open settings.conf for writing");
		return false;
	}

	String output;

	for (auto node : settings) {
		Setting &setting = *node.value;
		if (setting.type == Setting::Type::NONE) continue;
		AString name = node.key;
		output.Append(name);
		output.Append(' ');
		switch(setting.type) {
			case Setting::Type::BOOL: {
				output.Append(setting.GetBool() ? "true" : "false");
			} break;
			case Setting::Type::INT: {
				AppendToString(output, setting.GetInt());
			} break;
			case Setting::Type::REAL: {
				AppendToString(output, setting.GetReal());
			} break;
			case Setting::Type::STRING: {
				AppendToString(output, setting.GetString());
			} break;
			default: break;
		}
		output.Append('\n');
	}
	fwrite(output.data, 1, output.size, file);
	fclose(file);
	return true;
}

} // namespace Settings
} // namespace Az3D
