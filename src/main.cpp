/*
    File: main.cpp
    Author: Philip Haynes
    Description: High-level definition of the structure of our program.
*/

#include "io.hpp"
#include "vk.hpp"

#define pow(v, e) pow((double)v, (double)e)
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#undef pow

io::logStream cout("test.log");

#include "unit_tests.cpp"
#include "persistence.cpp"

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

    UnitTestArrayAndString(cout);

    // return 0;

    // BigIntTest();
    //
    // CheckNumbersForHighPersistence();

    // return 0;

    // PrintKeyCodeMapsEvdev(cout);
    // PrintKeyCodeMapsWinVK(cout);
    // PrintKeyCodeMapsWinScan(cout);

    struct Image {
        UniquePtr<u8, void(&)(void*)> pixels;
        i32 width;
        i32 height;
        i32 channels;
        Image(const char *filename) : pixels(stbi_load(filename, &width, &height, &channels, 4), stbi_image_free) {
            channels = 4;
        }
    };
    Image image("data/icon.png");
    if (image.pixels == nullptr) {
        cout << "Failed to load image!" << std::endl;
        return 1;
    }

    vk::Instance vkInstance;
    vkInstance.AppInfo("AzCore Test Program", 0, 1, 0);

    if (enableLayers) {
        cout << "Validation layers enabled." << std::endl;
        Array<const char*> layers = {
            "VK_LAYER_GOOGLE_threading",
    		"VK_LAYER_LUNARG_parameter_validation",
    		"VK_LAYER_LUNARG_object_tracker",
    		"VK_LAYER_GOOGLE_unique_objects"
        };
        if (enableCoreValidation) {
            layers.Append("VK_LAYER_LUNARG_core_validation");
        }
        vkInstance.AddLayers(layers);
    }

    Ptr<vk::Device> vkDevice = vkInstance.AddDevice();

    Ptr<vk::Queue> queueGraphics = vkDevice->AddQueue();
    Ptr<vk::Queue> queuePresent = vkDevice->AddQueue();
    Ptr<vk::Queue> queueTransfer = vkDevice->AddQueue();
    Ptr<vk::Queue> queueCompute = vkDevice->AddQueue();
    queueGraphics->queueType = vk::GRAPHICS;
    queuePresent->queueType = vk::PRESENT;
    queueTransfer->queueType = vk::TRANSFER;
    queueCompute->queueType = vk::COMPUTE;

    io::Window window;
    io::Input input;
    window.input = &input;
    if (!window.Open()) {
        cout << "Failed to open Window: " << io::error << std::endl;
        return 1;
    }

    Ptr<vk::Swapchain> vkSwapchain = vkDevice->AddSwapchain();
    vkSwapchain->window = vkInstance.AddWindowForSurface(&window);
    vkSwapchain->vsync = true;

    Ptr<vk::RenderPass> vkRenderPass = vkDevice->AddRenderPass();

    Ptr<vk::Attachment> attachment = vkRenderPass->AddAttachment(vkSwapchain);
    attachment->clearColor = true;
    attachment->clearColorValue = {0.0, 0.05, 0.1, 1.0};
    // attachment->sampleCount = VK_SAMPLE_COUNT_4_BIT;
    attachment->resolveColor = true;

    Ptr<vk::Subpass> subpass = vkRenderPass->AddSubpass();
    subpass->UseAttachment(attachment, vk::ATTACHMENT_COLOR,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    Ptr<vk::Memory> vkBufferStagingMemory = vkDevice->AddMemory();
    vkBufferStagingMemory->deviceLocal = false;
    Ptr<vk::Memory> vkBufferMemory = vkDevice->AddMemory();

    Ptr<vk::Memory> vkImageMemory = vkDevice->AddMemory();

    struct Vertex {
        vec2 position;
        vec2 texCoord;
    };

    Array<Vertex> vertices = {
        {vec2(-0.5, -0.5), vec2(0.0, 0.0)},
        {vec2(-0.5, 0.5), vec2(0.0, 1.0)},
        {vec2(0.5, 0.5), vec2(1.0, 1.0)},
        {vec2(0.5, -0.5), vec2(1.0, 0.0)}
    };
    Array<u32> indices = {0, 1, 2, 2, 3, 0};

    Range<vk::Buffer> vkStagingBuffers = vkBufferStagingMemory->AddBuffers(3);
    vkStagingBuffers[0].size = vertices.size * sizeof(Vertex);
    vkStagingBuffers[1].size = indices.size * sizeof(u32);
    vkStagingBuffers[2].size = image.width * image.height * image.channels;
    vkStagingBuffers[0].usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    vkStagingBuffers[1].usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    vkStagingBuffers[2].usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    Ptr<vk::Buffer> vkVertexBuffer = vkBufferMemory->AddBuffer();
    Ptr<vk::Buffer> vkIndexBuffer = vkBufferMemory->AddBuffer();
    vkVertexBuffer->size = vkStagingBuffers[0].size;
    vkIndexBuffer->size = vkStagingBuffers[1].size;
    vkVertexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkIndexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    Ptr<vk::Image> vkTextureImage = vkImageMemory->AddImage();
    vkTextureImage->format = VK_FORMAT_R8G8B8A8_UNORM;
    vkTextureImage->width = image.width;
    vkTextureImage->height = image.height;
    vkTextureImage->mipLevels = floor(log2(max(image.width, image.height))) + 1;
    vkTextureImage->usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    Ptr<vk::Sampler> vkSampler = vkDevice->AddSampler();
    vkSampler->maxLod = vkTextureImage->mipLevels;
    vkSampler->anisotropy = 16;
    vkSampler->mipLodBias = -0.5;

    Ptr<vk::Descriptors> vkDescriptors = vkDevice->AddDescriptors();
    Ptr<vk::DescriptorLayout> vkDescriptorLayoutTexture = vkDescriptors->AddLayout();
    vkDescriptorLayoutTexture->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    vkDescriptorLayoutTexture->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    vkDescriptorLayoutTexture->bindings.Resize(1);
    vkDescriptorLayoutTexture->bindings[0].binding = 0;
    vkDescriptorLayoutTexture->bindings[0].count = 1;
    Ptr<vk::DescriptorSet> vkDescriptorSetTexture = vkDescriptors->AddSet(vkDescriptorLayoutTexture);
    if (!vkDescriptorSetTexture->AddDescriptor(vkTextureImage, vkSampler, 0)) {
        cout << "Failed to add Texture Descriptor: " << vk::error << std::endl;
        return 1;
    }

    Range<vk::Shader> vkShaders = vkDevice->AddShaders(2);
    vkShaders[0].filename = "data/shaders/test.vert.spv";
    vkShaders[1].filename = "data/shaders/test.frag.spv";

    vk::ShaderRef vkShaderRefs[2] = {
        vk::ShaderRef(vkShaders.ToPtr(0), VK_SHADER_STAGE_VERTEX_BIT),
        vk::ShaderRef(vkShaders.ToPtr(1), VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    Ptr<vk::Pipeline> vkPipeline = vkDevice->AddPipeline();
    vkPipeline->renderPass = vkRenderPass;
    vkPipeline->subpass = 0;
    vkPipeline->shaders.Append(vkShaderRefs[0]);
    vkPipeline->shaders.Append(vkShaderRefs[1]);

    vkPipeline->descriptorLayouts.Append(vkDescriptorLayoutTexture);

    vkPipeline->dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkVertexInputAttributeDescription vertexInputAttributeDescription = {};
    vertexInputAttributeDescription.binding = 0;
    vertexInputAttributeDescription.location = 0;
    vertexInputAttributeDescription.offset = offsetof(Vertex, position);
    vertexInputAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
    vkPipeline->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
    vertexInputAttributeDescription.location = 1;
    vertexInputAttributeDescription.offset = offsetof(Vertex, texCoord);
    vertexInputAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
    vkPipeline->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
    VkVertexInputBindingDescription vertexInputBindingDescription = {};
    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexInputBindingDescription.stride = sizeof(Vertex);
    vkPipeline->inputBindingDescriptions.Append(vertexInputBindingDescription);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    vkPipeline->colorBlendAttachments.Append(colorBlendAttachment);

    Ptr<vk::CommandPool> vkCommandPool = vkDevice->AddCommandPool(queueGraphics);
    vkCommandPool->transient = true;
    vkCommandPool->resettable = true;
    Ptr<vk::CommandBuffer> vkCommandBuffer = vkCommandPool->AddCommandBuffer();
    vkCommandBuffer->oneTimeSubmit = true;

    Ptr<vk::Framebuffer> vkFramebuffer = vkDevice->AddFramebuffer();
    vkFramebuffer->renderPass = vkRenderPass;
    vkFramebuffer->swapchain = vkSwapchain;

    Ptr<vk::Semaphore> semaphoreRenderFinished = vkDevice->AddSemaphore();

    Ptr<vk::QueueSubmission> vkQueueSubmission = vkDevice->AddQueueSubmission();
    vkQueueSubmission->commandBuffers = {vkCommandBuffer};
    vkQueueSubmission->signalSemaphores = {semaphoreRenderFinished};

    Ptr<vk::QueueSubmission> vkTransferQueueSubmission = vkDevice->AddQueueSubmission();
    vkTransferQueueSubmission->commandBuffers = {vkCommandBuffer};
    vkTransferQueueSubmission->signalSemaphores = {};
    vkTransferQueueSubmission->waitSemaphores = {};

    if (!vkInstance.Init()) { // Do this once you've set up the structure of your program.
        cout << "Failed to initialize Vulkan: " << vk::error << std::endl;
        return 1;
    }

    vkStagingBuffers[0].CopyData(vertices.data);
    vkStagingBuffers[1].CopyData(indices.data);
    vkStagingBuffers[2].CopyData(image.pixels.get());

    VkCommandBuffer cmdBufCopy = vkCommandBuffer->Begin();
    vkVertexBuffer->Copy(cmdBufCopy, vkStagingBuffers.ToPtr(0));
    vkIndexBuffer->Copy(cmdBufCopy, vkStagingBuffers.ToPtr(1));

    vkTextureImage->TransitionLayout(cmdBufCopy, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkTextureImage->Copy(cmdBufCopy, vkStagingBuffers.ToPtr(2));
    vkTextureImage->GenerateMipMaps(cmdBufCopy, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if (!vkCommandBuffer->End()) {
        cout << "Failed to copy from staging buffers: " << vk::error << std::endl;
        return 1;
    }
    vkDevice->SubmitCommandBuffers(queueGraphics, {vkTransferQueueSubmission});
    vkQueueWaitIdle(queueGraphics->queue);

    if (!vkDescriptors->Update()) {
        cout << "Failed to update descriptors: " << vk::error << std::endl;
        return 1;
    }

    if(!window.Show()) {
        cout << "Failed to show Window: " << io::error << std::endl;
        return 1;
    }
    RandomNumberGenerator rng;
    bool resize = false;
    do {
        input.Tick(1.0/60.0);
        if (input.Any.Pressed()) {
            cout << "Pressed HID " << std::hex << (u32)input.codeAny << std::endl;
            cout << "\t" << window.InputName(input.codeAny) << std::endl;
        }
        if (input.Any.Released()) {
            cout << "Released  HID " << std::hex << (u32)input.codeAny << std::endl;
            cout << "\t" << window.InputName(input.codeAny) << std::endl;
        }
        if (input.Pressed(KC_KEY_T)) {
            UnitTestMat3(cout);
            UnitTestMat4(cout);
            UnitTestComplex(cout);
            UnitTestQuat(cout);
            UnitTestSlerp(cout);
            UnitTestList(cout);
        }
        if (input.Pressed(KC_KEY_R)) {
            UnitTestRNG(rng, cout);
        }

        if (window.resized || resize) {
            if (!vkSwapchain->Resize()) {
                cout << "Failed to resize vkSwapchain: " << vk::error << std::endl;
                return 1;
            }
            resize = false;
        }

        VkResult acquisitionResult = vkSwapchain->AcquireNextImage();

        if (acquisitionResult == VK_ERROR_OUT_OF_DATE_KHR || acquisitionResult == VK_NOT_READY) {
            cout << "Skipping a frame because acquisition returned: " << vk::ErrorString(acquisitionResult) << std::endl;
            resize = true;
            continue; // Don't render this frame.
        } else if (acquisitionResult == VK_TIMEOUT) {
            cout << "Skipping a frame because acquisition returned: " << vk::ErrorString(acquisitionResult) << std::endl;
            continue;
        } else if (acquisitionResult != VK_SUCCESS) {
            cout << vk::error << std::endl;
            return 1;
        }

        // Begin recording commands

        VkCommandBuffer cmdBuf = vkCommandBuffer->Begin();
        if (cmdBuf == VK_NULL_HANDLE) {
            cout << "Failed to Begin recording vkCommandBuffer: " << vk::error << std::endl;
            return 1;
        }

        vkRenderPass->Begin(cmdBuf, vkFramebuffer);

        vkPipeline->Bind(cmdBuf);

        VkViewport viewport{};
        viewport.width = window.width;
        viewport.height = window.height;
        viewport.maxDepth = 1.0;
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = {window.width, window.height};
        vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

        VkDeviceSize zeroOffset = 0;

        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &vkVertexBuffer->data.buffer, &zeroOffset);
        vkCmdBindIndexBuffer(cmdBuf, vkIndexBuffer->data.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->data.layout, 0, 1, &vkDescriptors->data.sets[0].data.set, 0, nullptr);

        vkCmdDrawIndexed(cmdBuf, 6, 1, 0, 0, 0);

        vkCmdEndRenderPass(cmdBuf);

        vkCommandBuffer->End();

        // We have a different semaphore to wait on every frame for multi-buffered swapchains.
        vkQueueSubmission->waitSemaphores = {
            {vkSwapchain->SemaphoreImageAvailable(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}
        };
        // Re-config because we changed which semaphore to wait on
        if (!vkQueueSubmission->Config()) {
            cout << "Failed to re-Config vkQueueSubmission: " << vk::error << std::endl;
            return 1;
        }

        // Submit to queue
        if (!vkDevice->SubmitCommandBuffers(queueGraphics, {vkQueueSubmission})) {
            cout << "Failed to SubmitCommandBuffers: " << vk::error << std::endl;
            return 1;
        }

        if (!vkSwapchain->Present(queuePresent, {semaphoreRenderFinished->semaphore})) {
            cout << vk::error << std::endl;
            return 1;
        }

        vkDeviceWaitIdle(vkDevice->data.device);

    } while (window.Update());
    // This should be all you need to call to clean everything up
    // But you also could just let the vk::Instance go out of scope and it will
    // clean itself up.
    if (!vkInstance.Deinit()) {
        cout << "Failed to cleanup Vulkan Tree: " << vk::error << std::endl;
    }
    if (!window.Close()) {
        cout << "Failed to close Window: " << io::error << std::endl;
        return 1;
    }
    cout << "Last io::error was \"" << io::error << "\"" << std::endl;
    cout << "Last vk::error was \"" << vk::error << "\"" << std::endl;

    return 0;
}
