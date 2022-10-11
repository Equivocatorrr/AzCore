/*
	File: entities.cpp
	Author: Philip Haynes
*/

#include "entities.hpp"
#include "globals.hpp"
#include "AzCore/Thread.hpp"

#include "entity_basics.cpp"

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
	{vec2(-20.0f), vec2(20.0f)},                // PhysicalBasis
	CIRCLE,                                     // fieldCollisionType
	{vec2(0.0f), 320.0f},                       // fieldPhysicalBasis
	TOWER_GUN,                                  // TowerType
	320.0f,                                     // range
	0.25f,                                      // shootInterval
	2.7f,                                       // bulletSpread (degrees)
	1,                                          // bulletCount
	18,                                         // damage
	800.0f,                                     // bulletSpeed
	50.0f,                                      // bulletSpeedVariability
	0,                                          // bulletExplosionDamage
	0.0f,                                       // bulletExplosionRange
	vec4(0.1f, 0.5f, 1.0f, 1.0f)                // color
);

const Tower towerShotgunTemplate = Tower(
	BOX,                                        // CollisionType
	{vec2(-16.0f), vec2(16.0f)},                // PhysicalBasis
	CIRCLE,                                     // fieldCollisionType
	{vec2(0.0f), 200.0f},                       // fieldPhysicalBasis
	TOWER_SHOTGUN,                              // TowerType
	200.0f,                                     // range
	1.0f,                                       // shootInterval
	12.0f,                                      // bulletSpread (degrees)
	12,                                         // bulletCount
	18,                                         // damage
	900.0f,                                     // bulletSpeed
	200.0f,                                     // bulletSpeedVariability
	0,                                          // bulletExplosionDamage
	0.0f,                                       // bulletExplosionRange
	vec4(0.1f, 1.0f, 0.5f, 1.0f)                // color
);

const Tower towerFanTemplate = Tower(
	BOX,                                        // CollisionType
	{vec2(-10.0f, -32.0f), vec2(10.0f, 32.0f)}, // PhysicalBasis
	BOX,                                        // fieldCollisionType
	{vec2(-50.0f, -40.0f), vec2(300.0f, 40.0f)},// fieldPhysicalBasis
	TOWER_FAN,                                  // TowerType
	300.0f,                                     // range
	0.1f,                                       // shootInterval
	10.0f,                                      // bulletSpread (degrees)
	2,                                          // bulletCount
	10,                                         // damage
	800.0f,                                     // bulletSpeed
	200.0f,                                     // bulletSpeedVariability
	0,                                          // bulletExplosionDamage
	0.0f,                                       // bulletExplosionRange
	vec4(0.5f, 1.0f, 0.1f, 1.0f)                // color
);

const Tower towerGaussTemplate = Tower(
	BOX,                                        // CollisionType
	{vec2(-32.0f), vec2(32.0f)},                // PhysicalBasis
	CIRCLE,                                     // fieldCollisionType
	{vec2(0.0f), 480.0f},                       // fieldPhysicalBasis
	TOWER_GAUSS,                                // TowerType
	400.0f,                                     // range
	1.8f,                                       // shootInterval
	4.8f,                                       // bulletSpread (degrees)
	1,                                          // bulletCount
	1200,                                       // damage
	2000.0f,                                    // bulletSpeed
	0.0f,                                       // bulletSpeedVariability
	0,                                          // bulletExplosionDamage
	0.0f,                                       // bulletExplosionRange
	vec4(0.1f, 1.0f, 0.8f, 1.0f)                // color
);

const Tower towerShockerTemplate = Tower(
	CIRCLE,                                     // CollisionType
	{vec2(0.0f), 16.0f},                        // PhysicalBasis
	CIRCLE,                                     // fieldCollisionType
	{vec2(0.0f), 120.0f},                       // fieldPhysicalBasis
	TOWER_SHOCKWAVE,                            // TowerType
	120.0f,                                     // range
	1.2f,                                       // shootInterval
	0.0f,                                       // bulletSpread (degrees)
	1,                                          // bulletCount
	60,                                         // damage
	1.0f,                                       // bulletSpeed
	0.0f,                                       // bulletSpeedVariability
	0,                                          // bulletExplosionDamage
	0.0f,                                       // bulletExplosionRange
	vec4(1.0f, 0.3f, 0.1f, 1.0f)                // color
);

