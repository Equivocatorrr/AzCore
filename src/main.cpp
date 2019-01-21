/*
    File: main.cpp
    Author: Philip Haynes
    Description: High-level definition of the structure of our program.
*/

#include "io.hpp"

i32 main(i32 argumentCount, char** argumentValues) {
    io::logStream cout("test.log");

    cout << "\nTest program.\n\tReceived " << argumentCount << " arguments:\n";
    for (i32 i = 0; i < argumentCount; i++) {
        cout << i << ": " << argumentValues[i] << std::endl;
    }

    io::Window window;
    io::Input input;
    window.input = &input;
    window.name = "Engine Test";
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
            cout << "Pressed HID " << io::HID_KEYCODE_NAMES[input.codeAny] << " '" << window.InputName(input.codeAny) << "'" << std::endl;
        }
        if (input.Any.Released()) {
            cout << "Released  HID " << io::HID_KEYCODE_NAMES[input.codeAny] << " '" << window.InputName(input.codeAny) << "'" << std::endl;
        }
        input.Tick(1.0/60.0);
    } while (window.Update());
    cout << "Last io::error was \"" << io::error << "\"" << std::endl;
    if (!window.Close()) {
        cout << "Failed to close Window: " << io::error << std::endl;
        return 1;
    }

    return 0;
}
