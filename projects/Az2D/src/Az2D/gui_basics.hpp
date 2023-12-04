/*
	File: gui.hpp
	Author: Philip Haynes
*/

#ifndef AZ2D_GUI_BASICS_HPP
#define AZ2D_GUI_BASICS_HPP

#include "AzCore/math.hpp"
#include "AzCore/gui.hpp"

#include "game_systems.hpp"
#include "assets.hpp"
#include "rendering.hpp"

namespace Az2D::Gui {


struct TextMetadata {
	Rendering::FontAlign alignH = Rendering::LEFT;
	Rendering::FontAlign alignV = Rendering::TOP;
};

struct ImageMetadata {
	i32 texIndex = Rendering::texBlank;
};

constexpr u32 CONSOLE_COMMAND_HISTORY_CAP = 100;
constexpr u32 CONSOLE_COMMAND_OUTPUT_LINES_CAP = 20;

struct DevConsole {
	az::GuiGeneric::System *system;
	az::GuiGeneric::Screen *screen;
	az::GuiGeneric::Textbox *textboxInput;
	az::GuiGeneric::Text *consoleOutput;
	az::UniquePtr<TextMetadata> consoleOutputMeta;
	az::WString previousCommands[CONSOLE_COMMAND_HISTORY_CAP];
	i32 recentCommand = -1;
	i32 nextCommand = 0;
	i32 numCommandsInHistory = 0;
	az::WString outputLines[CONSOLE_COMMAND_OUTPUT_LINES_CAP];
	
	void Initialize();
	void Update();
	void Draw(Rendering::DrawingContext &context);
};

struct GuiBasic : public GameSystems::System {
	// configuration
	const char *defaultFontFilename = "DroidSans.ttf";
	struct SoundDef {
		az::SimpleRange<char> filename;
		f32 gain;
		f32 pitch;
		Assets::SoundIndex soundIndex;
	};
	az::Array<SoundDef> sndClickInDefs = {
		{"click in 1.ogg", 0.15f, 1.2f},
		{"click in 2.ogg", 0.15f, 1.2f},
		{"click in 3.ogg", 0.15f, 1.2f},
		{"click in 4.ogg", 0.15f, 1.2f},
	};
	az::Array<SoundDef> sndClickOutDefs = {
		{"click out 1.ogg", 0.15f, 1.2f},
		{"click out 2.ogg", 0.15f, 1.2f},
		{"click out 3.ogg", 0.15f, 1.2f},
		{"click out 4.ogg", 0.15f, 1.2f},
	};
	az::Array<SoundDef> sndClickSoftDefs = {
		{"click soft 1.ogg", 0.01f, 1.2f},
		{"click soft 2.ogg", 0.01f, 1.2f},
	};
	SoundDef sndCheckboxOnDef = {"Pop High.ogg", 0.1f, 1.0f};
	SoundDef sndCheckboxOffDef = {"Pop Low.ogg", 0.1f, 1.0f};

	// assets
	Assets::FontIndex fontIndex;
	az::Array<Sound::Source> sndClickInSources;
	az::Array<Sound::Source> sndClickOutSources;
	az::Array<Sound::Source> sndClickSoftSources;
	Sound::Source sndCheckboxOn, sndCheckboxOff;
	Sound::MultiSource sndClickIn;
	Sound::MultiSource sndClickOut;
	Sound::MultiSource sndClickSoft;
	Assets::Font *font;

	bool console = false;
	DevConsole devConsole;
	
	az::GuiGeneric::System system;
	Rendering::DrawingContext *currentContext = nullptr;

	GuiBasic();

	void EventAssetsRequest() override;
	void EventAssetsAvailable() override;
	void EventInitialize() override;
	// When deriving, call this first, do your own sync, and then set readyForDraw to true at the end.
	void EventSync() override;
	void EventDraw(az::Array<Rendering::DrawingContext> &contexts) override;
};

// global accessor to our basic gui, should be derived from, created in main, and passed into GameSystems::Init
extern GuiBasic *guiBasic;

} // namespace Az2D

#endif // !Z2D_GUI_BASICS_HPP
