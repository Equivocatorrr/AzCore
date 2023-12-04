/*
	File: gui.hpp
	Author: Philip Haynes
*/

#ifndef GUI_HPP
#define GUI_HPP

#include "AzCore/gui.hpp"
#include "Az2D/gui_basics.hpp"

namespace Az2D::Gui {

namespace azgui = az::GuiGeneric;

struct Gui;

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
	azgui::Slider *sliderGuiScale;
	azgui::Textbox *textboxGuiScale;
	azgui::Button *buttonApply;
	azgui::Button *buttonBack;

	void Reset();
	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct UpgradesMenu {
	azgui::Screen *screen;
	azgui::Hideable *hideable;
	azgui::Text *towerName;
	// Information about the currently selected tower
	azgui::Text *selectedTowerStats;
	azgui::Hideable *towerPriorityHideable;
	// How the selected tower prioritizes target selection
	azgui::Switch *towerPriority;
	// Some upgrades aren't available on some towers
	azgui::Hideable *upgradeHideable[5];
	// The number indicator for the state of each attribute
	azgui::Text *upgradeStatus[5];
	// Button for purchasing an upgrade
	azgui::Button *upgradeButton[5];

	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct PlayMenu {
	azgui::Screen *screen;
	azgui::ListV *list;
	azgui::Text *waveTitle, *waveInfo, *towerInfo;
	az::Array<azgui::ListH*> towerButtonLists;
	az::Array<azgui::Button*> towerButtons;
	azgui::Button *buttonMenu;
	azgui::Button *buttonStartWave;
	azgui::Text *buttonTextStartWave;

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
	
	Menu currentMenu = Menu::MAIN;
	Menu nextMenu = Menu::MAIN;
	MainMenu menuMain;
	SettingsMenu menuSettings;
	PlayMenu menuPlay;
	
	Assets::TexIndex texCursor;
	Assets::FontIndex fontIndex;
	az::Array<Sound::Source> sndClickInSources;
	az::Array<Sound::Source> sndClickOutSources;
	az::Array<Sound::Source> sndClickSoftSources;
	Sound::Source sndCheckboxOn, sndCheckboxOff;
	Sound::MultiSource sndClickIn;
	Sound::MultiSource sndClickOut;
	Sound::MultiSource sndClickSoft;
	Assets::Font *font;

	Gui();
	~Gui() = default;

	void EventAssetsRequest() override;
	void EventInitialize() override;
	void EventSync() override;
	void EventDraw(az::Array<Rendering::DrawingContext> &contexts) override;
};

extern Gui *gui;

} // namespace Gui

#endif // GUI_HPP
