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
    assets->filesToLoad.Append("test.tga");
}

void Gui::EventAssetAcquire(Assets::Manager *assets) {
    fontIndex = assets->FindMapping("DroidSans.ttf");
    // fontIndex = assets->FindMapping("LiberationSerif-Regular.ttf");
    // fontIndex = assets->FindMapping("OpenSans-Regular.ttf");
    // fontIndex = assets->FindMapping("Literata[wght].ttf");
    texIndex = assets->FindMapping("test.tga");
    font = &assets->fonts[fontIndex];
}

void Gui::EventInitialize(Objects::Manager *objects, Rendering::Manager *rendering) {
    ListV *listWidget = new ListV();
    listWidget->size.x = 1000.0;
    listWidget->fractionWidth = false;
    AddWidget(&screenWidget, listWidget);

    Text *titleWidget = new Text();
    titleWidget->string = ToWString("Title");
    titleWidget->fontSize = 64.0;
    titleWidget->color = vec4(1.0, 0.5, 0.0, 1.0);
    titleWidget->colorOutline = vec4(0.4, 0.2, 0.0, 1.0);
    titleWidget->outline = true;
    titleWidget->alignH = Rendering::CENTER;
    AddWidget(listWidget, titleWidget);

    ListH *listHWidget = new ListH();
    listHWidget->minSize.y = 200.0;
    AddWidget(listWidget, listHWidget);

    textWidget = new Text();
    textWidget->string = ToWString("Hahaha look at me! There's so much to say! I don't know what else to do. ¡Hola señor Lopez! ¿Cómo está usted? Estoy muy bien. ¿Y cómo se llama? ありがとうお願いします私はハンバーガー 세계를 향한 대화, 유니코드로 하십시오. 経機速講著述元載説赤問台民。 Лорем ипсум долор сит амет Λορεμ ιπσθμ δολορ σιτ αμετ There once was a man named Chad. He was an incel. What a terrible sight! If only someone was there to teach him the ways of humility! Oh how he would wail and toil how all the girls would pass up a \"nice guy like me\". What a bitch.");
    textWidget->fontIndex = fontIndex;
    textWidget->size.x = 1.0 / 3.0;
    textWidget->alignH = Rendering::JUSTIFY;
    AddWidget(listHWidget, textWidget);

    Text *textWidget2 = new Text();
    textWidget2->string = ToWString("Hey now! You're an all star! Get your shit together!");
    textWidget2->fontIndex = fontIndex;
    textWidget2->size.x = 2.0 / 9.0;
    textWidget2->alignH = Rendering::JUSTIFY;
    AddWidget(listHWidget, textWidget2);
    Text *textWidget3 = new Text();
    textWidget3->string = ToWString("What else is there even to talk about? The whole world is going up in flames! I feel like a floop.");
    textWidget3->fontIndex = fontIndex;
    textWidget3->fontSize = 24.0;
    textWidget3->size.x = 2.0 / 9.0;
    textWidget3->alignH = Rendering::JUSTIFY;
    AddWidget(listHWidget, textWidget3);

    Image *imageWidget = new Image();
    imageWidget->size.x = 2.0 / 9.0;
    // imageWidget->size.x = 128.0;
    // imageWidget->size.y = 128.0;
    // imageWidget->fractionWidth = false;
    // imageWidget->fractionHeight = false;
    imageWidget->texIndex = texIndex;
    AddWidget(listHWidget, imageWidget);

    ListH *listHWidget2 = new ListH();
    listHWidget2->size.y = 64.0;
    listHWidget2->fractionHeight = false;
    listHWidget2->padding = vec2(0.0);
    listHWidget2->selectionDefault = 1;
    AddWidget(listWidget, listHWidget2);

    Button *buttonWidget1 = new Button();
    buttonWidget1->fontIndex = fontIndex;
    buttonWidget1->string = ToWString("Previous");
    buttonWidget1->size.x = 128.0;
    buttonWidget1->fractionWidth = false;
    AddWidget(listHWidget2, buttonWidget1);

    Button *buttonWidget2 = new Button();
    buttonWidget2->fontIndex = fontIndex;
    buttonWidget2->string = ToWString("Select");
    AddWidget(listHWidget2, buttonWidget2);

    Button *buttonWidget3 = new Button();
    buttonWidget3->fontIndex = fontIndex;
    buttonWidget3->string = ToWString("Next");
    buttonWidget3->size.x = 128.0;
    buttonWidget3->fractionWidth = false;
    AddWidget(listHWidget2, buttonWidget3);

    Button *buttonWidget4 = new Button();
    buttonWidget4->fontIndex = fontIndex;
    buttonWidget4->string = ToWString("ぷた");
    buttonWidget4->margin.x = 128.0 + 32.0;
    buttonWidget4->size.y = 32.0;
    buttonWidget4->fractionHeight = false;
    AddWidget(listWidget, buttonWidget4);

    Button *buttonWidget5 = new Button();
    buttonWidget5->fontIndex = fontIndex;
    buttonWidget5->string = ToWString("フィロヴァテンコ");
    buttonWidget5->margin.x = 128.0 + 32.0;
    buttonWidget5->size.y = 32.0;
    buttonWidget5->fractionHeight = false;
    AddWidget(listWidget, buttonWidget5);

    ListV *listVWidget2 = new ListV();
    listVWidget2->selectionDefault = 0;
    listVWidget2->size.y = 0.0;
    listVWidget2->highlight.rgb *= 2.0;
    AddWidget(listWidget, listVWidget2, true);

    const WString strings[] = {
        ToWString("Test1"),
        ToWString("I'm a thing."),
        ToWString("Hey you!")
    };
    for (i32 i = 0; i < 3; i++) {
        Button *buttonWidget6 = new Button();
        buttonWidget6->fontIndex = fontIndex;
        buttonWidget6->string = strings[i];
        buttonWidget6->margin.x = 128.0;
        buttonWidget6->size.y = 32.0;
        buttonWidget6->fractionHeight = false;
        AddWidget(listVWidget2, buttonWidget6);
    }

}

