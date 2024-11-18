/*
	File: rendering.hpp
	Author: Philip Haynes
	Utilities and structures to aid in rendering.
*/

#ifndef AZ3D_RENDERING_HPP
#define AZ3D_RENDERING_HPP

#include "AzCore/Math/Matrix.hpp"
#include "AzCore/Math/mat4_t.hpp"
#include "AzCore/Math/quat_t.hpp"
#include "Az3DObj.hpp"
#include "animation.hpp"

#include "AzCore/basictypes.hpp"
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
	DebugVertex() = default;
	DebugVertex(vec3 _pos, vec4 _color) : pos(_pos), color(_color) {}
};

constexpr i32 texBlank = 1;

extern String error;

enum PipelineEnum {
	PIPELINE_NONE=0,
	PIPELINE_DEBUG_LINES,
	PIPELINE_BASIC_3D,
	PIPELINE_BASIC_3D_VSM,
	// Special pipeline that renders backfaces
	PIPELINE_FOLIAGE_3D,
	PIPELINE_FOLIAGE_3D_VSM,
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
	alignas(16) vec3 ambientLightUp;
	alignas(16) vec3 ambientLightDown;
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
	// height / width
	f32 aspectRatio = 9.0f / 16.0f;
	// Horizontal field of view
	Degrees32 fov = 90.0f;
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
	Optional<Animation::ArmatureAction> armatureAction;
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
	static constexpr u32 bloomLayers = 6;
	struct {
		GPU::Device *device;
		GPU::Window *window;
		GPU::Framebuffer *windowFramebuffer;
		GPU::Context *contextMainRender;
		GPU::Context *contextDepthPrepass;
		GPU::Context *contextTransfer;
		GPU::Sampler *textureSampler;
		Array<GPU::Image*> textures;
		i32 concurrency = 1;

		// 3D Rendering
		GPU::Buffer *worldInfoBuffer;
		GPU::Buffer *objectBuffer;
		GPU::Buffer *bonesBuffer;
		GPU::Buffer *textBuffer;
		GPU::Buffer *vertexBuffer;
		GPU::Buffer *indexBuffer;
		Array<GPU::Buffer*> fontBuffers;
		Array<FontBuffer> fontBufferDatas;
		Array<GPU::Pipeline*> pipelines;
		GPU::Pipeline *pipelineBasic3DDepthPrepass;
		GPU::Pipeline *pipelineFoliage3DDepthPrepass;
		GPU::Pipeline *pipelineFont3DDepthPrepass;

		// VSM
		GPU::Context *contextShadowMap;
		GPU::Image *shadowMapImage;
		GPU::Framebuffer *framebufferShadowMaps;
		GPU::Image *shadowMapConvolutionImage;
		GPU::Framebuffer *framebufferConvolution[2];
		GPU::Pipeline *pipelineShadowMapConvolution;
		GPU::Sampler *shadowMapSampler;

		// For debug lines
		GPU::Buffer *debugVertexBuffer;


		// Post-processing
		GPU::Image *msaaDepthImage = nullptr;
		GPU::Image *msaaRawImage = nullptr;
		GPU::Image *depthImage;
		GPU::Framebuffer *depthPrepassFramebuffer;
		GPU::Sampler *aoDepthImageSampler;
		GPU::Image *aoImage;
		GPU::Image *aoSmoothedImage;
		GPU::Sampler *aoImageSampler; // One sampler for both AO images
		GPU::Framebuffer *aoFramebuffer;
		GPU::Framebuffer *aoSmoothedFramebuffer;
		GPU::Image *rawImage;
		GPU::Framebuffer *rawFramebuffer;
		GPU::Sampler *rawSampler;
		GPU::Image *bloomImage[2 * bloomLayers];
		GPU::Framebuffer *bloomFramebuffer[2 * bloomLayers];
		GPU::Sampler *bloomSampler;
		GPU::Pipeline *pipelineAOFromDepth;
		GPU::Pipeline *pipelineAOConvolution;
		GPU::Pipeline *pipelineBloomConvolution;
		GPU::Pipeline *pipelineBloomCombine;
		GPU::Pipeline *pipelineCompositing;

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
	vec3 backgroundRGB; // Calculated from HSV
	// Emptied at the beginning of every frame
	Array<Light> lights;
	WorldInfoBuffer worldInfo;
	Mutex lightsMutex;
	Camera camera;
	Camera debugCamera;
	bool debugCameraActive = false;
	bool debugCameraFly = false;
	vec2 debugCameraFacingDiff = vec2(0.0f);
	Frustum sunFrustum;

	bool Init();
	bool Deinit();
	void UpdateLights();
	bool UpdateFonts(GPU::Context *context);
	bool UpdateWorldInfo(GPU::Context *context);
	bool UpdateObjects(GPU::Context *context);
	bool UpdateDebugLines(GPU::Context *context);
	void UpdateDebugCamera();
	bool Draw();
	bool Present();

	void UpdateBackground();

	inline bool IsInDebugFlyCam() const {
		return debugCameraActive && debugCameraFly;
	}
};

void DrawMeshPart(DrawingContext &context, Assets::MeshPart *meshPart, const ArrayWithBucket<mat4, 1> &transforms, bool opaque, bool castsShadows, Optional<Animation::ArmatureAction> action=None);
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

void DrawCamera(DrawingContext &context, const Camera &camera, vec4 color);

void GetCameraFrustumCorners(const Camera &camera, vec3 dstPointsNear[4], vec3 dstPointsFar[4]);

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

inline mat4 GetMat4(quat orientation, vec3 offset) {
	mat3 rotation = normalize(orientation).ToMat3();
	mat4 result = mat4(rotation);
	result[3].xyz = offset;
	return result;
}

} // namespace Az3D::Rendering

#endif // AZ3D_RENDERING_HPP
