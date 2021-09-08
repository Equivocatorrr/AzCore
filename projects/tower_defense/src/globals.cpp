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

		std::cout << "localeString = " << localeString << std::endl;

		localeName += localeString[0];
		localeName += localeString[1];
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
			skipToNewline = buffer[i] == '#';
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
		memcpy(text.data, &buffer[start], text.size);
		locale[name] = ToWString(text);
		i++;
	}
}

bool Globals::LoadSettings() {
	FILE *file = fopen("settings.conf", "rb");
	if (!file) {
		error = "Failed to open settings.conf";
		return false;
	}
	String buffer;
	fseek(file, 0, SEEK_END);
	buffer.Resize(ftell(file));
	fseek(file, 0, SEEK_SET);
	if (0 == fread(buffer.data, 1, buffer.size, file)) {
		error = "Nothing read from settings.conf";
		return false;
	}
	fclose(file);
	// Now parse the buffer
	for (i32 i = 0; i < buffer.size;) {
		if (buffer[i] == ' ' || buffer[i] == '\n') {
			i++;
			continue;
		}
		String token[2];
		for (i32 t = 0; t < 2; t++) {
			for (i32 j = i; j <= buffer.size; j++) {
				if (j == buffer.size || buffer[j] == ' ' || buffer[j] == '\n') {
					token[t].Resize(j-i);
					memcpy(token[t].data, &buffer[i], token[t].size);
					i += token[t].size+1;
					break;
				}
			}
		}
		if (token[0] == "fullscreen") {
			if (token[1] == "true") {
				fullscreen = true;
			} else if (token[1] == "false") {
				fullscreen = false;
			}
		} else if (token[0] == "framerate") {
			Framerate(clamp(StringToF32(token[1]), 30.0f, 300.0f));
		} else if (token[0] == "volumeMain") {
			volumeMain = clamp(StringToF32(token[1]), 0.0f, 1.0f);
		} else if (token[0] == "volumeMusic") {
			volumeMusic = clamp(StringToF32(token[1]), 0.0f, 1.0f);
		} else if (token[0] == "volumeEffects") {
			volumeEffects = clamp(StringToF32(token[1]), 0.0f, 1.0f);
		} else if (token[0] == "localeOverride") {
			localeOverride[0] = token[1][0];
			localeOverride[1] = token[1][1];
		}
	}
	return true;
}

bool Globals::SaveSettings() {
	FILE *file = fopen("settings.conf", "w+");
	if (!file) {
		error = "Failed to open settings.conf for writing";
		return false;
	}
	String output;
	output = "fullscreen ";
	output += fullscreen ? "true\n" : "false\n";
	fwrite(output.data, 1, output.size, file);
	output = "framerate " + ToString((i32)framerate) + '\n';
	fwrite(output.data, 1, output.size, file);
	output = "volumeMain " + ToString(volumeMain) + '\n';
	fwrite(output.data, 1, output.size, file);
	output = "volumeMusic " + ToString(volumeMusic) + '\n';
	fwrite(output.data, 1, output.size, file);
	output = "volumeEffects " + ToString(volumeEffects) + '\n';
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
