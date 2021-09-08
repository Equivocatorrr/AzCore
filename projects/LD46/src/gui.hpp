/*
	File: gui.hpp
	Author: Philip Haynes
*/

#ifndef GUI_HPP
#define GUI_HPP

#include "objects.hpp"
#include "assets.hpp"
#include "rendering.hpp"

namespace Int { // Short for Interface

using namespace AzCore;

// Ways to define a GUI with a hierarchy
struct Gui;

// Base polymorphic interface, also usable as a blank spacer.
struct Widget {
	Array<Widget*> children;
	/*  Space surrounding the widget.
		Defaults:
		Widget: {8.0, 8.0}, Screen: {0.0, 0.0}, Hideable: {0.0, 0.0} */
	vec2 margin;
	/*  Either pixel size, or fraction of parent container. 0.0 means it grows for contents.
		Defaults:
		Widget: {1.0, 1.0}, Text: {1.0, 0.0}, Checkbox: {48.0, 24.0}, TextBox: {200.0, 0.0}, Hideable inherits from child */
	vec2 size;
	/*  Determines whether size.x is a fraction of the parent container (true) or a pixel size (false).
		Defaults:
		Widget: true, Checkbox: false, TextBox: false, Hideable inherits from child */
	bool fractionWidth;
	/*  Determines whether size.y is a fraction of the parent container (true) or a pixel size (false).
		Defaults:
		Widget: true, Checkbox: false, TextBox: false, Hideable inherits from child */
	bool fractionHeight;
	/*  Minimum absolute size (pixels). Negative values mean no limit.
		Default: {0.0, 0.0}, TextBox: {24.0, 0.0} */
	vec2 minSize;
	/*  Maximum absolute size (pixels). Negative values mean no limit.
		Default: {-1.0, -1.0} */
	vec2 maxSize;
	/*  Pixel offset from where it would be placed by a parent.
		Default: {0.0, 0.0} */
	vec2 position;
	/*  The absolute pixel size set by UpdateSize(). */
	vec2 sizeAbsolute;
	/*  Absolute pixel position set by Update(). */
	vec2 positionAbsolute;
	/*  How deeply nested we are in menus that offer exclusive access. Used for input culling for controllers.
		Can be made deeper than parent with AddWidget's last optional argument. */
	i32 depth;
	/*  Whether or not this widget can be used in a selection by a controller.
		Default: false */
	bool selectable;
	/*  Whether we should be drawn highlighted. (Typically true when selected) */
	bool highlighted;
	/*  Whether the widget counts for mouse occlusion.
		Useful for menus to block interaction with game objects that fall beneath them. */
	bool occludes;
	bool mouseover;
	Widget();
	virtual ~Widget() = default;
	virtual void UpdateSize(vec2 container);
	void LimitSize();
	void PushScissor(Rendering::DrawingContext &context) const;
	void PopScissor(Rendering::DrawingContext &context) const;
	inline vec2 GetSize() const { return sizeAbsolute + margin * 2.0f; }
	virtual void Update(vec2 pos, bool selected);
	virtual void Draw(Rendering::DrawingContext &context) const;

	// If a widget gets hidden by a Hideable, this allows it to configure itself as a response
	virtual void OnHide();

