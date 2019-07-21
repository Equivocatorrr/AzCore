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
    checkSum = CheckSum(filename);
}

bool Mapping::FilenameEquals(String name, u32 sum) {
    if (checkSum == sum) {
        return filename == name;
    }
    return false;
}

u32 Mapping::CheckSum(String name) {
    u32 checkSum = 0;
    for (i32 i = 0; i < name.size; i++) {
        checkSum += (name[i] << (i%4)*8);
    }
    return checkSum;
}

bool Texture::Load(String filename) {
    filename = "data/" + filename;
    pixels.data = stbi_load(filename.data, &width, &height, &channels, 4);
    if (pixels.data == nullptr) {
        error = "Failed to load Texture file: \"" + filename + "\"";
        return false;
    }
    channels = 4;
    pixels.allocated = width * height* channels;
    pixels.size = pixels.allocated;
    return true;
}

bool Font::Load(String filename) {
    font.filename = "data/" + filename;
    if (!font.Load()) {
        error = "Failed to load font: " + font::error;
        return false;
    }
    fontBuilder.font = &font;
    fontBuilder.AddRange(0, 255);
    if (!fontBuilder.Build()) {
        error = "Failed to load font: " + font::error;
        return false;
    }
    return true;
}

vec2 Font::StringSize(WString string, Font *fallback) const {
    vec2 size = vec2(0.0, 1.0);
    f32 lineSize = 0.0;
    for (i32 i = 0; i < string.size; i++) {
        const char32 character = string[i];
        if (character == '\n') {
            lineSize = 0.0;
            size.y += 1.0;
            continue;
        }
        const Font *actualFont = this;
        i32 glyphIndex = font.GetGlyphIndex(character);
        if (glyphIndex == 0) {
            i32 glyphIndexFallback = fallback->font.GetGlyphIndex(character);
            if (glyphIndexFallback != 0) {
                glyphIndex = glyphIndexFallback;
                actualFont = fallback;
            }
        }
        const i32 glyphId = actualFont->fontBuilder.indexToId[glyphIndex];
        lineSize += actualFont->fontBuilder.glyphs[glyphId].info.advance.x;
        if (lineSize > size.x) {
            size.x = lineSize;
        }
    }
    return size;
}

bool Manager::LoadAll() {
    for (i32 i = 0; i < filesToLoad.size; i++) {
        cout << "Loading asset \"" << filesToLoad[i] << "\": ";
        Type type = FilenameToType(filesToLoad[i]);
        i32 nextTexIndex = textures.size;
        i32 nextFontIndex = fonts.size;
        Mapping mapping;
        switch (type) {
        case NONE:
            cout << "Unknown file type." << std::endl;
            continue;
        case FONT:
            cout << "as font." << std::endl;
            fonts.Append(Font());
            if (!fonts[nextFontIndex].Load(filesToLoad[i])) {
                return false;
            }
            mapping.type = FONT;
            mapping.index = nextFontIndex;
            mapping.SetFilename(filesToLoad[i]);
            mappings.Append(std::move(mapping));
            break;
        case TEXTURE:
            cout << "as texture." << std::endl;
            textures.Append(Texture());
            if (!textures[nextTexIndex].Load(filesToLoad[i])) {
                return false;
            }
            mapping.type = TEXTURE;
            mapping.index = nextTexIndex;
            mapping.SetFilename(filesToLoad[i]);
            mappings.Append(std::move(mapping));
            break;
        }
    }
    return true;
}

i32 Manager::FindMapping(String filename) {
    i32 checkSum = Mapping::CheckSum(filename);
    for (Mapping& mapping : mappings) {
        if (mapping.FilenameEquals(filename, checkSum)) {
            return mapping.index;
        }
    }
    cout << "No mapping found for \"" << filename << "\"" << std::endl;
    return 0;
}

}
