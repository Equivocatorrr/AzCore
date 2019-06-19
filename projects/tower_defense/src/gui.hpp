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

    ~Gui() = default;

    void EventAssetInit(Assets::Manager *assets);
    void EventAssetAcquire(Assets::Manager *assets);
    // void EventUpdate(bool buffer, Manager *objects);
    void EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer);
};

#endif // GUI_HPP
