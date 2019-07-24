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
    textWidget[0] = new Text();
    textWidget[0]->string = ToWString("Hahaha look at me! There's so much to say! I don't know what else to do. ¡Hola señor Lopez! ¿Cómo está usted? Estoy muy bien. ¿Y cómo se llama? ありがとうお願いします私はハンバーガー 세계를 향한 대화, 유니코드로 하십시오. 経機速講著述元載説赤問台民。 Лорем ипсум долор сит амет Λορεμ ιπσθμ δολορ σιτ αμετ There once was a man named Chad. He was an incel. What a terrible sight! If only someone was there to teach him the ways of humility! Oh how he would wail and toil how all the girls would pass up a \"nice guy like me\". What a bitch.");
    textWidget[0]->fontIndex = fontIndex;
    textWidget[0]->size.x = 0.5;
    textWidget[0]->alignH = Rendering::JUSTIFY;
    ListV *listWidget = new ListV();
    ListH *listHWidget = new ListH();
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
    textWidget[1]->size.x = 0.5;
    textWidget[1]->alignH = Rendering::JUSTIFY;
    AddWidget(listHWidget, textWidget[1]);
}

void Gui::EventUpdate(bool buffer, Objects::Manager *objects, Rendering::Manager *rendering) {
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

Widget::Widget() : children(), margin(16.0), size(1.0), sizeIsFraction(true), sizeAbsolute(0.0), minSize(0.0), maxSize(-1.0), position(0.0), positionAbsolute(0.0), depth(0) {}

void Widget::LimitSize() {
    if (maxSize.x >= 0.0) {
        sizeAbsolute.x = median(minSize.x, sizeAbsolute.x, maxSize.x);
    } else {
        sizeAbsolute.x = max(minSize.x, sizeAbsolute.x);
    }
    if (maxSize.y >= 0.0) {
        sizeAbsolute.y = median(minSize.y, sizeAbsolute.y, maxSize.y);
    } else {
        sizeAbsolute.y = max(minSize.y, sizeAbsolute.y);
    }
}

void Widget::Update(vec2 pos, Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
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

Screen::Screen() {
    margin = vec2(0.0);
}

void Screen::Update(vec2 pos, Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    UpdateSize(rendering->screenSize);
    Widget::Update(pos, gui, objects, rendering);
}

void Screen::UpdateSize(vec2 container) {
    sizeAbsolute = container - margin * 2.0;
    for (Widget* child : children) {
        child->UpdateSize(sizeAbsolute);
    }
}

ListV::ListV() : padding(16.0) {}

void ListV::UpdateSize(vec2 container) {
    sizeAbsolute = vec2(0.0);
    if (size.x > 0.0) {
        sizeAbsolute.x = (sizeIsFraction ? container.x * size.x : size.x) - margin.x * 2.0;
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = (sizeIsFraction ? container.y * size.y : size.y) - margin.y * 2.0;
    }
    for (Widget* child : children) {
        child->UpdateSize(sizeAbsolute - padding * 2.0);
        vec2 childSize = child->GetSize();
        if (size.x == 0.0) {
            sizeAbsolute.x = max(sizeAbsolute.x, childSize.x);
        }
        if (size.y == 0.0) {
            sizeAbsolute.y += childSize.y;
        }
    }
    LimitSize();
}

void ListV::Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    pos += margin + padding;
    for (Widget *child : children) {
        child->Update(pos, gui, objects, rendering);
        pos.y += child->GetSize().y;
    }
}

ListH::ListH() : padding(0.0) {}

void ListH::UpdateSize(vec2 container) {
    sizeAbsolute = vec2(0.0);
    if (size.x > 0.0) {
        sizeAbsolute.x = (sizeIsFraction ? container.x * size.x : size.x) - margin.x * 2.0;
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = (sizeIsFraction ? container.y * size.y : size.y) - margin.y * 2.0;
    }
    for (Widget* child : children) {
        child->UpdateSize(sizeAbsolute - padding * 2.0);
        vec2 childSize = child->GetSize();
        if (size.x == 0.0) {
            sizeAbsolute.x += childSize.x;
        }
        if (size.y == 0.0) {
            sizeAbsolute.y = max(sizeAbsolute.y, childSize.y);
        }
    }
    LimitSize();
}

void ListH::Update(vec2 pos, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    pos += margin + padding;
    for (Widget *child : children) {
        child->Update(pos, gui, objects, rendering);
        pos.x += child->GetSize().x;
    }
}

Text::Text() : rendering(nullptr), stringFormatted(), string(), fontSize(32.0), fontIndex(1), alignH(Rendering::LEFT), alignV(Rendering::TOP), color(1.0), colorOutline(0.0, 0.0, 0.0, 1.0), outline(false) {
    size.y = 0.0;
}

void Text::UpdateSize(vec2 container) {
    if (size.x > 0.0) {
        sizeAbsolute.x = (sizeIsFraction ? container.x * size.x : size.x) - margin.x * 2.0;
    } else {
        sizeAbsolute.x = rendering->StringWidth(stringFormatted, fontIndex) * fontSize;
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = (sizeIsFraction ? container.y * size.y : size.y) - margin.y * 2.0;
    } else {
        sizeAbsolute.y = rendering->StringHeight(stringFormatted) * fontSize;
    }
    LimitSize();
}

void Text::Update(vec2 pos, Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    this->rendering = rendering;
    stringFormatted = rendering->StringAddNewlines(string, fontIndex, sizeAbsolute.x / fontSize);
    Widget::Update(pos, gui, objects, rendering);
}

void Text::Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const {
    const vec2 screenSizeFactor = vec2(2.0) / rendering->screenSize;
    const f32 edge = 0.3 + min(0.2, max(0.0, (fontSize - 12.0) / 12.0));
    const f32 bounds = 0.5 - min(0.05, max(0.0, (16.0 - fontSize) * 0.01));
    if (maxSize.x != 0.0 && maxSize.y != 0.0) {
        vk::CmdSetScissor(commandBuffer, (u32)sizeAbsolute.x, (u32)sizeAbsolute.y, (i32)positionAbsolute.x, (i32)positionAbsolute.y);
    }
    rendering->BindPipelineFont(commandBuffer);
    vec2 drawPos = positionAbsolute * screenSizeFactor + vec2(-1.0);
    vec2 scale = vec2(fontSize * screenSizeFactor.y);
    f32 maxWidth = sizeAbsolute.x * screenSizeFactor.x;
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
