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
	sys->assets.QueueFile(nameAlbedo);
	sys->assets.QueueLinearTexture(nameNormal);
}

void Sprite::AssetsAcquire() {
	texAlbedo = sys->assets.FindTexture(nameAlbedo);
	texNormal = sys->assets.FindTexture(nameNormal);
	if (texNormal == 0) {
		texNormal = 2;
	}
}

void Sprite::Update(f32 timestep) {
	if (nFrames <= 1) return;
	frame += timestep * framerate;
	while ((i32)frame >= nFrames) {
		frame -= (f32)nFrames;
	}
}

void Sprite::Draw(Rendering::DrawingContext &context, vec2 pos, vec2 scalePreRot, vec2 scalePostRot, az::Radians32 rotation, Rendering::PipelineIndex pipeline, vec4 color, f32 normalDepth) {
	pos = entitiesBasic->WorldPosToScreen(pos);
	vec2 size = Size();
	scalePreRot *= size * entitiesBasic->camZoom;
	vec2 actualOrigin = origin/size;
	sys->rendering.DrawQuad(context, texAlbedo, color, pos, scalePreRot, scalePostRot, actualOrigin, rotation, pipeline, texNormal, normalDepth);
	
}

vec2 Sprite::Size() const {
	i32 texChoice = texAlbedo != 0 ? texAlbedo : texNormal;
	const Assets::Texture &tex = sys->assets.textures[texChoice];
	vec2 result = vec2(tex.width / nFrames, tex.height);
	return result;
}

} // namespace Az2D