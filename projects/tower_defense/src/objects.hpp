/*
    File: objects.hpp
    Author: Philip Haynes
    Defines an abstract outline for enumerable objects.
*/

#ifndef OBJECTS_HPP
#define OBJECTS_HPP

#include "AzCore/memory.hpp"
#include "AzCore/io.hpp"
#include "AzCore/vk.hpp"

namespace Assets {
struct Manager;
}
namespace Rendering {
struct Manager;
}

namespace Objects {

struct Manager;

// The basis for any object registered with Manager
struct Object {
    virtual ~Object() = default;

    virtual void EventAssetInit(Assets::Manager *assets) = 0;
    virtual void EventAssetAcquire(Assets::Manager *assets) = 0;
    virtual void EventInitialize(Manager *objects, Rendering::Manager *rendering);
    virtual void EventUpdate(bool buffer, Manager *objects, Rendering::Manager *rendering);
    virtual void EventDraw(bool buffer, Rendering::Manager *rendering, VkCommandBuffer commandBuffer);
};

struct Manager {
    // buffer swaps every frame. Used for lockless multithreading.
    Array<Object*> objects;
    io::Input *input = nullptr;
    io::Window *window = nullptr;
    bool buffer = false;
    f32 timestep = 1.0/60.0;

    ~Manager();
    static void RenderCallback(void *userdata, Rendering::Manager *rendering, Array<VkCommandBuffer>& commandBuffers);

    // The first thing you do with the manager
    inline void Register(Object *object) {
        objects.Append(object);
    }
    // Registers the rendering callbacks
    void RegisterDrawing(Rendering::Manager *rendering);
    // Calls EventAssetInit for every type of object.
    void GetAssets(Assets::Manager *assets);
    // Calls EventAssetAcquire for every type of object.
    void UseAssets(Assets::Manager *assets);
    // Calls EventInitialize
    void CallInitialize(Rendering::Manager *rendering);
    // Calls different Update events.
    void Update(f32 timestep, Rendering::Manager *rendering);
    // Calls different Draw events.
    void Draw(Rendering::Manager *rendering, Array<VkCommandBuffer>& commandBuffers);
};

}


#endif // OBJECTS_HPP
