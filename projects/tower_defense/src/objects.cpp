/*
    File: objects.cpp
    Author: Philip Haynes
*/

#include "objects.hpp"
#include "rendering.hpp"

namespace Objects {

void Object::EventUpdate(bool buffer, Manager *objects, Rendering::Manager *rendering) {}
void Object::EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer) {}
void Object::EventInitialize(Manager *objects, Rendering::Manager *rendering) {}

Manager::~Manager() {
    for (Object* object : objects) {
        delete object;
    }
}

void Manager::RenderCallback(void *userdata, Rendering::Manager *rendering, Array<VkCommandBuffer> &commandBuffers) {
    ((Manager*)userdata)->Draw(rendering, commandBuffers);
}

void Manager::RegisterDrawing(Rendering::Manager *rendering) {
    rendering->AddRenderCallback(RenderCallback, this);
}

void Manager::GetAssets(Assets::Manager *assets) {
    for (Object* object : objects) {
        object->EventAssetInit(assets);
    }
}

void Manager::UseAssets(Assets::Manager *assets) {
    for (Object* object : objects) {
        object->EventAssetAcquire(assets);
    }
}

void Manager::CallInitialize(Rendering::Manager *rendering) {
    for (Object* object : objects) {
        object->EventInitialize(this, rendering);
    }
}

void Manager::Update(f32 timestep, Rendering::Manager *rendering) {
    buffer = !buffer;
    this->timestep = timestep;

    for (Object* object : objects) {
        object->EventUpdate(buffer, this, rendering);
    }
}

void Manager::Draw(Rendering::Manager *rendering, Array<VkCommandBuffer>& commandBuffers) {
    i32 concurrency = commandBuffers.size;
    for (i32 i = 0; i < objects.size; i+=concurrency) {
        for (i32 j = 0; j < concurrency; j++) {
            if (i+j >= objects.size) {
                break;
            }
            objects[i+j]->EventDraw(!buffer, rendering, commandBuffers[j]);
        }
    }
}

}
