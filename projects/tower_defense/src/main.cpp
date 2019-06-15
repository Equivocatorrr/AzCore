/*
    File: main.cpp
    Author: Philip Haynes
    Description: High-level definition of the structure of our program.
*/

#include "AzCore/io.hpp"
#include "AzCore/vk.hpp"

io::logStream cout("main.log");

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

    return 0;
}