void Gui::EventUpdate(bool buffer, Objects::Manager *objects, Rendering::Manager *rendering) {
    screenWidget.Update(vec2(0.0), true, this, objects, rendering);
    if (objects->input->Pressed(KC_KEY_1)) {
        font->SaveAtlas();
    }
    if (objects->input->Pressed(KC_KEY_2)) {
        (*rendering->fonts)[0].SaveAtlas();
    }
    if (objects->input->Pressed(KC_KEY_UP)) {
        textWidget->fontSize += 1.0;
    }
    if (objects->input->Pressed(KC_KEY_DOWN)) {
        textWidget->fontSize -= 1.0;
    }
}

void Gui::EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer) {
    screenWidget.Draw(rendering, commandBuffer);
}

void Gui::AddWidget(Widget *parent, Widget *newWidget, bool deeper) {
    newWidget->depth = parent->depth + (deeper ? 1 : 0);
    if (newWidget->selectable) {
        parent->selectable = true;
    }
    parent->children.Append(newWidget);
    if (allWidgets.count(newWidget) == 0) {
        allWidgets.emplace(newWidget);
    }
}

//
//      Widget implementations beyond this point
//

Widget::Widget() : children(), margin(8.0), size(1.0), fractionWidth(true), fractionHeight(true), sizeAbsolute(0.0), minSize(0.0), maxSize(-1.0), position(0.0), positionAbsolute(0.0), depth(0), selectable(false) {}

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

void Widget::Update(vec2 pos, bool selected, Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    pos += margin;
    positionAbsolute = pos;
    highlighted = selected;
    for (Widget* child : children) {
        child->Update(pos, selected, gui, objects, rendering);
    }
}

void Widget::Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const {
    for (const Widget* child : children) {
        child->Draw(rendering, commandBuffer);
    }
}

const bool Widget::MouseOver(const Objects::Manager *objects) const {
    const vec2 mouse = vec2((f32)objects->input->cursor.x, (f32)objects->input->cursor.y);
    return mouse.x == median(positionAbsolute.x, mouse.x, positionAbsolute.x + sizeAbsolute.x)
        && mouse.y == median(positionAbsolute.y, mouse.y, positionAbsolute.y + sizeAbsolute.y);
}

Screen::Screen() {
    margin = vec2(0.0);
}

void Screen::Update(vec2 pos, bool selected, Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    UpdateSize(rendering->screenSize);
    Widget::Update(pos, selected, gui, objects, rendering);
}

