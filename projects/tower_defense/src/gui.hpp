/*
    File: gui.hpp
    Author: Philip Haynes
*/

#ifndef GUI_HPP
#define GUI_HPP

#include "objects.hpp"
#include "assets.hpp"

namespace Int { // Short for Interface

// Ways to define a GUI with a hierarchy

// Base polymorphic interface
struct Widget {
protected:
    mutable vec2 size; // size is pixel space and often depends on contents.
    mutable bool sizeUpdated;
public:
    Array<Widget*> children;
    vec2 margin; // Space surrounding the widget.
    vec2 position; // Relative to the parent widget.
    vec2 positionAbsolute; // Where we are in pixel space.
    i32 depth; // How deeply nested we are. Used for input culling for controllers.
    Widget();
    virtual ~Widget() = default;
    vec2 GetSize() const;
    virtual void UpdateSize() const = 0;
    virtual void Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
    virtual void Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const;
};

// Lowest level widget, used for input for game objects.
struct Screen : public Widget {
    Rendering::Manager *rendering;
    Screen();
    ~Screen() = default;
    void Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
    void UpdateSize() const;
};

// A vertical list of items.
struct ListV : public Widget {
    void UpdateSize() const;
    void Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
};

// A horizontal list of items.
struct ListH : public Widget {
    void UpdateSize() const;
    void Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering);
};

struct Text : public Widget {
    WString string;
    f32 fontSize;
    i32 fontIndex;
    Rendering::Manager *rendering;
    Text();
    ~Text() = default;
    void UpdateSize() const;
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
    const WString text = ToWString(
        "Hahaha look at me! There's so much to say! I don't know what else to do. ¡Hola señor Lopez! ¿Cómo está usted? Estoy muy bien. ¿Y cómo se llama? ありがとうお願いします私はハンバーガー 세계를 향한 대화, 유니코드로 하십시오. 経機速講著述元載説赤問台民。 Лорем ипсум долор сит амет Λορεμ ιπσθμ δολορ σιτ αμετ There once was a man named Chad. He was an incel. What a terrible sight! If only someone was there to teach him the ways of humility! Oh how he would wail and toil how all the girls would pass up a \"nice guy like me\". What a bitch."
    );

    Array<Widget*> allWidgets; // So we can delete them at the end of the program.
    Screen screenWidget;

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
