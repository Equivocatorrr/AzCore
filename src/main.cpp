/*
    File: main.cpp
    Author: Philip Haynes
    Description: High-level definition of the structure of our program.
*/

#include "io.hpp"
#include "vk.hpp"

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

    // BigIntTest();
    //
    // CheckNumbersForHighPersistence();

    // return 0;

    // PrintKeyCodeMapsEvdev(cout);
    // PrintKeyCodeMapsWinVK(cout);
    // PrintKeyCodeMapsWinScan(cout);

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

    vk::Device* vkDevice = vkInstance.AddDevice();

    vk::Queue* queueGraphics = vkDevice->AddQueue();
    vk::Queue* queuePresent = vkDevice->AddQueue();
    vk::Queue* queueTransfer = vkDevice->AddQueue();
    vk::Queue* queueCompute = vkDevice->AddQueue();
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

    vk::Swapchain* vkSwapchain = vkDevice->AddSwapchain();
    vkSwapchain->window = vkInstance.AddWindowForSurface(&window);
    vkSwapchain->vsync = true;

    vk::RenderPass* vkRenderPass = vkDevice->AddRenderPass();

    ArrayPtr<vk::Attachment> attachment = vkRenderPass->AddAttachment(vkSwapchain);
    attachment->clearColor = true;
    attachment->clearColorValue = {0.0, 0.05, 0.1, 1.0};
    attachment->sampleCount = VK_SAMPLE_COUNT_8_BIT;
    attachment->resolveColor = true;

    ArrayPtr<vk::Subpass> subpass = vkRenderPass->AddSubpass();
    subpass->UseAttachment(attachment, vk::ATTACHMENT_COLOR,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    vk::Memory *vkImageStagingMemory = vkDevice->AddMemory();
    vkImageStagingMemory->deviceLocal = false;
    vk::Memory *vkImageMemory = vkDevice->AddMemory();

    vk::Image image{};
    image.height = 64;
    image.width = 64;
    image.format = VK_FORMAT_R8G8B8A8_UNORM;
    image.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    ArrayPtr<vk::Buffer> vkStagingImage = vkImageStagingMemory->AddBuffer();
    vkStagingImage->size = image.height * image.width * 4;
    vkStagingImage->usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    ArrayRange<vk::Image> vkImage = vkImageMemory->AddImages(2, image);

    vk::Memory *vkBufferStagingMemory = vkDevice->AddMemory();
    vkBufferStagingMemory->deviceLocal = false;
    vk::Memory *vkBufferMemory = vkDevice->AddMemory();

    ArrayRange<vk::Buffer> vkStagingBuffers = vkBufferStagingMemory->AddBuffers(2);
    vkStagingBuffers[0].size = sizeof(u32) * 4;
    vkStagingBuffers[1].size = sizeof(u32) * 16;
    vkStagingBuffers[0].usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    vkStagingBuffers[1].usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    ArrayRange<vk::Buffer> vkBuffers = vkBufferMemory->AddBuffers(2);
    vkBuffers[0].size = sizeof(u32) * 4;
    vkBuffers[1].size = sizeof(u32) * 16;
    vkBuffers[0].usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    vkBuffers[1].usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    ArrayPtr<vk::Sampler> vkSampler = vkDevice->AddSampler();
    vkSampler->anisotropy = 16;

    vk::Descriptors* vkDescriptors = vkDevice->AddDescriptors();
    ArrayPtr<vk::DescriptorLayout> vkDescriptorLayout[2] = {vkDescriptors->AddLayout(), vkDescriptors->AddLayout()};
    vkDescriptorLayout[0]->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    vkDescriptorLayout[0]->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    vkDescriptorLayout[0]->bindings.Append({0, 2});
    vkDescriptorLayout[1]->stage = VK_SHADER_STAGE_ALL_GRAPHICS;
    vkDescriptorLayout[1]->type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vkDescriptorLayout[1]->bindings.Append({0, 2});

    ArrayPtr<vk::DescriptorSet> vkDescriptorSets[2] = {
        vkDescriptors->AddSet(vkDescriptorLayout[0]),
        vkDescriptors->AddSet(vkDescriptorLayout[1])
    };
    if (!vkDescriptorSets[0]->AddDescriptor(vkImage, vkSampler, 0)) {
        cout << "Failed to add descriptor: " << vk::error << std::endl;
        return 1;
    }
    if (!vkDescriptorSets[1]->AddDescriptor(vkBuffers, 0)) {
        cout << "Failed to add descriptor: " << vk::error << std::endl;
        return 1;
    }

    ArrayRange<vk::Shader> vkShaders = vkDevice->AddShaders(2);
    vkShaders[0].filename = "data/shaders/test.vert.spv";
    vkShaders[1].filename = "data/shaders/test.frag.spv";

    vk::ShaderRef vkShaderRefs[2] = {
        vk::ShaderRef(vkShaders.Ptr(0), VK_SHADER_STAGE_VERTEX_BIT),
        vk::ShaderRef(vkShaders.Ptr(1), VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    vk::Pipeline *vkPipeline = vkDevice->AddPipeline();
    vkPipeline->renderPass = vkRenderPass;
    vkPipeline->subpass = 0;
    vkPipeline->shaders.Append(vkShaderRefs[0]);
    vkPipeline->shaders.Append(vkShaderRefs[1]);

    vkPipeline->dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    vkPipeline->colorBlendAttachments.Append(colorBlendAttachment);

    vkPipeline->descriptorLayouts.Append(vkDescriptorLayout[0]);
    vkPipeline->descriptorLayouts.Append(vkDescriptorLayout[1]);

    vk::CommandPool* vkCommandPool = vkDevice->AddCommandPool(queueGraphics);
    vkCommandPool->transient = true;
    vkCommandPool->resettable = true;
    ArrayPtr<vk::CommandBuffer> vkCommandBuffer = vkCommandPool->AddCommandBuffer();
    vkCommandBuffer->oneTimeSubmit = true;

    vk::Framebuffer* vkFramebuffer = vkDevice->AddFramebuffer();
    vkFramebuffer->renderPass = vkRenderPass;
    vkFramebuffer->swapchain = vkSwapchain;

    ArrayPtr<VkSemaphore> semaphoreRenderFinished = vkDevice->AddSemaphore();

    vk::QueueSubmission* vkQueueSubmission = vkDevice->AddQueueSubmission();
    vkQueueSubmission->commandBuffers = {vkCommandBuffer};
    vkQueueSubmission->signalSemaphores = {semaphoreRenderFinished};

    if (!vkInstance.Init()) { // Do this once you've set up the structure of your program.
        cout << "Failed to initialize Vulkan: " << vk::error << std::endl;
        return 1;
    }

    if(!window.Show()) {
        cout << "Failed to show Window: " << io::error << std::endl;
        return 1;
    }
    RandomNumberGenerator rng;
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

        if (window.resized) {
            if (!vkSwapchain->Resize()) {
                cout << "Failed to resize vkSwapchain: " << vk::error << std::endl;
                return 1;
            }
        }

        VkResult acquisitionResult = vkSwapchain->AcquireNextImage();

        if (acquisitionResult == VK_ERROR_OUT_OF_DATE_KHR || acquisitionResult == VK_TIMEOUT || acquisitionResult == VK_NOT_READY) {
            cout << "Skipping a frame because acquisition returned: " << vk::ErrorString(acquisitionResult) << std::endl;
            continue; // Don't render this frame.
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

        vkFramebuffer->RenderPassBegin(cmdBuf);

        vkPipeline->Bind(cmdBuf);

        VkViewport viewport{};
        viewport.width = window.width;
        viewport.height = window.height;
        viewport.maxDepth = 1.0;
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = {window.width, window.height};
        vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

        vkCmdDraw(cmdBuf, 3, 1, 0, 0);

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

        if (!vkSwapchain->Present(queuePresent, {*semaphoreRenderFinished})) {
            cout << vk::error << std::endl;
            return 1;
        }

        vkDeviceWaitIdle(vkDevice->data.device);

    } while (window.Update());
    if (!window.Close()) {
        cout << "Failed to close Window: " << io::error << std::endl;
        return 1;
    }
    // This should be all you need to call to clean everything up
    // But you also could just let the vk::Instance go out of scope and it will
    // clean itself up.
    if (!vkInstance.Deinit()) {
        cout << "Failed to cleanup Vulkan Tree: " << vk::error << std::endl;
    }
    cout << "Last io::error was \"" << io::error << "\"" << std::endl;
    cout << "Last vk::error was \"" << vk::error << "\"" << std::endl;

    return 0;
}
