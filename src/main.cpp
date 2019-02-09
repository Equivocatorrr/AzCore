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

    cout << "\nTest program.\n\tReceived " << argumentCount << " arguments:\n";
    for (i32 i = 0; i < argumentCount; i++) {
        cout << i << ": " << argumentValues[i] << std::endl;
    }

    // PrintKeyCodeMapsEvdev(cout);
    // PrintKeyCodeMapsWinVK(cout);
    // PrintKeyCodeMapsWinScan(cout);

    vk::Instance vkInstance;
    vkInstance.AppInfo("AzCore Test Program", 0, 1, 0);

    u32 vkDeviceId;
    {
        vk::Device vkDevice;
        vkDevice.deviceFeaturesRequired.depthClamp = VK_TRUE;
        vkDevice.deviceFeaturesOptional.samplerAnisotropy = VK_TRUE;

        vkDeviceId = vkInstance.AddDevice(vkDevice);
    }
    vk::Device *vkDevice = vkInstance.GetDevice(vkDeviceId);
    vkDevice->extensionsRequired = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    {
        vk::Queue vkQueue;
        vkQueue.queueType = vk::GRAPHICS;
        vkDevice->AddQueue(vkQueue);
    }
    {
        vk::Queue vkQueue;
        vkQueue.queueType = vk::PRESENT;
        vkDevice->AddQueue(vkQueue);
    }
    {
        vk::Queue vkQueue;
        vkQueue.queueType = vk::TRANSFER;
        vkDevice->AddQueue(vkQueue);
    }
    {
        vk::Queue vkQueue;
        vkQueue.queueType = vk::COMPUTE;
        vkDevice->AddQueue(vkQueue);
    }

    io::Window window;
    io::Input input;
    window.input = &input;
    if (!window.Open()) {
        cout << "Failed to open Window: " << io::error << std::endl;
        return 1;
    }
    vkInstance.SetWindowForSurface(&window);
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
    if (!vkInstance.Deinit()) { // This should be all you need to call to clean everything up
        cout << "Failed to cleanup Vulkan: " << vk::error << std::endl;
    }
    cout << "Last io::error was \"" << io::error << "\"" << std::endl;
    cout << "Last vk::error was \"" << vk::error << "\"" << std::endl;

    return 0;
}
