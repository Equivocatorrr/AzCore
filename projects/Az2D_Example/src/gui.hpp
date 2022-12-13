#include "Az2D/gui_basics.hpp"

namespace Az2D::Gui {

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
	Button *buttonApply;
	Button *buttonBack;

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

	void EventInitialize();
	void EventSync();
	void EventDraw(Array<Rendering::DrawingContext> &contexts);
};

extern Gui *gui;

} // namespace Az2D::Gui