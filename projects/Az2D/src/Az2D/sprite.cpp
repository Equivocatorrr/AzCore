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
	if (framesEnd == vec2i(-1)) {
		framesEnd = SpriteSheetSize();
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
	vec2 size = vec2(Size());
	// Add a lil tiny extra bit to prevent holes in tiled sprites from floating point precision errors
	scalePreRot *= size * entitiesBasic->camZoom * 1.0000001f;
	vec2 actualOrigin = origin/size;
	vec2 fullSize = vec2(SpriteSheetSize());
	vec2 texCoordScale = size / fullSize;
	sys->rendering.DrawQuad(context, pos, scalePreRot, scalePostRot, actualOrigin, rotation, pipeline, material, tex, zShear, zPos, texCoordScale, vec2(f32(framesStart.x)/fullSize.x + texCoordScale.x*floor(frame), f32(framesStart.y)/fullSize.y));
	
}

vec2i Sprite::Size() const {
	vec2i result = framesEnd-framesStart;
	result.x /= nFrames;
	return result;
}

vec2i Sprite::SpriteSheetSize() const {
	i32 texChoice = tex.albedo != 0 ? tex.albedo : tex.normal;
	const Assets::Texture &texture = sys->assets.textures[texChoice];
	vec2i result = vec2i(texture.width, texture.height);
	return result;
}

} // namespace Az2D