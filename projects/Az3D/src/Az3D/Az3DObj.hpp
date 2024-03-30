/*
	File: Az3DObj.hpp
	Author: Philip Haynes
	Functions to work with the Az3DObj format
*/

#ifndef AZ3D_AZ3DOBJ_HPP
#define AZ3D_AZ3DOBJ_HPP

#include "AzCore/memory.hpp"
#include "AzCore/Image.hpp"

namespace Az3D::Az3DObj {

struct Vertex {
	az::vec3 pos;
	az::vec3 normal;
	az::vec3 tangent;
	az::vec2 tex;
	u32 boneIDs;
	u32 boneWeights;
};
static_assert(sizeof(Vertex) == 4*13);

struct Material {
	// The following multiply with any texture bound (with default textures having a value of 1)
	az::vec4 color;
	az::vec3 emit;
	f32 normal;
	f32 metalness;
	f32 roughness;
	f32 sssFactor;
	az::vec3 sssColor;
	az::vec3 sssRadius;
	// Texture indices are relative to the file
	// 0 indicates no texture
	union {
		struct {
			u32 texAlbedo;
			u32 texEmit;
			u32 texNormal;
			u32 texMetalness;
			u32 texRoughness;
		};
		u32 tex[5];
	};
	bool isFoliage;
};

struct Mesh {
	az::String name;
	az::Array<Vertex> vertices;
	az::Array<u32> indices;
	Material material;
	az::String armatureName;
};

struct Empty {
	az::String name;
	az::vec3 pos;
	az::vec3_t<az::Angle32> eulerAngles;
};

struct Bone {
	using Id = u8;
	az::String name;
	az::mat3 basis;
	az::vec3 offset;
	Id parent;
	Id ikTarget;
	Id ikPole;
	bool deform;
	bool isInIkChain;
	struct IkInfo {
		// Scalar from 0 to 1 indicating how much this bone will stretch to reach the IK target where 0 means no stretching, and 1 means full stretching.
		f32 stretch;
		// Axis rotation limitations
		// No movement is allowed on this axis
		az::vec3_t<bool> locked;
		// Movement on this axis is bounded by the min and max values below.
		az::vec3_t<bool> limited;
		// Minimum angle allowed in degrees from -180 to 0
		az::vec3 min;
		// Maximum angle allowed in degrees from 0 to 180
		az::vec3 max;
		// How resistant each axis is to being rotated in the range 0 to 1
		az::vec3 stiffness;
	};
	az::Optional<IkInfo> ikInfo;
};

struct Armature {
	az::String name;
	az::Array<Bone> bones;
};

struct KeyFrame {
	union Point {
		az::vec2 vector;
		struct {
			f32 time;
			f32 value;
		};
	};
	Point point;
	enum Interp : u32 {
		CONSTANT = 0,
		LINEAR,
		BEZIER,
		SINE,
		QUADRATIC,
		CUBIC,
		QUARTIC,
		QUINTIC,
		EXPONENTIAL,
		CIRCULAR,
		BACK,
		BOUNCE,
		ELASTIC,
	} interpolation;
	enum Easing : u32 {
		EASE_IN = 0,
		EASE_OUT,
		EASE_IN_OUT,
		EASE_NONE = UINT32_MAX,
	} easing = EASE_NONE;
	union {
		struct {
			Point control[2];
		} bezier;
		struct {
			f32 factor;
		} back;
		struct {
			f32 amp;
			f32 period;
		} elastic;
	};
};

struct Action {
	az::String name;
	struct Curve {
		az::String boneName;
		u8 index;
		bool isOffset;
		az::Array<KeyFrame> keyframes;
		f32 Evaluate(f32 time) const;
	};
	az::Array<Curve> curves;
};

struct File {
	az::Array<Mesh> meshes;
	az::Array<Empty> empties;
	az::Array<az::Image> images;
	az::Array<Armature> armatures;
	az::Array<Action> actions;

	struct ImageData {
		az::Str filename;
		az::Str data;
		bool isLinear;
	};

	bool Load(az::Str filepath);
	// If dstImageData != nullptr then images won't be filled and instead the raw data is put into dstImageData undecoded.
	bool LoadFromBuffer(az::Str buffer, az::Array<ImageData> *dstImageData=nullptr);
};

} // namespace Az3D::Az3DObj

#endif // AZ3D_AZ3DOBJ_HPP