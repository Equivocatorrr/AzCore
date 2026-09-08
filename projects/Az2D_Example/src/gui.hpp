/*
	File: gui.hpp
	Author: Philip Haynes
*/

#ifndef GUI_HPP
#define GUI_HPP

#include "Az2D/gui_basics.hpp"

namespace Az2D::Gui {

namespace azgui = az::GuiGeneric;

// Now we can have some different screens
struct MainMenu {
	azgui::Screen *screen;
	azgui::Hideable *continueHideable;
	azgui::Button *buttonContinue;
	azgui::Button *buttonNewGame;
	azgui::Button *buttonSettings;
	azgui::Button *buttonExit;

	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct SettingsMenu {
	azgui::Screen *screen;
	azgui::Checkbox *checkFullscreen;
	azgui::Checkbox *checkVSync;
	azgui::Hideable *framerateHideable;
	azgui::Textbox *textboxFramerate;
	azgui::Slider *sliderVolumes[3];
	azgui::Textbox *textboxVolumes[3];
	azgui::Button *buttonApply;
	azgui::Button *buttonBack;

	void Initialize();
	void Update();
	void Reset();
	void Draw(Rendering::DrawingContext &context);
};

struct PlayMenu {
	azgui::Screen *screen;
	azgui::Button *buttonMenu;
	azgui::Button *buttonReset;

	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct Gui : public GuiBasic {
	enum class Menu {
		MAIN,
		SETTINGS,
		PLAY
	};

	Menu currentMenu = Menu::MAIN;
	Menu nextMenu = Menu::MAIN;
	MainMenu menuMain;
	SettingsMenu menuSettings;
	PlayMenu menuPlay;

	Gui();
	~Gui() = default;

	void EventInitialize() override;
	void EventSync() override;
	void EventDraw(az::Array<Rendering::DrawingContext> &contexts) override;
};

extern Gui *gui;

} // namespace Az2D::Gui

#endif // GUI_HPP