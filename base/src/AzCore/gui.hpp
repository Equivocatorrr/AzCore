/*
	File: gui.hpp
	Author: Philip Haynes
	Graphics-API-Agnostic Retained GUI System
*/

#ifndef AZCORE_GUI_HPP
#define AZCORE_GUI_HPP

#include "memory.hpp"
#include "math.hpp"
#include "Math/Color.hpp"
#include "IO/Input.hpp"
#include "keycodes.hpp"
#include "Memory/Any.hpp"

namespace AzCore::GuiGeneric {

struct System;

// Base polymorphic interface, also usable as a blank spacer.
struct Widget {
	// Passed into the various external functions as dataWidget, used for extra configuration.
	Any data;
	// The System we belong to.
	System *_system = nullptr;
	az::Array<Widget*> children;
	/*  Space surrounding the widget.
		Defaults:
		Widget: {8.0, 8.0}, Screen: {0.0, 0.0}, Hideable: {0.0, 0.0} */
	vec2 margin;
	/*  Either pixel size, or fraction of parent container. 0.0 means it grows for contents.
		Defaults:
		Widget: {1.0, 1.0}, Text: {1.0, 0.0}, Checkbox: {48.0, 24.0}, Textbox: {200.0, 0.0}, Hideable inherits from child */
	vec2 size;
	/*  Determines whether size.x is a fraction of the parent container (true) or a pixel size (false).
		Defaults:
		Widget: true, Checkbox: false, Textbox: false, Hideable inherits from child */
	bool fractionWidth;
	/*  Determines whether size.y is a fraction of the parent container (true) or a pixel size (false).
		Defaults:
		Widget: true, Checkbox: false, Textbox: false, Hideable inherits from child */
	bool fractionHeight;
	/*  Minimum absolute size (pixels). Negative values mean no limit.
		Default: {0.0, 0.0}, Textbox: {24.0, 0.0} */
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
	/*  Wether or not this widget will update whether it's selectable based on its children.
		Default: true */
	bool inheritSelectable;
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
	// Determines selectability recursively based on whether any children are selectable.
	void UpdateSelectable();
	virtual void UpdateSize(vec2 container, f32 _scale);
	void LimitSize();
	virtual void PushScissor() const;
	void PushScissor(vec2 pos, vec2 size) const;
	void PopScissor() const;
	inline vec2 GetSize() const { return sizeAbsolute + margin * 2.0f * scale; }
	virtual void Update(vec2 pos, bool selected);
	virtual void Draw() const;

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

// Top level widget. This is the one you want to call Update and Draw on.
struct Screen : public Widget {
	Screen();
	~Screen() = default;
	void Update(vec2 pos, bool selected) override;
	void UpdateSize(vec2 container, f32 _scale) override;
};

using Spacer = Widget;

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
		ListV: {0.05, 0.05, 0.05, 0.9}, ListH: {0.1, 0.1, 0.1, 0.9}, Switch: {0.4, 0.9, 1.0, 0.9} */
	vec4 colorHighlighted;
	/*  The color of a quad drawn beneath the selection.
		Default: {0.2, 0.2, 0.2, 0.0}, Switch: {0.0, 0.0, 0.0, 0.5} */
	vec4 colorSelection;
	/*  Which child we have selected, -1 for none, -2 for default. */
	i32 selection;
	/*  If we're ever selected and selection is -2, what should we select by default?
		Defaults:
		List: -1, Switch: 0 */
	i32 selectionDefault;
	/*  How far we've scrolled if our contents don't fit in the range 0 to 1. */
	vec2 scroll;
	/*  How far we want to scroll. scroll will decay towards this value. */
	vec2 scrollTarget;
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
	void Draw() const override;
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
	// Store the sizeAbsolute value for when we're opened here since we don't want to affect layout, but we still need this value for mouse picking.
	vec2 openSizeAbsolute;
	/*  The color of a quad drawn beneath the choice when we're open and it's not highlighted.
		Default: {0.0, 0.0, 0.0, 0.9} */
	vec4 colorChoice;
	Switch();
	~Switch() = default;
	void UpdateSize(vec2 container, f32 _scale) override;
	void Update(vec2 pos, bool selected) override;
	void Draw() const override;

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
	/*  Whether to draw the font bold.
		Default: false */
	bool bold;
	/*  Whether padding is pixels (false) or EM (true).
		Default: true */
	bool paddingEM;
	/*  Color of the text when not highlighted.
		Default: {vec3(1.0), 1.0} */
	vec4 color;
	/*  Color of the outline when not highlighted.
		Default: {vec3(0.0), 1.0} */
	vec4 colorOutline;
	/*  Color of the text when highlighted.
		Default: {vec3(0.0), 1.0} */
	vec4 colorHighlighted;
	/*  Color of the outline when highlighted.
		Default: {vec3(1.0), 1.0} */
	vec4 colorOutlineHighlighted;
	/*  Whether to draw the outline.
		Default: false */
	bool outline;
	Text();
	~Text() = default;
	void PushScissor() const override;
	void UpdateSize(vec2 container, f32 _scale) override;
	void Update(vec2 pos, bool selected) override;
	void Draw() const override;
};

struct Image : public Widget {
	/*  Color multiplier to draw the image with.
		Default: vec4(1.0f) */
	vec4 color;
	Image();
	~Image() = default;
	void Draw() const override;
};

struct Button : public Widget {
	/*  Space surrounding the contained Widget.
		Default: {0.0, 0.0} */
	vec2 padding;
	/*  Color of the button when not highlighted.
		Default: {vec3(0.15), 0.9} */
	vec4 color;
	/*  Color of the button when highlighted.
		Default: {0.4, 0.9, 1.0, 0.9} */
	vec4 colorHighlighted;
	//  The pressed, down, and released state of this button.
	io::ButtonState state;
	// Any input keycodes that can affect state without the widget being selected.
	Array<u8> keycodeActivators;
	// Adds a single child Text widget with default settings
	// Returns said Text widget
	Text* AddDefaultText(az::WString string);
	Button();
	~Button() = default;
	void UpdateSize(vec2 container, f32 _scale) override;
	void Update(vec2 pos, bool selected) override;
	void Draw() const override;
};

// Boolean widget.
struct Checkbox : public Widget {
	/*  Color of the background when turned off and not highlighted.
		Default: {vec3(0.15), 0.9} */
	vec4 colorBGOff;
	/*  Color of the background when turned off and highlighted.
		Default: {0.2, 0.45, 0.5, 0.9} */
	vec4 colorBGHighlightOff;
	/*  Color of the background when turned on and not highlighted.
		Default: {0.4, 0.9, 1.0, 1.0} */
	vec4 colorBGOn;
	/*  Color of the background when turned on and highlighted.
		Default: {0.9, 0.98, 1.0, 1.0} */
	vec4 colorBGHighlightOn;
	/*  Color of the knob when turned off and not highlighted.
		Default: {vec3(0.0), 0.9} */
	vec4 colorKnobOff;
	/*  Color of the knob when turned off and highlighted.
		Default: {vec3(0.0), 0.9} */
	vec4 colorKnobHighlightOff;
	/*  Color of the knob when turned on and not highlighted.
		Default: {vec3(0.0), 1.0} */
	vec4 colorKnobOn;
	/*  Color of the knob when turned on and highlighted.
		Default: {vec3(0.0), 1.0} */
	vec4 colorKnobHighlightOn;
	/*  Where the animation between states currently is.
		Default: 0.0 */
	f32 transition;
	/*  Whether the Checkbox is turned on.
		Default: false */
	bool checked;
	Checkbox();
	~Checkbox() = default;
	void Update(vec2 pos, bool selected) override;
	void Draw() const override;
};

// Returns whether a character is acceptable in a Textbox
typedef bool (*fpTextFilter)(char32);
// Returns whether a string is valid in a Textbox
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
struct Textbox : public Widget {
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
	vec4 colorBGHighlighted;
	/*  The color of the background when text validation failed.
		Default: {0.1, 0.0, 0.0, 0.9} */
	vec4 colorBGError;
	/*  The color of the text when not highlighted and text validation passed.
		Default: {vec3(1.0), 1.0} */
	vec4 colorText;
	/*  The color of the text when highlighted and text validation passed.
		Default: {vec3(1.0), 1.0} */
	vec4 colorTextHighlighted;
	/*  The color of the text when text validation failed.
		Default: {1.0, 0.5, 0.5, 1.0} */
	vec4 colorTextError;
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
	Textbox();
	~Textbox() = default;
	void CursorFromPosition(vec2 position);
	vec2 PositionFromCursor() const;
	void UpdateSize(vec2 container, f32 _scale) override;
	void Update(vec2 pos, bool selected) override;
	void Draw() const override;
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
	/*  Any Textbox that should reflect the value of the slider in text, and which can likewise affect our value.
		Default: nullptr */
	Textbox *mirror;
	/*  How many digits after the decimal point do we put into the mirror?
		Default: 1 */
	i32 mirrorPrecision;
	/*  Color of the background when not highlighted.
		Default: {vec3(0.15), 1.0} */
	vec4 colorBG;
	/*  Color of the slider knob when not highlighted.
		Default: {0.4, 0.9, 1.0, 1.0} */
	vec4 colorSlider;
	/*  Color of the background when highlighted.
		Default: {vec3(0.2), 0.9} */
	vec4 colorBGHighlighted;
	/*  Color of the slider knob when highlighted.
		Default: {0.9, 0.98, 1.0, 1.0} */
	vec4 colorSliderHighlighted;
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
	void Draw() const override;
	
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
	void Draw() const override;
	bool Selectable() const override;
};

struct Scissor {
	vec2i topLeft;
	vec2i botRight;
};

// External functions necessary for our function
struct Functions {
	// Basic commands. These must be set.
	
