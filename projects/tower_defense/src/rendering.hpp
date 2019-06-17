/*
    File: rendering.hpp
    Author: Philip Haynes
    Utilities and structures to aid in rendering.
*/

#ifndef RENDERING_HPP
#define RENDERING_HPP

#include "AzCore/memory.hpp"
#include "AzCore/vk.hpp"

namespace io {
    struct Window;
}

namespace Assets {
    struct Texture;
}

namespace Rendering {

struct Vertex {
    vec2 pos;
    vec2 tex;
};

struct PushConstants {
    struct {
        mat2 transform = mat2(1.0);
        vec2 origin = vec2(0.5);
        vec2 position = vec2(0.0);
    } vert;
    struct {
        vec4 color = vec4(1.0);
        int texIndex = 0;
    } frag;
};

extern String error;

typedef void (*fpRenderCallback_t)(void*, struct Manager*, Array<VkCommandBuffer>&);

struct RenderCallback {
    fpRenderCallback_t callback;
    void *userdata;
};

struct Manager {
    struct {
        vk::Instance instance;
        Ptr<vk::Device> device;
        Ptr<vk::Swapchain> swapchain;
        bool resized = false;
        Ptr<vk::Framebuffer> framebuffer;
        Ptr<vk::RenderPass> renderPass;
        Ptr<vk::Queue> queueGraphics;
        Ptr<vk::Queue> queuePresent;
        i32 concurrency = 2;
        Array<Ptr<vk::CommandPool>> commandPools;
        bool buffer = false; // Which primary command buffer we're on. Switches every frame.
        Ptr<vk::CommandBuffer> commandBufferPrimary[2]; // One for each buffer
        Array<Ptr<vk::CommandBuffer>> commandBuffersSecondary;

        Ptr<vk::Semaphore> semaphoreImageAvailable;
        Ptr<vk::Semaphore> semaphoreRenderComplete;
        Ptr<vk::QueueSubmission> queueSubmission[2]; // One for each buffer
        Ptr<vk::QueueSubmission> queueSubmissionTransfer;

        Ptr<vk::Sampler> textureSampler;

        Ptr<vk::Memory> stagingMemory;
        Ptr<vk::Memory> bufferMemory; // Uniform buffers, vertex buffers, index buffers
        Ptr<vk::Memory> textureMemory;

        Ptr<vk::Buffer> vertexBuffer;
        Ptr<vk::Buffer> indexBuffer;

        Ptr<vk::Pipeline> pipeline2D;
        Ptr<vk::Descriptors> descriptors;

        // Functions to call every time Draw is called, in the order they're added.
        Array<RenderCallback> renderCallbacks;
    } data;

    io::Window *window = nullptr;
    Array<Assets::Texture> *textures = nullptr;

    inline void AddRenderCallback(fpRenderCallback_t callback, void* userdata) {
        data.renderCallbacks.Append({callback, userdata});
    }

    bool Init();
    bool Draw();
};

}

#endif // RENDERING_HPP
