/*
	File: sprite.hpp
	Author: Philip Haynes
	Some utilities to make it easy to work with sprites.
*/

#ifndef AZ2D_SPRITE_HPP
#define AZ2D_SPRITE_HPP

#include "assets.hpp"
#include "rendering.hpp"

#include "AzCore/math.hpp"

namespace Az2D {

using az::vec2, az::vec3, az::vec4;

struct Sprite {
	az::String nameAlbedo;
	az::String nameNormal;
	Assets::TexIndex texAlbedo = 1;
	Assets::TexIndex texNormal = 2;
	f32 framerate = 10.0f;
	i32 nFrames = 1;
	vec2 origin = 0.0f;
	vec2 frameStart = 0.0f;
	
	f32 frame = 0.0f;
	inline void Reset() {
		frame = 0.0f;
	}
	void AssetsQueue(az::SimpleRange<char> name);
	void AssetsAcquire();
	// Update just progresses the animation if there is one
	void Update(f32 timestep);
	// TODO: Support spritesheet-based animation
	void Draw(Rendering::DrawingContext &context, vec2 pos, vec2 scalePreRot = 1.0f, vec2 scalePostRot = 1.0f, az::Radians32 rotation = 0.0f, Rendering::PipelineIndex pipeline = Rendering::PIPELINE_BASIC_2D, vec4 color = 1.0f, f32 normalDepth=1.0f);
	vec2 Size() const;
};

} // namespace Az2D

#endif // AZ2D_SPRITE_HPP