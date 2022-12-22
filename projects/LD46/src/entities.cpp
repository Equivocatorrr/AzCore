/*
	File: entities.cpp
	Author: Philip Haynes
*/

#include "entities.hpp"
#include "gui.hpp"

#include "Az2D/profiling.hpp"

#include "AzCore/Thread.hpp"
#include "AzCore/IO/Log.hpp"

namespace Az2D::Entities {

using namespace AzCore;

Manager *entities = nullptr;

Manager::Manager() {
	entities = this;
}

using GameSystems::sys;

template<typename T>
inline void ApplyFriction(T &obj, f32 friction, f32 timestep) {
	f32 mag = norm(obj);
	if (mag > friction * timestep) {
		obj -= obj * (friction * timestep / mag);
	} else {
		obj = T(0);
	}
}

void Manager::EventAssetsQueue() {
	sys->assets.QueueFile("Jump.png");
	sys->assets.QueueFile("Float.png");
	sys->assets.QueueFile("Run1.png");
	sys->assets.QueueFile("Run2.png");
	sys->assets.QueueFile("Wall_Touch.png");
	sys->assets.QueueFile("Wall_Back.png");
	sys->assets.QueueFile("Lantern.png");
	sys->assets.QueueFile("beacon.png");
	sys->assets.QueueFile("sprinkler.png");

	sys->assets.QueueFile("step-01.ogg");
	sys->assets.QueueFile("step-02.ogg");
	sys->assets.QueueFile("step-03.ogg");
	sys->assets.QueueFile("step-04.ogg");
	sys->assets.QueueFile("step-05.ogg");
	sys->assets.QueueFile("step-06.ogg");
	sys->assets.QueueFile("step-07.ogg");
	sys->assets.QueueFile("step-08.ogg");

	sys->assets.QueueFile("jump-01.ogg");
	sys->assets.QueueFile("jump-02.ogg");
	sys->assets.QueueFile("jump-03.ogg");
	sys->assets.QueueFile("jump-04.ogg");

	sys->assets.QueueFile("jump2-01.ogg");
	sys->assets.QueueFile("jump2-02.ogg");
	sys->assets.QueueFile("jump2-03.ogg");

	sys->assets.QueueFile("music.ogg", Assets::Type::STREAM);
}

void Manager::EventAssetsAcquire() {
	texPlayerJump = sys->assets.FindTexture("Jump.png");
	texPlayerFloat = sys->assets.FindTexture("Float.png");
	texPlayerStand = sys->assets.FindTexture("Run1.png");
	texPlayerRun = sys->assets.FindTexture("Run2.png");
	texPlayerWallTouch = sys->assets.FindTexture("Wall_Touch.png");
	texPlayerWallBack = sys->assets.FindTexture("Wall_Back.png");
	texLantern = sys->assets.FindTexture("Lantern.png");
	texBeacon = sys->assets.FindTexture("beacon.png");
	texSprinkler = sys->assets.FindTexture("sprinkler.png");

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
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Manager::EventInitialize)
	Array<char> levels = FileContents("data/levels.txt");
	Array<SimpleRange<char>> lines = SeparateByNewlines(levels);
	for (i32 i = 0; i < lines.size; i++) {
		if (lines[i].size == 0) continue;
		if (lines[i][0] == '#') continue;
		levelNames += lines[i];
		az::io::cout.PrintLn("Added level \"", lines[i], "\"");
	}

	failureText.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	failureText.text = sys->ReadLocale("Flameout!");
	successText.color = vec4(1.0f, 0.25f, 0.0f, 1.0f);
	successText.text = sys->ReadLocale("Beacon Lit!");
	winText.color = vec4(0.0f, 1.0f, 1.0f, 1.0f);
	winText.text = sys->ReadLocale("Message Received!");
}