	// Set the drawable region
	typedef void (*fp_SetScissor) (void *dataGlobal, Any *dataWidget, vec2 position, vec2 size);
	typedef void (*fp_DrawQuad)   (void *dataGlobal, Any *dataWidget, vec2 position, vec2 size, vec4 color);
	typedef void (*fp_DrawImage)  (void *dataGlobal, Any *dataWidget, vec2 position, vec2 size, vec4 color);
	typedef void (*fp_DrawText)   (void *dataGlobal, Any *dataWidget, vec2 position, vec2 area, vec2 fontSize, const WString &text, vec4 color, vec4 colorOutline, bool bold);
	// Units are in the font's EM square
	// Multiply this by the font size for the actual dimensions
	typedef vec2    (*fp_GetTextDimensions)(void *dataGlobal, Any *dataWidget, const WString &string);
	// Units are in the font's EM square
	// Divide the actual width by the font size for the EM size
	typedef WString (*fp_ApplyTextWrapping)(void *dataGlobal, Any *dataWidget, const WString &string, f32 maxWidth);
	// Returns the index into the text to place the cursor based on pickerPosition. It should aim to find the cursor position closest to the left of the character halfway between lines (a UV of {0, 0.5}).
	typedef i32     (*fp_GetCursorFromPositionInText)(void *dataGlobal, Any *dataWidget, vec2 position, vec2 area, vec2 fontSize, const SimpleRange<char32> text, vec2 pickerPosition);
	// Returns the absolute position of a UV within the character at cursor where a UV of {0, 0} is the top left, and {1, 1} is the bottom right.
	typedef vec2    (*fp_GetPositionFromCursorInText)(void *dataGlobal, Any *dataWidget, vec2 position, vec2 area, vec2 fontSize, const SimpleRange<char32> text, i32 cursor, vec2 charUV);
	// Returns the height of one line for the given fontSize for the given widget.
	typedef f32     (*fp_GetLineHeight)(void *dataGlobal, Any *dataWidget, f32 fontSize);
	
