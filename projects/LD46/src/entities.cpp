/*
	File: entities.cpp
	Author: Philip Haynes
*/

#include "entities.hpp"
#include "globals.hpp"
#include "AzCore/Thread.hpp"
#include "AzCore/IO/Log.hpp"

#include "entity_basics.cpp"

namespace Entities {

	AzCore::io::Log cout("entities.log", true, true);

template<typename T>
inline void ApplyFriction(T &obj, f32 friction, f32 timestep) {
	f32 mag = abs(obj);
	if (mag > friction * timestep) {
		obj -= obj * (friction * timestep / mag);
	} else {
		obj = T(0);
	}
}

void Manager::EventAssetInit() {
	globals->assets.QueueFile("Jump.png");
	globals->assets.QueueFile("Float.png");
	globals->assets.QueueFile("Run1.png");
	globals->assets.QueueFile("Run2.png");
	globals->assets.QueueFile("Wall_Touch.png");
	globals->assets.QueueFile("Wall_Back.png");
	globals->assets.QueueFile("Lantern.png");
	globals->assets.QueueFile("beacon.png");
	globals->assets.QueueFile("sprinkler.png");

	globals->assets.QueueFile("step-01.ogg");
	globals->assets.QueueFile("step-02.ogg");
	globals->assets.QueueFile("step-03.ogg");
	globals->assets.QueueFile("step-04.ogg");
	globals->assets.QueueFile("step-05.ogg");
	globals->assets.QueueFile("step-06.ogg");
	globals->assets.QueueFile("step-07.ogg");
	globals->assets.QueueFile("step-08.ogg");

	globals->assets.QueueFile("jump-01.ogg");
	globals->assets.QueueFile("jump-02.ogg");
	globals->assets.QueueFile("jump-03.ogg");
	globals->assets.QueueFile("jump-04.ogg");

	globals->assets.QueueFile("jump2-01.ogg");
	globals->assets.QueueFile("jump2-02.ogg");
	globals->assets.QueueFile("jump2-03.ogg");

	globals->assets.QueueFile("music.ogg", Assets::STREAM);
}

void Manager::EventAssetAcquire() {
	playerJump = globals->assets.FindMapping("Jump.png");
	playerFloat = globals->assets.FindMapping("Float.png");
	playerStand = globals->assets.FindMapping("Run1.png");
	playerRun = globals->assets.FindMapping("Run2.png");
	playerWallTouch = globals->assets.FindMapping("Wall_Touch.png");
	playerWallBack = globals->assets.FindMapping("Wall_Back.png");
	lantern = globals->assets.FindMapping("Lantern.png");
	beacon = globals->assets.FindMapping("beacon.png");
	sprinkler = globals->assets.FindMapping("sprinkler.png");

	stepSources[0].Create("step-01.ogg");
	stepSources[1].Create("step-02.ogg");
	stepSources[2].Create("step-03.ogg");
	stepSources[3].Create("step-04.ogg");
	stepSources[4].Create("step-05.ogg");
	stepSources[5].Create("step-06.ogg");
	stepSources[6].Create("step-07.ogg");
	stepSources[7].Create("step-08.ogg");

	for (i32 i = 0; i < 8; i++) {
		step.sources.Append(stepSources + i);
	}

	jump1Sources[0].Create("jump-01.ogg");
	jump1Sources[1].Create("jump-02.ogg");
	jump1Sources[2].Create("jump-03.ogg");
	jump1Sources[3].Create("jump-04.ogg");

	for (i32 i = 0; i < 4; i++) {
		jump1.sources.Append(jump1Sources + i);
	}

	jump2Sources[0].Create("jump2-01.ogg");
	jump2Sources[1].Create("jump2-02.ogg");
	jump2Sources[2].Create("jump2-03.ogg");

	for (i32 i = 0; i < 3; i++) {
		jump2.sources.Append(jump2Sources + i);
	}

	music.Create("music.ogg");
	music.SetLoopRange(44100*8, 44100*48);
}

void Manager::EventInitialize() {
	Array<char> levels = FileContents("data/levels.txt");
	Array<SimpleRange<char>> lines = SeparateByNewlines(levels);
	for (i32 i = 0; i < lines.size; i++) {
		if (lines[i].size == 0) continue;
		if (lines[i][0] == '#') continue;
		levelNames += lines[i];
		cout.PrintLn("Added level \"", lines[i], "\"");
	}

	failureText.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	failureText.text = globals->ReadLocale("Flameout!");
	successText.color = vec4(1.0f, 0.25f, 0.0f, 1.0f);
	successText.text = globals->ReadLocale("Beacon Lit!");
	winText.color = vec4(0.0f, 1.0f, 1.0f, 1.0f);
	winText.text = globals->ReadLocale("Message Received!");
}

void Manager::Reset() {
	players.Clear();
	sprinklers.Clear();
	droplets.Clear();
	flames.Clear();
	updateChunks.Clear();
	camZoom = 1.0;
	mouse = 0.0;
	gas = 15.0f;
	flame = 1.0f;
	goalFlame = 0.0f;
	failureText.Reset();
	successText.Reset();
	winText.Reset();
	camPos = vec2(world.size)*16.0f;
	goalPos = 0.0f;
	nextLevelTimer = 0.0f;
	if (globals->gui.currentMenu != Int::MENU_EDITOR) {
		camZoom = 1.5;
		for (i32 y = 0; y < world.size.y; y++) {
			for (i32 x = 0; x < world.size.x; x++) {
				vec2i pos = vec2i(x, y);
				u8 b = world[pos];
				if (b == World::BLOCK_PLAYER) {
					Player player;
					player.physical.pos = pos * 32;
					players.Create(player);
				} else if (b == World::BLOCK_GOAL) {
					goalPos = vec2(pos * 32) + vec2(16.0f);
				} else if (b == World::BLOCK_SPRINKLER) {
					Sprinkler sprinkler;
					sprinkler.physical.pos = vec2(pos * 32) + vec2(16.0f, 19.0f);
					sprinklers.Create(sprinkler);
				}
			}
		}
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
		HandleMouseUI();
	} else {
		HandleGamepadUI();
	}
	if (flame <= 0.0f) {
		failureText.Update(timestep);
		globals->objects.paused = false;
	}
	if (goalFlame > 0.5f) {
		if (level == levelNames.size-1) {
			winText.Update(timestep);
		} else {
			successText.Update(timestep);
		}
		nextLevelTimer += timestep;
		globals->objects.paused = false;
	}
	if (nextLevelTimer >= 3.0f) {
		if (level < levelNames.size-1) {
			level++;
			world.Load(levelNames[level]);
			if (random(0, 1, &globals->rng) == 1) {
				jump = &jump1;
			} else {
				jump = &jump2;
			}
			Reset();
		} else {
			// Start the ending cutscene
			globals->gui.mainMenu.continueHideable->hidden = true;
			globals->gui.nextMenu = Int::MENU_OUTTRO;
			globals->gui.cutsceneMenu.Begin();
			music.Stop(2.0f);
		}
	}
	if (globals->gui.playMenu.buttonReset->state.Released()) {
		Reset();
	}
}

inline void Manager::HandleGamepadUI() {

}

inline void Manager::HandleMouseUI() {

}

inline bool Manager::CursorVisible() const {
	return globals->gui.currentMenu == Int::MENU_PLAY;
}

void Manager::EventSync() {
	if (globals->gui.mainMenu.buttonContinue->state.Released()) {
		globals->gui.mainMenu.buttonContinue->state.Set(false, false, false);
	}
	if (globals->gui.mainMenu.buttonNewGame->state.Released()) {
		globals->gui.mainMenu.buttonNewGame->state.Set(false, false, false);
		level = 0;
		if (level == 0) {
			music.Play();
		}
		world.Load(levelNames[level]);
		Reset();
	}
	if (globals->gui.mainMenu.buttonLevelEditor->state.Released() && globals->gui.currentMenu == Int::MENU_EDITOR) {
		globals->gui.mainMenu.buttonLevelEditor->state.Set(false, false, false);
		Reset();
	}
	timestep = globals->objects.timestep * globals->objects.simulationRate;
	mouse = ScreenPosToWorld(globals->input.cursor);
	if (globals->gui.currentMenu == Int::MENU_PLAY) {
		HandleUI();
		if (goalFlame > 0.0f) {
			goalFlame = min(1.0f, goalFlame + timestep * 0.5f);
			if (flameTimer > 0.0f) flameTimer -= timestep;
			while (flameTimer <= 0.0f) {
				Flame flame;
				vec2 offset = vec2(random(-12.0f, 12.0f, &globals->rng), random(-8.0f, 16.0f, &globals->rng));
				flame.physical.pos = goalPos + offset;
				flame.physical.vel = 0.0f;
				flame.size = goalFlame;
				globals->entities.flames.Create(flame);
				flameTimer += 0.002f;
			}
		}
		if (players.count > 0) {
			vec2 targetPos = players[0].physical.pos;
			targetPos.x += (players[0].facingRight ? 1.0f : -1.0f) * globals->rendering.screenSize.x / 8.0f / camZoom;
			camPos = decay(camPos, targetPos, 0.5f, timestep);
		}
	} else if (globals->gui.currentMenu == Int::MENU_EDITOR) {
		// Camera
		if (globals->objects.Down(KC_KEY_UP)) {
			camPos.y -= 1000.0f * globals->objects.timestep;
		}
		if (globals->objects.Down(KC_KEY_DOWN)) {
			camPos.y += 1000.0f * globals->objects.timestep;
		}
		if (globals->objects.Down(KC_KEY_LEFT)) {
			camPos.x -= 1000.0f * globals->objects.timestep;
		}
		if (globals->objects.Down(KC_KEY_RIGHT)) {
			camPos.x += 1000.0f * globals->objects.timestep;
		}
		// Placing of blocks
		if (globals->gui.mouseoverWidget == nullptr) {
			toPlace = (World::Block)Int::EditorMenu::blockTypes[globals->gui.editorMenu.switchBlock->choice];
			vec2i pos = vec2i(mouse)/32;
			if (globals->objects.Down(KC_MOUSE_LEFT)) {
				if (pos.x >= 0 && pos.y >= 0 && pos.x < world.size.x && pos.y < world.size.y) {
					world[pos] = toPlace;
				}
				if (toPlace == World::BLOCK_WATER_TOP) {
					pos.y += 1;
					if (pos.x >= 0 && pos.y >= 0 && pos.x < world.size.x && pos.y < world.size.y) {
						if (world[pos] == World::BLOCK_WATER_TOP) {
							world[pos] = World::BLOCK_WATER_FULL;
						}
					}
				}
			}
			if (globals->objects.Down(KC_MOUSE_RIGHT)) {
				if (pos.x >= 0 && pos.y >= 0 && pos.x < world.size.x && pos.y < world.size.y) {
					world[pos] = World::BLOCK_AIR;
				}
				pos.y += 1;
				if (pos.x >= 0 && pos.y >= 0 && pos.x < world.size.x && pos.y < world.size.y) {
					if (world[pos] == World::BLOCK_WATER_FULL) {
						world[pos] = World::BLOCK_WATER_TOP;
					}
				}
			}
		}
	}
	players.Synchronize();
	sprinklers.Synchronize();
	droplets.Synchronize();
	flames.Synchronize();

	updateChunks.size = 0;

	players.GetUpdateChunks(updateChunks);
	sprinklers.GetUpdateChunks(updateChunks);
	droplets.GetUpdateChunks(updateChunks);
	flames.GetUpdateChunks(updateChunks);

	readyForDraw = true;
}

void Manager::EventUpdate() {
	if (timestep != 0.0f) {
		const i32 concurrency = 2;
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
	if (flame > 0.0f) {
		vec4 color = vec4(1.0f, 1.0f, 0.5f, flame*0.5f);
		f32 scale = flame * 400.0f;
		globals->rendering.DrawCircle(contexts[0], Rendering::texBlank, color, WorldPosToScreen(lanternPos), vec2(2.1f), vec2(scale), vec2(0.5f));
	}
	if (goalFlame > 0.0f) {
		vec4 color = vec4(1.0f, 1.0f, 0.5f, goalFlame*0.5f);
		f32 scale = goalFlame * 600.0f;
		globals->rendering.DrawCircle(contexts[0], Rendering::texBlank, color, WorldPosToScreen(goalPos), vec2(2.1f), vec2(scale), vec2(0.5f));
	}
	world.Draw(contexts[0], globals->gui.currentMenu != Int::MENU_EDITOR, true);
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

	world.Draw(contexts.Back(), globals->gui.currentMenu != Int::MENU_EDITOR, false);
	if (flame <= 0.0f) {
		failureText.Draw(contexts.Back());
	}
	if (goalFlame > 0.5f) {
		if (level == levelNames.size-1) {
			winText.Draw(contexts.Back());
		} else {
			successText.Draw(contexts.Back());
		}
	}
}

vec2 Manager::WorldPosToScreen(vec2 in) const {
	vec2 out = (in - camPos) * camZoom + vec2(globals->window.width, globals->window.height) / 2.0f;
	return out;
}
vec2 Manager::ScreenPosToWorld(vec2 in) const {
	vec2 out = (in - vec2(vec2i(globals->window.width, globals->window.height) / 2)) / camZoom + camPos;
	return out;
}

void MessageText::Reset() {
	angle = Radians32(Degrees32(random(-180.0f, 180.0f, &globals->rng))).value();
	position = vec2(cos(angle), sin(angle)) * 0.5;
	size = 0.001f;
	velocity = -position * 15.0;
	rotation = 0.0f;
	scaleSpeed = 1.0f;
	targetPosition = vec2(random(-0.25f, 0.25f, &globals->rng), random(-0.25f, 0.25f, &globals->rng));
	targetAngle = Radians32(Degrees32(random(-30.0f, 30.0f, &globals->rng))).value();
	targetSize = 0.3f;
}

void MessageText::Update(f32 timestep) {
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

void MessageText::Draw(Rendering::DrawingContext &context) {
	globals->rendering.DrawTextSS(context, text, globals->gui.fontIndex, vec4(vec3(0.0f), 1.0f), position, size, Rendering::CENTER, Rendering::CENTER, 0.0f, 0.5f, 0.225f, angle);
	globals->rendering.DrawTextSS(context, text, globals->gui.fontIndex, color, position, size, Rendering::CENTER, Rendering::CENTER, 0.0f, 0.5f, 0.425f, angle);
}

void World::Draw(Rendering::DrawingContext &context, bool playing, bool under) {
	for (i32 y = 0; y < size.y; y++) {
		for (i32 x = 0; x < size.x; x++) {
			bool draw = false;
			vec4 color;
			vec2 pos;
			vec2 scale;
			i32 tex = Rendering::texBlank;
			switch (operator[](vec2i(x, y))) {
				case BLOCK_PLAYER:
					if (under && !playing) {
						pos = globals->entities.WorldPosToScreen(vec2(f32(x*32), f32(y*32)) + vec2(2.0f));
						color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
						scale = vec2(28.0f);
						draw = true;
					}
					break;
				case BLOCK_WALL:
					if (under) {
						pos = globals->entities.WorldPosToScreen(vec2(f32(x*32), f32(y*32)));
						color = vec4(vec3(0.0f), 1.0f);
						scale = vec2(32.0f);
						draw = true;
					}
					break;
				case BLOCK_WATER_FULL:
					if (!under) {
						pos = globals->entities.WorldPosToScreen(vec2(f32(x*32), f32(y*32)));
						color = vec4(vec3(0.0f, 0.2f, 1.0f), 0.7f);
						scale = vec2(32.0f);
						draw = true;
					}
					break;
				case BLOCK_WATER_TOP:
					if (!under) {
						pos = globals->entities.WorldPosToScreen(vec2(f32(x*32), f32(y*32)+8.0f));
						color = vec4(vec3(0.0f, 0.2f, 1.0f), 0.7f);
						scale = vec2(32.0f, 24.0f);
						draw = true;
					}
					break;
				case BLOCK_GOAL:
					if (under) {
						pos = globals->entities.WorldPosToScreen(vec2(f32(x*32)+0.5f, f32(y*32)+12.5f));
						color = vec4(1.0f);
						scale = vec2(31.0f, 19.5f);
						tex = globals->entities.beacon;
						draw = true;
					}
					break;
				case BLOCK_SPRINKLER:
					if (under && !playing) {
						pos = globals->entities.WorldPosToScreen(vec2(f32(x*32)+3.5f, f32(y*32)+19.0f));
						color = vec4(1.0f);
						scale = vec2(25.0f, 13.0f);
						tex = globals->entities.sprinkler;
						draw = true;
					}
					break;
			}
			if (draw) {
				globals->rendering.DrawQuad(context, tex, color, pos, scale * globals->entities.camZoom, vec2(1.0f));
			}
		}
	}
}

struct Range2D {
	vec2i min, max;
	Range2D() = default;
	Range2D(AABB aabb, vec2i size) {
		min.x = i32(aabb.minPos.x)/32;
		max.x = i32(aabb.maxPos.x)/32;
		min.y = i32(aabb.minPos.y)/32;
		max.y = i32(aabb.maxPos.y)/32;
		if (min.x < 0) min.x = 0;
		if (min.y < 0) min.y = 0;
		if (max.x >= size.x) max.x = size.x-1;
		if (max.y >= size.y) max.y = size.y-1;
	}
};

bool World::Solid(AABB aabb) {
	Range2D range(aabb, size);
	for (i32 y = range.min.y; y <= range.max.y; y++) {
		for (i32 x = range.min.x; x <= range.max.x; x++) {
			if (operator[](vec2i(x, y)) == BLOCK_WALL) return true;
		}
	}
	return false;
}

bool World::Water(AABB aabb) {
	Range2D range(aabb, size);
	for (i32 y = range.min.y; y <= range.max.y; y++) {
		for (i32 x = range.min.x; x <= range.max.x; x++) {
			u8 block = operator[](vec2i(x, y));
			if (block == BLOCK_WATER_TOP || block == BLOCK_WATER_FULL) return true;
		}
	}
	return false;
}

bool World::Goal(AABB aabb) {
	Range2D range(aabb, size);
	for (i32 y = range.min.y; y <= range.max.y; y++) {
		for (i32 x = range.min.x; x <= range.max.x; x++) {
			u8 block = operator[](vec2i(x, y));
			if (block == BLOCK_GOAL) return true;
		}
	}
	return false;
}

bool World::Save(String filename) {
	filename = "data/levels/" + filename + ".world";
	FILE *file = fopen(filename.data, "wb");
	if (!file) return false;
	fwrite(&size, 4, 2, file);
	fwrite(data.data, 1, data.size, file);
	fclose(file);
	return true;
}

bool World::Load(String filename) {
	filename = "data/levels/" + filename + ".world";
	cout.PrintLn("Loading '", filename, "'");
	FILE *file = fopen(filename.data, "rb");
	if (!file) return false;
	if (fread(&size, 4, 2, file) != 2) {
		size = 0;
		return false;
	}
	data.Resize(size.x * size.y);
	if (fread(data.data, 1, data.size, file) != (size_t)data.size) return false;
	fclose(file);
	return true;
}

void Lantern::Update(f32 timestep) {
	vel = (pos - posPrev) / timestep;
	vec2 v = vec2(cos(angle), -sin(angle));
	if (particleTimer > 0.0f) {
		particleTimer -= timestep;
	}
	if (globals->entities.flame > 0.0f) {
		if (particleTimer <= 0.0f) {
			Flame flame;
			f32 a = random(-pi, pi, &globals->rng);
			vec2 offset = vec2(cos(a), sin(a)) * random(0.0f, 4.0f, &globals->rng);
			flame.physical.pos = pos + offset + v * 14.0f;
			flame.physical.vel = vel * 0.5f;
			flame.size = globals->entities.flame;
			globals->entities.flames.Create(flame);
			particleTimer += 0.02f;
		}
	}

	vec2 deltaVel = vel - velPrev;
	deltaVel = vec2(0.0f, 50.0f) - deltaVel;

	Angle32 impulseAngle = atan2(-deltaVel.y, deltaVel.x);
	rot -= (impulseAngle-angle) * timestep * cos(pi / 4.0f * (1.0f-dot(normalize(deltaVel), v))) * abs(deltaVel);

	ApplyFriction(rot.value(), pi * 2.0f, timestep);
	angle += rot * timestep;
	posPrev = pos;
	globals->entities.lanternPos = pos;
	velPrev = vel;
}

void Lantern::Draw(Rendering::DrawingContext &context) {
	globals->rendering.DrawQuad(context, globals->entities.lantern, vec4(1.0f), globals->entities.WorldPosToScreen(pos), vec2(41.0f, 66.0f) * 0.4f * globals->entities.camZoom, vec2(1.0f), vec2(0.5f, 0.05f), angle.value() + pi / 2.0f);
}

void Player::EventCreate() {
	physical.type = BOX;
	physical.basis.box.a = vec2(0.0f, 0.0f);
	physical.basis.box.b = vec2(32.0f, 32.0f);
	physical.angle = 0.0f;
	lantern.pos = physical.pos;
	lantern.posPrev = physical.pos;
}

void Player::Update(f32 timestep) {
	physical.Update(timestep);
	physical.UpdateActual();
	bool buttonJump = globals->objects.Down(KC_KEY_UP) || globals->objects.Down(KC_KEY_W);
	bool buttonLeft = globals->objects.Down(KC_KEY_LEFT) || globals->objects.Down(KC_KEY_A);
	bool buttonRight = globals->objects.Down(KC_KEY_RIGHT) || globals->objects.Down(KC_KEY_D);
	bool grounded = false;
	bool sliding = false;
	vec2 step = physical.vel * timestep;
	bool jumped = false;
	AABB below = physical.aabb;
	below.minPos.x += 1.0f - step.x;
	below.maxPos.x -= 1.0f + step.x;
	below.minPos.y += 1.0f;
	below.maxPos.y += 1.0f + step.y;
	if (globals->entities.world.Solid(below)) {
		grounded = true;
	}
	if (physical.vel.y > 0.0f) {
		anim = FLOAT;
	}
	f32 friction = grounded? 3000.0f : 500.0f;
	if (!buttonLeft && !buttonRight) {
		ApplyFriction(physical.vel.x, friction, timestep);
	}
	if (physical.vel.x > 0.0f) facingRight = true;
	if (physical.vel.x < 0.0f) facingRight = false;
	f32 moveControl = grounded? 10000.0f : 2500.0f;
	AABB right = physical.aabb;
	right.minPos.y += 1.0f - step.y;
	right.maxPos.y -= 1.0f + step.y;
	right.minPos.x += 1.0f;
	right.maxPos.x += 1.0f + step.x;
	if (globals->entities.world.Solid(right)) {
		if (physical.vel.x > 0.0f) {
			physical.pos.x = 32.0f * round(physical.pos.x/32.0f);
			physical.vel.x = 0.0f;
		}
		if (physical.vel.y > 0.0f) {
			if (buttonJump && buttonLeft) {
				physical.vel = vec2(-400.0f, -800.0f);
				physical.pos += physical.vel * timestep;
				anim = WALL_JUMP;
				jumped = true;
			} else {
				sliding = true;
				anim = WALL_TOUCH;
				facingRight = false;
			}
		}
	} else {
		if (buttonRight) {
			physical.ImpulseX(moveControl / max(physical.vel.x/50.0f, 1.0f), timestep);
		}
	}
	AABB left = physical.aabb;
	left.minPos.y += 1.0f - step.y;
	left.maxPos.y -= 1.0f + step.y;
	left.minPos.x -= 1.0f - step.x;
	left.maxPos.x -= 1.0f;
	if (globals->entities.world.Solid(left)) {
		if (physical.vel.x < 0.0f) {
			physical.pos.x = 32.0f * round(physical.pos.x/32.0f);
			physical.vel.x = 0.0f;
		}
		if (physical.vel.y > 0.0f) {
			if (buttonJump && buttonRight) {
				physical.vel = vec2(400.0f, -800.0f);
				physical.pos += physical.vel * timestep;
				anim = WALL_JUMP;
				jumped = true;
			} else {
				sliding = true;
				anim = WALL_TOUCH;
				facingRight = true;
			}
		}
	} else {
		if (buttonLeft) {
			physical.ImpulseX(-moveControl / max(-physical.vel.x/50.0f, 1.0f), timestep);
		}
	}

	if (!grounded) {
		if (buttonJump || physical.vel.y > 0.0f) {
			physical.ImpulseY(2000.0f, timestep);
		} else {
			physical.ImpulseY(6000.0f, timestep);
		}
		if (sliding) {
			f32 slideFriction;
			if (buttonJump) {
				slideFriction = 1500.0f;
			} else {
				slideFriction = 1000.0f;
			}
			if (physical.vel.y > slideFriction*timestep) {
				physical.ImpulseY(-slideFriction, timestep);
			}
		}
	} else {
		if (physical.vel.y > 0.0f) {
			globals->entities.step.Play(min(physical.vel.y / 2000.0f, 1.0f), 1.0f);
		}
		physical.pos.y = 32.0f * round(physical.pos.y/32.0f);
		physical.vel.y = 0.0f;
		if (buttonJump) {
			physical.vel.y = -800.0f;
			physical.pos.y -= 800.0f * timestep;
			anim = JUMP;
			jumped = true;
		} else {
			anim = RUN;
			if (abs(physical.vel.x) < 100.0f) {
				animTime = 0.0f;
			} else {
				animTime += abs(physical.vel.x)*timestep/100.0f;
				if (animTime > 1.0f) {
					animTime -= 1.0f;
					globals->entities.step.Play(0.5f, 1.0f);
				}
			}
		}
	}

	AABB above = physical.aabb;
	above.minPos.x += 1.0f - step.x;
	above.maxPos.x -= 1.0f + step.x;
	above.minPos.y -= 1.0f - step.y;
	above.maxPos.y -= 2.0f;
	if (globals->entities.world.Solid(above)) {
		physical.pos.y = 32.0f * round(physical.pos.y/32.0f) + 1.0f;
		physical.vel.y = 0.0f;
	}

	AABB smaller = physical.aabb;
	smaller.minPos += 2.0f;
	smaller.maxPos -= 2.0f;
	if (globals->entities.flame > 0.0f) {
		if (globals->entities.world.Water(smaller)) {
			globals->entities.flame -= 12.0f * timestep;
		} else {
			globals->entities.flame = min(min(1.0f, globals->entities.gas), globals->entities.flame + timestep * 0.25f);
		}
		if (globals->entities.world.Goal(smaller)) {
			globals->entities.goalFlame += 0.1f * timestep;
		}

		for (i32 i = 0; i < globals->entities.droplets.size; i++) {
			const Droplet &drop = globals->entities.droplets[i];
			if (drop.id.generation < 0) continue;
			if (physical.Collides(drop.physical)) {
				globals->entities.flame -= 8.0f * timestep;
				break;
			}
		}
	}
	if (jumped) {
		globals->entities.jump->Play(0.5f, random(0.90f, 1.1f, &globals->rng));
	}
	{ // Lantern pos
		vec2 delta = globals->entities.mouse - (physical.pos+vec2(16.0f, 0.0f));
		delta /= 10.0f;
		f32 mag = abs(delta);
		if (mag > 22.0f) {
			delta *= 22.0f / mag;
		}
		lantern.pos = delta + physical.pos + vec2(16.0f, 0.0f);
	}
	lantern.Update(timestep);
}

void Player::Draw(Rendering::DrawingContext &context) {
	vec2 pos = globals->entities.WorldPosToScreen(physical.pos+vec2(facingRight ? 44.0f : -9.0f, -11.0f));
	vec2 scale = vec2(facingRight? -53.0f : 53.0f, 57.0f) * globals->entities.camZoom;
	i32 tex = 0;
	switch (anim) {
		case RUN:
			tex = animTime < 0.5f ? globals->entities.playerStand : globals->entities.playerRun;
			break;
		case JUMP:
			tex = globals->entities.playerJump;
			break;
		case FLOAT:
			tex = globals->entities.playerFloat;
			break;
		case WALL_TOUCH:
			tex = globals->entities.playerWallTouch;
			break;
		case WALL_JUMP:
			tex = globals->entities.playerWallBack;
			break;
	}
	globals->rendering.DrawQuad(context, tex, vec4(1.0f), pos, scale, vec2(1.0f));
	lantern.Draw(context);
}

template struct DoubleBufferArray<Player>;

void Sprinkler::EventCreate() {
}

void Sprinkler::Update(f32 timestep) {
	angle += rot * timestep;
	if (angle.value() >= pi || angle.value() <= 0.0f) {
		rot *= -1.0f;
		angle += rot * timestep;
	}
	if (shootTimer > 0.0f) shootTimer -= timestep;
	while (shootTimer <= 0.0f) {
		Droplet drop;
		drop.physical.pos = physical.pos;
		drop.physical.vel = vec2(cos(angle), -sin(angle)) * 600.0f;
		globals->entities.droplets.Create(drop);
		shootTimer += 0.02f;
	}
}

void Sprinkler::Draw(Rendering::DrawingContext &context) {
	const vec2 scale = vec2(25.0f, 13.0f) * globals->entities.camZoom;
	const vec2 p = globals->entities.WorldPosToScreen(physical.pos) - vec2(scale.x * 0.5f, 0.0f);
	globals->rendering.DrawQuad(context, globals->entities.sprinkler, vec4(1.0f), p, scale, vec2(1.0f));
}

template struct DoubleBufferArray<Sprinkler>;

void Droplet::EventCreate() {
	lifetime = 2.0f;
	physical.type = SEGMENT;
	physical.basis.segment.a = vec2(-4.0f, -1.0f);
	physical.basis.segment.b = vec2(4.0f, 1.0f);
}

void Droplet::Update(f32 timestep) {
	physical.Update(timestep);
	physical.UpdateActual();
	lifetime -= timestep;
	if (lifetime <= 0.0f || globals->entities.world.Solid(physical.aabb)) {
		globals->entities.droplets.Destroy(id);
	}
	physical.ImpulseY(2000.0f, timestep);
	physical.angle = atan2(-physical.vel.y, physical.vel.x);
}

void Droplet::Draw(Rendering::DrawingContext &context) {
	vec4 color = vec4(0.2f, 0.6f, 1.0f, clamp01(lifetime) * 0.1f);
	physical.Draw(context, color);
}

template struct DoubleBufferArray<Droplet>;

void Flame::EventCreate() {
	physical.type = CIRCLE;
	physical.basis.circle.c = vec2(0.0f);
	physical.basis.circle.r = 4.0f + size * 6.0f;
}

void Flame::Update(f32 timestep) {
	physical.basis.circle.r -= 32.0f * timestep;
	physical.Update(timestep);
	physical.UpdateActual();
	ApplyFriction(physical.vel, 1000.0f, timestep);
	physical.ImpulseY(-1500.0f, timestep);
	if (physical.basis.circle.r < 0.5f) {
		globals->entities.flames.Destroy(id);
	}
}

void Flame::Draw(Rendering::DrawingContext &context) {
	f32 s = size * 6.0f + 4.0f;
	f32 prog = (s - physical.basis.circle.r) / s;
	vec4 color = vec4(
		hsvToRgb(vec3(
			0.2f - prog * 0.2f,
			0.8f + prog * 0.2f,
			1.0f
		)),
		clamp(1.0f - prog, 0.0f, 0.25f)
	);
	const f32 z = globals->entities.camZoom;
	const vec2 p = globals->entities.WorldPosToScreen(physical.pos);
	const vec2 scale = physical.basis.circle.r * 2.0f;
	globals->rendering.DrawCircle(context, Rendering::texBlank, color, p, scale * 0.5f, vec2(2.0f * z), -physical.basis.circle.c / scale + vec2(0.5f), physical.angle);
}

template struct DoubleBufferArray<Flame>;

} // namespace Objects
