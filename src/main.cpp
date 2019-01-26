/*
    File: main.cpp
    Author: Philip Haynes
    Description: High-level definition of the structure of our program.
*/

#include "io.hpp"
#include "vk.hpp"

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
    if (!vkInstance.CreateAll()) {
        cout << "Failed to initialize Vulkan: " << vk::error << std::endl;
    }
    io::Window window;
    io::Input input;
    window.input = &input;
    if (!window.Open()) {
        cout << "Failed to open Window: " << io::error << std::endl;
        return 1;
    }
    if(!window.Show()) {
        cout << "Failed to show Window: " << io::error << std::endl;
        return 1;
    }
    do {
        if (input.Any.Pressed()) {
            cout << "Pressed HID " << std::hex << (u32)input.codeAny << std::endl;
            cout << "\t" << window.InputName(input.codeAny) << std::endl;
        }
        if (input.Any.Released()) {
            cout << "Released  HID " << std::hex << (u32)input.codeAny << std::endl;
            cout << "\t" << window.InputName(input.codeAny) << std::endl;
        }
        input.Tick(1.0/60.0);
    } while (window.Update());
    if (!window.Close()) {
        cout << "Failed to close Window: " << io::error << std::endl;
        return 1;
    }
    if (!vkInstance.DestroyAll()) {
        cout << "Failed to cleanup Vulkan: " << vk::error << std::endl;
    }
    cout << "Last io::error was \"" << io::error << "\"" << std::endl;
    cout << "Last vk::error was \"" << vk::error << "\"" << std::endl;

    return 0;
}
