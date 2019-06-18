/*
    File: gui.cpp
    Author: Philip Haynes
*/

#include "gui.hpp"
#include "assets.hpp"
#include "rendering.hpp"

void Gui::EventAssetInit(Assets::Manager *assets) {
    assets->filesToLoad.Append("test.tga");
}

void Gui::EventAssetAcquire(Assets::Manager *assets) {
    imageIndex = assets->FindMapping("test.tga").index1;
}

void Gui::EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer) {
    Rendering::PushConstants pc = Rendering::PushConstants();
    f32 aspect = (f32)rendering->window->height / (f32)rendering->window->width;
    pc.vert.transform = pc.vert.transform.Scale(vec2(aspect, 1.0));
    rendering->data.pipeline2D->Bind(commandBuffer);
    vk::CmdBindVertexBuffer(commandBuffer, 0, rendering->data.vertexBuffer);
    vk::CmdBindIndexBuffer(commandBuffer, rendering->data.indexBuffer, VK_INDEX_TYPE_UINT32);
    vk::CmdSetViewportAndScissor(commandBuffer, rendering->window->width, rendering->window->height);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rendering->data.pipeline2D->data.layout,
            0, 1, &rendering->data.descriptors->data.sets[0].data.set, 0, nullptr);
    pc.Push(commandBuffer, rendering);
    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
}
