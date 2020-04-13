/*
    File: entities.cpp
    Author: Philip Haynes
*/

#include "entities.hpp"
#include "globals.hpp"
#include "AzCore/Thread.hpp"

namespace Entities {

const char* towerStrings[TOWER_MAX_RANGE+1] = {
    "Gun",
    "Shotgun",
    "Fan",
    "Shocker",
    "Gauss",
    "Flak"
};
const i32 towerCosts[TOWER_MAX_RANGE+1] = {
    2000,
    3000,
    5000,
    15000,
    25000,
    50000
};
const bool towerHasPriority[TOWER_MAX_RANGE+1] = {
    true,
    true,
    false,
    false,
    true,
    true
};
// data[0] is Range, data[1] is Firerate, data[2] is Accuracy, data[3] is Damage, data[4] is Multishot
const TowerUpgradeables towerUpgradeables[TOWER_MAX_RANGE+1] = {
    {true, true, true, true, true},
    {true, true, true, true, true},
    {false, false, false, true, false},
    {true, true, false, true, false},
    {true, true, false, true, true},
    {true, true, true, true, true}
};
const char *towerDescriptions[TOWER_MAX_RANGE+1] = {
    "GunDescription",
    "ShotgunDescription",
    "FanDescription",
    "ShockerDescription",
    "GaussDescription",
    "FlakDescription"
};
const char *Tower::priorityStrings[6] =  {
    "Nearest",
    "Furthest",
    "Weakest",
    "Strongest",
    "Newest",
    "Oldest"
};

const Tower towerGunTemplate = Tower(
    BOX,                                        // CollisionType
    {vec2(-20.0), vec2(20.0)},                  // PhysicalBasis
    CIRCLE,                                     // fieldCollisionType
    {vec2(0.0), 320.0},                         // fieldPhysicalBasis
    TOWER_GUN,                                  // TowerType
    320.0,                                      // range
    0.25,                                       // shootInterval
    2.7,                                        // bulletSpread (degrees)
    1,                                          // bulletCount
    24,                                         // damage
    800.0,                                      // bulletSpeed
    50.0,                                       // bulletSpeedVariability
    0,                                          // bulletExplosionDamage
    0.0,                                        // bulletExplosionRange
    vec4(0.1, 0.5, 1.0, 1.0)                    // color
);

const Tower towerShotgunTemplate = Tower(
    BOX,                                        // CollisionType
    {vec2(-16.0), vec2(16.0)},                  // PhysicalBasis
    CIRCLE,                                     // fieldCollisionType
    {vec2(0.0), 200.0},                         // fieldPhysicalBasis
    TOWER_SHOTGUN,                              // TowerType
    200.0,                                      // range
    1.0,                                        // shootInterval
    12.0,                                       // bulletSpread (degrees)
    15,                                         // bulletCount
    18,                                         // damage
    900.0,                                      // bulletSpeed
    200.0,                                      // bulletSpeedVariability
    0,                                          // bulletExplosionDamage
    0.0,                                        // bulletExplosionRange
    vec4(0.1, 1.0, 0.5, 1.0)                    // color
);

const Tower towerFanTemplate = Tower(
    BOX,                                        // CollisionType
    {vec2(-10.0, -32.0), vec2(10.0, 32.0)},     // PhysicalBasis
    BOX,                                        // fieldCollisionType
    {vec2(-50.0, -40.0), vec2(300.0, 40.0)},    // fieldPhysicalBasis
    TOWER_FAN,                                  // TowerType
    300.0,                                      // range
    0.1,                                        // shootInterval
    10.0,                                       // bulletSpread (degrees)
    2,                                          // bulletCount
    10,                                         // damage
    800.0,                                      // bulletSpeed
    200.0,                                      // bulletSpeedVariability
    0,                                          // bulletExplosionDamage
    0.0,                                        // bulletExplosionRange
    vec4(0.5, 1.0, 0.1, 1.0)                    // color
);

const Tower towerGaussTemplate = Tower(
    BOX,                                        // CollisionType
    {vec2(-32.0), vec2(32.0)},                  // PhysicalBasis
    CIRCLE,                                     // fieldCollisionType
    {vec2(0.0, 0.0), 480.0},                    // fieldPhysicalBasis
    TOWER_GAUSS,                                // TowerType
    400.0,                                      // range
    2.0,                                        // shootInterval
    4.8,                                        // bulletSpread (degrees)
    1,                                          // bulletCount
    1200,                                       // damage
    2000.0,                                     // bulletSpeed
    0.0,                                        // bulletSpeedVariability
    0,                                          // bulletExplosionDamage
    0.0,                                        // bulletExplosionRange
    vec4(0.1, 1.0, 0.8, 1.0)                    // color
);

const Tower towerShockerTemplate = Tower(
    CIRCLE,                                     // CollisionType
    {vec2(0.0), 16.0},                          // PhysicalBasis
    CIRCLE,                                     // fieldCollisionType
    {vec2(0.0, 0.0), 120.0},                    // fieldPhysicalBasis
    TOWER_SHOCKWAVE,                            // TowerType
    120.0,                                      // range
    1.2,                                        // shootInterval
    0.0,                                        // bulletSpread (degrees)
    1,                                          // bulletCount
    60,                                         // damage
    1.0,                                        // bulletSpeed
    0.0,                                        // bulletSpeedVariability
    0,                                          // bulletExplosionDamage
    0.0,                                        // bulletExplosionRange
    vec4(1.0, 0.3, 0.1, 1.0)                    // color
);

const Tower towerFlakTemplate = Tower(
    CIRCLE,                                     // CollisionType
    {vec2(0.0), 32.0},                          // PhysicalBasis
    CIRCLE,                                     // fieldCollisionType
    {vec2(0.0), 400.0},                         // fieldPhysicalBasis
    TOWER_FLAK,                                 // TowerType
    400.0,                                      // range
    1.8,                                        // shootInterval
    6.0,                                        // bulletSpread (degrees)
    5,                                          // bulletCount
    25,                                         // damage
    500.0,                                      // bulletSpeed
    100.0,                                      // bulletSpeedVariability
    25,                                         // bulletExplosionDamage
    80.0,                                       // bulletExplosionRange
    vec4(1.0, 0.0, 0.8, 1.0)                    // color
);

void Manager::EventAssetInit() {
    globals->assets.QueueFile("Money Cursed.ogg");
    globals->assets.QueueFile("Segment 1.ogg", Assets::STREAM);
}

void Manager::EventAssetAcquire() {
    sndMoney.Create("Money Cursed.ogg");
    sndMoney.SetGain(0.5);
    if (!streamSegment1.Create("Segment 1.ogg")) {
        std::cout << "Failed to create stream for \"Segment 1.ogg\": " << Sound::error.data << std::endl;
    }
}

void Manager::EventInitialize() {
    towers.granularity = 5;
    enemies.granularity = 25;
    bullets.granularity = 50;
    winds.granularity = 50;
    explosions.granularity = 10;
    basePhysical.type = CIRCLE;
    basePhysical.basis.circle.c = 0.0;
    basePhysical.basis.circle.r = 128.0;
    basePhysical.pos = 0.0;
    CreateSpawn();
    camPos = enemySpawns[0].pos * 0.5;
    camZoom = min(globals->rendering.screenSize.x, globals->rendering.screenSize.y) / 1500.0;
    HandleMusicLoops(1);
    streamSegment1.Play();
}

inline void Manager::HandleGamepadCamera() {
    vec2 screenBorder = (vec2(globals->window.width, globals->window.height) - vec2(50.0 * globals->gui.scale)) / 2.0 / camZoom;
    if (CursorVisible() || placeMode) {
        vec2 mouseMove = globals->gamepad->axis.vec.RS;
        f32 mag = abs(mouseMove);
        mouseMove *= sqrt(mag);
        mouseMove *= globals->objects.timestep * 800.0 / camZoom;
        mouse += mouseMove;
        if (mouseMove != vec2(0.0)) {
            if (mouse.x < camPos.x-screenBorder.x || mouse.x > camPos.x+screenBorder.x) {
                camPos.x += mouseMove.x;
            }
            if (mouse.y < camPos.y-screenBorder.y || mouse.y > camPos.y+screenBorder.y) {
                camPos.y += mouseMove.y;
            }
        }
    }

    if (!focusMenu && selectedTower == -1) {
        vec2 camMove = globals->gamepad->axis.vec.LS;
        f32 mag = abs(camMove);
        camMove *= sqrt(mag);
        camMove *= globals->objects.timestep * 800.0 / camZoom;
        camPos += camMove;
        if (camMove != vec2(0.0)) {
            if (mouse.x < camPos.x-screenBorder.x || mouse.x > camPos.x+screenBorder.x) {
                mouse.x += camMove.x;
            }
            if (mouse.y < camPos.y-screenBorder.y || mouse.y > camPos.y+screenBorder.y) {
                mouse.y += camMove.y;
            }
        }
    }
    f32 zoomMove = globals->gamepad->axis.vec.RT - globals->gamepad->axis.vec.LT;
    zoomMove *= globals->objects.timestep;
    if (zoomMove > 0) {
        screenBorder *= camZoom;
        camZoom *= 1.0 + zoomMove;
        screenBorder /= camZoom;
        mouse.x = median(camPos.x-screenBorder.x, mouse.x, camPos.x+screenBorder.x);
        mouse.y = median(camPos.y-screenBorder.y, mouse.y, camPos.y+screenBorder.y);
    } else {
        screenBorder *= camZoom;
        camZoom /= 1.0 - zoomMove;
        screenBorder /= camZoom;
        mouse.x = median(camPos.x-screenBorder.x, mouse.x, camPos.x+screenBorder.x);
        mouse.y = median(camPos.y-screenBorder.y, mouse.y, camPos.y+screenBorder.y);
    }
}

inline void Manager::HandleMouseCamera() {
    if (globals->gui.mouseoverDepth > 0) {
        return;
    }
    if (globals->objects.Pressed(KC_MOUSE_SCROLLUP)) {
        camZoom *= 1.1;
    } else if (globals->objects.Pressed(KC_MOUSE_SCROLLDOWN)) {
        camZoom /= 1.1;
    }
    if (globals->objects.Down(KC_MOUSE_LEFT)) {
        vec2 move = vec2(globals->input.cursor - globals->input.cursorPrevious) / camZoom;
        camPos -= move;
    }
}

bool TypedCode(String code) {
    if (code.size > globals->input.typingString.size) return false;
    Range<char> end = globals->input.typingString.GetRange(globals->input.typingString.size-code.size, code.size);
    if (code == end) {
        globals->input.typingString.Clear();
        return true;
    }
    return false;
}

inline void Manager::HandleUI() {
    if (globals->gui.usingMouse) {
        HandleMouseCamera();
        HandleMouseUI();
    } else {
        HandleGamepadCamera();
        HandleGamepadUI();
    }
    if (TypedCode("money")) {
        money += 50000;
        sndMoney.Play();
    }
    for (i32 i = 0; i <= TOWER_MAX_RANGE; i++) {
        if (globals->gui.playMenu.towerButtons[i]->state.Released()) {
            placeMode = true;
            focusMenu = false;
            selectedTower = -1;
            towerType = TowerType(i);
        }
    }
    if (globals->gui.playMenu.buttonStartWave->state.Released()) {
        if (!waveActive) {
            wave++;
            HandleMusicLoops(wave);
            f64 factor = pow((f64)1.2, (f64)(wave+3));
            hitpointsPerSecond = (f64)((i64)(factor * 5.0d) * 100);
            hitpointsLeft += hitpointsPerSecond;
            // Average wave length is wave+7 seconds
            hitpointsPerSecond /= wave+7;
            globals->objects.paused = false;
            waveActive = true;
            globals->gui.playMenu.buttonStartWave->string = globals->ReadLocale("Pause");
        } else {
            if (globals->objects.paused) {
                globals->gui.playMenu.buttonStartWave->string = globals->ReadLocale("Pause");
            } else {
                globals->gui.playMenu.buttonStartWave->string = globals->ReadLocale("Resume");
            }
            globals->objects.paused = !globals->objects.paused;
        }
    }
}

inline void Manager::HandleGamepadUI() {
    if (globals->objects.Pressed(KC_GP_BTN_X) && globals->gui.controlDepth == globals->gui.playMenu.list->depth) {
        focusMenu = !focusMenu;
        placeMode = false;
    }
    if (!placeMode) {
        if (globals->objects.Released(KC_GP_BTN_A) && !focusMenu && selectedTower == -1) {
            for (i32 i = 0; i < towers.size; i++) {
                if (towers[i].id.generation < 0) continue;
                if (towers[i].physical.MouseOver()) {
                    io::ButtonState *state = globals->objects.GetButtonState(KC_GP_BTN_A);
                    if (state) state->state = 0;
                    selectedTower = towers[i].id;
                    globals->gui.playMenu.upgradesMenu.towerPriority->choice = (i32)towers[i].priority;
                    break;
                }
            }
        }
        if (selectedTower != -1 && globals->objects.Pressed(KC_GP_BTN_B)) {
            selectedTower = -1;
        }
    } else { // placeMode == true
        if (globals->objects.Pressed(KC_GP_BTN_B)) {
            placeMode = false;
            focusMenu = true;
        }
        const Degrees32 increment30(30.0);
        const Degrees32 increment5(5.0);
        if (globals->objects.Pressed(KC_GP_AXIS_H0_LEFT)) {
            placingAngle += increment5;
        } else if (globals->objects.Pressed(KC_GP_AXIS_H0_RIGHT)) {
            placingAngle += -increment5;
        }
        if (globals->objects.Pressed(KC_GP_BTN_TL)) {
            placingAngle += increment30;
        } else if (globals->objects.Pressed(KC_GP_BTN_TR)) {
            placingAngle += -increment30;
        }
        HandleTowerPlacement(KC_GP_BTN_A);
    }
}

inline void Manager::HandleMouseUI() {
    if (globals->gui.playMenu.list->MouseOver()) {
        focusMenu = true;
        if (globals->objects.Pressed(KC_MOUSE_LEFT)) {
            placeMode = false;
            selectedTower = -1;
        }
    } else {
        focusMenu = false;
    }
    if (globals->gui.mouseoverDepth > 0) {
        return;
    }
    if (!placeMode) {
        if (globals->objects.Pressed(KC_MOUSE_LEFT)) {
            selectedTower = -1;
            for (i32 i = 0; i < towers.size; i++) {
                if (towers[i].id.generation < 0) continue;
                if (towers[i].physical.MouseOver()) {
                    selectedTower = towers[i].id;
                    globals->gui.playMenu.upgradesMenu.towerPriority->choice = (i32)towers[i].priority;
                    break;
                }
            }
        }
    } else { // placeMode == true
        const Degrees32 increment30(30.0);
        const Degrees32 increment5(5.0);
        Degrees32 increment = increment30;
        if (globals->objects.Down(KC_KEY_LEFTSHIFT) || globals->objects.Down(KC_KEY_RIGHTSHIFT)) {
            increment = increment5;
        }
        if (globals->objects.Pressed(KC_KEY_LEFT)) {
            placingAngle += increment;
        } else if (globals->objects.Pressed(KC_KEY_RIGHT)) {
            placingAngle += -increment;
        }
        HandleTowerPlacement(KC_MOUSE_LEFT);
    }
}

inline void Manager::HandleTowerPlacement(u8 keycodePlace) {
    Tower tower(towerType);
    tower.physical.pos = mouse;
    tower.physical.angle = placingAngle;
    canPlace = true;
    if (money < towerCosts[towerType]) {
        canPlace = false;
    } else {
        for (i32 i = 0; i < towers.size; i++) {
            const Tower &other = towers[i];
            if (other.id.generation < 0) continue;
            if (other.physical.Collides(tower.physical)) {
                canPlace = false;
                break;
            }
        }
    }
    if (globals->objects.Pressed(keycodePlace)) {
        if (canPlace) {
            tower.sunkCost = towerCosts[towerType];
            towers.Create(tower);
            money -= towerCosts[towerType];
        }
    }
}

inline void Manager::HandleMusicLoops(i32 w) {
    if (w >= 1 && w <= 10) {
        i32 section = 44100*16;
        i32 preLoop = 44100*0;
        streamSegment1.SetLoopRange(w*section+preLoop, (w+1)*section);
    }
}

inline bool Manager::CursorVisible() const {
    return globals->gui.currentMenu == Int::MENU_PLAY && !globals->gui.usingMouse && !placeMode && !focusMenu && selectedTower == -1;
}

void Manager::EventSync() {
    timestep = globals->objects.timestep * globals->objects.simulationRate;
    if (globals->input.cursorPrevious != globals->input.cursor) {
        mouse = ScreenPosToWorld(globals->input.cursor);
    }
    if (globals->gui.currentMenu == Int::MENU_PLAY) {
        HandleUI();
    } else {
        placeMode = false;
        focusMenu = false;
        selectedTower = -1;
    }
    towers.Synchronize();
    enemies.Synchronize();
    bullets.Synchronize();
    winds.Synchronize();
    explosions.Synchronize();

    updateChunks.size = 0;

    towers.GetUpdateChunks(updateChunks);
    enemies.GetUpdateChunks(updateChunks);
    bullets.GetUpdateChunks(updateChunks);
    winds.GetUpdateChunks(updateChunks);
    explosions.GetUpdateChunks(updateChunks);

    if (timestep != 0.0 && hitpointsLeft > 0) {
        enemyTimer -= timestep;
        if (enemies.count == 0) {
            enemyTimer = 0.0;
        }
        while (enemyTimer <= 0.0 && hitpointsLeft > 0) {
            Enemy enemy;
            enemies.Create(enemy); // Enemy::EventCreate() increases enemyTimer based on HP
        }
    }
    if (hitpointsLeft == 0 && waveActive && enemies.count == 0) {
        waveActive = false;
        globals->gui.playMenu.buttonStartWave->string = globals->ReadLocale("Start Wave");
    }
    readyForDraw = true;
}

void Manager::EventUpdate() {
    if (timestep != 0.0) {
        const i32 concurrency = 4;
        Array<Thread> threads(concurrency);
        for (i32 i = 0; i < updateChunks.size; i++) {
            for (i32 j = 0; j < concurrency; j++) {
                UpdateChunk &chunk = updateChunks[i];
                threads[j] = Thread(chunk.updateCallback, chunk.theThisPointer, j, concurrency);
            }
            for (i32 j = 0; j < concurrency; j++) {
                if (threads[j].Joinable()) {
                    threads[j].Join();
                }
            }
        }
    }
}

void Manager::EventDraw(Array<Rendering::DrawingContext> &contexts) {
    // if (globals->gui.currentMenu != Int::MENU_PLAY) return;

    const i32 concurrency = contexts.size;
    Array<Thread> threads(concurrency);
    for (i32 i = 0; i < updateChunks.size; i++) {
        for (i32 j = 0; j < concurrency; j++) {
            UpdateChunk &chunk = updateChunks[i];
            threads[j] = Thread(chunk.drawCallback, chunk.theThisPointer, &contexts[j], j, concurrency);
        }
        for (i32 j = 0; j < concurrency; j++) {
            if (threads[j].Joinable()) {
                threads[j].Join();
            }
        }
    }

    if (placeMode) {
        Tower tower(towerType);
        tower.physical.pos = mouse;
        tower.physical.angle = placingAngle;
        tower.physical.Draw(contexts.Back(), canPlace ? vec4(0.1, 1.0, 0.1, 0.9) : vec4(1.0, 0.1, 0.1, 0.9));
        tower.field.pos = tower.physical.pos;
        tower.field.angle = tower.physical.angle;
        tower.field.Draw(contexts.Back(), canPlace ? vec4(1.0, 1.0, 1.0, 0.1) : vec4(1.0, 0.5, 0.5, 0.2));
    }
    if (selectedTower != -1) {
        const Tower& selected = towers[selectedTower];
        selected.field.Draw(contexts.Back(), vec4(1.0, 1.0, 1.0, 0.1));
    }
    basePhysical.Draw(contexts.Back(), vec4(hsvToRgb(vec3((f32)lives / 3000.0, 1.0, 0.8)), 1.0));
    for (i32 i = 0; i < enemySpawns.size; i++) {
        enemySpawns[i].Draw(contexts.Back(), vec4(vec3(0.0), 1.0));
    }
    if (CursorVisible()) {
        vec2 cursor = WorldPosToScreen(mouse);
        globals->rendering.DrawQuad(contexts.Back(), globals->gui.cursorIndex, vec4(1.0), cursor, vec2(32.0 * globals->gui.scale), vec2(1.0), vec2(0.5));
    }
}

void Manager::CreateSpawn() {
    f32 angle = random(0.0, tau, globals->rng);
    vec2 place(sin(angle), cos(angle));
    place *= 1500.0;
    Physical newSpawn;
    newSpawn.type = BOX;
    newSpawn.basis.box.a = vec2(-128.0, -32.0);
    newSpawn.basis.box.b = vec2(128.0, 32.0);
    newSpawn.pos = place;
    newSpawn.angle = angle + pi;
    enemySpawns.Append(newSpawn);
}

vec2 Manager::WorldPosToScreen(vec2 in) const {
    vec2 out = (in - camPos) * camZoom + vec2(globals->window.width, globals->window.height) / 2.0;
    return out;
}
vec2 Manager::ScreenPosToWorld(vec2 in) const {
    vec2 out = vec2(globals->input.cursor - vec2i(globals->window.width, globals->window.height) / 2) / camZoom + camPos;
    return out;
}

bool AABB::Collides(const AABB &other) const {
    return minPos.x <= other.maxPos.x
        && maxPos.x >= other.minPos.x
        && minPos.y <= other.maxPos.y
        && maxPos.y >= other.minPos.y;
}

void AABB::Update(const Physical &physical) {
    switch (physical.type) {
    case SEGMENT: {
        minPos.x = min(physical.actual.segment.a.x, physical.actual.segment.b.x);
        minPos.y = min(physical.actual.segment.a.y, physical.actual.segment.b.y);
        maxPos.x = max(physical.actual.segment.a.x, physical.actual.segment.b.x);
        maxPos.y = max(physical.actual.segment.a.y, physical.actual.segment.b.y);
        break;
    }
    case CIRCLE: {
        minPos.x = physical.actual.circle.c.x - physical.actual.circle.r;
        minPos.y = physical.actual.circle.c.y - physical.actual.circle.r;
        maxPos.x = physical.actual.circle.c.x + physical.actual.circle.r;
        maxPos.y = physical.actual.circle.c.y + physical.actual.circle.r;
        break;
    }
    case BOX: {
        minPos.x = min(physical.actual.box.a.x,
                       min(physical.actual.box.b.x,
                           min(physical.actual.box.c.x,
                               physical.actual.box.d.x)));
        minPos.y = min(physical.actual.box.a.y,
                       min(physical.actual.box.b.y,
                           min(physical.actual.box.c.y,
                               physical.actual.box.d.y)));
        maxPos.x = max(physical.actual.box.a.x,
                       max(physical.actual.box.b.x,
                           max(physical.actual.box.c.x,
                               physical.actual.box.d.x)));
        maxPos.y = max(physical.actual.box.a.y,
                       max(physical.actual.box.b.y,
                           max(physical.actual.box.c.y,
                               physical.actual.box.d.y)));
        break;
    }
    }
}

bool CollisionSegmentSegment(const Physical &a, const Physical &b) {
    const vec2 A = a.actual.segment.a;
    const vec2 B = a.actual.segment.b;
    const vec2 C = b.actual.segment.a;
    const vec2 D = b.actual.segment.b;

    f32 denom, num1, num2;
    denom = ((B.x - A.x) * (D.y - C.y)) - ((B.y - A.y) * (D.x - C.x));
    num1  = ((A.y - C.y) * (D.x - C.x)) - ((A.x - C.x) * (D.y - C.y));
    num2  = ((A.y - C.y) * (B.x - A.x)) - ((A.x - C.x) * (B.y - A.y));

    if (denom == 0) return num1 == 0 && num2 == 0;

    f32 r = num1/denom;
    f32 s = num2/denom;

    return (r >= 0 && r <= 1 && s >= 0 && s <= 1);
}

inline bool CollisionSegmentCircle(const Physical &a, const Physical &b) {
    return distSqrToLine<true>(a.actual.segment.a, a.actual.segment.b, b.actual.circle.c) <= square(b.actual.circle.r);
}

bool SegmentInAABB(const vec2 &A, const vec2 &B, AABB aabb) {
    f32 t = (aabb.minPos.y - A.y) / (B.y - A.y);
    if (t == median(t, 0.0, 1.0)) {
        f32 x = A.x + (B.x - A.x) * t;
        if (x == median(x, aabb.minPos.x, aabb.maxPos.x)) {
            // Segment intersects
            return true;
        }
    }
    t = (aabb.maxPos.y - A.y) / (B.y - A.y);
    if (t == median(t, 0.0, 1.0)) {
        f32 x = A.x + (B.x - A.x) * t;
        if (x == median(x, aabb.minPos.x, aabb.maxPos.x)) {
            // Segment intersects
            return true;
        }
    }
    t = (aabb.minPos.x - A.x) / (B.x - A.x);
    if (t == median(t, 0.0, 1.0)) {
        f32 y = A.y + (B.y - A.y) * t;
        if (y == median(y, aabb.minPos.y, aabb.maxPos.y)) {
            // Segment intersects
            return true;
        }
    }
    t = (aabb.maxPos.x - A.x) / (B.x - A.x);
    if (t == median(t, 0.0, 1.0)) {
        f32 y = A.y + (B.y - A.y) * t;
        if (y == median(y, aabb.minPos.y, aabb.maxPos.y)) {
            // Segment intersects
            return true;
        }
    }
    return false;
}

bool CollisionSegmentBox(const Physical &a, const Physical &b) {
    const mat2 aRotation = mat2::Rotation(-b.angle.value());
    // const vec2 dPos = a.pos - b.pos;
    const vec2 A = (a.actual.segment.a - b.pos) * aRotation;
    if (A.x == median(A.x, b.basis.box.a.x, b.basis.box.b.x) && A.y == median(A.y, b.basis.box.a.y, b.basis.box.b.y)) {
        // Point is inside box
        return true;
    }
    const vec2 B = (a.actual.segment.b - b.pos) * aRotation;
    if (B.x == median(B.x, b.basis.box.a.x, b.basis.box.b.x) && B.y == median(B.y, b.basis.box.a.y, b.basis.box.b.y)) {
        // Point is inside box
        return true;
    }
    return SegmentInAABB(A, B, {b.basis.box.a, b.basis.box.b});
}

bool CollisionCircleCircle(const Physical &a, const Physical &b) {
    return absSqr(a.actual.circle.c - b.actual.circle.c) <= square(a.actual.circle.r + b.actual.circle.r);
}

bool CollisionCircleBox(const Physical &a, const Physical &b) {
    const f32 rSquared = square(a.actual.circle.r);
    if (absSqr(a.actual.circle.c - b.actual.box.a) <= rSquared) return true;
    if (absSqr(a.actual.circle.c - b.actual.box.b) <= rSquared) return true;
    if (absSqr(a.actual.circle.c - b.actual.box.c) <= rSquared) return true;
    if (absSqr(a.actual.circle.c - b.actual.box.d) <= rSquared) return true;

    const mat2 rotation = mat2::Rotation(-b.angle.value());
    const vec2 C = (a.actual.circle.c - b.pos) * rotation;
    if (C.x == median(C.x, b.basis.box.a.x, b.basis.box.b.x)
     && C.y + a.actual.circle.r >= b.basis.box.a.y
     && C.y - a.actual.circle.r <= b.basis.box.b.y) {
        return true;
    }
    if (C.y == median(C.y, b.basis.box.a.y, b.basis.box.b.y)
     && C.x + a.actual.circle.r >= b.basis.box.a.x
     && C.x - a.actual.circle.r <= b.basis.box.b.x) {
        return true;
    }
    return false;
}

bool CollisionBoxBoxPart(const Physical &a, const Physical &b) {
    const mat2 rotation = mat2::Rotation(-b.angle.value());
    const vec2 A = (a.actual.box.a - b.pos) * rotation;
    if (A.x == median(A.x, b.basis.box.a.x, b.basis.box.b.x)
     && A.y == median(A.y, b.basis.box.a.y, b.basis.box.b.y)) {
        return true;
    }
    const vec2 B = (a.actual.box.b - b.pos) * rotation;
    if (B.x == median(B.x, b.basis.box.a.x, b.basis.box.b.x)
     && B.y == median(B.y, b.basis.box.a.y, b.basis.box.b.y)) {
        return true;
    }
    const vec2 C = (a.actual.box.c - b.pos) * rotation;
    if (C.x == median(C.x, b.basis.box.a.x, b.basis.box.b.x)
     && C.y == median(C.y, b.basis.box.a.y, b.basis.box.b.y)) {
        return true;
    }
    const vec2 D = (a.actual.box.d - b.pos) * rotation;
    if (D.x == median(D.x, b.basis.box.a.x, b.basis.box.b.x)
     && D.y == median(D.y, b.basis.box.a.y, b.basis.box.b.y)) {
        return true;
    }

    const AABB aabb = {b.basis.box.a, b.basis.box.b};
    if (SegmentInAABB(A, C, aabb)) return true;
    if (SegmentInAABB(C, B, aabb)) return true;
    if (SegmentInAABB(B, D, aabb)) return true;
    if (SegmentInAABB(D, A, aabb)) return true;
    return false;
}

bool CollisionBoxBox(const Physical &a, const Physical &b) {
    // have to go both ways otherwise you can fit a smaller one inside a bigger one
    return CollisionBoxBoxPart(a, b) || CollisionBoxBoxPart(b, a);
}

bool Physical::Collides(const Physical &other) const {
    if (!updated) {
        UpdateActual();
    }
    if (!other.updated) {
        other.UpdateActual();
    }
    // return aabb.Collides(other.aabb);
    if (!aabb.Collides(other.aabb)) {
        return false;
    }
    switch (type) {
    case SEGMENT: {
        switch (other.type) {
        case SEGMENT: {
            return CollisionSegmentSegment(*this, other);
        }
        case CIRCLE: {
            return CollisionSegmentCircle(*this, other);
        }
        case BOX: {
            return CollisionSegmentBox(*this, other);
        }
        }
        break;
    }
    case CIRCLE: {
        switch (other.type) {
        case SEGMENT: {
            return CollisionSegmentCircle(other, *this);
        }
        case CIRCLE: {
            return CollisionCircleCircle(*this, other);
        }
        case BOX: {
            return CollisionCircleBox(*this, other);
        }
        }
        break;
    }
    case BOX: {
        switch (other.type) {
        case SEGMENT: {
            return CollisionSegmentBox(other, *this);
        }
        case CIRCLE: {
            return CollisionCircleBox(other, *this);
        }
        case BOX: {
            return CollisionBoxBox(*this, other);
        }
        }
        break;
    }
    }
    return false;
}

bool Physical::MouseOver() const {
    const vec2 mouse = globals->entities.mouse;
    if (!updated) {
        UpdateActual();
    }
    switch (type) {
    case SEGMENT: {
        return distSqrToLine<true>(actual.segment.a, actual.segment.b, mouse) < 16.0;
    }
    case CIRCLE: {
        return absSqr(actual.circle.c-mouse) <= square(actual.circle.r);
    }
    case BOX: {
        const mat2 rotation = mat2::Rotation(-angle.value());
        const vec2 A = (mouse - pos) * rotation;
        if (A.x == median(A.x, basis.box.a.x, basis.box.b.x)
         && A.y == median(A.y, basis.box.a.y, basis.box.b.y)) {
            return true;
        }
        break;
    }
    }
    return false;
}

void PhysicalAbsFromBasis(PhysicalAbs &actual, const PhysicalBasis &basis, const CollisionType &type, const vec2 &pos, const Angle32 &angle) {
    mat2 rotation(1.0);
    if (angle != 0.0) {
        rotation = mat2::Rotation(angle.value());
    }
    switch (type) {
    case SEGMENT: {
        if (angle != 0.0) {
            actual.segment.a = basis.segment.a * rotation + pos;
            actual.segment.b = basis.segment.b * rotation + pos;
        } else {
            actual.segment.a = basis.segment.a + pos;
            actual.segment.b = basis.segment.b + pos;
        }
        break;
    }
    case CIRCLE: {
        if (angle != 0.0) {
            actual.circle.c = basis.circle.c * rotation + pos;
        } else {
            actual.circle.c = basis.circle.c + pos;
        }
        actual.circle.r = basis.circle.r;
        break;
    }
    case BOX: {
        if (angle != 0.0) {
            actual.box.a = basis.box.a * rotation + pos;
            actual.box.b = basis.box.b * rotation + pos;
            actual.box.c = vec2(basis.box.b.x, basis.box.a.y) * rotation + pos;
            actual.box.d = vec2(basis.box.a.x, basis.box.b.y) * rotation + pos;
        } else {
            actual.box.a = basis.box.a + pos;
            actual.box.b = basis.box.b + pos;
            actual.box.c = vec2(basis.box.b.x, basis.box.a.y) + pos;
            actual.box.d = vec2(basis.box.a.x, basis.box.b.y) + pos;
        }
        break;
    }
    }
}

void Physical::Update(f32 timestep) {
    angle += rot * timestep;
    pos += vel * timestep;
    updated = false;
}

void Physical::UpdateActual() const {
    PhysicalAbsFromBasis(actual, basis, type, pos, angle);
    aabb.Update(*this);
    updated = true;
}

void Physical::Draw(Rendering::DrawingContext &context, vec4 color) const {
    const vec2 p = (pos - globals->entities.camPos) * globals->entities.camZoom
                 + vec2(globals->window.width / 2, globals->window.height / 2);
    if (type == BOX) {
        const vec2 scale = basis.box.b - basis.box.a;
        globals->rendering.DrawQuad(context, Rendering::texBlank, color, p, scale * globals->entities.camZoom, vec2(1.0), -basis.box.a / scale, angle);
    } else if (type == SEGMENT) {
        vec2 scale = basis.segment.b - basis.segment.a;
        scale.y = max(scale.y, 2.0);
        globals->rendering.DrawQuad(context, Rendering::texBlank, color, p, scale * globals->entities.camZoom, vec2(1.0), -basis.segment.a / scale, angle);
    } else {
        const vec2 scale = basis.circle.r * 2.0;
        globals->rendering.DrawCircle(context, Rendering::texBlank, color, p, scale * globals->entities.camZoom + 2.0, vec2(1.0), -basis.circle.c / (scale + 2.0) + vec2(0.5), angle);
    }
}

template<typename T>
void DoubleBufferArray<T>::Update(void *theThisPointer, i32 threadIndex, i32 concurrency) {
    DoubleBufferArray<T> *theActualThisPointer = (DoubleBufferArray<T>*)theThisPointer;
    i32 g = theActualThisPointer->granularity;
    for (i32 i = threadIndex*g; i < theActualThisPointer->array[theActualThisPointer->buffer].size; i += g*concurrency) {
        for (i32 j = 0; j < g; j++) {
            if (i+j >= theActualThisPointer->array[theActualThisPointer->buffer].size) break;
            T &obj = theActualThisPointer->array[theActualThisPointer->buffer][i+j];
            if (obj.id.generation >= 0)
                obj.Update(globals->entities.timestep);
        }
    }
}

template<typename T>
void DoubleBufferArray<T>::Draw(void *theThisPointer, Rendering::DrawingContext *context, i32 threadIndex, i32 concurrency) {
    DoubleBufferArray<T> *theActualThisPointer = (DoubleBufferArray<T>*)theThisPointer;
    i32 g = theActualThisPointer->granularity;
    for (i32 i = threadIndex*g; i < theActualThisPointer->array[!theActualThisPointer->buffer].size; i += g*concurrency) {
        for (i32 j = 0; j < g; j++) {
            if (i+j >= theActualThisPointer->array[!theActualThisPointer->buffer].size) break;
            T &obj = theActualThisPointer->array[!theActualThisPointer->buffer][i+j];
            if (obj.id.generation >= 0)
                obj.Draw(*context);
        }
    }
}

template<typename T>
void DoubleBufferArray<T>::Synchronize() {
    buffer = globals->objects.buffer;

    for (u16 &index : destroyed) {
        // array[!buffer][index].EventDestroy();
        array[!buffer][index].id.generation = -array[!buffer][index].id.generation-1;
        empty.Append(index);
    }
    count -= destroyed.size;
    destroyed.Clear();
    for (T &obj : created) {
        if (empty.size > 0) {
            obj.id.index = empty.Back();
            obj.id.generation = -array[!buffer][obj.id.index].id.generation;
            // obj.EventCreate();
            array[!buffer][obj.id.index] = std::move(obj);
            empty.Erase(empty.size-1);
        } else {
            obj.id.index = array[!buffer].size;
            obj.id.generation = 0;
            // obj.EventCreate();
            array[!buffer].Append(std::move(obj));
        }
    }
    count += created.size;
    created.Clear();
    array[buffer] = array[!buffer];
    size = array[0].size;
}

template<typename T>
void DoubleBufferArray<T>::GetUpdateChunks(Array<UpdateChunk> &dstUpdateChunks) {
    if (count == 0) return;
    UpdateChunk chunk;
    chunk.updateCallback = DoubleBufferArray<T>::Update;
    chunk.drawCallback = DoubleBufferArray<T>::Draw;
    chunk.theThisPointer = this;
    dstUpdateChunks.Append(chunk);
}

template<typename T>
void DoubleBufferArray<T>::Create(T &obj) {
    mutex.Lock();
    obj.EventCreate();
    created.Append(obj);
    mutex.Unlock();
}

template<typename T>
void DoubleBufferArray<T>::Destroy(Id id) {
    mutex.Lock();
    if (array[!buffer][id.index].id != id) {
        // std::cout << "Attempt to destroy an object of the wrong generation! Expected gen = "
        //           << id.generation << ", actual gen = " << array[!buffer][id.index].id.generation << std::endl;
        mutex.Unlock();
        return;
    }
    for (i32 i = 0; i < destroyed.size; i++) {
        if (destroyed[i] == id.index) {
            mutex.Unlock();
            return;
        }
    }
    array[!buffer][id.index].EventDestroy();
    destroyed.Append(id.index);
    array[!buffer][id.index].id.generation *= -1;
    mutex.Unlock();
}

Tower::Tower(TowerType _type) {
    type = _type;
    switch(type) {
        case TOWER_GUN:
            *this = towerGunTemplate;
            break;
        case TOWER_SHOTGUN:
            *this = towerShotgunTemplate;
            break;
        case TOWER_FAN:
            *this = towerFanTemplate;
            break;
        case TOWER_GAUSS:
            *this = towerGaussTemplate;
            break;
        case TOWER_SHOCKWAVE:
            *this = towerShockerTemplate;
            break;
        case TOWER_FLAK:
            *this = towerFlakTemplate;
            break;
        default:
            physical.type = CIRCLE;
            physical.basis.circle.c = vec2(0.0);
            physical.basis.circle.r = 16.0;
            color = vec4(0.0, 0.0, 0.0, 1.0);
            shootInterval = 0.8;
            bulletSpread = 15.0;
            bulletCount = 10;
            bulletSpeed = 800.0;
            break;
    }
}

void Tower::EventCreate() {
    selected = false;
    disabled = false;
    shootTimer = 0.0;
    field.pos = physical.pos;
    field.angle = physical.angle;
    priority = PRIORITY_NEAREST;
    kills = 0;
    damageDone = 0;
}

void Tower::Update(f32 timestep) {
    physical.Update(timestep);
    selected = globals->entities.selectedTower == id;
    if (shootTimer <= 0.0) disabled = false;
    for (i32 i = 0; i < globals->entities.enemies.size; i++) {
        const Enemy& other = globals->entities.enemies[i];
        if (other.id.generation < 0 || other.hitpoints <= 2000) continue;
        if (physical.Collides(other.physical)) {
            disabled = true;
            shootTimer = 0.5;
            break;
        }
    }
    shootTimer = max(shootTimer-timestep, -timestep);
    if (disabled) return;
    if (shootTimer <= 0.0) {
        if (type != TOWER_SHOCKWAVE && type != TOWER_FAN) {
            Id target = -1;
            f32 targetDist = range;
            switch (priority) {
                case PRIORITY_NEAREST: {
                    for (i32 i = 0; i < globals->entities.enemies.size; i++) {
                        const Enemy& other = globals->entities.enemies[i];
                        if (other.id.generation < 0 || other.hitpoints == 0) continue;
                        f32 dist = abs(other.physical.pos - physical.pos) - other.physical.basis.circle.r;
                        if (dist < targetDist) {
                            targetDist = dist;
                            target = other.id;
                        }
                    }
                } break;
                case PRIORITY_FURTHEST: {
                    targetDist = 0.0;
                    for (i32 i = 0; i < globals->entities.enemies.size; i++) {
                        const Enemy& other = globals->entities.enemies[i];
                        if (other.id.generation < 0 || other.hitpoints == 0) continue;
                        f32 dist = abs(other.physical.pos - physical.pos) - other.physical.basis.circle.r;
                        if (dist < range && dist > targetDist) {
                            targetDist = dist;
                            target = other.id;
                        }
                    }
                } break;
                case PRIORITY_WEAKEST: {
                    i32 lowestHP = INT32_MAX;
                    for (i32 i = 0; i < globals->entities.enemies.size; i++) {
                        const Enemy& other = globals->entities.enemies[i];
                        if (other.id.generation < 0 || other.hitpoints == 0) continue;
                        f32 dist = abs(other.physical.pos - physical.pos) - other.physical.basis.circle.r;
                        if (dist < range && other.hitpoints < lowestHP) {
                            lowestHP = other.hitpoints;
                            targetDist = dist;
                            target = other.id;
                        }
                    }
                } break;
                case PRIORITY_STRONGEST: {
                    i32 highestHP = 0;
                    for (i32 i = 0; i < globals->entities.enemies.size; i++) {
                        const Enemy& other = globals->entities.enemies[i];
                        if (other.id.generation < 0 || other.hitpoints == 0) continue;
                        f32 dist = abs(other.physical.pos - physical.pos) - other.physical.basis.circle.r;
                        if (dist < range && other.hitpoints > highestHP) {
                            highestHP = other.hitpoints;
                            targetDist = dist;
                            target = other.id;
                        }
                    }
                } break;
                case PRIORITY_NEWEST: {
                    f32 youngest = 1000000.0;
                    for (i32 i = 0; i < globals->entities.enemies.size; i++) {
                        const Enemy& other = globals->entities.enemies[i];
                        if (other.id.generation < 0 || other.hitpoints == 0) continue;
                        f32 dist = abs(other.physical.pos - physical.pos) - other.physical.basis.circle.r;
                        if (dist < range && other.age < youngest) {
                            youngest = other.age;
                            targetDist = dist;
                            target = other.id;
                        }
                    }
                } break;
                case PRIORITY_OLDEST: {
                    f32 oldest = 0.0;
                    for (i32 i = 0; i < globals->entities.enemies.size; i++) {
                        const Enemy& other = globals->entities.enemies[i];
                        if (other.id.generation < 0 || other.hitpoints == 0) continue;
                        f32 dist = abs(other.physical.pos - physical.pos) - other.physical.basis.circle.r;
                        if (dist < range && other.age > oldest) {
                            oldest = other.age;
                            targetDist = dist;
                            target = other.id;
                        }
                    }
                } break;
            }

            if (target != -1) {
                const Enemy& other = globals->entities.enemies[target];
                Bullet bullet;
                bullet.lifetime = range / (bulletSpeed * 0.9);
                bullet.explosionDamage = bulletExplosionDamage;
                bullet.explosionRange = bulletExplosionRange;
                bullet.owner = id;
                f32 dist = targetDist;
                vec2 deltaP;
                for (i32 i = 0; i < 2; i++) {
                    deltaP = other.physical.pos - physical.pos + other.physical.vel * dist / bulletSpeed;
                    dist = abs(deltaP);
                }
                deltaP = other.physical.pos - physical.pos + other.physical.vel * dist / bulletSpeed;
                Angle32 idealAngle = atan2(-deltaP.y, deltaP.x);
                for (i32 i = 0; i < bulletCount; i++) {
                    Angle32 angle = idealAngle + Degrees32(random(-bulletSpread.value(), bulletSpread.value(), globals->rng));
                    bullet.physical.vel.x = cos(angle);
                    bullet.physical.vel.y = -sin(angle);
                    bullet.physical.vel *= bulletSpeed + random(-bulletSpeedVariability, bulletSpeedVariability, globals->rng);
                    bullet.physical.pos = physical.pos + bullet.physical.vel * timestep;
                    bullet.damage = damage;
                    globals->entities.bullets.Create(bullet);
                }
                shootTimer += shootInterval;
            }
        } else if (type == TOWER_SHOCKWAVE) {
            bool shoot = false;
            for (i32 i = 0; i < globals->entities.enemies.size; i++) {
                const Enemy& other = globals->entities.enemies[i];
                if (other.id.generation < 0 || other.hitpoints == 0) continue;
                if (field.Collides(other.physical)) {
                    shoot = true;
                    break;
                }
            }
            if (shoot) {
                Explosion explosion;
                explosion.size = range;
                explosion.growth = 5.0;
                explosion.damage = damage;
                explosion.physical.pos = physical.pos;
                explosion.owner = id;
                globals->entities.explosions.Create(explosion);
                shootTimer += shootInterval;
            }
        } else if (type == TOWER_FAN) {
            Wind wind;
            wind.physical.pos = physical.pos;
            wind.lifetime = range / bulletSpeed;
            f32 randomPos = random(-20.0, 20.0, globals->rng);
            wind.physical.pos.x += cos(physical.angle.value() + pi * 0.5) * randomPos;
            wind.physical.pos.y -= sin(physical.angle.value() + pi * 0.5) * randomPos;
            for (i32 i = 0; i < bulletCount; i++) {
                Angle32 angle = physical.angle + Degrees32(random(-bulletSpread.value(), bulletSpread.value(), globals->rng));
                wind.physical.vel.x = cos(angle);
                wind.physical.vel.y = -sin(angle);
                wind.physical.vel *= bulletSpeed + random(-bulletSpeedVariability, bulletSpeedVariability, globals->rng);
                wind.physical.pos += wind.physical.vel * 0.03;
                globals->entities.winds.Create(wind);
            }
        }
    }
}

void Tower::Draw(Rendering::DrawingContext &context) {
    vec4 colorTemp;
    if (selected) {
        colorTemp = vec4(0.5) + color * 0.5;
    } else {
        colorTemp = color;
    }
    if (disabled) {
        colorTemp.rgb = (colorTemp.rgb + vec3(0.8 * 3.0)) / 4.0;
    }
    physical.Draw(context, colorTemp);
}

template struct DoubleBufferArray<Tower>;

const f32 honkerSpawnInterval = 2.0;

void Enemy::EventCreate() {
    physical.type = CIRCLE;
    physical.basis.circle.c = vec2(0.0);
    physical.basis.circle.r = 0.0;
    i32 multiplier = 1;
    if (!child) {
        i32 spawnPoint = random(0, globals->entities.enemySpawns.size-1, globals->rng);
        f32 s, c;
        s = sin(globals->entities.enemySpawns[spawnPoint].angle);
        c = cos(globals->entities.enemySpawns[spawnPoint].angle);
        vec2 x, y;
        x = vec2(c, -s) * globals->entities.enemySpawns[spawnPoint].basis.box.b.x
          * random(-1.0, 1.0, globals->rng);
        y = vec2(s, c) * globals->entities.enemySpawns[spawnPoint].basis.box.b.y
          * random(-1.0, 1.0, globals->rng);
        physical.pos = globals->entities.enemySpawns[spawnPoint].pos + x + y;
        physical.vel = 0.0;
        multiplier = random(1, 3, globals->rng);
        f32 honker = random(0.0, 10000.0, globals->rng);
        if (honker < 1.0) {
            multiplier = 1000;
        } else if (honker < 10.0) {
            multiplier = 500;
        } else if (honker < 100.0) {
            multiplier = 100;
        }
        hitpoints = multiplier * floor(80.0 * pow(1.16, (f32)(globals->entities.wave + 3))) / (globals->entities.wave+7);
        age = 0.0;
    }
    spawnTimer = honkerSpawnInterval;
    if (!child) {
        i64 limit = median(globals->entities.hitpointsLeft / 2, (i64)500, globals->entities.hitpointsLeft);
        if (hitpoints > limit) {
            hitpoints = limit;
        }
        globals->entities.hitpointsLeft -= hitpoints;
        size = hitpoints;
        color = vec4(hsvToRgb(vec3(sqrt(size)/(tau*16.0) + (f32)globals->entities.wave / 9.0, min(size / 100.0, 1.0), 1.0)), 0.7);
    }
    value = hitpoints;
    f32 speedDivisor = max(log10((f32)multiplier), 1.0);
    targetSpeed = 200.0 / speedDivisor;
    size = 0.0;
    if (!child) {
        globals->entities.enemyTimer += (f32)hitpoints / globals->entities.hitpointsPerSecond / speedDivisor;
    }
}

void Enemy::EventDestroy() {
    if (hitpoints <= 0) {
        globals->entities.money += value;
        for (const Id &damager : damageContributors) {
            globals->entities.towers.GetMutable(damager).kills++;
        }
    }
}

void Enemy::Update(f32 timestep) {
    age += timestep;
    if (hitpoints > 0) {
        size = decay(size, (f32)hitpoints, 0.1, timestep);
    } else {
        size = decay(size, 0.0, 0.025, timestep);
    }
    physical.basis.circle.r = cbrt(size) + min(2.0, sqrt(size)/10.0) + 2.0;
    physical.Update(timestep);
    physical.UpdateActual();
    if (physical.Collides(globals->entities.basePhysical) || (hitpoints <= 0 && size < 0.01)) {
        globals->entities.lives -= hitpoints;
        globals->entities.enemies.Destroy(id);
    }
    if (hitpoints == 0) return;
    if (hitpoints > 5000) {
        if (spawnTimer <= 0.0) {
            Enemy newEnemy;
            newEnemy.child = true;
            newEnemy.age = age;
            Angle32 spawnAngle = random(0.0, tau, globals->rng);
            vec2 spawnVector = vec2(cos(spawnAngle), -sin(spawnAngle)) * sqrt(random(0.0, 1.0, globals->rng));
            newEnemy.physical.pos = physical.pos + spawnVector * physical.basis.circle.r;
            newEnemy.physical.vel = physical.vel + spawnVector * 100.0;
            newEnemy.color = color;
            newEnemy.hitpoints = hitpoints/20;
            hitpoints -= newEnemy.hitpoints;
            globals->entities.enemies.Create(newEnemy);
            spawnTimer += honkerSpawnInterval;
        } else {
            spawnTimer -= timestep;
        }
    }
    for (i32 i = 0; i < globals->entities.towers.size; i++) {
        Tower &other = globals->entities.towers.GetMutable(i);
        if (other.id.generation < 0 || other.type != TOWER_FAN || other.disabled) continue;
        if (physical.Collides(other.field)) {
            vec2 deltaP = physical.pos - other.physical.pos;
            physical.Impulse(normalize(deltaP)
                * max((other.range + physical.basis.circle.r - abs(deltaP)), 0.0)
                * 5000.0 / pow(size, 1.5), timestep);
            if (other.damage != 0) {
                if (random(0.0, 1.0, globals->rng) <= (f32)other.damage*timestep) {
                    damageContributors.emplace(other.id);
                    other.damageDone++;
                    hitpoints--;
                }
            }
        }
    }
    for (i32 i = 0; i < globals->entities.explosions.size; i++) {
        const Explosion &other = globals->entities.explosions[i];
        if (other.id.generation < 0) continue;
        if (physical.Collides(other.physical)) {
            vec2 deltaP = physical.pos - other.physical.pos;
            physical.Impulse(normalize(deltaP) * max((other.size + physical.basis.circle.r - abs(deltaP)), 0.0) * 500.0 / pow(size, 1.5), timestep);
            if (other.damage != 0) {
                damageContributors.emplace(other.owner);
                f32 prob = (f32)other.damage*timestep;
                i32 hits = prob;
                prob -= (f32)hits;
                if (random(0.0, 1.0, globals->rng) <= prob) {
                    hits++;
                }
                hitpoints -= hits;
                globals->entities.towers.GetMutable(other.owner).damageDone += hits;
            }
        }
    }
    for (i32 i = 0; i < globals->entities.bullets.size; i++) {
        Bullet &other = globals->entities.bullets.GetMutable(i);
        if (other.id.generation < 0) continue;
        if (physical.Collides(other.physical)) {
            damageContributors.emplace(other.owner);
            if (other.damage > hitpoints) {
                other.damage -= hitpoints;
                globals->entities.towers.GetMutable(other.owner).damageDone += hitpoints;
                hitpoints = 0;
            } else {
                globals->entities.bullets.Destroy(other.id);
                hitpoints -= other.damage;
                globals->entities.towers.GetMutable(other.owner).damageDone += other.damage;
                physical.vel += normalize(other.physical.vel) * 100.0 / size;
            }
        }
    }
    vec2 norm = normalize(-physical.pos);
    f32 velocity = abs(physical.vel);
    f32 forward = dot(norm, physical.vel/velocity);
    if (forward < 0.2) {
        physical.vel += norm * (0.2 - forward) * velocity;
    }
    physical.Impulse(norm * targetSpeed, timestep);
    physical.vel = normalize(physical.vel) * targetSpeed;
}

void Enemy::Draw(Rendering::DrawingContext &context) {
    physical.Draw(context, color * vec4(vec3(1.0), clamp(size, 0.0, 1.0)));
}

template struct DoubleBufferArray<Enemy>;

void Bullet::EventCreate() {
    f32 length = abs(physical.vel) * 0.5 / 30.0;
    physical.type = SEGMENT;
    physical.basis.segment.a = vec2(-length, -1.0);
    physical.basis.segment.b = vec2(length, 1.0);
    physical.angle = atan2(-physical.vel.y, physical.vel.x);
}

void Bullet::EventDestroy() {
    if (explosionRange != 0.0) {
        Explosion explosion;
        explosion.damage = explosionDamage;
        explosion.size = explosionRange;
        explosion.growth = 8.0;
        explosion.physical.pos = physical.pos;
        explosion.physical.vel = physical.vel;
        explosion.owner = owner;
        globals->entities.explosions.Create(explosion);
    }
}

void Bullet::Update(f32 timestep) {
    physical.Update(timestep);
    physical.UpdateActual();
    lifetime -= timestep;
    if (lifetime <= 0.0) {
        globals->entities.bullets.Destroy(id);
    }
}

void Bullet::Draw(Rendering::DrawingContext &context) {
    vec4 color = vec4(1.0, 1.0, 0.5, clamp(0.0, 1.0, lifetime * 8.0));
    if (explosionDamage != 0) {
        color.rgb = vec3(1.0, 0.25, 0.0);
    }
    physical.Draw(context, color);
}

template struct DoubleBufferArray<Bullet>;

void Wind::EventCreate() {
    physical.type = CIRCLE;
    physical.basis.circle.c = vec2(random(-8.0, 8.0, globals->rng), random(-8.0, 8.0, globals->rng));
    physical.basis.circle.r = random(16.0, 32.0, globals->rng);
    physical.angle = random(0.0, tau, globals->rng);
    physical.rot = random(-tau, tau, globals->rng);
}

void Wind::Update(f32 timestep) {
    physical.Update(timestep);
    physical.UpdateActual();
    lifetime -= timestep;
    if (lifetime <= 0.0) {
        globals->entities.winds.Destroy(id);
    }
}

void Wind::Draw(Rendering::DrawingContext &context) {
    vec4 color = vec4(1.0, 1.0, 1.0, clamp(0.0, 0.1, lifetime * 0.1));
    const f32 z = globals->entities.camZoom;
    const vec2 p = (physical.pos - globals->entities.camPos) * z
                 + vec2(globals->window.width / 2, globals->window.height / 2);
    const vec2 scale = physical.basis.circle.r * 2.0;
    globals->rendering.DrawCircle(context, Rendering::texBlank, color, p, scale * 0.1, vec2(10.0 * z), -physical.basis.circle.c / scale + vec2(0.5), physical.angle);
}

template struct DoubleBufferArray<Wind>;

void Explosion::EventCreate() {
    physical.type = CIRCLE;
    physical.basis.circle.c = vec2(0.0);
    physical.basis.circle.r = 0.0;
}

void Explosion::Update(f32 timestep) {
    // shockwaves have a growth of 5.0
    // bullet explosions have a growth of 8.0
    physical.basis.circle.r = decay(physical.basis.circle.r, size, 1.0 / growth, timestep);
    physical.Update(timestep);
    physical.UpdateActual();
    // Cutoff is after 5 half-lives
    // shockwaves last 1 second
    // bullet explosions last 5/8th seconds
    if (physical.basis.circle.r >= size * 0.9375) {
        globals->entities.explosions.Destroy(id);
    }
}

void Explosion::Draw(Rendering::DrawingContext &context) {
    f32 prog = physical.basis.circle.r / size / 0.9375;
    vec4 color = vec4(
        hsvToRgb(vec3(
            0.5 - prog * 0.5,
            prog,
            1.0
        )),
        clamp((1.0 - prog) * 5.0, 0.0, 0.8)
    );
    const f32 z = globals->entities.camZoom;
    const vec2 p = (physical.pos - globals->entities.camPos) * z
                 + vec2(globals->window.width / 2, globals->window.height / 2);
    const vec2 scale = physical.basis.circle.r * 2.0;
    globals->rendering.DrawCircle(context, Rendering::texBlank, color, p, scale * 0.05, vec2(20.0 * z), -physical.basis.circle.c / scale + vec2(0.5), physical.angle);
}

template struct DoubleBufferArray<Explosion>;

} // namespace Objects
