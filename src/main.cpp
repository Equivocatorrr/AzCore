/*
    File: main.cpp
    Author: Philip Haynes
    Description: High-level definition of the structure of our program.
*/

#include "io.hpp"
#include "vk.hpp"

#include "unit_tests.cpp"

i32 main(i32 argumentCount, char** argumentValues) {
    io::logStream cout("test.log");

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
            layers.push_back("VK_LAYER_LUNARG_core_validation");
        }
        vkInstance.AddLayers(layers);
    }

    vk::Device* vkDevice = vkInstance.AddDevice();
    vkDevice->deviceFeaturesRequired.depthClamp = VK_TRUE;
    vkDevice->deviceFeaturesOptional.samplerAnisotropy = VK_TRUE;
    vkDevice->extensionsRequired = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

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
    vkSwapchain->vsync = false;

    vk::RenderPass* renderPass = vkDevice->AddRenderPass();

    ArrayPtr<vk::Attachment> attachment[2] = {renderPass->AddAttachment(), renderPass->AddAttachment(vkSwapchain)};
    attachment[0]->bufferColor = true;
    attachment[0]->clearColor = true;
    attachment[0]->keepColor = true;
    attachment[0]->sampleCount = VK_SAMPLE_COUNT_4_BIT;
    attachment[0]->resolveColor = true;

    ArrayPtr<vk::Subpass> subpass[2] = {renderPass->AddSubpass(), renderPass->AddSubpass()};
    subpass[0]->UseAttachment(attachment[0], vk::ATTACHMENT_ALL,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    subpass[1]->UseAttachment(attachment[0], vk::ATTACHMENT_RESOLVE,
        VK_ACCESS_SHADER_READ_BIT);
    subpass[1]->UseAttachment(attachment[1], vk::ATTACHMENT_COLOR,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    vk::Memory *vkImageStagingMemory = vkDevice->AddMemory();
    vkImageStagingMemory->deviceLocal = false;
    vk::Memory *vkImageMemory = vkDevice->AddMemory();

    vk::Image image{};
    image.height = 64;
    image.width = 64;
    image.channels = 4;
    image.format = VK_FORMAT_R8G8B8A8_UNORM;
    image.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    image.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    ArrayPtr<vk::Image> vkStagingImage = vkImageStagingMemory->AddImage(image);
    vkStagingImage->usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    ArrayRange<vk::Image> vkImage = vkImageMemory->AddImages(3, image);

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
    vkDescriptorLayout[0]->bindings.push_back({0, 3});
    vkDescriptorLayout[1]->stage = VK_SHADER_STAGE_ALL_GRAPHICS;
    vkDescriptorLayout[1]->type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vkDescriptorLayout[1]->bindings.push_back({0, 2});

    ArrayPtr<vk::DescriptorSet> vkDescriptorSets[2] = {
        vkDescriptors->AddSet(vkDescriptorLayout[0]),
        vkDescriptors->AddSet(vkDescriptorLayout[1])
    };
    vkDescriptorSets[0]->AddDescriptor(vkImage, vkSampler, 0);
    vkDescriptorSets[1]->AddDescriptor(vkBuffers, 0);

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
        input.Tick(1.0/60.0);
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
