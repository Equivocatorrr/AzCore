/*
    File: assets.cpp
    Author: Philip Haynes
*/

#include "assets.hpp"

#define pow(v, e) pow((double)v, (double)e)
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#undef pow

namespace Assets {

String error = "No error.";

Texture::~Texture() {
    if (data.pixels != nullptr) {
        stbi_image_free(data.pixels);
    }
}

bool Texture::Load(u16 channels) {
    data.pixels = stbi_load(filename.data, &data.width, &data.height, &data.channels, channels);
    if (data.pixels == nullptr) {
        error = "Failed to load Texture file: \"" + filename + "\"";
        return false;
    }
    return true;
}

bool Manager::LoadAll() {
    for (Texture& texture : textures) {
        if (!texture.Load()) {
            return false;
        }
    }
    return true;
}

}