void Manager::Reset() {
	players.Clear();
	sprinklers.Clear();
	droplets.Clear();
	flames.Clear();
	gas = 15.0f;
	flame = 1.0f;
	goalFlame = 0.0f;
	failureText.Reset();
	successText.Reset();
	winText.Reset();
	camPos = vec2(world.size)*16.0f;
	goalPos = 0.0f;
	nextLevelTimer = 0.0f;
	if (Gui::gui->menuCurrent != Gui::Gui::Menu::EDITOR) {
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

inline void Manager::HandleUI() {
	if (flame <= 0.0f) {
		failureText.Update(timestep);
		sys->paused = false;
	}
	if (goalFlame > 0.5f) {
		if (level == levelNames.size-1) {
			winText.Update(timestep);
		} else {
			successText.Update(timestep);
		}
		nextLevelTimer += timestep;
		sys->paused = false;
	}
	if (nextLevelTimer >= 3.0f) {
		if (level < levelNames.size-1) {
			level++;
			world.Load(levelNames[level]);
			if (random(0, 1) == 1) {
				jump = &jump1;
			} else {
				jump = &jump2;
			}
			Reset();
		} else {
			// Start the ending cutscene
			Gui::gui->menuMain.continueHideable->hidden = true;
			Gui::gui->menuNext = Gui::Gui::Menu::OUTTRO;
			Gui::gui->menuCutscene.Begin();
			music.Stop(2.0f);
		}
	}
	if (Gui::gui->menuPlay.buttonReset->state.Released()) {
		Reset();
	}
}

void Manager::EventSync() {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Manager::EventSync)
	camZoom = (f32)sys->window.height / 1080.0f * 1.5f;
	if (Gui::gui->menuMain.buttonContinue->state.Released()) {
		Gui::gui->menuMain.buttonContinue->state.Set(false, false, false);
	}
	if (Gui::gui->menuMain.buttonNewGame->state.Released()) {
		Gui::gui->menuMain.buttonNewGame->state.Set(false, false, false);
		level = 0;
		if (level == 0) {
			music.Play();
		}
		world.Load(levelNames[level]);
		Reset();
	}
	if (Gui::gui->menuMain.buttonLevelEditor->state.Released() && Gui::gui->menuCurrent == Gui::Gui::Menu::EDITOR) {
		Gui::gui->menuMain.buttonLevelEditor->state.Set(false, false, false);
		Reset();
	}
	timestep = sys->timestep * sys->simulationRate;
	mouse = ScreenPosToWorld(sys->input.cursor);
	if (Gui::gui->menuCurrent == Gui::Gui::Menu::PLAY) {
		HandleUI();
		if (goalFlame > 0.0f) {
			goalFlame = min(1.0f, goalFlame + timestep * 0.5f);
			if (flameTimer > 0.0f) flameTimer -= timestep;
			while (flameTimer <= 0.0f) {
				Flame flame;
				vec2 offset = vec2(random(-12.0f, 12.0f), random(-8.0f, 16.0f));
				flame.physical.pos = goalPos + offset;
				flame.physical.vel = 0.0f;
				flame.size = goalFlame;
				entities->flames.Create(flame);
				flameTimer += 0.002f;
			}
		}
		if (players.count > 0) {
			vec2 targetPos = players[0].physical.pos;
			targetPos.x += (players[0].facingRight ? 1.0f : -1.0f) * sys->rendering.screenSize.x / 8.0f / camZoom;
			camPos = decay(camPos, targetPos, 0.5f, timestep);
		}
	} else if (Gui::gui->menuCurrent == Gui::Gui::Menu::EDITOR) {
		// Camera
		if (sys->Down(KC_KEY_UP)) {
			camPos.y -= 1000.0f * sys->timestep;
		}
		if (sys->Down(KC_KEY_DOWN)) {
			camPos.y += 1000.0f * sys->timestep;
		}
		if (sys->Down(KC_KEY_LEFT)) {
			camPos.x -= 1000.0f * sys->timestep;
		}
		if (sys->Down(KC_KEY_RIGHT)) {
			camPos.x += 1000.0f * sys->timestep;
		}
		// Placing of blocks
		if (Gui::gui->mouseoverWidget == nullptr) {
			toPlace = (World::Block)Gui::EditorMenu::blockTypes[Gui::gui->menuEditor.switchBlock->choice];
			vec2i pos = vec2i(mouse)/32;
			if (sys->Down(KC_MOUSE_LEFT)) {
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
			if (sys->Down(KC_MOUSE_RIGHT)) {
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

	ManagerBasic::EventSync();

	players.GetWorkChunks(workChunks);
	sprinklers.GetWorkChunks(workChunks);
	droplets.GetWorkChunks(workChunks);
	flames.GetWorkChunks(workChunks);
}

void Manager::EventDraw(Array<Rendering::DrawingContext> &contexts) {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Manager::EventDraw)
	// if (gui->currentMenu != Int::MENU_PLAY) return;
	if (flame > 0.0f) {
		vec4 color = vec4(1.0f, 1.0f, 0.5f, flame*0.5f);
		f32 scale = flame * 400.0f;
		sys->rendering.DrawCircle(contexts[0], Rendering::texBlank, color, WorldPosToScreen(lanternPos), vec2(2.1f), vec2(scale), vec2(0.5f));
	}
	if (goalFlame > 0.0f) {
		vec4 color = vec4(1.0f, 1.0f, 0.5f, goalFlame*0.5f);
		f32 scale = goalFlame * 600.0f;
		sys->rendering.DrawCircle(contexts[0], Rendering::texBlank, color, WorldPosToScreen(goalPos), vec2(2.1f), vec2(scale), vec2(0.5f));
	}
	world.Draw(contexts[0], Gui::gui->menuCurrent != Gui::Gui::Menu::EDITOR, true);
	
	ManagerBasic::EventDraw(contexts);

	world.Draw(contexts.Back(), Gui::gui->menuCurrent != Gui::Gui::Menu::EDITOR, false);
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

void MessageText::Reset() {
	angle = Radians32(Degrees32(random(-180.0f, 180.0f))).value();
	position = vec2(cos(angle), sin(angle)) * 0.5;
	size = 0.001f;
	velocity = -position * 15.0;
	rotation = 0.0f;
	scaleSpeed = 1.0f;
	targetPosition = vec2(random(-0.25f, 0.25f), random(-0.25f, 0.25f));
	targetAngle = Radians32(Degrees32(random(-30.0f, 30.0f))).value();
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
	sys->rendering.DrawTextSS(context, text, Gui::gui->fontIndex, vec4(vec3(0.0f), 1.0f), position, size, Rendering::CENTER, Rendering::CENTER, 0.0f, 0.5f, 0.225f, angle);
	sys->rendering.DrawTextSS(context, text, Gui::gui->fontIndex, color, position, size, Rendering::CENTER, Rendering::CENTER, 0.0f, 0.5f, 0.425f, angle);
}

void World::Draw(Rendering::DrawingContext &context, bool playing, bool under) {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::World::Draw)
	vec4 color;
	vec2 pos;
	vec2 scale;
	i32 tex;
	AABB bounds = entities->CamBounds();
	vec2i topLeft = vec2i(
		max((i32)floor(bounds.minPos.x/32.0f), 0),
		max((i32)floor(bounds.minPos.y/32.0f), 0)
	);
	vec2i bottomRight = vec2i(
		min((i32)ceil(bounds.maxPos.x/32.0f), size.x),
		min((i32)ceil(bounds.maxPos.y/32.0f), size.y)
	);
	for (i32 y = topLeft.y; y < bottomRight.y; y++) {
		for (i32 x = topLeft.x; x < bottomRight.x; x++) {
			switch (operator[](vec2i(x, y))) {
				case BLOCK_PLAYER:
					if (under && !playing) {
						pos = entities->WorldPosToScreen(vec2(f32(x*32), f32(y*32)) + vec2(2.0f));
						color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
						scale = vec2(28.0f);
						tex = Rendering::texBlank;
					} else {
						continue;
					}
					break;
				case BLOCK_WALL:
					if (under) {
						pos = entities->WorldPosToScreen(vec2(f32(x*32), f32(y*32)));
						color = vec4(vec3(0.0f), 1.0f);
						scale = vec2(32.0f);
						tex = Rendering::texBlank;
					} else {
						continue;
					}
					break;
				case BLOCK_WATER_FULL:
					if (!under) {
						pos = entities->WorldPosToScreen(vec2(f32(x*32), f32(y*32)));
						color = vec4(vec3(0.0f, 0.2f, 1.0f), 0.7f);
						scale = vec2(32.0f);
						tex = Rendering::texBlank;
					} else {
						continue;
					}
					break;
				case BLOCK_WATER_TOP:
					if (!under) {
						pos = entities->WorldPosToScreen(vec2(f32(x*32), f32(y*32)+8.0f));
						color = vec4(vec3(0.0f, 0.2f, 1.0f), 0.7f);
						scale = vec2(32.0f, 24.0f);
						tex = Rendering::texBlank;
					} else {
						continue;
					}
					break;
				case BLOCK_GOAL:
					if (under) {
						pos = entities->WorldPosToScreen(vec2(f32(x*32)+0.5f, f32(y*32)+12.5f));
						color = vec4(1.0f);
						scale = vec2(31.0f, 19.5f);
						tex = entities->texBeacon;
					} else {
						continue;
					}
					break;
				case BLOCK_SPRINKLER:
					if (under && !playing) {
						pos = entities->WorldPosToScreen(vec2(f32(x*32)+3.5f, f32(y*32)+19.0f));
						color = vec4(1.0f);
						scale = vec2(25.0f, 13.0f);
						tex = entities->texSprinkler;
					} else {
						continue;
					}
					break;
				default: continue;
			}
			sys->rendering.DrawQuad(context, tex, color, pos, scale * entities->camZoom, vec2(1.0f));
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
	io::cout.PrintLn("Loading '", filename, "'");
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
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Lantern::Update)
	vel = (pos - posPrev) / timestep;
	vec2 v = vec2(cos(angle), -sin(angle));
	if (particleTimer > 0.0f) {
		particleTimer -= timestep;
	}
	if (entities->flame > 0.0f) {
		if (particleTimer <= 0.0f) {
			Flame flame;
			f32 a = random(-pi, pi);
			vec2 offset = vec2(cos(a), sin(a)) * random(0.0f, 4.0f);
			flame.physical.pos = pos + offset + v * 14.0f;
			flame.physical.vel = vel * 0.5f;
			flame.size = entities->flame;
			entities->flames.Create(flame);
			particleTimer += 0.02f;
		}
	}

	vec2 deltaVel = vel - velPrev;
	deltaVel = vec2(0.0f, 50.0f) - deltaVel;

	Angle32 impulseAngle = atan2(-deltaVel.y, deltaVel.x);
	rot -= (impulseAngle-angle) * timestep * cos(pi / 4.0f * (1.0f-dot(normalize(deltaVel), v))) * norm(deltaVel);

	ApplyFriction(rot.value(), pi * 2.0f, timestep);
	angle += rot * timestep;
	posPrev = pos;
	entities->lanternPos = pos;
	velPrev = vel;
}

void Lantern::Draw(Rendering::DrawingContext &context) {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Lantern::Draw)
	sys->rendering.DrawQuad(context, entities->texLantern, vec4(1.0f), entities->WorldPosToScreen(pos), vec2(41.0f, 66.0f) * 0.4f * entities->camZoom, vec2(1.0f), vec2(0.5f, 0.05f), angle.value() + pi / 2.0f);
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
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Player::Update)
	physical.Update(timestep);
	physical.UpdateActual();
	bool buttonJump = sys->Down(KC_KEY_UP) || sys->Down(KC_KEY_W);
	bool buttonLeft = sys->Down(KC_KEY_LEFT) || sys->Down(KC_KEY_A);
	bool buttonRight = sys->Down(KC_KEY_RIGHT) || sys->Down(KC_KEY_D);
	bool grounded = false;
	bool sliding = false;
	vec2 step = physical.vel * timestep;
	bool jumped = false;
	AABB below = physical.aabb;
	below.minPos.x += 1.0f - step.x;
	below.maxPos.x -= 1.0f + step.x;
	below.minPos.y += 1.0f;
	below.maxPos.y += 1.0f + step.y;
	if (entities->world.Solid(below)) {
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
	if (entities->world.Solid(right)) {
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
	if (entities->world.Solid(left)) {
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
			entities->step.Play(min(physical.vel.y / 2000.0f, 1.0f), 1.0f);
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
					entities->step.Play(0.5f, 1.0f);
				}
			}
		}
	}

	AABB above = physical.aabb;
	above.minPos.x += 1.0f - step.x;
	above.maxPos.x -= 1.0f + step.x;
	above.minPos.y -= 1.0f - step.y;
	above.maxPos.y -= 2.0f;
	if (entities->world.Solid(above)) {
		physical.pos.y = 32.0f * round(physical.pos.y/32.0f) + 1.0f;
		physical.vel.y = 0.0f;
	}

	AABB smaller = physical.aabb;
	smaller.minPos += 2.0f;
	smaller.maxPos -= 2.0f;
	if (entities->flame > 0.0f) {
		if (entities->world.Water(smaller)) {
			entities->flame -= 12.0f * timestep;
		} else {
			entities->flame = min(min(1.0f, entities->gas), entities->flame + timestep * 0.25f);
		}
		if (entities->world.Goal(smaller)) {
			entities->goalFlame += 0.1f * timestep;
		}

		for (i32 i = 0; i < entities->droplets.size; i++) {
			const Droplet &drop = entities->droplets[i];
			if (drop.id.generation < 0) continue;
			if (physical.Collides(drop.physical)) {
				entities->flame -= 8.0f * timestep;
				break;
			}
		}
	}
	if (jumped) {
		entities->jump->Play(0.5f, random(0.90f, 1.1f));
	}
	{ // Lantern pos
		vec2 delta = entities->mouse - (physical.pos+vec2(16.0f, 0.0f));
		delta /= 10.0f;
		f32 mag = norm(delta);
		if (mag > 22.0f) {
			delta *= 22.0f / mag;
		}
		lantern.pos = delta + physical.pos + vec2(16.0f, 0.0f);
	}
	lantern.Update(timestep);
}

void Player::Draw(Rendering::DrawingContext &context) {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Player::Draw)
	vec2 pos = entities->WorldPosToScreen(physical.pos+vec2(facingRight ? 44.0f : -9.0f, -11.0f));
	vec2 scale = vec2(facingRight? -53.0f : 53.0f, 57.0f) * entities->camZoom;
	i32 tex = 0;
	switch (anim) {
		case RUN:
			tex = animTime < 0.5f ? entities->texPlayerStand : entities->texPlayerRun;
			break;
		case JUMP:
			tex = entities->texPlayerJump;
			break;
		case FLOAT:
			tex = entities->texPlayerFloat;
			break;
		case WALL_TOUCH:
			tex = entities->texPlayerWallTouch;
			break;
		case WALL_JUMP:
			tex = entities->texPlayerWallBack;
			break;
	}
	sys->rendering.DrawQuad(context, tex, vec4(1.0f), pos, scale, vec2(1.0f));
	lantern.Draw(context);
}

template struct DoubleBufferArray<Player>;

void Sprinkler::EventCreate() {
}

void Sprinkler::Update(f32 timestep) {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Sprinkler::Update)
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
		entities->droplets.Create(drop);
		shootTimer += 0.02f;
	}
}

