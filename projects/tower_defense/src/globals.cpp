/*
    File: globals.cpp
    Author: Philip Haynes
*/

#include "globals.hpp"
#include <cstdio>

Globals *globals;

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
    fread(buffer.data, 1, buffer.size, file);
    fclose(file);
    // Now parse the buffer
    for (i32 i = 0; i < buffer.size;) {
        if (buffer[i] == ' ' || buffer[i] == '\n') {
            i++;
            continue;
        }
        String token[2];
        for (i32 t = 0; t < 2; t++) {
            for (i32 j = i; j < buffer.size; j++) {
                if (buffer[j] == ' ' || buffer[j] == '\n') {
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
            Framerate(clamp(StringToF32(token[1]), 30.0, 300.0));
        } else if (token[0] == "volumeMain") {
            volumeMain = clamp(StringToF32(token[1]), 0.0, 1.0);
        } else if (token[0] == "volumeMusic") {
            volumeMusic = clamp(StringToF32(token[1]), 0.0, 1.0);
        } else if (token[0] == "volumeEffects") {
            volumeEffects = clamp(StringToF32(token[1]), 0.0, 1.0);
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
    if (fullscreen) output += "true\n"; else output += "false\n";
    fwrite(output.data, 1, output.size, file);
    output = "framerate " + ToString((i32)framerate) + '\n';
    fwrite(output.data, 1, output.size, file);
    output = "volumeMain " + ToString(volumeMain) + '\n';
    fwrite(output.data, 1, output.size, file);
    output = "volumeMusic " + ToString(volumeMusic) + '\n';
    fwrite(output.data, 1, output.size, file);
    output = "volumeEffects " + ToString(volumeEffects) + '\n';
    fwrite(output.data, 1, output.size, file);
    fclose(file);
    return true;
}
