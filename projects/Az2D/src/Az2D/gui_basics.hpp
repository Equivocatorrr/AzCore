/*
	File: gui.hpp
	Author: Philip Haynes
*/

#ifndef AZ2D_GUI_BASICS_HPP
#define AZ2D_GUI_BASICS_HPP

#include "AzCore/math.hpp"

#include "game_systems.hpp"
#include "assets.hpp"
#include "rendering.hpp"

namespace Az2D::Gui {

using az::vec2, az::vec3, az::vec4;
struct Widget;

extern const vec3 colorBack;
extern const vec3 colorHighlightLow;
extern const vec3 colorHighlightMedium;
extern const vec3 colorHighlightHigh;

struct GuiBasic : public GameSystems::System {
	// configuration
	const char *defaultFontFilename = "DroidSans.ttf";
	struct SoundDef {
		az::SimpleRange<char> filename;
		f32 gain;
		f32 pitch;
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

	i32 controlDepth = 0;
	f32 scale = 2.0f;
	bool usingMouse = true;
	bool usingArrows = false;
	bool usingGamepad = false;
	// Used to make sure the mouse can only interact with top-most widgets.
	// Also provides an easy test to see if the mouse can interact with items below it.
	Widget *mouseoverWidget;
	i32 mouseoverDepth;
	vec2 selectedCenter;

	az::HashSet<Widget*> allWidgets; // So we can delete them at the end of the program.

	GuiBasic();
	~GuiBasic();

	void EventAssetsQueue() override;
	void EventAssetsAcquire() override;
	// When deriving, call this first, do your own sync, and then set readyForDraw to true at the end.
	void EventSync() override;
};

// global accessor to our basic gui, should be derived from, created in main, and passed into GameSystems::Init
extern GuiBasic *guiBasic;

// Base polymorphic interface, also usable as a blank spacer.
struct Widget {
	az::Array<Widget*> children;
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
	/*  Scaling factor that affects everything. This will be set internally.
		Default: 1.0f */
	f32 scale = 1.0f;
	Widget();
	virtual ~Widget() = default;
	virtual void UpdateSize(vec2 container, f32 _scale);
	void LimitSize();
	virtual void PushScissor(Rendering::DrawingContext &context) const;
	void PopScissor(Rendering::DrawingContext &context) const;
	inline vec2 GetSize() const { return sizeAbsolute + margin * 2.0f * scale; }
	virtual void Update(vec2 pos, bool selected);
	virtual void Draw(Rendering::DrawingContext &context) const;

	// If a widget gets hidden by a Hideable, this allows it to configure itself as a response
	virtual void OnHide();

	virtual bool Selectable() const;
	bool MouseOver() const;
	void FindMouseoverDepth(i32 actualDepth);
	
	// Helpers to make it easier to read and edit GUI definitions
	
	inline void SetWidthPixel(f32 width) {
		AzAssert(width > 0.0f, "Pixel width must be > 0");
		size.x = width;
		fractionWidth = false;
	}
	inline void SetWidthFraction(f32 width) {
		AzAssert(width <= 1.0f && width > 0.0f, "Fractional width must be > 0 and <= 1");
		size.x = width;
		fractionWidth = true;
	}
	
	inline void SetWidthContents() {
		size.x = 0.0f;
	}
	inline void SetHeightPixel(f32 height) {
		AzAssert(height > 0.0f, "Pixel height must be > 0");
		size.y = height;
		fractionHeight = false;
	}
	inline void SetHeightFraction(f32 height) {
		AzAssert(height <= 1.0f && height > 0.0f, "Fractional height must be > 0 and <= 1");
		size.y = height;
		fractionHeight = true;
	}
	
