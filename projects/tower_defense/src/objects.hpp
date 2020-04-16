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
#include "rendering.hpp"

namespace Assets {
struct Manager;
}
namespace Rendering {
struct Manager;
}

namespace Objects {

using namespace AzCore;

struct Manager;

// The basis for any object registered with Manager
struct Object {
    bool readyForDraw = false;
    virtual ~Object() = default;

    virtual void EventAssetInit() = 0;
    virtual void EventAssetAcquire() = 0;
    virtual void EventInitialize();
    virtual void EventSync();
    virtual void EventUpdate();
    virtual void EventDraw(Array<Rendering::DrawingContext> &contexts);
};

struct Manager {
    // buffer swaps every frame. Used for lockless multithreading.
    Array<Object*> objects;
    bool buffer = false;
    f32 timestep = 1.0f/60.0f;
    f32 simulationRate = 1.0f;
    bool paused = false;

    static void RenderCallback(void *userdata, Rendering::Manager *rendering, Array<Rendering::DrawingContext>& drawingContexts);

    // The first thing you do with the manager
    inline void Register(Object *object) {
        objects.Append(object);
    }
    // Registers the rendering callbacks
    void RegisterDrawing(Rendering::Manager *rendering);
    // Calls EventAssetInit for every type of object.
    void GetAssets();
    // Calls EventAssetAcquire for every type of object.
    void UseAssets();
    // Calls EventInitialize
    void CallInitialize();
    // Calls different Sync events.
    void Sync();
    // Calls different Update events.
    void Update();
    // Calls different Draw events.
    void Draw(Array<Rendering::DrawingContext>& drawingContexts);

    bool Pressed(u8 keyCode);
    bool Down(u8 keyCode);
    bool Released(u8 keyCode);
    io::ButtonState* GetButtonState(u8 keyCode);
};

}


#endif // OBJECTS_HPP