const Tower towerFlakTemplate = Tower(
	CIRCLE,                                     // CollisionType
	{vec2(0.0f), 32.0f},                        // PhysicalBasis
	CIRCLE,                                     // fieldCollisionType
	{vec2(0.0f), 400.0f},                       // fieldPhysicalBasis
	TOWER_FLAK,                                 // TowerType
	400.0f,                                     // range
	1.8f,                                       // shootInterval
	6.0f,                                       // bulletSpread (degrees)
	5,                                          // bulletCount
	25,                                         // damage
	500.0f,                                     // bulletSpeed
	100.0f,                                     // bulletSpeedVariability
	50,                                         // bulletExplosionDamage
	80.0f,                                      // bulletExplosionRange
	vec4(1.0f, 0.0f, 0.8f, 1.0f)                // color
);

void Manager::EventAssetInit() {
	globals->assets.QueueFile("Money Cursed.ogg");
	globals->assets.QueueFile("Segment 1.ogg", Assets::STREAM);
	globals->assets.QueueFile("Segment 2.ogg", Assets::STREAM);
}

void Manager::EventAssetAcquire() {
	sndMoney.Create("Money Cursed.ogg");
	sndMoney.SetGain(0.5f);
	if (!streamSegment1.Create("Segment 1.ogg")) {
		io::cerr.PrintLn("Failed to create stream for \"Segment 1.ogg\": ", Sound::error);
	}
	if (!streamSegment2.Create("Segment 2.ogg")) {
		io::cerr.PrintLn("Failed to create stream for \"Segment 2.ogg\": ", Sound::error);
	}
}

void Manager::EventInitialize() {
	towers.granularity = 5;
	enemies.granularity = 25;
	bullets.granularity = 50;
	winds.granularity = 50;
	explosions.granularity = 10;
	// Reset();
}

void Manager::Reset() {
	towers.Clear();
	enemies.Clear();
	bullets.Clear();
	winds.Clear();
	explosions.Clear();
	updateChunks.Clear();
	selectedTower = -1;
	focusMenu = false;
	placeMode = false;
	towerType = TOWER_GUN;
	placingAngle = 0.0;
	canPlace = false;
	enemyTimer = 0.0;
	wave = 0;
	hitpointsLeft = 0;
	hitpointsPerSecond = 200.0;
	lives = 1000;
	money = 5000;
	waveActive = true;
	failed = false;
	camZoom = 1.0;
	backgroundTransition = -1.0f;
	backgroundFrom = vec3(215.0f/360.0f, 0.7f, 0.5f);
	backgroundTo = vec3(50.0f/360.0f, 0.5f, 0.5f);
	camPos = 0.0;
	mouse = 0.0;
	failureText.Reset();
	basePhysical.type = CIRCLE;
	basePhysical.basis.circle.c = 0.0f;
	basePhysical.basis.circle.r = 128.0f;
	basePhysical.pos = 0.0f;
	enemySpawns.Clear();
	CreateSpawn();
	camPos = enemySpawns[0].pos * 0.5f;
	camZoom = min(globals->rendering.screenSize.x, globals->rendering.screenSize.y) / 1500.0f;
	HandleMusicLoops(1);
	streamSegment1.Play();
	globals->rendering.backgroundHSV = backgroundFrom;
	globals->rendering.UpdateBackground();
}