	virtual bool Selectable() const;
	bool MouseOver() const;
	void FindMouseoverDepth(i32 actualDepth);
};

// Lowest level widget, used for input for game objects.
struct Screen : public Widget {
	Screen();
	~Screen() = default;
	void Update(vec2 pos, bool selected);
	void UpdateSize(vec2 container);
};

struct List : public Widget {
	/*  Space surrounding the contained Widgets.
		Default: {8.0, 8.0} */
	vec2 padding;
	/*  The color of the background when not highlighted.
		Defaults:
		ListV: {0.05, 0.05, 0.05, 0.9}, ListH: {0.0, 0.0, 0.0, 0.9}, Switch: {0.2, 0.2, 0.2, 0.9} */
	vec4 color;
	/*  The color of the background when highlighted.
		Defaults:
		ListV: {0.05, 0.05, 0.05, 0.9}, ListH: {0.1, 0.1, 0.1, 0.9}, Switch: {colorHighlightMedium, 0.9} */
	vec4 highlight;
	/*  The color of a quad drawn beneath the selection.
		Default: {0.2, 0.2, 0.2, 0.0}, Switch: {0.0, 0.0, 0.0, 0.5} */
	vec4 select;
	/*  Which child we have selected, -1 for none, -2 for default. */
	i32 selection;
	/*  If we're ever selected and selection is -2, what should we select by default?
		Defaults:
		List: -1, Switch: 0 */
	i32 selectionDefault;
	List();
	~List() = default;
	// returns whether or not to update the selection based on the mouse position
	bool UpdateSelection(bool selected, u8 keyCodeSelect, u8 keyCodeBack, u8 keyCodeIncrement, u8 keyCodeDecrement);
	void Draw(Rendering::DrawingContext &context) const;
};

// A vertical list of items.
struct ListV : public List {
	void UpdateSize(vec2 container);
	void Update(vec2 pos, bool selected);
};

// A horizontal list of items.
struct ListH : public List {
	ListH();
	void UpdateSize(vec2 container);
	void Update(vec2 pos, bool selected);
};

// Allows the user to choose from a selection of widgets (usually Text).
struct Switch : public ListV {
	// Which child is the one shown when not open
	i32 choice;
	// The depth of this widget's parent, used when closing
	i32 parentDepth;
	// Whether this widget acts as a single widget or a list
	bool open;
	// Whether the choice was changed
	bool changed;
	Switch();
	~Switch() = default;
	void UpdateSize(vec2 container);
	void Update(vec2 pos, bool selected);
	void Draw(Rendering::DrawingContext &context) const;

