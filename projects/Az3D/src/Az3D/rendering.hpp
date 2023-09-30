/*
	File: rendering.hpp
	Author: Philip Haynes
	Utilities and structures to aid in rendering.
*/

#ifndef AZ3D_RENDERING_HPP
#define AZ3D_RENDERING_HPP

#include "Az3DObj.hpp"

#include "AzCore/basictypes.hpp"
#include "AzCore/memory.hpp"
#include "AzCore/gpu.hpp"

namespace AzCore {
namespace io {
	struct Window;
}
}

namespace Az3D::Assets {
	struct Texture;
	struct Font;
	struct Mesh;
	struct MeshPart;
}

namespace Az3D::Rendering {

using namespace AzCore;

// void AddPointLight(vec3 pos, vec3 color, f32 distMin, f32 distMax);
// void AddLight(vec3 pos, vec3 color, vec3 direction, f32 angleMin, f32 angleMax, f32 distMin, f32 distMax);

constexpr f32 lineHeight = 1.3f;

struct Manager;

enum FontAlign {
	// Horizontal
	LEFT,
	RIGHT,
	JUSTIFY,
	// Either
	MIDDLE,
	CENTER = MIDDLE,
	// Vertical
	TOP,
	BOTTOM
};

using Vertex = Az3DObj::Vertex;

// For debug lines
struct DebugVertex {
	vec3 pos;
	vec4 color;
};

struct alignas(16) Material {
	// The following multiply with any texture bound (with default textures having a value of 1)
	vec4 color;
	vec3 emit;
	f32 normal;
	vec3 sssColor;
	f32 metalness;
	vec3 sssRadius;
	f32 roughness;
	f32 sssFactor;
	u32 isFoliage;
	// Texture indices
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
	
	inline static Material Blank() {
		return Material{
			vec4(1.0f),
			vec3(0.0f),
			1.0f,
			0.0f,
			0.5f,
			0,
			1, 1, 1, 1, 1};
	}
};

constexpr i32 texBlank = 1;

extern String error;

enum PipelineEnum {
	PIPELINE_NONE=0,
	PIPELINE_DEBUG_LINES,
	PIPELINE_BASIC_3D,
	// Special pipeline that renders backfaces
	PIPELINE_FOLIAGE_3D,
	PIPELINE_FONT_3D,
	PIPELINE_COUNT,
	// Range of pipelines that use Basic3D.vert
	PIPELINE_3D_RANGE_START=PIPELINE_BASIC_3D,
	PIPELINE_3D_RANGE_END=PIPELINE_COUNT,
};

typedef u32 PipelineIndex;

constexpr i32 MAX_LIGHTS = 256;
constexpr i32 MAX_LIGHTS_PER_BIN = 16;
constexpr i32 LIGHT_BIN_COUNT_X = 32;
constexpr i32 LIGHT_BIN_COUNT_Y = 18;
constexpr i32 LIGHT_BIN_COUNT = LIGHT_BIN_COUNT_X * LIGHT_BIN_COUNT_Y;

struct Light {
	// world-space position
	alignas(16) vec4 position;
	alignas(16) vec3 color;
	// A normalized vector
	alignas(16) vec3 direction;
	// angular falloff in cos(radians) where < min is 100% brightness, between min and max blends, and > max is 0% brightness
	f32 angleMin;
	f32 angleMax;
	// distance-based falloff in world-space where < min is 100% brightness, between min and max blends, and > max is 0% brightness
	f32 distMin;
	f32 distMax;
};

struct LightBin {
	u8 lightIndices[MAX_LIGHTS_PER_BIN];
};

struct UniformBuffer {
	mat4 proj;
	mat4 view;
	mat4 viewProj;
	mat4 sun;
	alignas(16) vec3 sunDir;
	alignas(16) vec3 eyePos;
	alignas(16) vec3 ambientLight;
	LightBin lightBins[LIGHT_BIN_COUNT];
	// lights[0] is always a zero-brightness light
	Light lights[MAX_LIGHTS];
};

struct Camera {
	vec3 pos = 0.0f;
	vec3 forward = vec3(0.0f, 1.0f, 0.0f);
	vec3 up = vec3(0.0f, 0.0f, 1.0f);
	f32 nearClip = 0.1f;
	f32 farClip = 100.0f;
	// Horizontal field of view
	Degrees32 fov = 90.0f;
};

// I fucking hate Microsoft and every decision they've ever made
// This should never be fucking necessary
#ifdef DrawText
#undef DrawText
#endif

// Contains all the info for a single indexed draw call
struct DrawCallInfo {
	ArrayWithBucket<mat4, 1> transforms;
	// World-space culling info, also used for depth sorting
	vec3 boundingSphereCenter;
	f32 boundingSphereRadius;
	// Used for sorting, calculated based on location and camera
	f32 depth;
	// vertex offset is indexStart * indexSize (defined by the index buffer)
	u32 indexStart;
	u32 indexCount;
	// This will be set once the calls have been sorted
	u32 instanceStart;
	// This will be set to transforms.size
	u32 instanceCount;
	Material material;
	PipelineIndex pipeline;
	// If this is false, this call gets sorted later than opaque calls
	bool opaque;
	// Whether to be considered for shadow passes, set to false if culled by shadow frustums
	bool castsShadows;
	// Should be set to false when created with a draw call, and evaluated for the main camera frustum
	bool culled;
};

struct alignas(16) ObjectShaderInfo {
	mat4 model;
	Material material;
};

struct DrawingContext {
	Array<DrawCallInfo> thingsToDraw;
	Array<DebugVertex> debugLines;
};

struct Manager {
	struct {
		GPU::Device *device;
		GPU::Window *window;
		GPU::Image *msaaImage;
		GPU::Image *depthBuffer;
		GPU::Framebuffer *framebuffer;
		GPU::Context *contextGraphics;
		GPU::Context *contextTransfer;
		GPU::Sampler *textureSampler;
		Array<GPU::Image*> textures;
		i32 concurrency = 1;

