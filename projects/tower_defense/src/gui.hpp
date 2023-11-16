/*
	File: gui.hpp
	Author: Philip Haynes
*/

#ifndef GUI_HPP
#define GUI_HPP

#include "AzCore/gui.hpp"
#include "Az2D/gui_basics.hpp"

namespace Az2D::Gui {

struct Gui;

// Now we can have some different screens
struct MainMenu {
	az::GuiGeneric::Screen *screen;
	az::GuiGeneric::Hideable *continueHideable;
	az::GuiGeneric::Button *buttonContinue;
	az::GuiGeneric::Button *buttonNewGame;
	az::GuiGeneric::Button *buttonSettings;
	az::GuiGeneric::Button *buttonExit;

	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct SettingsMenu {
	az::GuiGeneric::Screen *screen;
	az::GuiGeneric::Checkbox *checkFullscreen;
	az::GuiGeneric::Checkbox *checkVSync;
	az::GuiGeneric::Hideable *framerateHideable;
	az::GuiGeneric::Textbox *textboxFramerate;
	az::GuiGeneric::Slider *sliderVolumes[3];
	az::GuiGeneric::Textbox *textboxVolumes[3];
	az::GuiGeneric::Slider *sliderGuiScale;
	az::GuiGeneric::Textbox *textboxGuiScale;
	az::GuiGeneric::Button *buttonApply;
	az::GuiGeneric::Button *buttonBack;

	void Reset();
	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct UpgradesMenu {
	az::GuiGeneric::Screen *screen;
	az::GuiGeneric::Hideable *hideable;
	az::GuiGeneric::Text *towerName;
	// Information about the currently selected tower
	az::GuiGeneric::Text *selectedTowerStats;
	az::GuiGeneric::Hideable *towerPriorityHideable;
	// How the selected tower prioritizes target selection
	az::GuiGeneric::Switch *towerPriority;
	// Some upgrades aren't available on some towers
	az::GuiGeneric::Hideable *upgradeHideable[5];
	// The number indicator for the state of each attribute
	az::GuiGeneric::Text *upgradeStatus[5];
	// Button for purchasing an upgrade
	az::GuiGeneric::Button *upgradeButton[5];

	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct PlayMenu {
	az::GuiGeneric::Screen *screen;
	az::GuiGeneric::ListV *list;
	az::GuiGeneric::Text *waveTitle, *waveInfo, *towerInfo;
	az::Array<az::GuiGeneric::ListH*> towerButtonLists;
	az::Array<az::GuiGeneric::Button*> towerButtons;
	az::GuiGeneric::Button *buttonMenu;
	az::GuiGeneric::Button *buttonStartWave;
	az::GuiGeneric::Text *buttonTextStartWave;

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

	void EventAssetsQueue() override;
	void EventAssetsAcquire() override;
	void EventInitialize() override;
	void EventSync() override;
	void EventDraw(az::Array<Rendering::DrawingContext> &contexts) override;
};

extern Gui *gui;

} // namespace Gui

#endif // GUI_HPP
