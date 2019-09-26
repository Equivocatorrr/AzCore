/*
    File: assets.cpp
    Author: Philip Haynes
*/

#include "assets.hpp"
#include "globals.hpp"

#include "AzCore/log_stream.hpp"

// #define pow(v, e) pow((double)(v), (double)(e))
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include "stb/stb_vorbis.c"
// #undef pow

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
    const char *soundExtensions[] = {
        ".ogg"
    };

    if (4 >= filename.size) {
        return Type::NONE;
    }

    for (const char *ext : soundExtensions) {
        const i32 len = 4;
        bool fnd = true;
        for (i32 i = 0; i < len; i++) {
            if (ext[i] != filename[filename.size-len+i]) {
                fnd = false;
                break;
            }
        }
        if (fnd) {
            return Type::SOUND;
        }
    }
    for (const char *ext : fontExtensions) {
        const i32 len = 4;
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
    for (const char *ext : texExtensions) {
        const i32 len = StringLength(ext);
        if (len >= filename.size) {
            return Type::NONE;
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
    fontBuilder.AddRange(0, 128);
    if (!fontBuilder.Build()) {
        error = "Failed to load font: " + font::error;
        return false;
    }
    return true;
}

void Font::SaveAtlas() {
    cout << "Saving png of font " << font.filename << std::endl;
    stbi_write_png((font.filename + ".png").data, fontBuilder.dimensions.x, fontBuilder.dimensions.y, 1, fontBuilder.pixels.data, fontBuilder.dimensions.x);
}

bool Sound::Load(String filename) {
    filename = "data/" + filename;
    if (!buffer.Create()) {
        error = "Sound::Load: Failed to create buffer: " + ::Sound::error;
        return false;
    }
    valid = true;
    i16 *decoded;
    i32 channels, samplerate, length;
    length = stb_vorbis_decode_filename(filename.data, &channels, &samplerate, &decoded);
    if (length <= 0) {
        error = "Failed to decode sound file (" + filename + ")";
        return false;
    }
    if (!decoded) {
        error = "Decoded is nullptr!";
        return false;
    }
    if (channels > 2 || channels < 1) {
        error = "Unsupported number of channels in sound file (" + filename + "): " + ToString(channels);
        free(decoded);
        return false;
    }
    // TODO: Is length the number frames or samples (does this change for stereo?)
    if (!buffer.Load(decoded, channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, length * 2, samplerate)) {
        error = "Sound::Load: Failed to load buffer: " + ::Sound::error
            + " channels=" + ToString(channels) + " length=" + ToString(length)
            + " samplerate=" + ToString(samplerate) + " bufferid=" + ToString(buffer.buffer) + " &decoded=0x" + ToString((i64)decoded, 16);
        free(decoded);
        return false;
    }
    free(decoded);
    return true;
}

Sound::Sound() : valid(false), buffer({UINT32_MAX, false}) {}

Sound::~Sound() {
    if (valid) {
        if (!buffer.Clean()) {
            cout << "Failed to clean Sound buffer: " << ::Sound::error << std::endl;
        }
    }
}

bool Manager::LoadAll() {
    for (i32 i = 0; i < filesToLoad.size; i++) {
        cout << "Loading asset \"" << filesToLoad[i] << "\": ";
        Type type = FilenameToType(filesToLoad[i]);
        i32 nextTexIndex = textures.size;
        i32 nextFontIndex = fonts.size;
        i32 nextSoundIndex = sounds.size;
        Mapping mapping;
        switch (type) {
        case NONE:
            cout << "Unknown file type." << std::endl;
            continue;
        case FONT:
            cout << "as font." << std::endl;
            fonts.Append(Font());
            fonts[nextFontIndex].fontBuilder.resolution = font::FontBuilder::HIGH;
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
        case SOUND:
            cout << "as sound." << std::endl;
            sounds.Append(Sound());
            if (!sounds[nextSoundIndex].Load(filesToLoad[i])) {
                return false;
            }
            mapping.type = SOUND;
            mapping.index = nextSoundIndex;
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

f32 Manager::CharacterWidth(char32 c, i32 fontIndex) const {
    return globals->rendering.CharacterWidth(c, &fonts[fontIndex], &fonts[0]);
}

}