void Screen::UpdateSize(vec2 container) {
    sizeAbsolute = container - margin * 2.0;
    for (Widget* child : children) {
        child->UpdateSize(sizeAbsolute);
    }
}

List::List() : padding(8.0), color(0.05, 0.05, 0.05, 0.9), highlight(0.05, 0.03, 0.03, 0.9), selection(-2), selectionDefault(-1) {}

bool List::UpdateSelection(bool selected, Gui *gui, Objects::Manager *objects, u8 keyCodeSelect, u8 keyCodeBack, u8 keyCodeIncrement, u8 keyCodeDecrement) {
    highlighted = selected;
    if (selected) {
        if (gui->controlDepth == depth) {
            if (selection >= 0 && selection < children.size && objects->Released(keyCodeSelect)) {
                gui->controlDepth = children[selection]->depth;
            }
            if (objects->Pressed(keyCodeIncrement)) {
                for (selection = max(selection+1, 0); selection < children.size; selection++) {
                    if (children[selection]->selectable) {
                        break;
                    }
                }
                if (selection == children.size) {
                    selection = -1;
                }
            } else if (objects->Pressed(keyCodeDecrement)) {
                if (selection < 0) {
                    selection = children.size - 1;
                } else {
                    --selection;
                }
                for (; selection >= 0; selection--) {
                    if (children[selection]->selectable) {
                        break;
                    }
                }
            }
            if (selection == -2) {
                selection = selectionDefault;
            }
        } else if (gui->controlDepth == depth+1 && objects->Released(keyCodeBack)) {
            gui->controlDepth = depth;
        }
        if (gui->controlDepth > depth) {
            highlighted = false;
        }
    } else {
        selection = -2;
    }
    if (gui->controlDepth == depth && selected) {
        bool reselect = false;
        if (objects->input->cursor != objects->input->cursorPrevious) {
            if (MouseOver(objects)) {
                reselect = true;
            }
            selection = -1;
        }
        return reselect;
    }
    return false;
}

void List::Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const {
    if (color.a > 0.0) {
        // const vec2 screenSizeFactor = vec2(2.0) / rendering->screenSize;
        rendering->BindPipeline2D(commandBuffer);
        rendering->DrawQuad(commandBuffer, Rendering::texBlank, highlighted ? highlight : color, positionAbsolute, sizeAbsolute);
    }
    rendering->PushScissor(commandBuffer,
        vec2i((i32)positionAbsolute.x+margin.x+padding.x, (i32)positionAbsolute.y+margin.y+padding.y),
        vec2i((i32)(sizeAbsolute.x+positionAbsolute.x-margin.x-padding.x), (i32)(sizeAbsolute.y+positionAbsolute.y-margin.y-padding.y)));
    Widget::Draw(rendering, commandBuffer);
    rendering->PopScissor(commandBuffer);
}

