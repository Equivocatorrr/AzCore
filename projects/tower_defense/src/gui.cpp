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
    screenWidget.rendering = rendering;
    textWidget[0] = new Text();
    textWidget[0]->string = ToWString("Hahaha look at me! There's so much to say! I don't know what else to do. ¡Hola señor Lopez! ¿Cómo está usted? Estoy muy bien. ¿Y cómo se llama? ありがとうお願いします私はハンバーガー 세계를 향한 대화, 유니코드로 하십시오. 経機速講著述元載説赤問台民。 Лорем ипсум долор сит амет Λορεμ ιπσθμ δολορ σιτ αμετ There once was a man named Chad. He was an incel. What a terrible sight! If only someone was there to teach him the ways of humility! Oh how he would wail and toil how all the girls would pass up a \"nice guy like me\". What a bitch.");
    textWidget[0]->fontIndex = fontIndex;
    ListV *listWidget = new ListV();
    ListH *listHWidget = new ListH();
    listHWidget->padding = vec2(0.0);
    listHWidget->margin = vec2(0.0);
    AddWidget(&screenWidget, listWidget);
    textWidget[2] = new Text();
    textWidget[2]->string = ToWString("Title");
    textWidget[2]->fontSize = 64.0;
    textWidget[2]->color = vec4(1.0, 0.5, 0.0, 1.0);
    textWidget[2]->colorOutline = vec4(0.2, 0.1, 0.0, 1.0);
    textWidget[2]->outline = true;
    textWidget[2]->alignH = Rendering::CENTER;
    AddWidget(listWidget, textWidget[2]);
    AddWidget(listWidget, listHWidget);
    AddWidget(listHWidget, textWidget[0]);
    textWidget[1] = new Text();
    textWidget[1]->string = ToWString("Hey now! You're an all star! Get your shit together!");
    textWidget[1]->fontIndex = fontIndex;
    AddWidget(listHWidget, textWidget[1]);
}

void Gui::EventUpdate(bool buffer, Objects::Manager *objects, Rendering::Manager *rendering) {
    for (i32 i = 0; i < 2; i++) {
        textWidget[i]->maxSize.x = (screenWidget.GetSize().x - 128.0) * 0.5;
    }
    textWidget[2]->maxSize.x = (screenWidget.GetSize().x - 128.0);
    screenWidget.Update(vec2(0.0), this, objects, rendering);
    if (objects->input->Pressed(KC_KEY_1)) {
        font->SaveAtlas();
    }
    if (objects->input->Pressed(KC_KEY_2)) {
        (*rendering->fonts)[0].SaveAtlas();
    }
    if (objects->input->Pressed(KC_KEY_UP)) {
        textWidget[0]->fontSize += 1.0;
    }
    if (objects->input->Pressed(KC_KEY_DOWN)) {
        textWidget[0]->fontSize -= 1.0;
    }
}

void Gui::EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer) {
    // rendering->BindPipelineFont(commandBuffer);
    // rendering->DrawCharSS(commandBuffer, text[25], fontIndex, pos, vec2(size));
    // rendering->DrawTextSS(commandBuffer, text, fontIndex, pos, vec2(size));
    screenWidget.Draw(rendering, commandBuffer);
}

void Gui::AddWidget(Widget *parent, Widget *newWidget) {
    parent->children.Append(newWidget);
    if (allWidgets.count(newWidget) == 0) {
        allWidgets.emplace(newWidget);
    }
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

ListV::ListV() : minSize(0.0), maxSize(0.0), padding(16.0) {}

void ListV::UpdateSize() const {
    size = vec2(0.0);
    for (const Widget* child : children) {
        const vec2 childSize = child->GetSize();
        size.y += childSize.y;
        if (childSize.x > size.x) {
            size.x = childSize.x;
        }
    }
    size += padding * 2.0;
    if (maxSize.x > 0.0) {
        size.x = median(minSize.x, size.x, maxSize.x);
    } else {
        size.x = max(minSize.x, size.x);
    }
    if (maxSize.y > 0.0) {
        size.y = median(minSize.y, size.y, maxSize.y);
    } else {
        size.y = max(minSize.y, size.y);
    }
    sizeUpdated = true;
}

void ListV::Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    sizeUpdated = false;
    pos += margin + padding;
    for (Widget *child : children) {
        child->Update(pos, gui, objects, rendering);
        pos.y += child->GetSize().y;
    }
}

