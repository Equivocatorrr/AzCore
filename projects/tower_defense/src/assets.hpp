/*
    File: assets.hpp
    Author: Philip Haynes
    Manages loading of file assets.
*/

#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "AzCore/memory.hpp"
#include "AzCore/font.hpp"

namespace Rendering {
    struct Manager;
}

namespace Assets {

extern String error;

enum Type {
    NONE,
    TEXTURE,
    FONT
};

// Used to retrieve indices to actual assets
// Should be consistent with indices in the Rendering Manager
struct Mapping {
    u32 checkSum; // Used as a simple hash value for filenames
    String filename; // Actual filename to be loaded
    Type type; // Determines what arrays contain our asset
    i32 index;
    // Sets both the filename and the checksum.
    void SetFilename(String name);
    bool FilenameEquals(String name, u32 sum);

    static u32 CheckSum(String name);
};

struct Texture {
    Array<u8> pixels;
    i32 width, height, channels;

    bool Load(String filename);
};

struct Font {
    font::Font font;
    font::FontBuilder fontBuilder;

    bool Load(String filename);
};

constexpr i32 textureIndexBlank = 1;

struct Manager {
    Array<String> filesToLoad{"TextureMissing.png", "blank.bmp", "DroidSansFallback.ttf"}; // Everything we want to actually load.
    Array<Mapping> mappings{};
    Array<Texture> textures{};
    Array<Font> fonts{};

    bool LoadAll();
    i32 FindMapping(String filename);
};

} // namespace Assets

#endif // ASSETS_HPP
