/*
    File: main.cpp
    Author: Philip Haynes
    Description: High-level definition of the structure of our program.
*/

#include "rendering.hpp"
#include "assets.hpp"

#include "AzCore/io.hpp"

const char *title = "AzCore Tower Defense";

io::logStream cout("main.log");

void renderCallbackTest(void *userdata, Rendering::Manager *rendering, Array<VkCommandBuffer>& commandBuffers) {
    Rendering::PushConstants pc = Rendering::PushConstants();
    f32 aspect = (f32)rendering->window->height / (f32)rendering->window->width;
    pc.vert.transform = pc.vert.transform.Scale(vec2(aspect, 1.0));
    rendering->data.pipeline2D->Bind(commandBuffers[0]);
    vk::CmdBindVertexBuffer(commandBuffers[0], 0, rendering->data.vertexBuffer);
    vk::CmdBindIndexBuffer(commandBuffers[0], rendering->data.indexBuffer, VK_INDEX_TYPE_UINT32);
    vk::CmdSetViewportAndScissor(commandBuffers[0], rendering->window->width, rendering->window->height);
    vkCmdBindDescriptorSets(commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS, rendering->data.pipeline2D->data.layout,
            0, 1, &rendering->data.descriptors->data.sets[0].data.set, 0, nullptr);
    vkCmdPushConstants(commandBuffers[0], rendering->data.pipeline2D->data.layout,
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc.vert), &pc.vert);
    vkCmdPushConstants(commandBuffers[0], rendering->data.pipeline2D->data.layout,
            VK_SHADER_STAGE_FRAGMENT_BIT, offsetof(Rendering::PushConstants, frag), sizeof(pc.frag), &pc.frag);
    vkCmdDrawIndexed(commandBuffers[0], 6, 1, 0, 0, 0);
}

i32 main(i32 argumentCount, char** argumentValues) {

    bool enableLayers = false, enableCoreValidation = false;

    cout << "\nTest program received " << argumentCount << " arguments:\n";
    for (i32 i = 0; i < argumentCount; i++) {
        cout << i << ": " << argumentValues[i] << std::endl;
        if (equals(argumentValues[i], "--enable-layers")) {
            enableLayers = true;
        } else if (equals(argumentValues[i], "--core-validation")) {
            enableCoreValidation = true;
        }
    }

    cout << "Starting with layers " << (enableLayers ? "enabled" : "disabled")
         << " and core validation " << (enableCoreValidation ? "enabled" : "disabled") << std::endl;

    io::Input input;
    io::Window window;
    window.name = title;
    window.input = &input;

    Assets::Manager assets;
    assets.textures.Resize(1);
    assets.textures[0].filename = "data/test.tga";
    assets.LoadAll();

    Rendering::Manager rendering;
    rendering.textures = &assets.textures;
    rendering.data.instance.AppInfo(title, 1, 0, 0);
    rendering.AddRenderCallback(renderCallbackTest, nullptr);

    if (enableLayers) {
        Array<const char*> layers = {
            "VK_LAYER_GOOGLE_threading",
            "VK_LAYER_LUNARG_parameter_validation",
            "VK_LAYER_LUNARG_object_tracker",
            "VK_LAYER_GOOGLE_unique_objects"
        };
        if (enableCoreValidation) {
            layers.Append("VK_LAYER_LUNARG_core_validation");
        }
        rendering.data.instance.AddLayers(layers);
    }

    rendering.window = &window;

    if (!window.Open()) {
        cout << "Failed to open window: " << io::error << std::endl;
        return 1;
    }

    if (!rendering.Init()) {
        cout << "Failed to init Rendering::Manager: " << Rendering::error << std::endl;
        return 1;
    }

    if (!window.Show()) {
        cout << "Failed to show window: " << io::error << std::endl;
        return 1;
    }

    while (window.Update()) {
        if (input.Released(KC_KEY_ESC)) {
            break;
        }
        input.Tick(1.0/60.0);
        if (!rendering.Draw()) {
            cout << "Error in Rendering::Manager::Draw: " << Rendering::error << std::endl;
            return 1;
        }
    }

    return 0;
}
