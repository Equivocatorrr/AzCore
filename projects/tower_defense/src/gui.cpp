/*
    File: gui.cpp
    Author: Philip Haynes
*/

#include "gui.hpp"
#include "assets.hpp"
#include "rendering.hpp"

void Gui::EventAssetInit(Assets::Manager *assets) {
    assets->filesToLoad.Append("DroidSans.ttf");
    // assets->filesToLoad.Append("LiberationSerif-Regular.ttf");
    // assets->filesToLoad.Append("OpenSans-Regular.ttf");
    // assets->filesToLoad.Append("Literata[wght].ttf");
}

void Gui::EventAssetAcquire(Assets::Manager *assets) {
    fontIndex = assets->FindMapping("DroidSans.ttf");
    // fontIndex = assets->FindMapping("LiberationSerif-Regular.ttf");
    // fontIndex = assets->FindMapping("OpenSans-Regular.ttf");
    // fontIndex = assets->FindMapping("Literata[wght].ttf");
    font = &assets->fonts[fontIndex];
}

void Gui::EventUpdate(bool buffer, Objects::Manager *objects, Rendering::Manager *rendering) {
    size += 0.005 * (size + 1.0) * objects->timestep * dir;
    if (size >= 0.2) {
        dir = -1.0;
    } else if (size <= 0.1) {
        dir = 1.0;
    }
    pos += vel * objects->timestep * 0.25;
    vec2 textSize = font->StringSize(text, &(*rendering->fonts)[0]);
    if (pos.x + (size * textSize.x) * rendering->aspectRatio > 1.0) {
        vel.x = -0.25;
    } else if (pos.x < -1.0) {
        vel.x = 0.25;
    }
    if (pos.y + size * (textSize.y - 1.0) > 1.0) {
        vel.y = -0.2;
    } else if (pos.y - size < -1.0) {
        vel.y = 0.2;
    }
}

void Gui::EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer) {
    rendering->BindPipelineFont(commandBuffer);
    // rendering->DrawCharSS(commandBuffer, text[25], fontIndex, pos, vec2(size));
    rendering->DrawTextSS(commandBuffer, text, fontIndex, pos, vec2(size));
}
