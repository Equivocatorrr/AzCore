/*
	File: gui.hpp
	Author: Philip Haynes
*/

#ifndef GUI_HPP
#define GUI_HPP

#include "Az2D/gui_basics.hpp"

namespace Az2D::Gui { // Short for Interface

using namespace AzCore;

// Ways to define a GUI with a hierarchy
struct Gui;

// Now we can have some different screens
struct MainMenu {
	Screen screen;
	Hideable *continueHideable;
	Button *buttonContinue;
	Button *buttonNewGame;
	Button *buttonSettings;
	Button *buttonExit;

	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct SettingsMenu {
	Screen screen;
	Checkbox *checkFullscreen;
	Checkbox *checkVSync;
	Hideable *framerateHideable;
	TextBox *textboxFramerate;
	Slider *sliderVolumes[3];
	TextBox *textboxVolumes[3];
	Slider *sliderGuiScale;
	TextBox *textboxGuiScale;
	Button *buttonApply;
	Button *buttonBack;

	void Reset();
	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct UpgradesMenu {
	Screen screen;
	Hideable *hideable;
	Text *towerName;
	// Information about the currently selected tower
	Text *selectedTowerStats;
	Hideable *towerPriorityHideable;
	// How the selected tower prioritizes target selection
	Switch *towerPriority;
	// Some upgrades aren't available on some towers
	Hideable *upgradeHideable[5];
	// The number indicator for the state of each attribute
	Text *upgradeStatus[5];
	// Button for purchasing an upgrade
	Button *upgradeButton[5];

	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct PlayMenu {
	Screen screen;
	ListV *list;
	Text *waveTitle, *waveInfo, *towerInfo;
	Array<ListH*> towerButtonLists;
	Array<Button*> towerButtons;
	Button *buttonMenu;
	Button *buttonStartWave;
	Text *buttonTextStartWave;

	UpgradesMenu upgradesMenu;

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
	
	Assets::TexIndex texCursor;

	Menu currentMenu = Menu::MAIN;
	Menu nextMenu = Menu::MAIN;
	MainMenu menuMain;
	SettingsMenu menuSettings;
	PlayMenu menuPlay;

	Gui();
	~Gui() = default;

	void EventAssetsQueue() override;
	void EventAssetsAcquire() override;
	void EventInitialize() override;
	void EventSync() override;
	void EventDraw(Array<Rendering::DrawingContext> &contexts) override;
};

extern Gui *gui;

} // namespace Gui

#endif // GUI_HPP
