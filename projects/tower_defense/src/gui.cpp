/*
    File: gui.cpp
    Author: Philip Haynes
*/

#include "gui.hpp"
#include "assets.hpp"
#include "rendering.hpp"

void Gui::EventAssetInit(Assets::Manager *assets) {
    // assets->filesToLoad.Append("OpenSans-Regular.ttf");
    assets->filesToLoad.Append("Literata[wght].ttf");
    // assets->filesToLoad.Append("DroidSansFallback.ttf");
}

void Gui::EventAssetAcquire(Assets::Manager *assets) {
    // fontIndex = assets->FindMapping("OpenSans-Regular.ttf");
    fontIndex = assets->FindMapping("Literata[wght].ttf");
    // fontIndex = assets->FindMapping("DroidSansFallback.ttf");
    font = &assets->fonts[fontIndex];
}

void Gui::EventUpdate(bool buffer, Objects::Manager *objects) {
    size += 0.02 * (size + 1.0) * objects->timestep * dir;
    if (size >= 0.4) {
        dir = -1.0;
    } else if (size <= 0.1) {
        dir = 1.0;
    }
    pos += vel * objects->timestep * 0.25;
    if (pos.x + size * 3.0 > 1.0) {
        vel.x = -1.0;
    } else if (pos.x < -1.0) {
        vel.x = 1.0;
    }
    if (pos.y > 1.0) {
        vel.y = -0.5;
    } else if (pos.y - size * 0.4 < -1.0) {
        vel.y = 0.5;
    }
}

void Gui::EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer) {
    static WString text = ToWString("Hahaha look at me!\nありがとうお願いします私はハンバーガー");
    rendering->BindPipelineFont(commandBuffer);
    // rendering->DrawCharSS(commandBuffer, text[25], fontIndex, pos, vec2(size));
    rendering->DrawTextSS(commandBuffer, text, fontIndex, pos, vec2(size));
}