inline void Manager::HandleGamepadCamera() {
	vec2 screenBorder = (vec2(globals->window.width, globals->window.height) - vec2(50.0f * globals->gui.scale)) / 2.0f / camZoom;
	if (CursorVisible() || placeMode) {
		vec2 mouseMove = globals->gamepad->axis.vec.RS;
		f32 mag = abs(mouseMove);
		mouseMove *= sqrt(mag);
		mouseMove *= globals->objects.timestep * 800.0f / camZoom;
		mouse += mouseMove;
		if (mouseMove != vec2(0.0f)) {
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
		camMove *= globals->objects.timestep * 800.0f / camZoom;
		camPos += camMove;
		if (camMove != vec2(0.0f)) {
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
		camZoom *= 1.0f + zoomMove;
		screenBorder /= camZoom;
		mouse.x = median(camPos.x-screenBorder.x, mouse.x, camPos.x+screenBorder.x);
		mouse.y = median(camPos.y-screenBorder.y, mouse.y, camPos.y+screenBorder.y);
	} else {
		screenBorder *= camZoom;
		camZoom /= 1.0f - zoomMove;
		screenBorder /= camZoom;
		mouse.x = median(camPos.x-screenBorder.x, mouse.x, camPos.x+screenBorder.x);
		mouse.y = median(camPos.y-screenBorder.y, mouse.y, camPos.y+screenBorder.y);
	}
}

inline void Manager::HandleMouseCamera() {
	if (globals->gui.mouseoverDepth > 0) {
		return;
	}
	bool changed = false;
	if (globals->objects.Pressed(KC_MOUSE_SCROLLUP)) {
		camZoom *= 1.1f;
		changed = true;
	} else if (globals->objects.Pressed(KC_MOUSE_SCROLLDOWN)) {
		camZoom /= 1.1f;
		changed = true;
	}
	if (changed) {
		mouse = ScreenPosToWorld(globals->input.cursor);
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
	if (TypedCode("wave9")) {
		wave = 9;
	}
	if (backgroundTransition >= 0.0f) {
		backgroundTransition += timestep / 30.0f;
		if (backgroundTransition > 1.0f) backgroundTransition = 1.0f;
		globals->rendering.backgroundHSV = lerp(backgroundFrom, backgroundTo, backgroundTransition);
		globals->rendering.UpdateBackground();
		if (backgroundTransition == 1.0f) {
			backgroundTransition = -1.0f;
		}
	}
	if (lives == 0 && !failed) {
		if (streamSegment1.playing) {
			streamSegment1.Stop(2.0f);
		}
		if (streamSegment2.playing) {
			streamSegment2.Stop(2.0f);
		}
		placeMode = false;
		failed = true;
	}
	if (failed) {
		failureText.Update(timestep);
		globals->objects.paused = false;
		return;
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
			if (wave == 11) {
				backgroundTransition = 0.0f;
			}
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
		const Degrees32 increment30(30.0f);
		const Degrees32 increment5(5.0f);
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
		const Degrees32 increment30(30.0f);
		const Degrees32 increment5(5.0f);
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
	} else if (w >= 11 && w <= 20) {
		if (w == 11) {
			streamSegment1.SetLoopRange(0, -1);
		}
		i32 section = 192*4410;
		i32 preLoop = 0;
		w -= 10;
		streamSegment2.SetLoopRange(w*section+preLoop, (w+1)*section);
	} else if (w >= 21 && w <= 30) {
		if (w == 21) {
			streamSegment2.SetLoopRange(0, -1);
		}
	}
}

inline bool Manager::CursorVisible() const {
	return globals->gui.currentMenu == Int::MENU_PLAY && !globals->gui.usingMouse && !placeMode && !focusMenu && selectedTower == -1;
}

void Manager::EventSync() {
	if (globals->gui.mainMenu.buttonNewGame->state.Released()) {
		globals->gui.mainMenu.buttonNewGame->state.Set(false, false, false);
		Reset();
	}
	timestep = globals->objects.timestep * globals->objects.simulationRate;
	if (globals->input.Down(KC_KEY_F)) timestep *= 2.0f;
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
	if (globals->input.Pressed(KC_KEY_R)) {
		failureText.Reset();
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

	if (timestep != 0.0f && hitpointsLeft > 0 && waveActive) {
		enemyTimer -= timestep;
		if (enemies.count == 0) {
			enemyTimer = 0.0f;
		}
		while (enemyTimer <= 0.0f && hitpointsLeft > 0) {
			Enemy enemy;
			for (i32 i = 0; i < 3; i++) {
				enemy.type = (Enemy::Type)random(0, 3, &globals->rng);
				if (enemy.type != Enemy::HONKER) break;
			}
			enemies.Create(enemy); // Enemy::EventCreate() increases enemyTimer based on HP
		}
	}
	if (hitpointsLeft == 0 && waveActive && enemies.count == 0
	 && !globals->gui.playMenu.buttonStartWave->state.Released()) {
		waveActive = false;
		wave++;
		HandleMusicLoops(wave);
		f64 factor = pow((f64)1.2, (f64)(wave+3));
		hitpointsPerSecond = (f64)((i64)(factor * 5.0) * 100);
		hitpointsLeft += (i64)hitpointsPerSecond;
		// Average wave length is wave+7 seconds
		hitpointsPerSecond /= wave+7;
		globals->gui.playMenu.buttonStartWave->string = globals->ReadLocale("Start Wave");
	}
	if (wave >= 11 && wave < 20) {
		if (!streamSegment1.playing && !streamSegment2.playing) {
			streamSegment2.Play();
		}
	}
	readyForDraw = true;
}

void Manager::EventUpdate() {
	if (timestep != 0.0f) {
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
		tower.physical.Draw(contexts.Back(), canPlace ? vec4(0.1f, 1.0f, 0.1f, 0.9f) : vec4(1.0f, 0.1f, 0.1f, 0.9f));
		tower.field.pos = tower.physical.pos;
		tower.field.angle = tower.physical.angle;
		tower.field.Draw(contexts.Back(), canPlace ? vec4(1.0f, 1.0f, 1.0f, 0.1f) : vec4(1.0f, 0.5f, 0.5f, 0.2f));
	}
	if (selectedTower != -1) {
		const Tower& selected = towers[selectedTower];
		selected.field.Draw(contexts.Back(), vec4(1.0f, 1.0f, 1.0f, 0.1f));
	}
	basePhysical.Draw(contexts.Back(), vec4(hsvToRgb(vec3((f32)lives / 3000.0f, 1.0f, 0.8f)), 1.0f));
	for (i32 i = 0; i < enemySpawns.size; i++) {
		enemySpawns[i].Draw(contexts.Back(), vec4(vec3(0.0f), 1.0f));
	}
	if (CursorVisible()) {
		vec2 cursor = WorldPosToScreen(mouse);
		globals->rendering.DrawQuad(contexts.Back(), globals->gui.cursorIndex, vec4(1.0f), cursor, vec2(32.0f * globals->gui.scale), vec2(1.0f), vec2(0.5f));
	}
	if (lives == 0) {
		failureText.Draw(contexts.Back());
	}
}

void Manager::CreateSpawn() {
	f32 angle = random(0.0f, tau, &globals->rng);
	vec2 place(sin(angle), cos(angle));
	place *= 1500.0f;
	Physical newSpawn;
	newSpawn.type = BOX;
	newSpawn.basis.box.a = vec2(-128.0f, -32.0f);
	newSpawn.basis.box.b = vec2(128.0f, 32.0f);
	newSpawn.pos = place;
	newSpawn.angle = angle + pi;
	enemySpawns.Append(newSpawn);
}

vec2 Manager::WorldPosToScreen(vec2 in) const {
	vec2 out = (in - camPos) * camZoom + vec2(globals->window.width, globals->window.height) / 2.0f;
	return out;
}
vec2 Manager::ScreenPosToWorld(vec2 in) const {
	vec2 out = vec2(in - vec2(globals->window.width, globals->window.height) / 2.0f) / camZoom + camPos;
	return out;
}

void FailureText::Reset() {
	angle = Radians32(Degrees32(random(-180.0f, 180.0f, &globals->rng))).value();
	position = vec2(cos(angle), sin(angle)) * 0.5;
	size = 0.001f;
	velocity = -position * 15.0;
	rotation = 0.0f;
	scaleSpeed = 1.0f;
	targetPosition = vec2(random(-0.25f, 0.25f, &globals->rng), random(-0.25f, 0.25f, &globals->rng));
	targetAngle = Radians32(Degrees32(random(-30.0f, 30.0f, &globals->rng))).value();
	targetSize = 0.3f;
	text = globals->ReadLocale("Game Over");
}

void FailureText::Update(f32 timestep) {
	const f32 rate = 30.0f;
	velocity   += (targetPosition - position) * timestep * rate;
	rotation   += (targetAngle - angle) * timestep * rate;
	scaleSpeed += (targetSize - size) * timestep * rate;
	velocity   = decay(velocity,   vec2(0.0f), 0.125f, timestep);
	rotation   = decay(rotation,   0.0f,       0.125f, timestep);
	scaleSpeed = decay(scaleSpeed, 0.0f,       0.125f, timestep);

	position += velocity * timestep;
	angle += rotation * timestep;
	size += scaleSpeed * timestep;
}

void FailureText::Draw(Rendering::DrawingContext &context) {
	globals->rendering.DrawTextSS(context, text, globals->gui.fontIndex, vec4(vec3(0.0f), 1.0f), position, size, Rendering::CENTER, Rendering::CENTER, 0.0f, 0.5f, 0.325f, angle);
	globals->rendering.DrawTextSS(context, text, globals->gui.fontIndex, vec4(1.0f, 0.0f, 0.0f, 1.0f), position, size, Rendering::CENTER, Rendering::CENTER, 0.0f, 0.5f, 0.525f, angle);
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
			physical.basis.circle.c = vec2(0.0f);
			physical.basis.circle.r = 16.0f;
			color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
			shootInterval = 0.8f;
			bulletSpread = 15.0f;
			bulletCount = 10;
			bulletSpeed = 800.0f;
			break;
	}
}

void Tower::EventCreate() {
	selected = false;
	disabled = false;
	shootTimer = 0.0f;
	field.pos = physical.pos;
	field.angle = physical.angle;
	priority = PRIORITY_NEAREST;
	kills = 0;
	damageDone = 0;
}

void Tower::Update(f32 timestep) {
	physical.Update(timestep);
	selected = globals->entities.selectedTower == id;
	// if (shootTimer <= 0.0f) disabled = false;
	// for (i32 i = 0; i < globals->entities.enemies.size; i++) {
	//	 const Enemy& other = globals->entities.enemies[i];
	//	 if (other.id.generation < 0 || other.hitpoints <= 2000) continue;
	//	 if (physical.Collides(other.physical)) {
	//		 disabled = true;
	//		 shootTimer = 0.5f;
	//		 break;
	//	 }
	// }
	shootTimer = max(shootTimer-timestep, -timestep);
	if (disabled) return;
	if (shootTimer <= 0.0f) {
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
					targetDist = 0.0f;
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
					f32 youngest = 1000000.0f;
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
					f32 oldest = 0.0f;
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
				bullet.lifetime = range / (bulletSpeed * 0.9f);
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
					Angle32 angle = idealAngle + Degrees32(random(-bulletSpread.value(), bulletSpread.value(), &globals->rng));
					bullet.physical.vel.x = cos(angle);
					bullet.physical.vel.y = -sin(angle);
					bullet.physical.vel *= bulletSpeed + random(-bulletSpeedVariability, bulletSpeedVariability, &globals->rng);
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
				explosion.growth = 5.0f;
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
			f32 randomPos = random(-20.0f, 20.0f, &globals->rng);
			wind.physical.pos.x += cos(physical.angle.value() + pi * 0.5f) * randomPos;
			wind.physical.pos.y -= sin(physical.angle.value() + pi * 0.5f) * randomPos;
			for (i32 i = 0; i < bulletCount; i++) {
				Angle32 angle = physical.angle + Degrees32(random(-bulletSpread.value(), bulletSpread.value(), &globals->rng));
				wind.physical.vel.x = cos(angle);
				wind.physical.vel.y = -sin(angle);
				wind.physical.vel *= bulletSpeed + random(-bulletSpeedVariability, bulletSpeedVariability, &globals->rng);
				wind.physical.pos += wind.physical.vel * 0.03f;
				globals->entities.winds.Create(wind);
			}
		}
	}
}

void Tower::Draw(Rendering::DrawingContext &context) {
	vec4 colorTemp;
	if (selected) {
		colorTemp = vec4(0.5f) + color * 0.5f;
	} else {
		colorTemp = color;
	}
	if (disabled) {
		colorTemp.rgb = (colorTemp.rgb + vec3(0.8f * 3.0f)) / 4.0f;
	}
	physical.Draw(context, colorTemp);
}

template struct DoubleBufferArray<Tower>;

const f32 honkerSpawnInterval = 2.0f;

vec2 GetSpawnLocation() {
	i32 spawnPoint = random(0, globals->entities.enemySpawns.size-1, &globals->rng);
	f32 s, c;
	s = sin(globals->entities.enemySpawns[spawnPoint].angle);
	c = cos(globals->entities.enemySpawns[spawnPoint].angle);
	vec2 x, y;
	x = vec2(c, -s) * globals->entities.enemySpawns[spawnPoint].basis.box.b.x
	  * random(-1.0f, 1.0f, &globals->rng);
	y = vec2(s, c) * globals->entities.enemySpawns[spawnPoint].basis.box.b.y
	  * random(-1.0f, 1.0f, &globals->rng);
	return globals->entities.enemySpawns[spawnPoint].pos + x + y;
}

void Enemy::EventCreate() {
	physical.type = CIRCLE;
	physical.basis.circle.c = vec2(0.0f);
	physical.basis.circle.r = 0.0f;
	i32 multiplier = 1;
	if (!child) {
		physical.pos = GetSpawnLocation();
		physical.vel = vec2(random(-2.0f, 2.0f, &globals->rng), random(-2.0f, 2.0f, &globals->rng));
		switch (type) {
			case BASIC:
			case STUNNER:
				multiplier = random(1, 3, &globals->rng);
				break;
			case HONKER: {
				f32 honker = random(0.0f, 100.0f, &globals->rng);
				if (honker < 1.0f) {
					multiplier = 1000;
				} else if (honker < 10.0f) {
					multiplier = 500;
				} else {
					multiplier = 100;
				}
			} break;
			case ORBITER:
				multiplier = random(1, 2, &globals->rng);
				break;
		}
		hitpoints = multiplier * (i32)floor(80.0f * pow(1.16f, (f32)(globals->entities.wave + 3))) / (globals->entities.wave+7);
		age = 0.0f;
	}
	spawnTimer = honkerSpawnInterval;
	if (!child) {
		i64 limit = median(
			globals->entities.hitpointsLeft / 2,
			(i64)500,
			globals->entities.hitpointsLeft
		);
		if (hitpoints > limit) {
			hitpoints = limit;
		}
		globals->entities.hitpointsLeft -= hitpoints;
		color = vec4(hsvToRgb(vec3(
			sqrt((f32)hitpoints)/(tau*16.0f) + (f32)globals->entities.wave / 9.0f,
			min((f32)hitpoints / 100.0f, 1.0f),
			1.0f
		)), 0.7f);
	}
	value = hitpoints;
	f64 speedDivisor = max(log10((f64)multiplier), 1.0);
	targetSpeed = f32(200.0 / speedDivisor);
	if (type == ORBITER) {
		targetSpeed *= 2.0f;
	}
	size = 0.0f;
	if (!child) {
		globals->entities.enemyTimer += f32((f64)hitpoints / globals->entities.hitpointsPerSecond / speedDivisor);
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

inline i32 DamageOverTime(i32 dps, f32 timestep) {
	f32 prob = (f32)dps*timestep;
	i32 hits = (i32)prob;
	prob -= (f32)hits;
	if (random(0.0f, 1.0f, &globals->rng) <= prob) {
		hits++;
	}
	return hits;
}

void Enemy::Update(f32 timestep) {
	age += timestep;
	if (hitpoints > 0) {
		size = decay(size, (f32)hitpoints, 0.1f, timestep);
	} else {
		size = decay(size, 0.0f, 0.025f, timestep);
	}
	physical.basis.circle.r = sqrt(size) + 2.0f;
	physical.Update(timestep);
	physical.UpdateActual();
	if (physical.Collides(globals->entities.basePhysical) || (hitpoints <= 0 && size < 0.01f)) {
		if (hitpoints > 0) {
			globals->entities.lives = max(globals->entities.lives-hitpoints, (i64)0);
		}
		globals->entities.enemies.Destroy(id);
	}
	if (hitpoints == 0) return;
	if (type == HONKER) {
		if (spawnTimer <= 0.0f) {
			Enemy newEnemy;
			newEnemy.type = ORBITER;
			newEnemy.child = true;
			newEnemy.age = age;
			Angle32 spawnAngle = random(0.0f, tau, &globals->rng);
			vec2 spawnVector = vec2(cos(spawnAngle), -sin(spawnAngle)) * sqrt(random(0.0f, 1.0f, &globals->rng));
			newEnemy.physical.pos = physical.pos + spawnVector * physical.basis.circle.r;
			newEnemy.physical.vel = physical.vel + spawnVector * 100.0f;
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
				* max((other.range + physical.basis.circle.r - abs(deltaP)), 0.0f)
				* (type == HONKER? 0.1f : 5.0f), timestep);
			if (other.damage != 0) {
				i32 hits = DamageOverTime(other.damage, timestep);
				if (hits) {
					damageContributors.Emplace(other.id);
					other.damageDone += hits;
					hitpoints -= hits;
				}
			}
		}
	}
	for (i32 i = 0; i < globals->entities.explosions.size; i++) {
		const Explosion &other = globals->entities.explosions[i];
		if (other.id.generation < 0) continue;
		if (physical.Collides(other.physical)) {
			vec2 deltaP = physical.pos - other.physical.pos;
			physical.Impulse(normalize(deltaP) * max((other.size + physical.basis.circle.r - abs(deltaP)), 0.0f) * 500.0f / pow(size, 1.5f), timestep);
			if (other.damage != 0) {
				i32 hits = DamageOverTime(other.damage, timestep);
				if (hits) {
					damageContributors.Emplace(other.owner);
					globals->entities.towers.GetMutable(other.owner).damageDone += hits;
					hitpoints -= hits;
				}
			}
		}
	}
	for (i32 i = 0; i < globals->entities.bullets.size; i++) {
		Bullet &other = globals->entities.bullets.GetMutable(i);
		if (other.id.generation < 0) continue;
		if (physical.Collides(other.physical)) {
			damageContributors.Emplace(other.owner);
			if (other.damage > hitpoints) {
				other.damage -= hitpoints;
				globals->entities.towers.GetMutable(other.owner).damageDone += hitpoints;
				hitpoints = 0;
			} else {
				globals->entities.bullets.Destroy(other.id);
				hitpoints -= other.damage;
				globals->entities.towers.GetMutable(other.owner).damageDone += other.damage;
				physical.vel += normalize(other.physical.vel) * 100.0f / size;
			}
		}
	}
	vec2 norm = normalize(-physical.pos);
	f32 velocity = abs(physical.vel);
	f32 forward = dot(norm, physical.vel/velocity);
	const f32 outerMost = cos(Radians32(Degrees32(72.0f)).value());
	if (forward < outerMost) {
		physical.vel += norm * (outerMost - forward) * velocity;
	}
	if (type == ORBITER) {
		const f32 innerMost = cos(Radians32(Degrees32(62.0f)).value());
		if (forward > innerMost) {
			physical.vel += norm * (innerMost - forward) * velocity;
		}
	} else {
		physical.Impulse(norm * targetSpeed, timestep);
	}
	physical.vel = normalize(physical.vel) * targetSpeed;
}

void Enemy::Draw(Rendering::DrawingContext &context) {
	physical.Draw(context, color * vec4(vec3(1.0f), clamp(size, 0.0f, 1.0f)));
}

template struct DoubleBufferArray<Enemy>;

void Bullet::EventCreate() {
	f32 length = abs(physical.vel) * 0.5f / 30.0f;
	physical.type = SEGMENT;
	physical.basis.segment.a = vec2(-length, -1.0f);
	physical.basis.segment.b = vec2(length, 1.0f);
	physical.angle = atan2(-physical.vel.y, physical.vel.x);
}

void Bullet::EventDestroy() {
	if (explosionRange != 0.0f) {
		Explosion explosion;
		explosion.damage = explosionDamage;
		explosion.size = explosionRange;
		explosion.growth = 8.0f;
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
	if (lifetime <= 0.0f) {
		globals->entities.bullets.Destroy(id);
	}
}

void Bullet::Draw(Rendering::DrawingContext &context) {
	vec4 color = vec4(1.0f, 1.0f, 0.5f, clamp(0.0f, 1.0f, lifetime * 8.0f));
	if (explosionDamage != 0) {
		color.rgb = vec3(1.0f, 0.25f, 0.0f);
	}
	physical.Draw(context, color);
}

template struct DoubleBufferArray<Bullet>;

void Wind::EventCreate() {
	physical.type = CIRCLE;
	physical.basis.circle.c = vec2(random(-8.0f, 8.0f, &globals->rng), random(-8.0f, 8.0f, &globals->rng));
	physical.basis.circle.r = random(16.0f, 32.0f, &globals->rng);
	physical.angle = random(0.0f, tau, &globals->rng);
	physical.rot = random(-tau, tau, &globals->rng);
}

void Wind::Update(f32 timestep) {
	physical.Update(timestep);
	physical.UpdateActual();
	lifetime -= timestep;
	if (lifetime <= 0.0f) {
		globals->entities.winds.Destroy(id);
	}
}

void Wind::Draw(Rendering::DrawingContext &context) {
	vec4 color = vec4(1.0f, 1.0f, 1.0f, clamp(0.0f, 0.1f, lifetime * 0.1f));
	const f32 z = globals->entities.camZoom;
	const vec2 p = (physical.pos - globals->entities.camPos) * z
				 + vec2(globals->window.width / 2, globals->window.height / 2);
	const vec2 scale = physical.basis.circle.r * 2.0f;
	globals->rendering.DrawCircle(context, Rendering::texBlank, color, p, scale * 0.1f, vec2(10.0f * z), -physical.basis.circle.c / scale + vec2(0.5f), physical.angle);
}

template struct DoubleBufferArray<Wind>;

void Explosion::EventCreate() {
	physical.type = CIRCLE;
	physical.basis.circle.c = vec2(0.0f);
	physical.basis.circle.r = 0.0f;
}

void Explosion::Update(f32 timestep) {
	// shockwaves have a growth of 5.0
	// bullet explosions have a growth of 8.0
	physical.basis.circle.r = decay(physical.basis.circle.r, size, 1.0f / growth, timestep);
	physical.Update(timestep);
	physical.UpdateActual();
	// Cutoff is after 5 half-lives
	// shockwaves last 1 second
	// bullet explosions last 5/8th seconds
	if (physical.basis.circle.r >= size * 0.9375f) {
		globals->entities.explosions.Destroy(id);
	}
}

void Explosion::Draw(Rendering::DrawingContext &context) {
	f32 prog = physical.basis.circle.r / size / 0.9375f;
	vec4 color = vec4(
		hsvToRgb(vec3(
			0.5f - prog * 0.5f,
			prog,
			1.0f
		)),
		clamp((1.0f - prog) * 5.0f, 0.0f, 0.8f)
	);
	const f32 z = globals->entities.camZoom;
	const vec2 p = (physical.pos - globals->entities.camPos) * z
				 + vec2(globals->window.width / 2, globals->window.height / 2);
	const vec2 scale = physical.basis.circle.r * 2.0f;
	globals->rendering.DrawCircle(context, Rendering::texBlank, color, p, scale * 0.05f, vec2(20.0f * z), -physical.basis.circle.c / scale + vec2(0.5f), physical.angle);
}

template struct DoubleBufferArray<Explosion>;

} // namespace Objects
