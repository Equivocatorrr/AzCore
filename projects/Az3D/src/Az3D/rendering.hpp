/*
	File: rendering.hpp
	Author: Philip Haynes
	Utilities and structures to aid in rendering.
*/

#ifndef AZ3D_RENDERING_HPP
#define AZ3D_RENDERING_HPP

#include "AzCore/Math/Matrix.hpp"
#include "AzCore/Math/mat4_t.hpp"
#include "Az3DObj.hpp"

#include "AzCore/basictypes.hpp"
#include "AzCore/memory.hpp"
#include "AzCore/gpu.hpp"

#include "assets.hpp"

// namespace AzCore {
// 	template<typename T>
// 	struct Vector;
// };

namespace AzCore::io {
	struct Window;
} // namespace AzCore::io

namespace Az3D::Rendering {

using namespace AzCore;

// void AddPointLight(vec3 pos, vec3 color, f32 distMin, f32 distMax);
// void AddLight(vec3 pos, vec3 color, vec3 direction, f32 angleMin, f32 angleMax, f32 distMin, f32 distMax);

constexpr f32 lineHeight = 1.3f;

extern i32 numNewtonIterations;
extern i32 numBinarySearchIterations;

struct Manager;

struct GlyphInfo {
	vec2 uvs[2];
	vec2 offsets[2];
};

struct FontBuffer {
	u32 texAtlas;
	Array<GlyphInfo> glyphs;
	inline i64 TotalSize() const {
		return sizeof(u32) * 2 + glyphs.size * sizeof(GlyphInfo);
	}
};

struct TextShaderInfo {
	static constexpr u32 MAX_GLYPHS = 36;
	mat2 glyphTransforms[MAX_GLYPHS];
	vec2 glyphOffsets[MAX_GLYPHS];
	u32 glyphIndices[MAX_GLYPHS];
	u32 fontIndex;
	u32 objectIndex;
	u32 _pad[2];
};
static_assert(sizeof(TextShaderInfo) == 256 * 4);

using Vertex = Az3DObj::Vertex;

// For debug lines
struct DebugVertex {
	vec3 pos;
	vec4 color;
};

constexpr i32 texBlank = 1;

extern String error;

enum PipelineEnum {
	PIPELINE_NONE=0,
	PIPELINE_DEBUG_LINES,
	PIPELINE_BASIC_3D,
	// Special pipeline that renders backfaces
	PIPELINE_FOLIAGE_3D,
	PIPELINE_BASIC_3D_VSM,
	PIPELINE_FONT_3D,
	PIPELINE_FONT_3D_VSM,
	PIPELINE_COUNT,
	// Range of pipelines that use Basic3D.vert
	PIPELINE_3D_RANGE_START=PIPELINE_BASIC_3D,
	PIPELINE_3D_RANGE_END=PIPELINE_FONT_3D,
	// Range of pipelines that use Font3D.vert
	PIPELINE_FONT_3D_RANGE_START=PIPELINE_FONT_3D,
	PIPELINE_FONT_3D_RANGE_END=PIPELINE_COUNT,
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

struct WorldInfoBuffer {
	mat4 proj;
	mat4 view;
	mat4 viewProj;
	mat4 sun;
	alignas(16) vec3 sunDir;
	alignas(16) vec3 eyePos;
	alignas(16) vec3 ambientLight;
	alignas(16) vec3 fogColor;
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

struct ArmatureAction {
	Assets::MeshIndex meshIndex;
	Assets::ActionIndex actionIndex;
	f32 actionTime;
	bool operator==(const ArmatureAction &other) const;
};

struct DrawTextInfo {
	TextShaderInfo shaderInfo;
	u32 glyphCount;
};

// Contains all the info for a single indexed draw call
struct DrawCallInfo {
	ArrayWithBucket<mat4, 1> transforms;
	Array<DrawTextInfo> textsToDraw;
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
	Assets::Material material;
	PipelineIndex pipeline;
	Optional<ArmatureAction> armatureAction;
	Array<Vector<f32>> *ikParameters = nullptr;
	// If this is false, this call gets sorted later than opaque calls
	bool opaque;
	// Whether to be considered for shadow passes, set to false if culled by shadow frustums
	bool castsShadows;
	// Should be set to false when created with a draw call, and evaluated for the main camera frustum
	bool culled;
};

struct alignas(16) ObjectShaderInfo {
	mat4 model;
	Assets::Material material;
	u32 bonesOffset;
};
// Verify that bonesOffset is nuzzled snugly after material (since material would otherwise be padded with 4 bytes anyway).
static_assert(sizeof(ObjectShaderInfo) == 40*4);

struct DrawingContext {
	Array<DrawCallInfo> thingsToDraw;
	// Array<DrawTextInfo> textsToDraw;
	Array<DebugVertex> debugLines;
};

struct Plane {
	vec3 normal;
	// distance from origin in the normal direction
	f32 dist;
};

struct Frustum {
	Plane near;
	Plane far;
	Plane left;
	Plane right;
	Plane top;
	Plane bottom;
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

