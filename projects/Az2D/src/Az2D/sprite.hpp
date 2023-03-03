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

using az::vec2, az::vec3, az::vec4, az::vec2i;

struct Sprite {
	az::String nameAlbedo;
	az::String nameNormal;
	az::String nameEmit;
	Rendering::TexIndices tex = Rendering::TexIndices(1);
	f32 framerate = 10.0f;
	i32 nFrames = 1;
	vec2 origin = 0.0f;
	vec2i framesStart = 0;
	// the default value of -1 means it will be set to the full size of the sprite in AssetsAcquire
	vec2i framesEnd = -1;
	
	f32 frame = 0.0f;
	inline void Reset() {
		frame = 0.0f;
	}
	void AssetsQueue(az::SimpleRange<char> name);
	void AssetsAcquire();
	// Update just progresses the animation if there is one
	void Update(f32 timestep);
	// TODO: Support spritesheet-based animation
	void Draw(Rendering::DrawingContext &context, vec2 pos, vec2 scalePreRot = 1.0f, vec2 scalePostRot = 1.0f, az::Radians32 rotation = 0.0f, Rendering::PipelineIndex pipeline = Rendering::PIPELINE_BASIC_2D, Rendering::Material material=Rendering::Material(1.0f), f32 zShear=0.0f, f32 zPos=0.0f);
	vec2i Size() const;
	vec2i SpriteSheetSize() const;
};

} // namespace Az2D

#endif // AZ2D_SPRITE_HPP