void ListV::UpdateSize(vec2 container) {
    sizeAbsolute = vec2(0.0);
    if (size.x > 0.0) {
        sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0) : size.x;
    } else {
        sizeAbsolute.x = padding.x * 2.0;
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0) : size.y;
    } else {
        sizeAbsolute.y = padding.y * 2.0;
    }
    LimitSize();
    vec2 sizeForInheritance = sizeAbsolute - padding * 2.0;
    for (Widget* child : children) {
        if (child->size.y == 0.0) {
            child->UpdateSize(sizeForInheritance);
            sizeForInheritance.y -= child->GetSize().y;
        } else {
            if (!child->fractionHeight) {
                sizeForInheritance.y -= child->size.y + child->margin.y * 2.0;
            }
        }
    }
    for (Widget* child : children) {
        child->UpdateSize(sizeForInheritance);
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

void ListV::Update(vec2 pos, bool selected, Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    pos += margin;
    positionAbsolute = pos;
    const bool mouseSelect = UpdateSelection(selected, gui, objects, KC_GP_BTN_A, KC_GP_BTN_B, KC_GP_AXIS_LS_DOWN, KC_GP_AXIS_LS_UP);
    pos += padding;
    if (mouseSelect) {
        f32 childY = pos.y;
        for (selection = 0; selection < children.size; selection++) {
            Widget *child = children[selection];
            if (!child->selectable) {
                childY += child->GetSize().y;
                continue;
            }
            child->positionAbsolute.x = pos.x + child->margin.x;
            child->positionAbsolute.y = childY + child->margin.y;
            if (child->MouseOver(objects)) {
                break;
            }
            childY += child->GetSize().y;
        }
        if (selection == children.size) {
            selection = -1;
        }
    }
    for (i32 i = 0; i < children.size; i++) {
        Widget *child = children[i];
        child->Update(pos, selected && i == selection, gui, objects, rendering);
        pos.y += child->GetSize().y;
    }
}

ListH::ListH() {
    color = vec4(0.1, 0.1, 0.1, 0.9);
    highlight = vec4(0.1, 0.06, 0.06, 0.9);
}

void ListH::UpdateSize(vec2 container) {
    sizeAbsolute = vec2(0.0);
    if (size.x > 0.0) {
        sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0) : size.x;
    } else {
        sizeAbsolute.x = padding.x * 2.0;
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0) : size.y;
    } else {
        sizeAbsolute.y = padding.y * 2.0;
    }
    LimitSize();
    vec2 sizeForInheritance = sizeAbsolute - padding * 2.0;
    for (Widget* child : children) {
        if (child->size.x == 0.0) {
            child->UpdateSize(sizeForInheritance);
            sizeForInheritance.x -= child->GetSize().x;
        } else {
            if (!child->fractionWidth) {
                sizeForInheritance.x -= child->size.x + child->margin.x * 2.0;
            }
        }
    }
    for (Widget* child : children) {
        child->UpdateSize(sizeForInheritance);
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

void ListH::Update(vec2 pos, bool selected, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    pos += margin;
    positionAbsolute = pos;
    const bool mouseSelect = UpdateSelection(selected, gui, objects, KC_GP_BTN_A, KC_GP_BTN_B, KC_GP_AXIS_LS_RIGHT, KC_GP_AXIS_LS_LEFT);
    pos += padding;
    if (mouseSelect) {
        f32 childX = pos.x;
        for (selection = 0; selection < children.size; selection++) {
            Widget *child = children[selection];
            if (child->selectable) {
                child->positionAbsolute.x = childX + child->margin.x;
                child->positionAbsolute.y = pos.y + child->margin.y;
                if (child->MouseOver(objects)) {
                    break;
                }
            }
            childX += child->GetSize().x;
        }
        if (selection == children.size) {
            selection = -1;
        }
    }
    for (i32 i = 0; i < children.size; i++) {
        Widget *child = children[i];
        child->Update(pos, selected && i == selection, gui, objects, rendering);
        pos.x += child->GetSize().x;
    }
}

Text::Text() : rendering(nullptr), stringFormatted(), string(), fontSize(32.0), fontIndex(1), alignH(Rendering::LEFT), alignV(Rendering::TOP), color(1.0), colorOutline(0.0, 0.0, 0.0, 1.0), outline(false) {
    size.y = 0.0;
}

void Text::UpdateSize(vec2 container) {
    if (size.x > 0.0) {
        sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0) : size.x;
    } else {
        sizeAbsolute.x = rendering->StringWidth(stringFormatted, fontIndex) * fontSize;
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0) : size.y;
    } else {
        sizeAbsolute.y = Rendering::StringHeight(stringFormatted) * fontSize;
    }
    LimitSize();
}

void Text::Update(vec2 pos, bool selected, Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    this->rendering = rendering;
    stringFormatted = rendering->StringAddNewlines(string, fontIndex, sizeAbsolute.x / fontSize);
    Widget::Update(pos, selected, gui, objects, rendering);
}