		GPU::Buffer *worldInfoBuffer;
		GPU::Buffer *objectBuffer;
		GPU::Buffer *bonesBuffer;
		GPU::Buffer *textBuffer;
		GPU::Buffer *vertexBuffer;
		GPU::Buffer *indexBuffer;

		GPU::Context *contextShadowMap;
		GPU::Image *shadowMapImage;
		GPU::Framebuffer *framebufferShadowMaps;
		GPU::Image *shadowMapConvolutionImage;
		GPU::Framebuffer *framebufferConvolution[2];
		GPU::Pipeline *pipelineShadowMapConvolution;
		GPU::Sampler *shadowMapSampler;

		// For debug lines
		GPU::Buffer *debugVertexBuffer;

		Array<GPU::Buffer*> fontBuffers;
		Array<FontBuffer> fontBufferDatas;

		Array<GPU::Pipeline*> pipelines;

		Assets::MeshPart *meshPartUnitSquare;
		// One for each draw call, sent to the shader
		Array<ObjectShaderInfo> objectShaderInfos;
		Array<mat4> bones;
		Array<TextShaderInfo> textShaderInfos;
		// One for each thread
		Array<DrawingContext> drawingContexts;
		Array<DebugVertex> debugVertices;
	} data;

	vec2 screenSize = vec2(1280.0f, 720.0f);
	f32 aspectRatio; // height/width
	vec3 backgroundHSV = vec3(197.4f/360.0f, 42.6f/100.0f, 92.2f/100.0f);
	vec3 backgroundRGB; // Derivative of HSV
	// Emptied at the beginning of every frame
	Array<Light> lights;
	WorldInfoBuffer worldInfo;
	Mutex lightsMutex;
	Camera camera;
	Frustum sunFrustum;

	bool Init();
	bool Deinit();
	void UpdateLights();
	bool UpdateFonts(GPU::Context *context);
	bool UpdateWorldInfo(GPU::Context *context);
	bool UpdateObjects(GPU::Context *context);
	bool UpdateDebugLines(GPU::Context *context);
	bool Draw();
	bool Present();

	void UpdateBackground();
};

void DrawMeshPart(DrawingContext &context, Assets::MeshPart *meshPart, const ArrayWithBucket<mat4, 1> &transforms, bool opaque, bool castsShadows, az::Optional<ArmatureAction> action=az::Optional<ArmatureAction>());
void DrawMesh(DrawingContext &context, Assets::MeshIndex mesh, const ArrayWithBucket<mat4, 1> &transforms, bool opaque, bool castsShadows);
void DrawMeshAnimated(DrawingContext &context, Assets::MeshIndex mesh, Assets::ActionIndex action, f32 time, const ArrayWithBucket<mat4, 1> &transforms, bool opaque, bool castsShadows, Array<Vector<f32>> *ikParameters);

struct TextJustify {
	az::Optional<f32> maxWidth;
	static inline TextJustify Justified(f32 _maxWidth) {
		return {_maxWidth};
	}
	static inline TextJustify Unjustified() {
		return {};
	}
	inline operator bool () const {
		return maxWidth.Exists();
	}
	inline f32 MaxWidth() const {
		return maxWidth.ValueOrAssert();
	}
};

f32 CharacterWidth(char32 character, const Assets::Font *fontDesired, const Assets::Font *fontFallback);
f32 LineWidth(const char32 *string, Assets::FontIndex fontIndex);
vec2 StringSize(const WString &string, Assets::FontIndex fontIndex);
f32 StringWidth(const WString &string, Assets::FontIndex fontIndex);
WString StringAddNewlines(WString string, Assets::FontIndex fontIndex, f32 maxWidth);
void LineCursorStartAndSpaceScale(f32 &dstCursor, f32 &dstSpaceScale, f32 textOrigin, f32 spaceWidth, Assets::FontIndex fontIndex, const char32 *string, TextJustify justify);

void DrawText(DrawingContext &context, Assets::FontIndex fontIndex, vec2 textOrigin, const WString &string, mat4 transform, bool castsShadows, Assets::Material material = Assets::Material::Blank(), TextJustify justify = TextJustify::Unjustified());

inline void DrawDebugLine(DrawingContext &context, DebugVertex point1, DebugVertex point2) {
	context.debugLines.Append(point1);
	context.debugLines.Append(point2);
}

void DrawDebugSphere(DrawingContext &context, vec3 center, f32 radius, vec4 color);

f32 StringHeight(const WString &string);

inline mat4 GetTransform(vec3 pos, quat rotation, vec3 scale) {
	mat4 transform = mat4(rotation.ToMat3());
	transform[0].xyz *= scale.x;
	transform[1].xyz *= scale.y;
	transform[2].xyz *= scale.z;
	transform[3][0] = pos.x;
	transform[3][1] = pos.y;
	transform[3][2] = pos.z;
	return transform;
}

} // namespace Az3D::Rendering

#endif // AZ3D_RENDERING_HPP