ListH::ListH() : minSize(0.0), maxSize(0.0), padding(16.0) {}

void ListH::UpdateSize() const {
    size = 0.0;
    for (const Widget* child : children) {
        const vec2 childSize = child->GetSize();
        size.x += childSize.x;
        if (childSize.y > size.y) {
            size.y = childSize.y;
        }
    }
    size += padding * 2.0;
    if (maxSize.x > 0.0) {
        size.x = median(minSize.x, size.x, maxSize.x);
    } else {
        size.x = max(minSize.x, size.x);
    }
    if (maxSize.y > 0.0) {
        size.y = median(minSize.y, size.y, maxSize.y);
    } else {
        size.y = max(minSize.y, size.y);
    }
    sizeUpdated = true;
}

void ListH::Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    sizeUpdated = false;
    pos += margin + padding;
    for (Widget *child : children) {
        child->Update(pos, gui, objects, rendering);
        pos.x += child->GetSize().x;
    }
}

Text::Text() : rendering(nullptr), stringFormatted(), string(), fontSize(32.0), fontIndex(1), alignH(Rendering::LEFT), alignV(Rendering::TOP), maxSize(0.0), color(1.0), colorOutline(0.0, 0.0, 0.0, 1.0), outline(false) {}

void Text::UpdateSize() const {
    if (maxSize.x > 0.0) {
        size.x = maxSize.x;
    } else {
        size.x = rendering->StringWidth(stringFormatted, fontIndex) * fontSize;
    }
    if (maxSize.y > 0.0) {
        size.y = maxSize.y;
    } else {
        size.y = rendering->StringHeight(stringFormatted) * fontSize;
    }
    sizeUpdated = true;
}

void Text::Update(vec2 pos, Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    this->rendering = rendering;
    stringFormatted = rendering->StringAddNewlines(string, fontIndex, maxSize.x / fontSize);
    Widget::Update(pos, gui, objects, rendering);
}

void Text::Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const {
    const vec2 screenSizeFactor = vec2(2.0) / rendering->screenSize;
    const f32 edge = 0.3 + min(0.2, max(0.0, (fontSize - 12.0) / 12.0));
    const f32 bounds = 0.5 - min(0.05, max(0.0, (16.0 - fontSize) * 0.01));
    if (maxSize.x != 0.0 && maxSize.y != 0.0) {
        vk::CmdSetScissor(commandBuffer, (u32)maxSize.x, (u32)maxSize.y, (i32)positionAbsolute.x, (i32)positionAbsolute.y);
    }
    rendering->BindPipelineFont(commandBuffer);
    vec2 drawPos = positionAbsolute * screenSizeFactor + vec2(-1.0);
    vec2 scale = vec2(fontSize * screenSizeFactor.y);
    f32 maxWidth = maxSize.x * screenSizeFactor.x;
    if (alignH == Rendering::CENTER) {
        drawPos.x += maxWidth * 0.5;
    } else if (alignH == Rendering::RIGHT) {
        drawPos.x += maxWidth;
    }
    if (alignV == Rendering::CENTER) {
        drawPos.y += GetSize().y * screenSizeFactor.y * 0.5;
    } else if (alignV == Rendering::BOTTOM) {
        drawPos.y += GetSize().y * screenSizeFactor.y;
    }
    if (outline) {
        rendering->DrawTextSS(commandBuffer, stringFormatted, fontIndex, colorOutline, drawPos, scale, alignH, alignV, maxWidth, edge+0.1, bounds-0.3);
    }
    rendering->DrawTextSS(commandBuffer, stringFormatted, fontIndex, color, drawPos, scale, alignH, alignV, maxWidth, edge, bounds);
    if (maxSize.x != 0.0 && maxSize.y != 0.0) {
        vk::CmdSetScissor(commandBuffer, rendering->window->width, rendering->window->height, 0, 0);
    }
}

} // namespace Int