		GPU::Buffer *uniformBuffer;
		GPU::Buffer *objectBuffer;
		GPU::Buffer *vertexBuffer;
		GPU::Buffer *indexBuffer;
		
		GPU::Image *shadowMapImage;
		GPU::Framebuffer *framebufferShadowMaps;
		GPU::Pipeline *pipelineShadowMaps;
		
		// For debug lines
		GPU::Buffer *debugVertexBuffer;

		GPU::Buffer *fontVertexBuffer;
		Array<GPU::Image*> fontImages;

		Array<GPU::Pipeline*> pipelines;

		Assets::MeshPart *meshPartUnitSquare;
		// One for each draw call, sent to the shader
		Array<ObjectShaderInfo> objectShaderInfos;
		// One for each thread
		Array<DrawingContext> drawingContexts;
		Array<DebugVertex> debugVertices;
	} data;

	Array<u32> fontIndexOffsets{0};
	vec2 screenSize = vec2(1280.0f, 720.0f);
	f32 aspectRatio; // height/width
	vec3 backgroundHSV = vec3(215.0f/360.0f, 0.7f, 0.125f);
	vec3 backgroundRGB; // Derivative of HSV
	// Emptied at the beginning of every frame
	Array<Light> lights;
	UniformBuffer uniforms;
	Mutex lightsMutex;
	Camera camera;

	bool Init();
	bool Deinit();
	void UpdateLights();
	bool UpdateFonts();
	bool UpdateUniforms(GPU::Context *context);
	bool UpdateObjects(GPU::Context *context);
	bool UpdateDebugLines(GPU::Context *context);
	bool Draw();
	bool Present();

	void UpdateBackground();

	f32 CharacterWidth(char32 character, const Assets::Font *fontDesired, const Assets::Font *fontFallback) const;
	f32 LineWidth(const char32 *string, i32 fontIndex) const;
	vec2 StringSize(WString string, i32 fontIndex) const;
	f32 StringWidth(WString string, i32 fontIndex) const;
	WString StringAddNewlines(WString string, i32 fontIndex, f32 maxWidth) const;
	void LineCursorStartAndSpaceScale(f32 &dstCursor, f32 &dstSpaceScale, f32 scale, f32 spaceWidth, i32 fontIndex, const char32 *string, f32 maxWidth, FontAlign alignH) const;

};

void DrawMeshPart(DrawingContext &context, Assets::MeshPart *meshPart, const ArrayWithBucket<mat4, 1> &transforms, bool opaque, bool castsShadows);
void DrawMesh(DrawingContext &context, Assets::Mesh mesh, const ArrayWithBucket<mat4, 1> &transforms, bool opaque, bool castsShadows);

inline void DrawDebugLine(DrawingContext &context, DebugVertex point1, DebugVertex point2) {
	context.debugLines.Append(point1);
	context.debugLines.Append(point2);
}

void DrawDebugSphere(DrawingContext &context, vec3 center, f32 radius, vec4 color);

f32 StringHeight(WString string);

inline mat4 GetTransform(vec3 pos, quat rotation, vec3 scale) {
	mat4 transform = mat4(mat3::Scaler(scale) * rotation.ToMat3());
	transform.h.w1 = pos.x;
	transform.h.w2 = pos.y;
	transform.h.w3 = pos.z;
	return transform;
}

} // namespace Az3D::Rendering

#endif // AZ3D_RENDERING_HPP
