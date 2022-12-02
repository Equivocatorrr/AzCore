/*
	File: globals.cpp
	Author: Philip Haynes
*/

#include "globals.hpp"
#include <cstdio>
#include <locale>

using namespace AzCore;

Globals *globals;

void Globals::LoadLocale() {
	String localeName;
	localeName.Reserve(21);
	localeName = "data/locale/";

	if (localeOverride[0] != 0) {
		localeName += localeOverride[0];
		localeName += localeOverride[1];
	} else {
		std::setlocale(LC_ALL, "");
		char *localeString = std::setlocale(LC_CTYPE, NULL);

		io::cout.PrintLn("localeString = ", localeString);

		localeName += CharToLower(localeString[0]);
		localeName += CharToLower(localeString[1]);
	}

	localeName += ".locale";

	FILE *file = fopen(localeName.data, "rb");
	if (!file) {
		file = fopen("data/locale/en.locale", "rb");
		if (!file)
			return;
	}
	String buffer;
	fseek(file, 0, SEEK_END);
	buffer.Resize(ftell(file));
	fseek(file, 0, SEEK_SET);
	if (0 == fread(buffer.data, 1, buffer.size, file))
	{
		return;
	}
	fclose(file);

	bool skipToNewline = buffer[0] == '#';

	for (i32 i = 0; i < buffer.size;)
	{
		if (buffer[i] == '\n') {
			i++;
			if (i < buffer.size) {
				skipToNewline = buffer[i] == '#';
			}
			continue;
		}
		if (skipToNewline) {
			i += CharLen(buffer[i]);
			continue;
		}
		String name;
		String text;
		for (i32 j = i; j < buffer.size; j += CharLen(buffer[j])) {
			if (buffer[j] == '=')
			{
				name.Resize(j - i);
				memcpy(name.data, &buffer[i], name.size);
				i += name.size + 1;
				break;
			}
		}
		for (; i < buffer.size; i += CharLen(buffer[i])) {
			if (buffer[i] == '"') {
				i++;
				break;
			}
		}
		i32 start = i;
		for (; i < buffer.size; i += CharLen(buffer[i])) {
			if (buffer[i] == '"')
				break;
		}
		text.Resize(i - start);
		if (text.size) {
			memcpy(text.data, &buffer[start], text.size);
		}
		locale[name] = ToWString(text);
		i++;
	}
}

bool ReadBool(Range<char> val, bool def) {
	if (val == "true") {
		return true;
	} else if (val == "false") {
		return false;
	} else {
		return def;
	}
}

bool Globals::LoadSettings() {
	Array<char> buffer = FileContents("settings.conf");
	Array<Range<char>> ranges = SeparateByValues(buffer, {'\n', '\r', '\t', ' '});
	for (i32 i = 0; i < ranges.size-1; i++) {
		if (ranges[i] == "fullscreen") {
			fullscreen = ReadBool(ranges[i+1], false);
		} else if (ranges[i] == "vsync") {
			vsync = ReadBool(ranges[i+1], true);
		} else if (ranges[i] == "debugInfo") {
			debugInfo = ReadBool(ranges[i+1], false);
		} else if (ranges[i] == "framerate") {
			f32 fr;
			StringToF32(ranges[i+1], &fr);
			Framerate(clamp(fr, 30.0f, 300.0f));
		} else if (ranges[i] == "volumeMain") {
			StringToF32(ranges[i+1], &volumeMain);
			volumeMain = clamp01(volumeMain);
		} else if (ranges[i] == "volumeMusic") {
			StringToF32(ranges[i+1], &volumeMusic);
			volumeMusic = clamp01(volumeMusic);
		} else if (ranges[i] == "volumeEffects") {
			StringToF32(ranges[i+1], &volumeEffects);
			volumeEffects = clamp01(volumeEffects);
		} else if (ranges[i] == "localeOverride") {
			localeOverride[0] = ranges[i+1][0];
			localeOverride[1] = ranges[i+1][1];
		}
	}
	return true;
}

void WriteBool(String &output, const char *name, bool val) {
	output.Append(name);
	output.Append(' ');
	output.Append(val ? "true\n" : "false\n");
}

bool Globals::SaveSettings() {
	FILE *file = fopen("settings.conf", "w+");
	if (!file) {
		error = "Failed to open settings.conf for writing";
		return false;
	}
	String output;
	WriteBool(output, "fullscreen", fullscreen);
	WriteBool(output, "vsync", vsync);
	WriteBool(output, "debugInfo", debugInfo);
	fwrite(output.data, 1, output.size, file);
	output = "framerate " + ToString((i32)round(framerate)) + '\n';
	fwrite(output.data, 1, output.size, file);
	output = "volumeMain " + ToString(volumeMain, 10, 3) + '\n';
	fwrite(output.data, 1, output.size, file);
	output = "volumeMusic " + ToString(volumeMusic, 10, 3) + '\n';
	fwrite(output.data, 1, output.size, file);
	output = "volumeEffects " + ToString(volumeEffects, 10, 3) + '\n';
	fwrite(output.data, 1, output.size, file);
	if (localeOverride[0] != 0) {
		output = "localeOverride ";
		output += localeOverride[0];
		output += localeOverride[1];
		output += '\n';
		fwrite(output.data, 1, output.size, file);
	}
	fclose(file);
	return true;
}
