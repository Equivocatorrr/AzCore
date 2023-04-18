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
};

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
};

struct Mesh {
	az::String name;
	az::Array<Vertex> vertices;
	az::Array<u32> indices;
	Material material;
};

struct Empty {
	az::String name;
	az::vec3 pos;
	az::vec3_t<az::Angle32> eulerAngles;
};

struct File {
	az::Array<Mesh> meshes;
	az::Array<Empty> empties;
	az::Array<az::Image> images;
	
	bool Load(az::Str filepath);
	bool LoadFromBuffer(az::Str buffer);
};

} // namespace Az3D::Az3DObj

#endif // AZ3D_AZ3DOBJ_HPP