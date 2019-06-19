/*
    File: gui.cpp
    Author: Philip Haynes
*/

#include "gui.hpp"
#include "assets.hpp"
#include "rendering.hpp"

void Gui::EventAssetInit(Assets::Manager *assets) {
    assets->filesToLoad.Append("OpenSans-Regular.ttf");
}

void Gui::EventAssetAcquire(Assets::Manager *assets) {
    fontIndex = assets->FindMapping("OpenSans-Regular.ttf");
}

void Gui::EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer) {
    Rendering::PushConstants pc = Rendering::PushConstants();
    pc.frag.texIndex = fontIndex;
    pc.font.edge = 16.0 / font::sdfDistance / (f32)rendering->window->height / 2.0;
    f32 aspect = (f32)rendering->window->height / (f32)rendering->window->width;
    pc.vert.transform = pc.vert.transform.Scale(vec2(aspect, 1.0));
    rendering->BindPipelineFont(commandBuffer);
    pc.PushFont(commandBuffer, rendering);
    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
}