	void OnHide();
};

struct Text : public Widget {
private:
	WString stringFormatted;
public:
	// The unformatted text to be displayed
	WString string;
	/*  Either the pixel size or EM size surrounding the text.
		Default: {0.1, 0.1} */
	vec2 padding;
	/*  Pixel dimensions of the font's EM square.
		Default: 32.0 */
	f32 fontSize;
	/*  Which font is used for drawing the text.
		Default: 1 */
	i32 fontIndex;
	/*  Whether to draw the font bold.
		Default: false */
	bool bold;
	/*  Whether padding is pixels (false) or EM (true).
		Default: true */
	bool paddingEM;
	/*  How the text is aligned horizontally.
		Default: Rendering::LEFT */
	Rendering::FontAlign alignH;
	/*  How the text is aligned vertically.
		Default: Rendering::TOP */
	Rendering::FontAlign alignV;
	/*  Color of the text when not highlighted.
		Default: {vec3(1.0), 1.0} */
	vec4 color;
	/*  Color of the outline when not highlighted.
		Default: {vec3(0.0), 1.0} */
	vec4 colorOutline;
	/*  Color of the text when highlighted.
		Default: {vec3(0.0), 1.0} */
	vec4 highlight;
	/*  Color of the outline when highlighted.
		Default: {vec3(1.0), 1.0} */
	vec4 highlightOutline;
	/*  Whether to draw the outline.
		Default: false */
	bool outline;
	Text();
	~Text() = default;
	void UpdateSize(vec2 container);
	void Update(vec2 pos, bool selected);
	void Draw(Rendering::DrawingContext &context) const;
};

struct Image : public Widget {
	/*  Which image to draw.
		Default: 0 */
	i32 texIndex;
	/*  Color multiplier to draw the image with.
		Default: vec4(1.0f) */
	vec4 color;
	Image();
	~Image() = default;
	void Draw(Rendering::DrawingContext &context) const;
};

struct Button : public Widget {
	//  Text label for the button.
	WString string;
	/*  Color of the button when not highlighted.
		Default: {vec3(0.15), 0.9} */
	vec4 colorBG;
	/*  Color of the button when highlighted.
		Default: {colorHighlightMedium, 0.9} */
	vec4 highlightBG;
	/*  Color of the label text when not highlighted.
		Default: {vec3(1.0), 1.0} */
	vec4 colorText;
	/*  Color of the label text when highlighted.
		Default: {vec3(0.0), 1.0} */
	vec4 highlightText;
	/*  Which font is used for drawing the text.
		Default: 1 */
	i32 fontIndex;
	/*  Pixel dimensions of the font's EM square.
	Default: 28.0 */
	f32 fontSize;
	//  The pressed, down, and released state of this button.
	io::ButtonState state;
	// Any input keycodes that can affect state without the widget being selected.
	Array<u8> keycodeActivators;
	Button();
	~Button() = default;
	void Update(vec2 pos, bool selected);
	void Draw(Rendering::DrawingContext &context) const;
};

// Boolean widget.
struct Checkbox : public Widget {
	/*  Color of the background when turned off and not highlighted.
		Default: {vec3(0.15), 0.9} */
	vec4 colorOff;
	/*  Color of the background when turned off and highlighted.
		Default: {colorHighlightLow, 0.9} */
	vec4 highlightOff;
	/*  Color of the background when turned on and not highlighted.
		Default: {colorHighlightMedium, 1.0} */
	vec4 colorOn;
	/*  Color of the background when turned on and highlighted.
		Default: {colorHighlightHigh, 1.0} */
	vec4 highlightOn;
	/*  Where the animation between states currently is.
		Default: 0.0 */
	f32 transition;
	/*  Whether the Checkbox is turned on.
		Default: false */
	bool checked;
	Checkbox();
	~Checkbox() = default;
	void Update(vec2 pos, bool selected);
	void Draw(Rendering::DrawingContext &context) const;
};

// Returns whether a character is acceptable in a TextBox
typedef bool (*fpTextFilter)(char32);
// Returns whether a string is valid in a TextBox
typedef bool (*fpTextValidate)(const WString&);

// Some premade filters
bool TextFilterBasic(char32 c);
bool TextFilterWordSingle(char32 c);
bool TextFilterWordMultiple(char32 c);
bool TextFilterDecimals(char32 c);
bool TextFilterDecimalsPositive(char32 c);
bool TextFilterIntegers(char32 c);
bool TextFilterDigits(char32 c);

bool TextValidateAll(const WString &string); // Only returns true
bool TextValidateNonempty(const WString &string); // String size must not be zero
bool TextValidateDecimals(const WString &string); // Confirms the format of -123.456
bool TextValidateDecimalsPositive(const WString &string); // Confirms the format of 123.456
bool TextValidateIntegers(const WString &string); // Confirms the format of -123456
// Digits validation would be the same as TextFilterDigits + TextValidateAll

// Text entry with filters
struct TextBox : public Widget {
	// The currently entered text unformatted.
	WString string;
	// The formatted text for drawing.
	WString stringFormatted;
	/*  The color of the background when not highlighted and text validation passed.
		Default: {vec3(0.15), 0.9} */
	vec4 colorBG;
	/*  The color of the background when highlighted and text validation passed.
		Default: {vec3(0.2), 0.9} */
	vec4 highlightBG;
	/*  The color of the background when text validation failed.
		Default: {0.1, 0.0, 0.0, 0.9} */
	vec4 errorBG;
	/*  The color of the text when not highlighted and text validation passed.
		Default: {vec3(1.0), 1.0} */
	vec4 colorText;
	/*  The color of the text when highlighted and text validation passed.
		Default: {vec3(1.0), 1.0} */
	vec4 highlightText;
	/*  The color of the text when text validation failed.
		Default: {1.0, 0.5, 0.5, 1.0} */
	vec4 errorText;
	/*  How much space in pixels surrounds the text.
		Default: {2.0, 2.0} */
	vec2 padding;
	/*  Which index in the string the cursor is on.
		Default: 0 */
	i32 cursor;
	/*  Which font is used for drawing the text.
		Default: 1 */
	i32 fontIndex;
	/*  Pixel dimensions of the font's EM square.
		Default: 17.39 */
	f32 fontSize;
	// Timer between 0.0 and 1.0 seconds where values < 0.5 indicate the cursor being visible.
	f32 cursorBlinkTimer;
	/*  How the text is aligned horizontally.
		Default: Rendering::LEFT */
	Rendering::FontAlign alignH;
	/*  Which filter should be used to disallow certain characters from being added.
		Default: TextFilterBasic */
	fpTextFilter textFilter;
	/*  Which form of validation should be used to check whether a given string is valid.
		Default: TextValidateAll */
	fpTextValidate textValidate;
	/*  Whether the textbox is currently accepting keyboard input.
		Default: false */
	bool entry;
	/*  Whether the textbox allows multiple lines of text to be used.
		Default: false */
	bool multiline;
	TextBox();
	~TextBox() = default;
	void CursorFromPosition(vec2 position);
	vec2 PositionFromCursor() const;
	void UpdateSize(vec2 container);
	void Update(vec2 pos, bool selected);
	void Draw(Rendering::DrawingContext &context) const;
};

// A scalar within a range.
struct Slider : public Widget {
	/*  Starting value for the slider. Will be bound between valueMin and valueMax.
		Default: 1.0 */
	f32 value;
	/*  Minimum bounds for value.
		Default: 0.0 */
	f32 valueMin;
	/*  Maximum bounds for value.
		Default: 1.0 */
	f32 valueMax;
	/*  Any TextBox that should reflect the value of the slider in text, and which can likewise affect our value.
		Default: nullptr */
	TextBox *mirror;
	/*  Color of the background when not highlighted.
		Default: {vec3(0.15), 1.0} */
	vec4 colorBG;
	/*  Color of the slider knob when not highlighted.
		Default: {colorHighlightMedium, 1.0} */
	vec4 colorSlider;
	/*  Color of the background when highlighted.
		Default: {vec3(0.2), 0.9} */
	vec4 highlightBG;
	/*  Color of the slider knob when highlighted.
		Default: {colorHighlightHigh, 1.0} */
	vec4 highlightSlider;
	/*  Whether the mouse has grabbed the slider knob and we should respond to mouse movements.
		Default: false */
	bool grabbed;
	//  Pressed() is whether the value should move left by an increment.
	io::ButtonState left;
	//  Pressed() is whether the value should move right by an increment.
	io::ButtonState right;
	Slider();
	~Slider() = default;
	void Update(vec2 pos, bool selected);
	void Draw(Rendering::DrawingContext &context) const;
};

// Must have exactly one child or else!
struct Hideable : public Widget {
	/*  Whether our child is hidden and we should pretend they're invisible and of zero size.
		Default: false */
	bool hidden;
	// The value of hidden from the previous frame. Used to know when to call OnHide().
	bool hiddenPrev;

