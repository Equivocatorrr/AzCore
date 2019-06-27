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
    vec2 pos{0.0, 0.0};
    vec2 vel{0.5, 0.25};
    f32 size = 0.02;
    f32 dir = 1.0;

    ~Gui() = default;

    void EventAssetInit(Assets::Manager *assets);
    void EventAssetAcquire(Assets::Manager *assets);
    void EventUpdate(bool buffer, Objects::Manager *objects);
    void EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer);
};

#endif // GUI_HPP
