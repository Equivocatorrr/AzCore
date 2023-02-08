/*
	File: sprite.cpp
	Author: Philip Haynes
*/

#include "sprite.hpp"
#include "entity_basics.hpp"

namespace Az2D {

using namespace AzCore;
using GameSystems::sys;
using Entities::entitiesBasic;

void Sprite::Update(f32 timestep) {
	if (nFrames <= 1) return;
	frame += timestep * framerate;
	while ((i32)frame >= nFrames) {
		frame -= (f32)nFrames;
	}
}

void Sprite::Draw(Rendering::DrawingContext &context, vec2 pos, vec2 scalePreRot, vec2 scalePostRot, az::Radians32 rotation, Rendering::PipelineIndex pipeline) {
	pos = entitiesBasic->WorldPosToScreen(pos);
	vec2 size = Size();
	scalePreRot *= size * entitiesBasic->camZoom;
	vec2 actualOrigin = origin/size;
	sys->rendering.DrawQuad(context, texture, vec4(1.0f), pos, scalePreRot, scalePostRot, actualOrigin, rotation, pipeline, textureNormal);
	
}

vec2 Sprite::Size() const {
	const Assets::Texture &tex = sys->assets.textures[texture];
	vec2 result = vec2(tex.width / nFrames, tex.height);
	return result;
}

} // namespace Az2D