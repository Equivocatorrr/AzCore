/*
    File: objects.cpp
    Author: Philip Haynes
*/

#include "objects.hpp"
#include "globals.hpp"

namespace Objects {

void Object::EventSync() { readyForDraw = false; }
void Object::EventUpdate() { readyForDraw = true; }
void Object::EventDraw(Array<Rendering::DrawingContext> &contexts) {}
void Object::EventInitialize() {}

void Manager::RenderCallback(void *userdata, Rendering::Manager *rendering, Array<Rendering::DrawingContext> &contexts) {
    ((Manager*)userdata)->Draw(contexts);
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

void Manager::Sync() {
    buffer = !buffer;
    if (!paused) {
        globals->objects.simulationRate = min(1.0, globals->objects.simulationRate + globals->objects.timestep);
    } else {
        globals->objects.simulationRate = max(0.0, globals->objects.simulationRate - globals->objects.timestep);
    }
    if (globals->rawInput.AnyGP.Pressed()) {
        globals->gamepad = &globals->rawInput.gamepads[globals->rawInput.AnyGPIndex];
    }
    for (Object* object : objects) {
        object->EventSync();
    }
}

void Manager::Update() {
    for (Object* object : objects) {
        object->EventUpdate();
    }
}

// void DrawThreadProc(Object *object, Array<Rendering::DrawingContext> *contexts) {
//     object->EventDraw(*contexts);
// }

void Manager::Draw(Array<Rendering::DrawingContext>& contexts) {
    for (Object *object : objects) {
        while (!object->readyForDraw) { std::this_thread::sleep_for(Nanoseconds(1000)); }
        object->EventDraw(contexts);
    }
}

bool Manager::Pressed(u8 keyCode) const {
    if (KeyCodeIsGamepad(keyCode)) {
        if (globals->gamepad == nullptr) {
            return false;
        }
        return globals->gamepad->Pressed(keyCode);
    } else {
        return globals->input.Pressed(keyCode);
    }
}

bool Manager::Down(u8 keyCode) const {
    if (KeyCodeIsGamepad(keyCode)) {
        if (globals->gamepad == nullptr) {
            return false;
        }
        return globals->gamepad->Down(keyCode);
    } else {
        return globals->input.Down(keyCode);
    }
}

bool Manager::Released(u8 keyCode) const {
    if (KeyCodeIsGamepad(keyCode)) {
        if (globals->gamepad == nullptr) {
            return false;
        }
        return globals->gamepad->Released(keyCode);
    } else {
        return globals->input.Released(keyCode);
    }
}

}
