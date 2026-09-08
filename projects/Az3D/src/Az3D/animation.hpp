/*
	File: animation.hpp
	Author: Philip Haynes
	Animation system based on Blender curves, as stored in Az3DObj files.
*/

#ifndef AZ3D_ANIMATION_HPP
#define AZ3D_ANIMATION_HPP

#include "assets.hpp"

#include "AzCore/Memory/Array.hpp"
#include "AzCore/Math/mat4_t.hpp"
#include "AzCore/Math/Matrix.hpp"

namespace Az3D::Animation {

extern i32 numNewtonIterations;
extern i32 numBinarySearchIterations;

struct ArmatureAction {
	Assets::MeshIndex meshIndex;
	Assets::ActionIndex actionIndex;
	f32 actionTime;
	bool operator==(const ArmatureAction &other) const;
};

// Appends the animated bones to the end of dstBones
void AnimateArmature(az::Array<az::mat4> &dstBones, ArmatureAction armatureAction, az::mat4 &modelTransform, az::Array<az::Vector<f32>> *ikParameters);

} // namespace Az3D::Animation

#endif // AZ3D_ANIMATION_HPP