	inline void SetHeightContents() {
		size.y = 0.0f;
	}
	inline void SetSizePixel(vec2 _size) {
		AzAssert(_size.x > 0.0f, "Pixel width must be > 0");
		AzAssert(_size.y > 0.0f, "Pixel height must be > 0");
		size = _size;
		fractionWidth = false;
		fractionHeight = false;
	}
	inline void SetSizeFraction(vec2 _size) {
		AzAssert(_size.x <= 1.0f && _size.x > 0.0f, "Fractional width must be > 0 and <= 1");
		AzAssert(_size.y <= 1.0f && _size.y > 0.0f, "Fractional height must be > 0 and <= 1");
		size = _size;
		fractionWidth = true;
		fractionHeight = true;
	}
	
	inline void SetSizeContents() {
		size = 0.0f;
	}
};

// Lowest level widget, used for input for game objects.
struct Screen : public Widget {
	Screen();
	~Screen() = default;
	void Update(vec2 pos, bool selected) override;
	void UpdateSize(vec2 container, f32 _scale) override;
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
	/*  How far we've scrolled if our contents don't fit in the range 0 to 1. */
	vec2 scroll;
	/*  How big our contents are in absolute size. */
	vec2 sizeContents;
	/*  Whether we can scroll horizontally.
		Defaults:
		ListH: true, ListV: false */
	bool scrollableX;
	/*  Whether we can scroll vertically.
		Defaults:
		ListH: false, ListV: true */
	bool scrollableY;
	List();
	~List() = default;
	// returns whether or not to update the selection based on the mouse position
	bool UpdateSelection(bool selected, az::StaticArray<u8, 4> keyCodeSelect, az::StaticArray<u8, 4> keyCodeBack, az::StaticArray<u8, 4> keyCodeIncrement, az::StaticArray<u8, 4> keyCodeDecrement);
	void Draw(Rendering::DrawingContext &context) const override;
};

// A vertical list of items.
struct ListV : public List {
	void UpdateSize(vec2 container, f32 _scale) override;
	void Update(vec2 pos, bool selected) override;
};

// A horizontal list of items.
struct ListH : public List {
	ListH();
	void UpdateSize(vec2 container, f32 _scale) override;
	void Update(vec2 pos, bool selected) override;
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
	void UpdateSize(vec2 container, f32 _scale) override;
	void Update(vec2 pos, bool selected) override;
	void Draw(Rendering::DrawingContext &context) const override;