void Text::Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const {
    if (sizeAbsolute.x != 0.0 && sizeAbsolute.y != 0.0) {
        rendering->PushScissor(commandBuffer,
            vec2i((i32)positionAbsolute.x, (i32)positionAbsolute.y),
            vec2i((i32)(sizeAbsolute.x+positionAbsolute.x), (i32)(sizeAbsolute.y+positionAbsolute.y)));
    }
    rendering->BindPipelineFont(commandBuffer);
    vec2 drawPos = positionAbsolute;
    vec2 scale = vec2(fontSize);
    f32 maxWidth = sizeAbsolute.x;
    if (alignH == Rendering::CENTER) {
        drawPos.x += maxWidth * 0.5;
    } else if (alignH == Rendering::RIGHT) {
        drawPos.x += maxWidth;
    }
    if (alignV == Rendering::CENTER) {
        drawPos.y += sizeAbsolute.y * 0.5;
    } else if (alignV == Rendering::BOTTOM) {
        drawPos.y += sizeAbsolute.y;
    }
    if (outline) {
        rendering->DrawText(commandBuffer, stringFormatted, fontIndex, colorOutline, drawPos, scale, alignH, alignV, maxWidth, 0.1, 0.3);
    }
    rendering->DrawText(commandBuffer, stringFormatted, fontIndex, color, drawPos, scale, alignH, alignV, maxWidth);
    if (sizeAbsolute.x != 0.0 && sizeAbsolute.y != 0.0) {
        rendering->PopScissor(commandBuffer);
    }
}

Image::Image() : texIndex(0) {}

void Image::UpdateSize(vec2 container) {
    sizeAbsolute = vec2(0.0);
    if (size.x > 0.0) {
        sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0) : size.x;
    } else {
        sizeAbsolute.x = 0.0;
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0) : size.y;
    } else {
        sizeAbsolute.y = 0.0;
    }
}

void Image::Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const {
    rendering->BindPipeline2D(commandBuffer);
    rendering->DrawQuad(commandBuffer, texIndex, vec4(1.0), positionAbsolute, sizeAbsolute);
}

Button::Button() : string(), colorBG(0.15, 0.15, 0.15, 0.9), highlightBG(0.2, 0.6, 0.5, 0.9), colorText(1.0), highlightText(1.0), fontIndex(1), fontSize(24.0), mouseover(false), state() {
    state.canRepeat = false;
    selectable = true;
}

void Button::UpdateSize(vec2 container) {
    if (size.x > 0.0) {
        sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0) : size.x;
    } else {
        sizeAbsolute.x = 0.0;
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0) : size.y;
    } else {
        sizeAbsolute.y = 0.0;
    }
    LimitSize();
}

void Button::Update(vec2 pos, bool selected, struct Gui *gui, Objects::Manager *objects, Rendering::Manager *rendering) {
    Widget::Update(pos, selected, gui, objects, rendering);
    mouseover = MouseOver(objects);
    state.Tick(0.0);
    if (gui->controlDepth != depth) {
        highlighted = false;
    }
    if (mouseover) {
        highlighted = true;
        if (objects->Pressed(KC_MOUSE_LEFT)) {
            state.Press();
        }
        if (objects->Released(KC_MOUSE_LEFT)) {
            state.Release();
        }
    }
    if (gui->controlDepth == depth) {
        if (selected) {
            if (objects->Pressed(KC_GP_BTN_A)) {
                state.Press();
            }
            if (objects->Released(KC_GP_BTN_A)) {
                state.Release();
            }
        } else if (!mouseover) {
            state.Set(false, false, false);
        }
    }
}

void Button::Draw(Rendering::Manager *rendering, VkCommandBuffer commandBuffer) const {
    if (sizeAbsolute.x != 0.0 && sizeAbsolute.y != 0.0) {
        rendering->PushScissor(commandBuffer,
            vec2i((i32)positionAbsolute.x, (i32)positionAbsolute.y),
            vec2i((i32)(sizeAbsolute.x+positionAbsolute.x), (i32)(sizeAbsolute.y+positionAbsolute.y)));
    }
    f32 scale;
    if (state.Down()) {
        scale = 0.9;
    } else {
        scale = 1.0;
    }
    vec2 drawPos = positionAbsolute + sizeAbsolute * 0.5;
    rendering->BindPipeline2D(commandBuffer);
    rendering->DrawQuad(commandBuffer, 1, highlighted ? highlightBG : colorBG, drawPos, sizeAbsolute * scale, vec2(0.5));
    rendering->BindPipelineFont(commandBuffer);
    rendering->DrawText(commandBuffer, string, fontIndex,  highlighted ? highlightText : colorText, drawPos, vec2(fontSize * scale), Rendering::CENTER, Rendering::CENTER, sizeAbsolute.x);
    if (sizeAbsolute.x != 0.0 && sizeAbsolute.y != 0.0) {
        rendering->PopScissor(commandBuffer);
    }
}

} // namespace Int
