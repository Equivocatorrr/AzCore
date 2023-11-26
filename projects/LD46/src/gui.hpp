/*
	File: gui.hpp
	Author: Philip Haynes
*/

#ifndef GUI_HPP
#define GUI_HPP

#include "Az2D/gui_basics.hpp"

namespace Az2D::Gui {

namespace azgui = az::GuiGeneric;

struct MainMenu {
	azgui::Screen *screen;
	azgui::Hideable *continueHideable;
	azgui::Button *buttonContinue;
	azgui::Button *buttonNewGame;
	azgui::Button *buttonLevelEditor;
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
	void Draw(Rendering::DrawingContext &context);
};

struct CutsceneMenu {
	bool intro = true;
	struct Frame {
		const char *text;
		Sound::Source *sound;
		f32 fadein;
		f32 duration;
		f32 fadeout;
		i32 image;
		bool useImage;
	};
	i32 currentFrame;
	f32 frameTimer;
	az::Array<Frame> introFrames;
	az::Array<Frame> outtroFrames;
	azgui::Screen *screen;
	azgui::Text *text;
	azgui::Image *image;
	azgui::Button *buttonSkip;

	inline void Begin() {
		currentFrame = -1;
	}

	void Initialize();
	void Update();
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

struct EditorMenu {
	azgui::Screen *screen;
	azgui::Switch *switchBlock;
	azgui::Button *buttonMenu;
	azgui::Button *buttonNew;
	azgui::Button *buttonSave;
	azgui::Button *buttonLoad;
	// Dialogs
	azgui::Hideable *hideableSave;
	azgui::Hideable *hideableLoad;
	azgui::Hideable *hideableResize;
	azgui::Textbox *textboxFilename;
	azgui::Textbox *textboxWidth;
	azgui::Textbox *textboxHeight;
	azgui::Button *buttonCancel;
	azgui::Button *buttonConfirm;

	static const u8 blockTypes[5];

	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct Gui : public GuiBasic {
	enum class Menu {
		MAIN,
		SETTINGS,
		PLAY,
		EDITOR,
		INTRO,
		OUTTRO,
	};
	Sound::Source sndBeepShort;
	Sound::Source sndBeepLong;
	Sound::Source sndPhoneBuzz;

	Assets::TexIndex texIntro[5];
	Assets::TexIndex texOutro[6];
	Assets::TexIndex texCreditsEquivocator, texCreditsFlubz;

	Menu menuCurrent = Menu::MAIN;
	Menu menuNext = Menu::MAIN;
	MainMenu menuMain;
	SettingsMenu menuSettings;
	PlayMenu menuPlay;
	EditorMenu menuEditor;
	CutsceneMenu menuCutscene;

	Gui();
	~Gui() = default;

	void EventAssetsQueue() override;
	void EventAssetsAcquire() override;
	void EventInitialize() override;
	void EventSync() override;
	void EventDraw(az::Array<Rendering::DrawingContext> &contexts) override;
};

extern Gui *gui;

} // namespace Az2D::Gui

#endif // GUI_HPP
