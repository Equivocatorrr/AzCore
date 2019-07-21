/*
    File: gui.cpp
    Author: Philip Haynes
*/

#include "gui.hpp"
#include "assets.hpp"
#include "rendering.hpp"

namespace Int {

Gui::~Gui() {
    for (Widget* widget : allWidgets) {
        delete widget;
    }
}

void Gui::EventAssetInit(Assets::Manager *assets) {
    assets->filesToLoad.Append("DroidSans.ttf");
    // assets->filesToLoad.Append("LiberationSerif-Regular.ttf");
    // assets->filesToLoad.Append("OpenSans-Regular.ttf");
    // assets->filesToLoad.Append("Literata[wght].ttf");
}

void Gui::EventAssetAcquire(Assets::Manager *assets) {
    fontIndex = assets->FindMapping("DroidSans.ttf");
    // fontIndex = assets->FindMapping("LiberationSerif-Regular.ttf");
    // fontIndex = assets->FindMapping("OpenSans-Regular.ttf");
    // fontIndex = assets->FindMapping("Literata[wght].ttf");
    font = &assets->fonts[fontIndex];
}

void Gui::EventInitialize(Objects::Manager *objects, Rendering::Manager *rendering) {
    Text *textWidget = new Text();
    textWidget->string = text;
    textWidget->fontIndex = fontIndex;
    AddWidget(&screenWidget, textWidget);
}

void Gui::EventUpdate(bool buffer, Objects::Manager *objects, Rendering::Manager *rendering) {
    screenWidget.Update(vec2(0.0), this, objects, rendering);
}

void Gui::EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer) {
    // rendering->BindPipelineFont(commandBuffer);
    // rendering->DrawCharSS(commandBuffer, text[25], fontIndex, pos, vec2(size));
    // rendering->DrawTextSS(commandBuffer, text, fontIndex, pos, vec2(size));
    screenWidget.Draw(rendering, commandBuffer);
}

void Gui::AddWidget(Widget *parent, Widget *newWidget) {
    parent->children.Append(newWidget);
    allWidgets.Append(newWidget);
}

//
//      Widget implementations beyond this point
//

Widget::Widget() : size(0.0), sizeUpdated(false), children(), margin(16.0), position(0.0), positionAbsolute(0.0), depth(0) {}

vec2 Widget::GetSize() const {
    if (!sizeUpdated) {
        UpdateSize();
    }
    return size + margin * 2.0;
}

void Widget::Update(vec2 pos, Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    sizeUpdated = false;
    pos += margin;
    positionAbsolute = pos;
    for (Widget* child : children) {
        child->Update(pos, gui, objects, rendering);
    }
}

void Widget::Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const {
    for (const Widget* child : children) {
        child->Draw(rendering, commandBuffer);
    }
}

Screen::Screen() : rendering(nullptr) {
    margin = vec2(0.0);
}

void Screen::Update(vec2 pos, Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    this->rendering = rendering;
    Widget::Update(pos, gui, objects, rendering);
}

void Screen::UpdateSize() const {
    size = rendering->screenSize;
    sizeUpdated = true;
}

void ListV::UpdateSize() const {
    size = 0.0;
    for (const Widget* child : children) {
        const vec2 childSize = child->GetSize();
        size.y += childSize.y;
        if (childSize.x > size.x) {
            size.x = childSize.x;
        }
    }
    sizeUpdated = true;
}

void ListV::Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    sizeUpdated = false;
    pos += margin;
    for (Widget *child : children) {
        child->Update(pos, gui, objects, rendering);
        pos.y += child->GetSize().y;
    }
}

void ListH::UpdateSize() const {
    size = 0.0;
    for (const Widget* child : children) {
        const vec2 childSize = child->GetSize();
        size.x += childSize.x;
        if (childSize.y > size.y) {
            size.y = childSize.y;
        }
    }
    sizeUpdated = true;
}

void ListH::Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    sizeUpdated = false;
    pos += margin;
    for (Widget *child : children) {
        child->Update(pos, gui, objects, rendering);
        pos.x += child->GetSize().x;
    }
}

Text::Text() : string(), fontSize(64.0), fontIndex(1), rendering(nullptr) {}

void Text::UpdateSize() const {
    size = rendering->StringSize(string, fontIndex) * vec2(rendering->aspectRatio, 1.0) * fontSize;
    sizeUpdated = true;
}

void Text::Update(vec2 pos, Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    this->rendering = rendering;
    Widget::Update(pos, gui, objects, rendering);
}

void Text::Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const {
    const vec2 screenSizeFactor = vec2(2.0) / rendering->screenSize;
    rendering->BindPipelineFont(commandBuffer);
    rendering->DrawTextSS(commandBuffer, string, fontIndex, positionAbsolute * screenSizeFactor + vec2(-1.0), vec2(fontSize * screenSizeFactor.y), Rendering::FontAlign::JUSTIFY, Rendering::FontAlign::TOP, 1248.0 * screenSizeFactor.x);
}

} // namespace Int
