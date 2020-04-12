/*
    File: assets.cpp
    Author: Philip Haynes
*/

#include "assets.hpp"
#include "globals.hpp"

#include "AzCore/IO/LogStream.hpp"

// #define pow(v, e) pow((double)(v), (double)(e))
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include "stb/stb_vorbis.c"
// #undef pow

namespace Assets {

io::LogStream cout("assets.log");

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
    filename = "data/textures/" + filename;
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
    font.filename = "data/fonts/" + filename;
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
    filename = "data/sound/" + filename;
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
    if (!buffer.Load(decoded, channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, length * 2 * channels, samplerate)) {
        error = "Sound::Load: Failed to load buffer: " + ::Sound::error
            + " channels=" + ToString(channels) + " length=" + ToString(length)
            + " samplerate=" + ToString(samplerate) + " bufferid=" + ToString(buffer.buffer) + " &decoded=0x" + ToString((i64)decoded, 16);
        free(decoded);
        return false;
    }
    free(decoded);
    return true;
}

Sound::~Sound() {
    if (valid) {
        if (!buffer.Clean()) {
            cout << "Failed to clean Sound buffer: " << ::Sound::error << std::endl;
        }
    }
}

bool Stream::Open(String filename) {
    filename = "data/sound/" + filename;
    for (i32 i = 0; i < numStreamBuffers; i++) {
        if (!buffers[i].Create()) {
            error = "Stream::Open: Failed to create buffer: " + ::Sound::error;
            return false;
        }
    }
    i32 iError = 0;
    vorbis = stb_vorbis_open_filename(filename.data, &iError, nullptr);
    if (!vorbis) {
        error = "Stream::Open: Failed to open \"" + filename + "\", error code " + ToString(iError);
        return false;
    }
    totalSamples = stb_vorbis_stream_length_in_samples(vorbis);
    stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    channels = info.channels;
    samplerate = info.sample_rate;
    if (channels > 2 || channels < 1) {
        error = "Unsupported number of channels in sound file (" + filename + "): " + ToString(channels);
        stb_vorbis_close(vorbis);
        return false;
    }
    valid = true;
    return true;
}

bool Stream::Decode(i32 sampleCount) {
    if (!valid) {
        error = "Stream::Decode: Stream not valid!";
        return false;
    }
    if (cursorSample >= totalSamples) {
        cursorSample = 0;
        stb_vorbis_seek_start(vorbis);
    }
    Array<i16> samples(sampleCount * channels);

    i32 length =
    stb_vorbis_get_samples_short_interleaved(vorbis, channels, samples.data, samples.size);

    cursorSample += length;
    ::Sound::Buffer &buffer = buffers[(i32)currentBuffer];
    if (!buffer.Load(samples.data, channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, length * 2 * channels, samplerate)) {
        error = "Stream::Decode: Failed to load buffer: " + ::Sound::error
            + " channels=" + ToString(channels) + " length=" + ToString(length)
            + " samplerate=" + ToString(samplerate) + " bufferid=" + ToString(buffer.buffer) + " &decoded=0x" + ToString((i64)samples.data, 16);
        return false;
    }
    currentBuffer = (currentBuffer + 1) % numStreamBuffers;
    return true;
}

ALuint Stream::LastBuffer() {
    if (currentBuffer == 0) {
        return buffers[numStreamBuffers-1].buffer;
    } else {
        return buffers[currentBuffer-1].buffer;
    }
}

bool Stream::Close() {
    if (!valid) {
        error = "Stream::Close: Stream not valid!";
        return false;
    }
    stb_vorbis_close(vorbis);
    return true;
}

Stream::~Stream() {
    if (valid) {
        for (i32 i = 0; i < numStreamBuffers; i++) {
            if (!buffers[i].Clean()) {
                cout << "Failed to clean Stream buffer: " << ::Sound::error << std::endl;
            }
        }
    }
}

bool Manager::LoadAll() {
    for (i32 i = 0; i < filesToLoad.size; i++) {
        cout << "Loading asset \"" << filesToLoad[i].filename << "\": ";
        Type type;
        if (filesToLoad[i].type == NONE) {
            type = FilenameToType(filesToLoad[i].filename);
        } else {
            type = filesToLoad[i].type;
        }
        i32 nextTexIndex = textures.size;
        i32 nextFontIndex = fonts.size;
        i32 nextSoundIndex = sounds.size;
        i32 nextStreamIndex = streams.size;
        Mapping mapping;
        mapping.type = type;
        switch (type) {
        case NONE:
            cout << "Unknown file type." << std::endl;
            continue;
        case FONT:
            cout << "as font." << std::endl;
            fonts.Append(Font());
            fonts[nextFontIndex].fontBuilder.resolution = font::FontBuilder::HIGH;
            if (!fonts[nextFontIndex].Load(filesToLoad[i].filename)) {
                return false;
            }
            mapping.index = nextFontIndex;
            break;
        case TEXTURE:
            cout << "as texture." << std::endl;
            textures.Append(Texture());
            if (!textures[nextTexIndex].Load(filesToLoad[i].filename)) {
                return false;
            }
            mapping.index = nextTexIndex;
            break;
        case SOUND:
            cout << "as sound." << std::endl;
            sounds.Append(Sound());
            if (!sounds[nextSoundIndex].Load(filesToLoad[i].filename)) {
                return false;
            }
            mapping.index = nextSoundIndex;
            break;
        case STREAM:
            cout << "as stream." << std::endl;
            streams.Append(Stream());
            if (!streams[nextStreamIndex].Open(filesToLoad[i].filename)) {
                return false;
            }
            mapping.index = nextStreamIndex;
            break;
        }
        mapping.SetFilename(filesToLoad[i].filename);
        mappings.Append(std::move(mapping));
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
    return -1;
}

f32 Manager::CharacterWidth(char32 c, i32 fontIndex) const {
    return globals->rendering.CharacterWidth(c, &fonts[fontIndex], &fonts[0]);
}

}