	fp_SetScissor SetScissor = nullptr;
	fp_DrawQuad DrawQuad = nullptr;
	fp_DrawImage DrawImage = nullptr;
	fp_DrawText DrawText = nullptr;
	fp_GetTextDimensions GetTextDimensions = nullptr;
	fp_ApplyTextWrapping ApplyTextWrapping = nullptr;
	// These two are only used by Textboxes, so feel free to leave them unset when not using Textboxes.
	fp_GetCursorFromPositionInText GetCursorFromPositionInText = nullptr;
	fp_GetPositionFromCursorInText GetPositionFromCursorInText = nullptr;
	fp_GetLineHeight GetLineHeight = nullptr;
	
	// Input functions. These must be set.
	
	typedef bool (*fp_GetKeycodeState)(void *dataGlobal, Any *dataWidget, u8 keycode);
	
	fp_GetKeycodeState KeycodePressed = nullptr;
	fp_GetKeycodeState KeycodeRepeated = nullptr;
	fp_GetKeycodeState KeycodeDown = nullptr;
	fp_GetKeycodeState KeycodeReleased = nullptr;
	
	// Returns any characters that were typed since the last call
	typedef WString (*fp_ConsumeTypingString)(void *dataGlobal, Any *dataWidget);
	
	// Required for Textbox input
	fp_ConsumeTypingString ConsumeTypingString = nullptr;
	
