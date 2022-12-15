/*
	File: entities.cpp
	Author: Philip Haynes
*/

#include "AzCore/Thread.hpp"
#include "AzCore/IO/Log.hpp"

#include "entities.hpp"
#include "gui.hpp"

namespace Az2D::Entities {

using namespace AzCore;

constexpr bool DEBUG_COLLISIONS = false;

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
	sys->assets.QueueFile("Player.tga");
	sys->assets.QueueFile("PlayerScream.tga");
	sys->assets.QueueFile("guy.tga");
	sys->assets.QueueFile("scream.ogg");
	sys->assets.QueueFile("music.ogg", Assets::Type::STREAM);
}

void Manager::EventAssetsAcquire() {
	texPlayer = sys->assets.FindTexture("Player.tga");
	texPlayerScream = sys->assets.FindTexture("PlayerScream.tga");
	texGuy = sys->assets.FindTexture("guy.tga");
	
	sndScream.Create("scream.ogg");

	sndMusic.Create("music.ogg");
	sndMusic.SetLoopRange(44100*8, 44100*24);
}

void Manager::Reset() {
	players.Clear();
	Player player;
	player.physical.pos = vec2(0.0f);
	players.Create(player);
	tails.Clear();
	/*
	AABB bounds = CamBounds();
	Array<Ptr<Tail>> allTails;
	for (i32 i = 0; i < 50; i++) {
		Tail tail;
		tail.physical.pos = vec2(random(bounds.minPos.x, bounds.maxPos.x), random(bounds.minPos.y, bounds.maxPos.y));
		Ptr<Tail> ptr = tails.Create(tail);
		allTails.Append(ptr);
	}
	allTails[0]->target = player.idGeneric;
	for (i32 i = 1; i < allTails.size; i++) {
		Tail &head = *allTails[i-1];
		Tail &tail = *allTails[i];
		tail.target = head.idGeneric;
	}
	*/
	pitch = 1.0f;
	sndMusic.SetPitch(1.0f);
	sndScream.Stop();
}

bool TypedCode(String code) {
	if (code.size > sys->input.typingString.size) return false;
	Range<char> end = sys->input.typingString.GetRange(sys->input.typingString.size-code.size, code.size);
	if (code == end) {
		sys->input.typingString.Clear();
		return true;
	}
	return false;
}

void Manager::HandleUI() {
	if (Gui::gui->menuPlay.buttonReset->state.Released()) {
		Reset();
	}
}

void Manager::EventSync() {
	camZoom = (f32)sys->window.height / 720.0f;
	if (Gui::gui->menuMain.buttonContinue->state.Released()) {
		Gui::gui->menuMain.buttonContinue->state.Set(false, false, false);
	}
	if (Gui::gui->menuMain.buttonNewGame->state.Released()) {
		Gui::gui->menuMain.buttonNewGame->state.Set(false, false, false);
		sndMusic.Play();
		Reset();
	}
	if (Gui::gui->currentMenu == Gui::Gui::Menu::PLAY) {
		HandleUI();
		/*
		if (sys->Down(KC_KEY_E)) {
			pitch *= (1.0f + 0.1f * timestep);
			pitch = min(pitch, 4.0f);
			sndMusic.SetPitch(pitch);
		}
		if (sys->Down(KC_KEY_Q)) {
			pitch /= (1.0f + 0.1f * timestep);
			pitch = max(pitch, 0.25f);
			sndMusic.SetPitch(pitch);
		}
		if (sys->Down(KC_KEY_F)) {
			pitch = 1.0f;
			sndMusic.SetPitch(pitch);
		}
		*/
	}
	
	for (Tail &tail : tails.ArrayMut()) {
		tail.UpdateSync(timestep);
	}

	players.Synchronize();
	tails.Synchronize();

	ManagerBasic::EventSync();

	players.GetWorkChunks(workChunks);
	tails.GetWorkChunks(workChunks);
}

void Manager::EventClose() {
	sndMusic.Stop();
}


