/*
    File: assets.cpp
    Author: Philip Haynes
*/

#include "assets.hpp"

#include "AzCore/log_stream.hpp"

#define pow(v, e) pow((double)v, (double)e)
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#undef pow

namespace Assets {

io::logStream cout("assets.log");

String error = "No error.";

Type FilenameToType(String filename) {
    const char *texExtensions[] = {
        ".tga",
        ".png",
        ".jpg",
        ".jpeg",
        ".bmp",
        ".hdr"
    };
    const char *fontExtensions[] = {
        ".ttf",
        ".otf",
        ".ttc"
    };

    for (const char *ext : texExtensions) {
        const i32 len = StringLength(ext);
        if (len >= filename.size) {
            continue;
        }
        bool fnd = true;
        for (i32 i = 0; i < len; i++) {
            if (ext[i] != filename[filename.size-len+i]) {
                fnd = false;
                break;
            }
        }
        if (fnd) {
            return Type::TEXTURE;
        }
    }
    for (const char *ext : fontExtensions) {
        const i32 len = 4;
        if (len >= filename.size) {
            continue;
        }
        bool fnd = true;
        for (i32 i = 0; i < len; i++) {
            if (ext[i] != filename[filename.size-len+i]) {
                fnd = false;
                break;
            }
        }
        if (fnd) {
            return Type::FONT;
        }
    }
    return Type::NONE;
}

void Mapping::SetFilename(String name) {
    filename = std::move(name);
    checkSum = 0;
    for (i32 i = 0; i < filename.size; i++) {
        checkSum += (filename[i] << (i%4)*8);
    }
}

bool Mapping::FilenameEquals(String name, u32 sum) {
    if (checkSum == sum) {
        return filename == name;
    }
    return false;
}

bool Texture::Load(String filename) {
    filename = "data/" + filename;
    pixels.data = stbi_load(filename.data, &width, &height, &channels, 0);
    if (pixels.data == nullptr) {
        error = "Failed to load Texture file: \"" + filename + "\"";
        return false;
    }
    pixels.allocated = width * height* channels;
    pixels.size = pixels.allocated;
    return true;
}

bool Manager::LoadAll() {
    for (i32 i = 0; i < filesToLoad.size; i++) {
        cout << "Loading asset \"" << filesToLoad[i] << "\": ";
        Type type = FilenameToType(filesToLoad[i]);
        i32 nextTexIndex = textures.size;
        switch (type) {
        case NONE:
            cout << "Unknown file type." << std::endl;
            continue;
        case FONT:
            cout << "as font." << std::endl;
            break;
        case TEXTURE:
            cout << "as texture." << std::endl;
            Texture texture;
            texture.Load(filesToLoad[i]);
            textures.Append(std::move(texture));
            Mapping mapping;
            mapping.type = TEXTURE;
            mapping.ids.index1 = nextTexIndex;
            mapping.SetFilename(filesToLoad[i]);
            break;
        }
    }
    return true;
}

}