	// Event Callbacks (used for custom behavior). These are optional.
	// NOTE: For responding to Button inputs use Button::state instead.
	
	// dataWidget CAN be nullptr, which means it's being called by System
	typedef void (*fp_Event)(void *dataGlobal, Any *dataWidget);
	
	fp_Event OnButtonPressed     = nullptr;
	fp_Event OnButtonRepeated    = nullptr;
	fp_Event OnButtonReleased    = nullptr;
	fp_Event OnButtonHighlighted = nullptr;
	
	fp_Event OnCheckboxTurnedOn  = nullptr;
	fp_Event OnCheckboxTurnedOff = nullptr;
};

// Default settings applied to Widgets whenever new Widgets are Created
struct Defaults {
	Spacer spacer;
	ListV listV;
	ListH listH;
	Switch switch_;
	Text text;
	Image image;
	Button button;
	Text buttonText;
	Checkbox checkbox;
	Textbox textbox;
	Slider slider;
};

enum class InputMethod {
	MOUSE,
	ARROWS,
	GAMEPAD,
};

struct System {
	Array<UniquePtr<Widget>> _allWidgets;
	Array<Scissor> stackScissors = {Scissor{vec2i(INT32_MIN), vec2i(INT32_MAX)}};
	
	Functions functions;
	Defaults defaults;
	// Passed into the various external functions
	void *data = nullptr;
	
	bool _goneBack = false;
	i32 controlDepth = 0;
	f32 scale = 1.0f;
	InputMethod inputMethod = InputMethod::MOUSE;
	vec2 canvasSize = vec2(1280.0f, 720.0f);
	vec2 mouseCursor = 0.0f;
	vec2 mouseCursorPrev = 0.0f;
	// Used to make sure the mouse can only interact with top-most widgets.
	// Also provides an easy test to see if the mouse can interact with items below it.
	Widget *mouseoverWidget;
	i32 mouseoverDepth;
	vec2 selectedCenter;
	f32 timestep = 1.0f / 60.0f;
	
	System() = default;
	System(const System &system) = delete;
	System(System &&system) = delete;
	
	void Update(vec2 newMouseCursor, vec2 _canvasSize, f32 _timestep);
	
	// Create functions use defaults, so be sure to set up defaults before calling these.
	
	Screen*   CreateScreen();
	
	Spacer*   CreateSpacer  (ListH *parent, f32 fraction);
	Spacer*   CreateSpacer  (ListV *parent, f32 fraction);
	ListV*    CreateListV   (Widget *parent, bool deeper=false);
	ListH*    CreateListH   (Widget *parent, bool deeper=false);
	Switch*   CreateSwitch  (Widget *parent);
	Text*     CreateText    (Widget *parent, bool deeper=false);
	Image*    CreateImage   (Widget *parent, bool deeper=false);
	Button*   CreateButton  (Widget *parent, bool deeper=false);
	Checkbox* CreateCheckbox(Widget *parent, bool deeper=false);
	Textbox*  CreateTextbox (Widget *parent, bool deeper=false);
	Slider*   CreateSlider  (Widget *parent, bool deeper=false);
	Hideable* CreateHideable(Widget *parent, Widget *child, bool deeper=false);
	Hideable* CreateHideable(Widget *parent, Switch *child, bool deeper=false);
	