void Sprinkler::Draw(Rendering::DrawingContext &context) {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Sprinkler::Draw)
	const vec2 scale = vec2(25.0f, 13.0f) * entities->camZoom;
	const vec2 p = entities->WorldPosToScreen(physical.pos) - vec2(scale.x * 0.5f, 0.0f);
	sys->rendering.DrawQuad(context, entities->texSprinkler, vec4(1.0f), p, scale, vec2(1.0f));
}

template struct DoubleBufferArray<Sprinkler>;

void Droplet::EventCreate() {
	lifetime = 2.0f;
	physical.type = SEGMENT;
	physical.basis.segment.a = vec2(-4.0f, -1.0f);
	physical.basis.segment.b = vec2(4.0f, 1.0f);
}

void Droplet::Update(f32 timestep) {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Droplet::Update)
	physical.Update(timestep);
	physical.UpdateActual();
	lifetime -= timestep;
	if (lifetime <= 0.0f || entities->world.Solid(physical.aabb)) {
		entities->droplets.Destroy(id);
	}
	physical.ImpulseY(2000.0f, timestep);
	physical.angle = atan2(-physical.vel.y, physical.vel.x);
}

void Droplet::Draw(Rendering::DrawingContext &context) {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Droplet::Draw)
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
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Flame::Update)
	physical.basis.circle.r -= 32.0f * timestep;
	physical.Update(timestep);
	physical.UpdateActual();
	ApplyFriction(physical.vel, 1000.0f, timestep);
	physical.ImpulseY(-1500.0f, timestep);
	if (physical.basis.circle.r < 0.5f) {
		entities->flames.Destroy(id);
	}
}

void Flame::Draw(Rendering::DrawingContext &context) {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Entities::Flame::Update)
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
	const f32 z = entities->camZoom;
	const vec2 p = entities->WorldPosToScreen(physical.pos);
	const vec2 scale = physical.basis.circle.r * 2.0f;
	sys->rendering.DrawCircle(context, Rendering::texBlank, color, p, scale * 0.5f, vec2(2.0f * z), -physical.basis.circle.c / scale + vec2(0.5f), physical.angle);
}

template struct DoubleBufferArray<Flame>;

} // namespace Objects
