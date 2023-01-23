/*
	File: main.cpp
	Author: Philip Haynes
	A basic test of io::Window and its interplay with io::Input
*/

#include "AzCore/io.hpp"
#include "AzCore/Thread.hpp"
#include "AzCore/Time.hpp"

using namespace AzCore;

io::Log cout("test.log");

i32 main(i32 argumentCount, char** argumentValues) {

	f32 scale = 1.0f;

	io::Window window;
	io::Input input;
	window.input = &input;
	window.width = 480;
	window.height = 480;
	if (!window.Open()) {
		cout.PrintLn("Failed to open Window: ", io::error);
		return 1;
	}

	scale = (f32)window.GetDPI() / 96.0f;
	window.Resize(u32((f32)window.width * scale), u32((u32)window.height * scale));

	cout.PrintLn("Window DPI: ", window.GetDPI(), ", scale = ", scale);

	if(!window.Show()) {
		cout.PrintLn("Failed to show Window: ", io::error);
		return 1;
	}
	do {
		for (i32 i = 0; i < 256; i++) {
			if (input.inputs[i].Pressed()) {
				cout.PrintLn("Pressed   HID 0x", FormatInt(i, 16), "\t", window.InputName(i));
			}
			if (input.inputs[i].Released()) {
				cout.PrintLn("Released  HID 0x", FormatInt(i, 16), "\t", window.InputName(i));
			}
		}
		if (input.Pressed(KC_KEY_H)) {
			cout.PrintLn("Toggling cursor visibility");
			window.HideCursor(!window.cursorHidden);
		}
		Thread::Sleep(Milliseconds(16));
		input.Tick(1.0f/60.0f);
	} while (window.Update());
	if (!window.Close()) {
		cout.PrintLn("Failed to close Window: ", io::error);
		return 1;
	}
	cout.PrintLn("Last io::error was \"", io::error, "\"");

	return 0;
}