void Player::EventCreate() {
	// physical.type = CIRCLE;
	// physical.basis.circle.c = vec2(0.0f, 0.0f);
	// physical.basis.circle.r = 8.0f;
	// physical.angle = 0.0f;
	physical.type = BOX;
	physical.basis.box.a = vec2(-6.0f, -7.0f);
	physical.basis.box.b = vec2(5.0f, 7.0f);
	physical.angle = 0.0f;
	physical.rot = pi/8.0f;
	screamTimer = 0.0f;
	facing = 1.0f;
	hue = 0.0f;
}

void Player::Update(f32 timestep) {
	physical.ImpulseY(10.0f, timestep);
	ApplyFriction(physical.vel, 2.0f, timestep);
	bool buttonUp = sys->Down(KC_KEY_UP) || sys->Down(KC_KEY_W);
	bool buttonLeft = sys->Down(KC_KEY_LEFT) || sys->Down(KC_KEY_A);
	bool buttonRight = sys->Down(KC_KEY_RIGHT) || sys->Down(KC_KEY_D);
	bool buttonDown = sys->Down(KC_KEY_DOWN) || sys->Down(KC_KEY_S);
	if (buttonRight) {
		physical.ImpulseX(20.0f, timestep);
		facing = 1.0f;
	}
	if (buttonLeft) {
		physical.ImpulseX(-20.0f, timestep);
		facing = -1.0f;
	}
	if (buttonUp) {
		physical.ImpulseY(-40.0f, timestep);
	}
	if (buttonDown) {
		physical.ImpulseY(20.0f, timestep);
	}
	
	vec2 nextPos = physical.pos + physical.vel * timestep;
	vec2 topLeft = entities->CamTopLeft();
	vec2 bottomRight = entities->CamBottomRight();
	if (nextPos.x < topLeft.x || nextPos.x > bottomRight.x) {
		physical.vel.x *= -0.5f;
		physical.pos.x = clamp(physical.pos.x, topLeft.x, bottomRight.x);
	}
	if (nextPos.y < topLeft.y || nextPos.y > bottomRight.y) {
		physical.vel.y *= -0.5f;
		physical.pos.y = clamp(physical.pos.y, topLeft.y, bottomRight.y);
	}
	
	physical.Update(timestep);
	physical.UpdateActual();
	
	screamTimer = max(0.0f, screamTimer - timestep);
	if (sys->Pressed(KC_KEY_SPACE)) {
		entities->sndScream.Play();
		screamTimer = 0.85f;
	}
	if (sys->Released(KC_KEY_SPACE)) {
		entities->sndScream.Stop(0.05f);
		screamTimer = min(screamTimer, 0.025f);
	}
	// f32 speed = norm(physical.vel);
	// entities->sndMusic.SetPitch(pow(2.0f, speed / 10000.0f));
	hue += 0.3f * timestep;
	if (hue > 1.0f) hue -= 1.0f;
	if (sys->Down(KC_MOUSE_LEFT) && Gui::gui->mouseoverWidget == nullptr) {
		vec2 prevPos = physical.pos;
		physical.pos = entities->ScreenPosToWorld(vec2(sys->input.cursor));
		physical.vel += (physical.pos - prevPos) / max(timestep, 0.01f);
	}
}

void Player::Draw(Rendering::DrawingContext &context) {
	Assets::TexIndex tex;
	// if (screamTimer > 0.0f) tex = entities->texPlayerScream;
	// else tex = entities->texPlayer;
	tex = entities->texGuy;
	vec2 pos = entities->WorldPosToScreen(physical.pos);
	vec2 scale = vec2(11.0f, 16.0f) * entities->camZoom * 5.0f;
	scale.x *= facing;
	// vec4 color = vec4(hsvToRgb(vec3(hue, 0.5f, 1.0f)), 1.0f);
	vec4 color = 1.0f;
	sys->rendering.DrawQuadPixel(context, tex, color, pos, scale, vec2(1.0f), vec2(0.5f), physical.angle);
	
	if constexpr (DEBUG_COLLISIONS) {
		physical.Draw(context, vec4(0.5));
	}
}

template struct DoubleBufferArray<Player>;

