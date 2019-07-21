/*
    File: gui.hpp
    Author: Philip Haynes
*/

#ifndef GUI_HPP
#define GUI_HPP

#include "objects.hpp"
#include "assets.hpp"

struct Gui : public Objects::Object {
    i32 fontIndex;
    Assets::Font *font;
    vec2 pos{-1.0, -0.5};
    vec2 vel{0.25, 0.2};
    f32 size = 0.15;
    f32 dir = -1.0;
    const WString text = ToWString("Hahaha look at me!\n¡Hola señor Lopez!¿?èÎ\nありがとうお願いします私はハンバーガー\n세계를 향한 대화, 유니코드로 하십시오.\n経機速講著述元載説赤問台民。\nЛорем ипсум долор сит амет\nΛορεμ ιπσθμ δολορ σιτ αμετ");

    ~Gui() = default;

    void EventAssetInit(Assets::Manager *assets);
    void EventAssetAcquire(Assets::Manager *assets);
    void EventUpdate(bool buffer, Objects::Manager *objects, Rendering::Manager *rendering);
    void EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer);
};

#endif // GUI_HPP
