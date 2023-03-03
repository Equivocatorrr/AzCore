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
	// filename of the albedo texture
	az::String nameAlbedo;
	// filename of the normal map
	az::String nameNormal;
	// filename of the emission texture
	az::String nameEmit;
	Rendering::TexIndices tex = Rendering::TexIndices(1);
	// frames per second
	f32 framerate = 10.0f;
	// how many frames in the animation
	i32 nFrames = 1;
	// center of rotation and scaling
	vec2 origin = 0.0f;
	// pixel coords of the first frame's top left
	vec2i framesStart = 0;
	// pixel coords of the last frame's bottom right (not inclusive)
	// the default value of -1 means it will be set to the full size of the sprite in AssetsAcquire
	vec2i framesEnd = -1;
	
	// current animation frame
	f32 frame = 0.0f;
	inline void Reset() {
		frame = 0.0f;
	}
	void AssetsQueue(az::Str name, az::Str fileExtension="tga");
	void AssetsAcquire();
	// Update just progresses the animation if there is one
	void Update(f32 timestep);
	void Draw(Rendering::DrawingContext &context, vec2 pos, vec2 scalePreRot = 1.0f, vec2 scalePostRot = 1.0f, az::Radians32 rotation = 0.0f, Rendering::PipelineIndex pipeline = Rendering::PIPELINE_BASIC_2D, Rendering::Material material=Rendering::Material(1.0f), f32 zShear=0.0f, f32 zPos=0.0f);
	// returns the pixel size of a single frame
	vec2i Size() const;
	// returns the pixel size of the whole spritesheet
	vec2i SpriteSheetSize() const;
};

} // namespace Az2D

#endif // AZ2D_SPRITE_HPP