/*
    File: assets.hpp
    Author: Philip Haynes
    Manages loading of file assets.
*/

#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "AzCore/memory.hpp"

namespace Assets {

extern String error;

enum Type {
    NONE,
    TEXTURE,
    FONT
};

struct MapIndices {
    // For assets that have a texture, index1 is the texture index,
    // which refers to both the textures array and the index for Rendering Manager
    // For fonts, which have a texture and extra font-specific data, index2 is the index of the fonts array.
    i16 index1, index2;
};

// Used to retrieve indices to actual assets
// Should be consistent with indices in the Rendering Manager
struct Mapping {
    u32 checkSum; // Used as a simple hash value for filenames
    String filename; // Actual filename to be loaded
    Type type; // Determines what arrays contain our asset
    MapIndices ids;
    // Sets both the filename and the checksum.
    void SetFilename(String name);
    bool FilenameEquals(String name, u32 sum);
};

struct Texture {
    Array<u8> pixels;
    i32 width, height, channels;

    bool Load(String filename); // set channels to force conversion, else inherit from the file.
};

struct Manager {
    Array<String> filesToLoad{}; // Everything we want to actually load.
    Array<Mapping> mappings{};
    Array<Texture> textures{};

    bool LoadAll();
    MapIndices FindMapping(String filename);
};

} // namespace Assets

#endif // ASSETS_HPP
