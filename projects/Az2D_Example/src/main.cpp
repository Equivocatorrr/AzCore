/*
	File: main.cpp
	Author: Philip Haynes
	Description: High-level definition of the structure of our program.
*/

#include "gui.hpp"
#include "entities.hpp"

#include "Az2D/game_systems.hpp"
#include "Az2D/settings.hpp"
#include "Az2D/profiling.hpp"

i32 main(i32 argumentCount, char** argumentValues) {

	Az2D::Entities::Manager entities;
	Az2D::Gui::Gui gui;
	
	Az2D::Settings::Name sTest = "testSetting";
	Az2D::Settings::Add(sTest, Az2D::Settings::Setting(az::String("HEY! You there!")));

	bool enableLayers = false;

	az::io::cout.PrintLn("\nTest program received ", argumentCount, " arguments:");
	for (i32 i = 0; i < argumentCount; i++) {
		az::io::cout.PrintLn(i, ": ", argumentValues[i]);
		if (az::equals(argumentValues[i], "--validation")) {
			enableLayers = true;
		} else if (az::equals(argumentValues[i], "--profiling")) {
			az::io::cout.PrintLn("Enabling profiling");
			Az2D::Profiling::Enable();
		}
	}

	az::io::cout.PrintLn("Starting with layers ", (enableLayers ? "enabled" : "disabled"));
	
	if (!Az2D::GameSystems::Init("Az2D Example", {&entities, &gui}, enableLayers)) {
		az::io::cerr.PrintLn("Failed to Init: ", Az2D::GameSystems::sys->error);
		return 1;
	}

	az::io::cout.PrintLn("testSetting = \"", Az2D::Settings::ReadString(sTest), "\"");

	Az2D::GameSystems::UpdateLoop();

	Az2D::GameSystems::Deinit();
	return 0;
}
