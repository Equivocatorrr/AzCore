/*
	File: sprite.cpp
	Author: Philip Haynes
*/

#include "sprite.hpp"
#include "entity_basics.hpp"
#include "assets.hpp"

namespace Az2D {

using namespace AzCore;
using GameSystems::sys;
using Entities::entitiesBasic;

void Sprite::AssetsQueue(az::SimpleRange<char> name) {
	nameAlbedo = Stringify(name, ".tga");
	nameNormal = Stringify(name, "_n.tga");
	nameEmit = Stringify(name, "_e.tga");
	sys->assets.QueueFile(nameAlbedo);
	sys->assets.QueueLinearTexture(nameNormal);
	sys->assets.QueueFile(nameEmit);
}

void Sprite::AssetsAcquire() {
	tex.albedo = sys->assets.FindTexture(nameAlbedo);
	tex.normal = sys->assets.FindTexture(nameNormal);
	tex.emit = sys->assets.FindTexture(nameEmit);
	if (tex.normal == 0) {
		tex.normal = 2;
	}
	if (tex.emit == 0) {
		tex.emit = 3;
	}
}

void Sprite::Update(f32 timestep) {
	if (nFrames <= 1) return;
	frame += timestep * framerate;
	while ((i32)frame >= nFrames) {
		frame -= (f32)nFrames;
	}
}

void Sprite::Draw(Rendering::DrawingContext &context, vec2 pos, vec2 scalePreRot, vec2 scalePostRot, az::Radians32 rotation, Rendering::PipelineIndex pipeline, Rendering::Material material, f32 zShear, f32 zPos) {
	pos = entitiesBasic->WorldPosToScreen(pos);
	zPos = zPos * entitiesBasic->camZoom + (f32)sys->window.height / 2.0f;
	// Add a lil tiny extra bit to prevent holes in tiled sprites from floating point precision errors
	vec2 size = Size() * 1.0000001f;
	scalePreRot *= size * entitiesBasic->camZoom;
	vec2 actualOrigin = origin/size;
	sys->rendering.DrawQuad(context, pos, scalePreRot, scalePostRot, actualOrigin, rotation, pipeline, material, tex, zShear, zPos);
	
}

vec2 Sprite::Size() const {
	i32 texChoice = tex.albedo != 0 ? tex.albedo : tex.normal;
	const Assets::Texture &texture = sys->assets.textures[texChoice];
	vec2 result = vec2(texture.width / nFrames, texture.height);
	return result;
}

} // namespace Az2D