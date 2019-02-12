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

    cout << "\nTest program.\n\tReceived " << argumentCount << " arguments:\n";
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

    ListPtr<vk::Device> vkDevice = vkInstance.AddDevice();
    vkDevice->deviceFeaturesRequired.depthClamp = VK_TRUE;
    vkDevice->deviceFeaturesOptional.samplerAnisotropy = VK_TRUE;
    vkDevice->extensionsRequired = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    ListPtr<vk::Queue> queueGraphics = vkDevice->AddQueue();
    ListPtr<vk::Queue> queuePresent = vkDevice->AddQueue();
    ListPtr<vk::Queue> queueTransfer = vkDevice->AddQueue();
    ListPtr<vk::Queue> queueCompute = vkDevice->AddQueue();
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
    ListPtr<vk::Swapchain> vkSwapchain = vkDevice->AddSwapchain();
    vkSwapchain->windowIndex = vkInstance.AddWindowForSurface(&window);
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