void Tail::EventCreate() {
	physical.type = CIRCLE;
	physical.basis.circle.c = vec2(0.0f, 0.0f);
	physical.basis.circle.r = 8.0f;
	physical.angle = 0.0f;
}

vec2 TargetPos(vec2 pos, vec2 target, f32 distance) {
	vec2 diff = pos-target;
	vec2 result = target + normalize(diff) * distance;
	return result;
}

void Tail::Update(f32 timestep) {
	ApplyFriction(physical.vel, max(250.0f, 0.25f * norm(physical.vel)), timestep);
	physical.ImpulseY(1000.0f, timestep);
	vec2 nextPos = physical.pos + physical.vel * timestep;
	vec2 topLeft = entities->CamTopLeft();
	vec2 bottomRight = entities->CamBottomRight();
	if (nextPos.x < topLeft.x || nextPos.x > bottomRight.x) {
		physical.vel.x *= -1.0f;
		physical.pos.x = clamp(physical.pos.x, topLeft.x, bottomRight.x);
	}
	if (nextPos.y < topLeft.y || nextPos.y > bottomRight.y) {
		physical.vel.y *= -1.0f;
		physical.pos.y = clamp(physical.pos.y, topLeft.y, bottomRight.y);
	}
	physical.Update(timestep);
	physical.UpdateActual();
}

void Collide(Entity &me, Entity &other, f32 timestep) {
	if (me.physical.Collides(other.physical)) {
		vec2 posDiff = me.physical.pos-other.physical.pos;
		vec2 dir = normalize(posDiff);
		f32 dist = norm(posDiff);
		f32 vel = dot(dir, me.physical.vel-other.physical.vel);
		vec2 velDiff = dir * vel * 0.6f;
		me.physical.vel -= velDiff;
		other.physical.vel += velDiff;
		vec2 impulse = dir * 1000.0f * (16.0f / dist + 1.0f);
		me.physical.Impulse(impulse, timestep);
		other.physical.Impulse(-impulse, timestep);
	}
}

void Tail::UpdateSync(f32 timestep) {
	// /*
	Entity &targetEntity = target.GetMut();
	vec2 targetPos = TargetPos(physical.pos, targetEntity.physical.pos, 16.0f);
	vec2 velDiff = (targetPos - physical.pos) / max(timestep, 0.0025f);
	physical.vel = normalize(physical.vel) * clamp(norm(physical.vel), 0.0f, 10000.0f);
	physical.pos = targetPos;
	if (target.type == 0) {
		vec2 v1 = velDiff * 0.1f;
		vec2 v2 = v1 * 0.5f * square(timestep);
		physical.vel += v1 * 9.0f;
		physical.pos += v2 * 9.0f;
		targetEntity.physical.vel -= v1;
		targetEntity.physical.pos -= v2;
	} else {
		vec2 v1 = velDiff * 0.5f;
		vec2 v2 = v1 * 0.5f * square(timestep);
		physical.vel += v1;
		physical.pos += v2;
		targetEntity.physical.vel -= v1;
		targetEntity.physical.pos -= v2;
	}
	// */
	for (Tail &tail : entities->tails.ArrayMut()) {
		if (tail.idGeneric == target) continue;
		if (tail.id.index >= id.index) break;
		Collide(*this, tail, timestep);
	}
	for (Player &player : entities->players.ArrayMut()) {
		if (player.idGeneric == target) continue;
		Collide(*this, player, timestep);
	}
}

void Tail::Draw(Rendering::DrawingContext &context) {
	vec2 pos = entities->WorldPosToScreen(physical.pos);
	vec2 scale = vec2(16.0f * entities->camZoom);
	sys->rendering.DrawQuad(context, entities->texPlayer, vec4(1.0f), pos, vec2(1.0f), scale, vec2(0.5f)/*, atan2(physical.vel.y, physical.vel.x)*/);
	
	if constexpr (DEBUG_COLLISIONS) {
		physical.Draw(context, vec4(0.5));
	}
}

template struct DoubleBufferArray<Tail>;

} // namespace Objects
