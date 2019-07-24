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
    bool sizeIsFraction;
    vec2 sizeAbsolute; // sizeAbsolute is pixel space and can depend on contents.
    vec2 minSize, maxSize; // Absolute limits, negative values are ignored.
    vec2 position; // Relative to the parent widget.
    vec2 positionAbsolute; // Where we are in pixel space.
    i32 depth; // How deeply nested we are. Used for input culling for controllers.
    Widget();
    virtual ~Widget() = default;
    virtual void UpdateSize(vec2 container) = 0;
    void LimitSize();
    inline vec2 GetSize() const { return sizeAbsolute + margin * 2.0; }
    virtual void Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
    virtual void Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const;
};

// Lowest level widget, used for input for game objects.
struct Screen : public Widget {
    Screen();
    ~Screen() = default;
    void Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
    void UpdateSize(vec2 container);
};

// A vertical list of items.
struct ListV : public Widget {
    vec2 padding;
    ListV();
    ~ListV() = default;
    void UpdateSize(vec2 container);
    void Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
};

// A horizontal list of items.
struct ListH : public Widget {
    vec2 padding;
    ListH();
    ~ListH() = default;
    void UpdateSize(vec2 container);
    void Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
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
    void Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
    void Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const;
};

// struct Image;

// struct Button; // May contain a single widget.

// struct Checkbox; // Boolean widget.

// struct Switch; // Allows the user to choose from a selection of widgets (usually Text).

// struct Slider; // A scalar within a range.

struct Gui : public Objects::Object {
    i32 fontIndex;
    Assets::Font *font;
    i32 controlDepth = 0;

    Set<Widget*> allWidgets; // So we can delete them at the end of the program.
    Screen screenWidget;
    Text *textWidget[3];

    ~Gui();

    void EventAssetInit(Assets::Manager *assets);
    void EventAssetAcquire(Assets::Manager *assets);
    void EventInitialize(Objects::Manager *objects, Rendering::Manager *rendering);
    void EventUpdate(bool buffer, Objects::Manager *objects, Rendering::Manager *rendering);
    void EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer);

    void AddWidget(Widget *parent, Widget *newWidget);
};



} // namespace Int

#endif // GUI_HPP
