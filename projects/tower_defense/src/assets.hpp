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

struct Texture {
    struct {
        u8* pixels = nullptr;
        i32 width, height, channels;
    } data;

    String filename;

    ~Texture();
    bool Load(u16 channels = 0); // set channels to force conversion, else inherit from the file.
};

struct Manager {
    Array<Texture> textures;

    bool LoadAll();
};

} // namespace Assets

#endif // ASSETS_HPP
