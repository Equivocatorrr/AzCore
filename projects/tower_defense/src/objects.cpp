/*
    File: objects.cpp
    Author: Philip Haynes
*/

#include "objects.hpp"
#include "globals.hpp"

namespace Objects {

void Object::EventUpdate() {}
void Object::EventDraw(VkCommandBuffer commandBuffer) {}
void Object::EventInitialize() {}

void Manager::RenderCallback(void *userdata, Rendering::Manager *rendering, Array<VkCommandBuffer> &commandBuffers) {
    ((Manager*)userdata)->Draw(commandBuffers);
}

void Manager::RegisterDrawing(Rendering::Manager *rendering) {
    rendering->AddRenderCallback(RenderCallback, this);
}

void Manager::GetAssets() {
    for (Object* object : objects) {
        object->EventAssetInit();
    }
}

void Manager::UseAssets() {
    for (Object* object : objects) {
        object->EventAssetAcquire();
    }
}

void Manager::CallInitialize() {
    for (Object* object : objects) {
        object->EventInitialize();
    }
}

void Manager::Update() {
    buffer = !buffer;
    if (globals.rawInput.AnyGP.Pressed()) {
        globals.gamepad = &globals.rawInput.gamepads[globals.rawInput.AnyGPIndex];
    }

    for (Object* object : objects) {
        object->EventUpdate();
    }
}

void Manager::Draw(Array<VkCommandBuffer>& commandBuffers) {
    i32 concurrency = commandBuffers.size;
    for (i32 i = 0; i < objects.size; i+=concurrency) {
        for (i32 j = 0; j < concurrency; j++) {
            if (i+j >= objects.size) {
                break;
            }
            objects[i+j]->EventDraw(commandBuffers[j]);
        }
    }
}

bool Manager::Pressed(u8 keyCode) const {
    if (KeyCodeIsGamepad(keyCode)) {
        if (globals.gamepad == nullptr) {
            return false;
        }
        return globals.gamepad->Pressed(keyCode);
    } else {
        return globals.input.Pressed(keyCode);
    }
}

bool Manager::Down(u8 keyCode) const {
    if (KeyCodeIsGamepad(keyCode)) {
        if (globals.gamepad == nullptr) {
            return false;
        }
        return globals.gamepad->Down(keyCode);
    } else {
        return globals.input.Down(keyCode);
    }
}

bool Manager::Released(u8 keyCode) const {
    if (KeyCodeIsGamepad(keyCode)) {
        if (globals.gamepad == nullptr) {
            return false;
        }
        return globals.gamepad->Released(keyCode);
    } else {
        return globals.input.Released(keyCode);
    }
}

}
