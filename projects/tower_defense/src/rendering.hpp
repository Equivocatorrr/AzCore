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

extern String error;

typedef void (*fpRenderCallback_t)(struct Manager*, Array<VkCommandBuffer>&);

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
        Array<fpRenderCallback_t> renderCallbacks;
    } data;

    io::Window *window = nullptr;
    Array<Assets::Texture> *textures = nullptr;

    inline void AddRenderCallback(fpRenderCallback_t callback) {
        data.renderCallbacks.Append(callback);
    }

    bool Init();
    bool Draw();
};

}

#endif // RENDERING_HPP