	Hideable(Widget *child);
	~Hideable() = default;
	void UpdateSize(vec2 container);
	void Update(vec2 pos, bool selected);
	void Draw(Rendering::DrawingContext &context) const;
	bool Selectable() const;
};

enum MenuEnum {
	MENU_MAIN,
	MENU_SETTINGS,
	MENU_PLAY,
	MENU_EDITOR,
	MENU_INTRO,
	MENU_OUTTRO,
};

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
	Array<Frame> introFrames;
	Array<Frame> outtroFrames;
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

struct Gui : public Objects::Object {
	i32 fontIndex;
	i32 cursorIndex;
	Sound::Source sndClickInSources[4];
	Sound::Source sndClickOutSources[4];
	Sound::Source sndClickSoftSources[2];
	Sound::Source sndPopHigh, sndPopLow;
	Sound::MultiSource sndClickIn;
	Sound::MultiSource sndClickOut;
	Sound::MultiSource sndClickSoft;
	Sound::Source sndBeepShort;
	Sound::Source sndBeepLong;
	Sound::Source sndPhoneBuzz;
	Assets::Font *font;

	i32 introIndex[5];
	i32 outtroIndex[6];
	i32 creditsEquivocator, creditsFlubz;

	i32 controlDepth = 0;
	f32 scale = 2.0f;
	// false for gamepad, true for mouse
	bool usingMouse = true;
	// Used to make sure the mouse can only interact with top-most widgets.
	// Also provides an easy test to see if the mouse can interact with items below it.
	Widget *mouseoverWidget;
	i32 mouseoverDepth;

	HashSet<Widget*> allWidgets; // So we can delete them at the end of the program.

	MenuEnum currentMenu = MENU_MAIN;
	MenuEnum nextMenu = MENU_MAIN;
	MainMenu mainMenu;
	SettingsMenu settingsMenu;
	PlayMenu playMenu;
	EditorMenu editorMenu;
	CutsceneMenu cutsceneMenu;

	~Gui();

	void EventAssetInit();
	void EventAssetAcquire();
	void EventInitialize();
	void EventSync();
	void EventDraw(Array<Rendering::DrawingContext> &contexts);
};



} // namespace Int

#endif // GUI_HPP
