/*
	File: gui.hpp
	Author: Philip Haynes
*/

#ifndef GUI_HPP
#define GUI_HPP

#include "Az2D/gui_basics.hpp"

namespace Az2D::Gui {

// Now we can have some different screens
struct MainMenu {
	Screen screen;
	Hideable *continueHideable;
	Button *buttonContinue;
	Button *buttonNewGame;
	Button *buttonLevelEditor;
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
	Button *buttonApply;
	Button *buttonBack;

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
	Screen screen;
	Text *text;
	Image *image;
	Button *buttonSkip;

	inline void Begin() {
		currentFrame = -1;
	}

	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct PlayMenu {
	Screen screen;
	Button *buttonMenu;
	Button *buttonReset;

	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct EditorMenu {
	Screen screen;
	Switch *switchBlock;
	Button *buttonMenu;
	Button *buttonNew;
	Button *buttonSave;
	Button *buttonLoad;
	// Dialogs
	Hideable *hideableSave;
	Hideable *hideableLoad;
	Hideable *hideableResize;
	TextBox *textboxFilename;
	TextBox *textboxWidth;
	TextBox *textboxHeight;
	Button *buttonCancel;
	Button *buttonConfirm;

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
