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

// Base polymorphic interface
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
    Widget();
    virtual ~Widget() = default;
    virtual void UpdateSize(vec2 container) = 0;
    void LimitSize();
    inline vec2 GetSize() const { return sizeAbsolute + margin * 2.0; }
    virtual void Update(vec2 pos, bool selected, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
    virtual void Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const;

    const bool MouseOver(const Objects::Manager *objects) const;
};

// Lowest level widget, used for input for game objects.
struct Screen : public Widget {
    Screen();
    ~Screen() = default;
    void Update(vec2 pos, bool selected, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
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
    bool UpdateSelection(bool selected, struct Gui *gui, Objects::Manager *objects, u8 keyCodeSelect, u8 keyCodeBack, u8 keyCodeIncrement, u8 keyCodeDecrement);
    void Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const;
};

// A vertical list of items.
struct ListV : public List {
    void UpdateSize(vec2 container);
    void Update(vec2 pos, bool selected, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
};

// A horizontal list of items.
struct ListH : public List {
    ListH();
    void UpdateSize(vec2 container);
    void Update(vec2 pos, bool selected, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
};

struct Text : public Widget {
    Rendering::Manager *rendering;
private:
    WString stringFormatted;
public:
    WString string;
    f32 fontSize;
    i32 fontIndex;
    Rendering::FontAlign alignH, alignV;
    vec4 color, colorOutline;
    bool outline;
    Text();
    ~Text() = default;
    void UpdateSize(vec2 container);
    void Update(vec2 pos, bool selected, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
    void Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const;
};

// struct Image;

struct Button : public Widget {
    WString string;
    vec4 colorBG, highlightBG, colorText, highlightText;
    i32 fontIndex;
    f32 fontSize;
    bool mouseover;
    io::ButtonState state;
    Button();
    ~Button() = default;
    void UpdateSize(vec2 container);
    void Update(vec2 pos, bool selected, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
    void Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const;
};

// struct Checkbox; // Boolean widget.

// struct Switch; // Allows the user to choose from a selection of widgets (usually Text).

// struct Slider; // A scalar within a range.

struct Gui : public Objects::Object {
    i32 fontIndex;
    Assets::Font *font;
    i32 controlDepth = 0;

    Set<Widget*> allWidgets; // So we can delete them at the end of the program.
    Screen screenWidget;
    Text *textWidget;

    ~Gui();

    void EventAssetInit(Assets::Manager *assets);
    void EventAssetAcquire(Assets::Manager *assets);
    void EventInitialize(Objects::Manager *objects, Rendering::Manager *rendering);
    void EventUpdate(bool buffer, Objects::Manager *objects, Rendering::Manager *rendering);
    void EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer);

    // deeper means the widget can only be interacted with by selecting it first
    void AddWidget(Widget *parent, Widget *newWidget, bool deeper = false);
};



} // namespace Int

#endif // GUI_HPP