	void OnHide();
};

struct Text : public Widget {
private:
	az::WString stringFormatted;
public:
	// The unformatted text to be displayed
	az::WString string;
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
	void PushScissor(Rendering::DrawingContext &context) const override;
	void UpdateSize(vec2 container, f32 _scale) override;
	void Update(vec2 pos, bool selected) override;
	void Draw(Rendering::DrawingContext &context) const override;
};

struct Image : public Widget {
	/*  Which image to draw.
		Default: 0 */
	i32 texIndex;
	/*  Which shader to use when drawing the image
		Default: PIPELINE_BASIC_2D */
	Rendering::PipelineIndex pipeline;
	/*  Color multiplier to draw the image with.
		Default: vec4(1.0f) */
	vec4 color;
	Image();
	~Image() = default;
	void Draw(Rendering::DrawingContext &context) const override;
};

struct Button : public Widget {
	/*  Space surrounding the contained Widget.
		Default: {0.0, 0.0} */
	vec2 padding;
	/*  Color of the button when not highlighted.
		Default: {vec3(0.15), 0.9} */
	vec4 colorBG;
	/*  Color of the button when highlighted.
		Default: {colorHighlightMedium, 0.9} */
	vec4 highlightBG;
	//  The pressed, down, and released state of this button.
	az::io::ButtonState state;
	// Any input keycodes that can affect state without the widget being selected.
	az::Array<u8> keycodeActivators;
	// Adds a single child Text widget with default settings
	// Returns said Text widget
	Text* AddDefaultText(az::WString string);
	Button();
	~Button() = default;
	void UpdateSize(vec2 container, f32 _scale) override;
	void Update(vec2 pos, bool selected) override;
	void Draw(Rendering::DrawingContext &context) const override;
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
	void Update(vec2 pos, bool selected) override;
	void Draw(Rendering::DrawingContext &context) const override;
};

// Returns whether a character is acceptable in a TextBox
typedef bool (*fpTextFilter)(char32);
// Returns whether a string is valid in a TextBox
typedef bool (*fpTextValidate)(const az::WString&);

// Some premade filters

bool TextFilterBasic(char32 c);
bool TextFilterWordSingle(char32 c);
bool TextFilterWordMultiple(char32 c);
bool TextFilterDecimals(char32 c);
bool TextFilterDecimalsPositive(char32 c);
bool TextFilterIntegers(char32 c);
bool TextFilterDigits(char32 c);

bool TextValidateAll(const az::WString &string); // Only returns true
bool TextValidateNonempty(const az::WString &string); // String size must not be zero
bool TextValidateDecimals(const az::WString &string); // Confirms the format of -123.456
bool TextValidateDecimalsNegative(const az::WString &string); // Confirms the format of -123.456 with enforced negative
bool TextValidateDecimalsNegativeAndInfinity(const az::WString &string); // Confirms the format of -123.456 with enforced negative and allowing -infinity
bool TextValidateDecimalsPositive(const az::WString &string); // Confirms the format of 123.456
bool TextValidateIntegers(const az::WString &string); // Confirms the format of -123456
// Digits validation would be the same as TextFilterDigits + TextValidateAll

// Text entry with filters
struct TextBox : public Widget {
	// The currently entered text unformatted.
	az::WString string;
	// The formatted text for drawing.
	az::WString stringFormatted;
	/* Suffix drawn in the textbox that can't be interacted with
		Default: "" */
	az::WString stringSuffix;
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
	void UpdateSize(vec2 container, f32 _scale) override;
	void Update(vec2 pos, bool selected) override;
	void Draw(Rendering::DrawingContext &context) const override;
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
	/*  Forces values to be quantized to multiples of valueStep relative to valueMin. A value of 0 disables this behavior.
		Default: 0.0f */
	f32 valueStep;
	/*  How much the value changes when clicked on either side of the knob or moved by a controller or arrow key input. Negative values indicate a factor of the total allowed range.
		Default: -0.1f */
	f32 valueTick;
	/*  Multiplier for valueTick when SHIFT is held by the user.
		Default: 0.1f */
	f32 valueTickShiftMult;
	/*  Whether to override the minimum slider value when it's in the minimum position.
		Default: false */
	bool minOverride;
	/*  The value used when overriding the minimum value.
		Default: 0.0f */
	f32 minOverrideValue;
	/*  Whether to override the maximum slider value when it's in the maximum position.
		Default: false */
	bool maxOverride;
	/*  The value used when overriding the maximum value.
		Default: 1.0f */
	f32 maxOverrideValue;
	/*  Any TextBox that should reflect the value of the slider in text, and which can likewise affect our value.
		Default: nullptr */
	TextBox *mirror;
	/*  How many digits after the decimal point do we put into the mirror?
		Default: 1 */
	i32 mirrorPrecision;
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
	az::io::ButtonState left;
	//  Pressed() is whether the value should move right by an increment.
	az::io::ButtonState right;
	Slider();
	~Slider() = default;
	void Update(vec2 pos, bool selected) override;
	void Draw(Rendering::DrawingContext &context) const override;
	
	void SetValue(f32 newValue);
	f32 GetActualValue();
	void UpdateMirror();
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
	void UpdateSize(vec2 container, f32 _scale) override;
	void Update(vec2 pos, bool selected) override;
	void Draw(Rendering::DrawingContext &context) const override;
	bool Selectable() const override;
};

void AddWidget(Widget *parent, Widget *newWidget, bool deeper = false);
void AddWidget(Widget *parent, Switch *newWidget);
void AddWidgetAsDefault(List *parent, Widget *newWidget, bool deeper = false);
void AddWidgetAsDefault(List *parent, Switch *newWidget);

} // namespace Az2D

#endif // !Z2D_GUI_BASICS_HPP
