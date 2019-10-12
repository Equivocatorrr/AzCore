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

// Ways to define a GUI with a hierarchy
struct Gui;

// Base polymorphic interface, also usable as a blank spacer.
struct Widget {
    Array<Widget*> children;
    vec2 margin; // Space surrounding the widget.
    vec2 size; // Either pixel size, or fraction of parent container. 0.0 means it depends on contents.
    bool fractionWidth, fractionHeight;
    vec2 sizeAbsolute; // sizeAbsolute is pixel space and can depend on contents.
    vec2 minSize, maxSize; // Absolute limits, negative values are ignored.
    vec2 position; // Relative to the parent widget.
    vec2 positionAbsolute; // Where we are in pixel space.
    i32 depth; // How deeply nested we are. Used for input culling for controllers.
    bool selectable; // Whether or not this widget can be used in a selection
    bool highlighted; // True when selected
    bool occludes; // Whether the widget counts for mouse occlusion
    bool mouseover;
    Widget();
    virtual ~Widget() = default;
    virtual void UpdateSize(vec2 container);
    void LimitSize();
    void PushScissor(Rendering::DrawingContext &context) const;
    void PopScissor(Rendering::DrawingContext &context) const;
    inline vec2 GetSize() const { return sizeAbsolute + margin * 2.0; }
    virtual void Update(vec2 pos, bool selected);
    virtual void Draw(Rendering::DrawingContext &context) const;

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
    vec2 padding;
    vec4 color, highlight;
    i32 selection; // Which child we have selected, -1 for none
    i32 selectionDefault; // If we're ever selected, what should we select by default?
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

struct Text : public Widget {
private:
    WString stringFormatted;
public:
    WString string;
    vec2 padding;
    f32 fontSize;
    i32 fontIndex;
    bool bold, paddingEM;
    Rendering::FontAlign alignH, alignV;
    vec4 color, colorOutline;
    bool outline;
    Text();
    ~Text() = default;
    void UpdateSize(vec2 container);
    void Update(vec2 pos, bool selected);
    void Draw(Rendering::DrawingContext &context) const;
};

struct Image : public Widget {
    i32 texIndex;
    Image();
    ~Image() = default;
    void Draw(Rendering::DrawingContext &context) const;
};

struct Button : public Widget {
    WString string;
    vec4 colorBG, highlightBG, colorText, highlightText;
    i32 fontIndex;
    f32 fontSize;
    io::ButtonState state;
    Button();
    ~Button() = default;
    void Update(vec2 pos, bool selected);
    void Draw(Rendering::DrawingContext &context) const;
};

// Boolean widget.
struct Checkbox : public Widget {
    vec4 colorOff, highlightOff, colorOn, highlightOn;
    f32 transition;
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
    WString string, stringFormatted;
    vec4 colorBG, highlightBG, errorBG, colorText, highlightText, errorText;
    vec2 padding;
    i32 cursor;
    i32 fontIndex;
    f32 fontSize;
    f32 cursorBlinkTimer;
    Rendering::FontAlign alignH;
    fpTextFilter textFilter;
    fpTextValidate textValidate;
    bool entry;
    bool multiline;
    TextBox();
    ~TextBox() = default;
    void CursorFromPosition(vec2 position);
    vec2 PositionFromCursor() const;
    void UpdateSize(vec2 container);
    void Update(vec2 pos, bool selected);
    void Draw(Rendering::DrawingContext &context) const;
};

// struct Switch; // Allows the user to choose from a selection of widgets (usually Text).

// A scalar within a range.
struct Slider : public Widget {
    f32 value, valueMin, valueMax;
    TextBox *mirror;
    vec4 colorBG, colorSlider, highlightBG, highlightSlider;
    bool highlighted, grabbed;
    io::ButtonState left, right;
    Slider();
    ~Slider() = default;
    void Update(vec2 pos, bool selected);
    void Draw(Rendering::DrawingContext &context) const;
};

enum MenuEnum {
    MENU_MAIN,
    MENU_SETTINGS,
    MENU_PLAY
};

// Now we can have some different screens
struct MainMenu {
    Screen screen;
    Button *buttonStart;
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

struct PlayMenu {
    Screen screen;
    ListV *list;
    Text *waveInfo, *towerInfo;
    Array<Button*> towerButtons;
    Button *buttonMenu;
    Button *buttonStartWave;

    void Initialize();
    void Update();
    void Draw(Rendering::DrawingContext &context);
};

struct Gui : public Objects::Object {
    i32 fontIndex;
    i32 texIndex;
    Sound::Source sndClickInSources[4];
    Sound::Source sndClickOutSources[4];
    Sound::Source sndClickSoftSources[2];
    Sound::Source sndPopHigh, sndPopLow;
    Sound::MultiSource sndClickIn;
    Sound::MultiSource sndClickOut;
    Sound::MultiSource sndClickSoft;
    Assets::Font *font;
    i32 controlDepth = 0;
    f32 scale = 1.0;
    // Used to make sure the mouse can only interact with top-most widgets.
    // Also provides an easy test to see if the mouse can interact with items below it.
    Widget *mouseoverWidget;
    i32 mouseoverDepth;

    Set<Widget*> allWidgets; // So we can delete them at the end of the program.

    MenuEnum currentMenu = MENU_MAIN;
    MenuEnum nextMenu = MENU_MAIN;
    MainMenu mainMenu;
    SettingsMenu settingsMenu;
    PlayMenu playMenu;

    ~Gui();

    void EventAssetInit();
    void EventAssetAcquire();
    void EventInitialize();
    void EventSync();
    void EventDraw(Array<Rendering::DrawingContext> &contexts);
};



} // namespace Int

#endif // GUI_HPP
