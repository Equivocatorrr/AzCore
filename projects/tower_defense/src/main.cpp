/*
	File: main.cpp
	Author: Philip Haynes
	Description: High-level definition of the structure of our program.
*/

#include "Az2D/game_systems.hpp"
#include "gui.hpp"
#include "entities.hpp"
#include "Az2D/settings.hpp"

i32 main(i32 argumentCount, char** argumentValues) {

	Az2D::Entities::Manager entities;
	Az2D::Gui::Gui gui;
	
	bool enableLayers = false;

	az::io::cout.PrintLn("\nTest program received ", argumentCount, " arguments:");
	for (i32 i = 0; i < argumentCount; i++) {
		az::io::cout.PrintLn(i, ": ", argumentValues[i]);
		if (az::equals(argumentValues[i], "--validation")) {
			enableLayers = true;
		}
	}

	az::io::cout.PrintLn("Starting with layers ", (enableLayers ? "enabled" : "disabled"));
	
	if (!Az2D::GameSystems::Init("AzCore Tower Defense", {&entities, &gui}, enableLayers)) {
		az::io::cerr.PrintLn("Failed to Init: ", Az2D::GameSystems::sys->error);
		return 1;
	}
	Az2D::GameSystems::sys->window.HideCursor(true);

	Az2D::GameSystems::UpdateLoop();

	Az2D::GameSystems::Deinit();
	return 0;
}