	// Create From functions use template objects instead of the defaults
	
	ListV*    CreateListVFrom   (Widget *parent, const ListV &src, bool deeper=false);
	ListH*    CreateListHFrom   (Widget *parent, const ListH &src, bool deeper=false);
	Switch*   CreateSwitchFrom  (Widget *parent, const Switch &src);
	Text*     CreateTextFrom    (Widget *parent, const Text &src, bool deeper=false);
	Image*    CreateImageFrom   (Widget *parent, const Image &src, bool deeper=false);
	Button*   CreateButtonFrom  (Widget *parent, const Button &src, bool deeper=false);
	Checkbox* CreateCheckboxFrom(Widget *parent, const Checkbox &src, bool deeper=false);
	Textbox*  CreateTextboxFrom (Widget *parent, const Textbox &src, bool deeper=false);
	Slider*   CreateSliderFrom  (Widget *parent, const Slider &src, bool deeper=false);
	
	inline ListV*    CreateListVAsDefault   (List *parent, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateListV(parent, deeper);
	}
	inline ListH*    CreateListHAsDefault   (List *parent, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateListH(parent, deeper);
	}
	inline Switch*   CreateSwitchAsDefault  (List *parent) {
		parent->selectionDefault = parent->children.size;
		return CreateSwitch(parent);
	}
	inline Text*     CreateTextAsDefault    (List *parent, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateText(parent, deeper);
	}
	inline Image*    CreateImageAsDefault   (List *parent, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateImage(parent, deeper);
	}
	inline Button*   CreateButtonAsDefault  (List *parent, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateButton(parent, deeper);
	}
	inline Checkbox* CreateCheckboxAsDefault(List *parent, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateCheckbox(parent, deeper);
	}
	inline Textbox*  CreateTextboxAsDefault (List *parent, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateTextbox(parent, deeper);
	}
	inline Slider*   CreateSliderAsDefault  (List *parent, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateSlider(parent, deeper);
	}
	inline Hideable* CreateHideableAsDefault(List *parent, Widget *child, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateHideable(parent, child, deeper);
	}
	inline Hideable* CreateHideableAsDefault(List *parent, Switch *child, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateHideable(parent, child, deeper);
	}
	inline ListV*    CreateListVAsDefaultFrom   (List *parent, const ListV &src, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateListVFrom(parent, src, deeper);
	}
	inline ListH*    CreateListHAsDefaultFrom   (List *parent, const ListH &src, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateListHFrom(parent, src, deeper);
	}
	inline Switch*   CreateSwitchAsDefaultFrom  (List *parent, const Switch &src) {
		parent->selectionDefault = parent->children.size;
		return CreateSwitchFrom(parent, src);
	}
	inline Text*     CreateTextAsDefaultFrom    (List *parent, const Text &src, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateTextFrom(parent, src, deeper);
	}
	inline Image*    CreateImageAsDefaultFrom   (List *parent, const Image &src, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateImageFrom(parent, src, deeper);
	}
	inline Button*   CreateButtonAsDefaultFrom  (List *parent, const Button &src, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateButtonFrom(parent, src, deeper);
	}
	inline Checkbox* CreateCheckboxAsDefaultFrom(List *parent, const Checkbox &src, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateCheckboxFrom(parent, src, deeper);
	}
	inline Textbox*  CreateTextboxAsDefaultFrom (List *parent, const Textbox &src, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateTextboxFrom(parent, src, deeper);
	}
	inline Slider*   CreateSliderAsDefaultFrom  (List *parent, const Slider &src, bool deeper=false) {
		parent->selectionDefault = parent->children.size;
		return CreateSliderFrom(parent, src, deeper);
	}
	
	void AddWidget(Widget *parent, Widget *newWidget, bool deeper=false);
	void AddWidget(Widget *parent, Switch *newWidget);
	void AddWidgetAsDefault(List *parent, Widget *newWidget, bool deeper=false);
	void AddWidgetAsDefault(List *parent, Switch *newWidget);
};

} // namespace AzCore::GuiGeneric

#endif // AZCORE_GUI_HPP
