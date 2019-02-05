/*
    File: main.cpp
    Author: Philip Haynes
    Description: High-level definition of the structure of our program.
*/

#include "io.hpp"
#include "vk.hpp"

void Print(vec3 v, io::logStream& cout) {
    cout << "{";
    for (u32 i = 0; i < 3; i++) {
        if (v[i] >= 0.0)
            cout << " ";
        if (v[i] < 10.0)
            cout << " ";
        if (v[i] < 100.0)
            cout << " ";
        cout << v[i];
        if (i != 2)
            cout << ", ";
    }
    cout << "}";
}

void Print(mat3 m, io::logStream& cout) {
    cout << "[" << std::fixed << std::setprecision(3);
    Print(m.Row1(), cout);
    cout << "\n ";
    Print(m.Row2(), cout);
    cout << "\n ";
    Print(m.Row3(), cout);
    cout << "]" << std::endl;
}

void Print(vec4 v, io::logStream& cout) {
    cout << "{";
    for (u32 i = 0; i < 4; i++) {
        if (v[i] >= 0.0)
            cout << " ";
        if (v[i] < 10.0)
            cout << " ";
        if (v[i] < 100.0)
            cout << " ";
        cout << v[i];
        if (i != 3)
            cout << ", ";
    }
    cout << "}";
}

void Print(mat4 m, io::logStream& cout) {
    cout << "[" << std::fixed << std::setprecision(3);
    Print(m.Row1(), cout);
    cout << "\n ";
    Print(m.Row2(), cout);
    cout << "\n ";
    Print(m.Row3(), cout);
    cout << "\n ";
    Print(m.Row4(), cout);
    cout << "]" << std::endl;
}

void UnitTestMat3(io::logStream& cout) {
    cout << "Unit testing mat3\n";
    cout << "identity = \n";
    Print(mat3(), cout);
    cout << "\nRow1 = ";
    Print(mat3().Row1(), cout);
    cout << "\nRow2 = ";
    Print(mat3().Row2(), cout);
    cout << "\nRow3 = ";
    Print(mat3().Row3(), cout);
    cout << "\nCol1 = ";
    Print(mat3().Col1(), cout);
    cout << "\nCol2 = ";
    Print(mat3().Col2(), cout);
    cout << "\nCol3 = ";
    Print(mat3().Col3(), cout);
    cout << "\nRotated pi/4 around x-axis:\n";
    Print(mat3::RotationBasic(halfpi/2, Axis::X), cout);
    Print(mat3::Rotation(halfpi/2, {1.0, 0.0, 0.0}), cout);
    cout << "\nRotated pi/4 around y-axis:\n";
    Print(mat3::RotationBasic(halfpi/2, Axis::Y), cout);
    Print(mat3::Rotation(halfpi/2, {0.0, 1.0, 0.0}), cout);
    cout << "\nRotated pi/4 around z-axis:\n";
    Print(mat3::RotationBasic(halfpi/2, Axis::Z), cout);
    Print(mat3::Rotation(halfpi/2, {0.0, 0.0, 1.0}), cout);
    cout << "\nScaled by {2.0, 2.0, 2.0}:\n";
    Print(mat3::Scaler({2.0, 2.0, 2.0}), cout);
    cout << "\nRotated by pi about {0.5, 0.5, 0.0}:\n";
    Print(mat3::Rotation(pi, {0.5, 0.5, 0.0}), cout);
    cout << "\nRotated by pi about {0.5, 0.5, 0.5}:\n";
    Print(mat3::Rotation(pi, {0.5, 0.5, 0.5}), cout);
    mat3 m = mat3(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    cout << "New mat3 = \n";
    Print(m, cout);
    cout << "Transpose:\n";
    Print(m.Transpose(), cout);
}

void UnitTestMat4(io::logStream& cout) {
    cout << "Unit testing mat4\n";
    cout << "identity = \n";
    Print(mat4(), cout);
    cout << "\nRow1 = ";
    Print(mat4().Row1(), cout);
    cout << "\nRow2 = ";
    Print(mat4().Row2(), cout);
    cout << "\nRow3 = ";
    Print(mat4().Row3(), cout);
    cout << "\nRow4 = ";
    Print(mat4().Row4(), cout);
    cout << "\nCol1 = ";
    Print(mat4().Col1(), cout);
    cout << "\nCol2 = ";
    Print(mat4().Col2(), cout);
    cout << "\nCol3 = ";
    Print(mat4().Col3(), cout);
    cout << "\nCol4 = ";
    Print(mat4().Col4(), cout);
    cout << "\nRotated pi/4 around xy-plane:\n";
    Print(mat4::RotationBasic(halfpi/2, Plane::XY), cout);
    cout << "\nRotated pi/4 around xz-plane:\n";
    Print(mat4::RotationBasic(halfpi/2, Plane::XZ), cout);
    cout << "\nRotated pi/4 around xw-plane:\n";
    Print(mat4::RotationBasic(halfpi/2, Plane::XW), cout);
    cout << "\nRotated pi/4 around yz-plane:\n";
    Print(mat4::RotationBasic(halfpi/2, Plane::YZ), cout);
    cout << "\nRotated pi/4 around yw-plane:\n";
    Print(mat4::RotationBasic(halfpi/2, Plane::YW), cout);
    cout << "\nRotated pi/4 around zw-plane:\n";
    Print(mat4::RotationBasic(halfpi/2, Plane::ZW), cout);
    cout << "\nScaled by {2.0, 2.0, 2.0, 2.0}:\n";
    Print(mat4::Scaler({2.0, 2.0, 2.0, 2.0}), cout);
    cout << "\nRotated by pi about {0.5, 0.5, 0.0}:\n";
    Print(mat4::Rotation(pi, {0.5, 0.5, 0.0}), cout);
    cout << "\nRotated by pi about {0.5, 0.5, 0.5}:\n";
    Print(mat4::Rotation(pi, {0.5, 0.5, 0.5}), cout);
    mat4 m = mat4( 1.0,  2.0,  3.0,  4.0,
                   5.0,  6.0,  7.0,  8.0,
                   9.0, 10.0, 11.0, 12.0,
                  13.0, 14.0, 15.0, 16.0);
    cout << "New mat4 = \n";
    Print(m, cout);
    cout << "Transpose:\n";
    Print(m.Transpose(), cout);
}

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
            UnitTestMat4(cout);
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
