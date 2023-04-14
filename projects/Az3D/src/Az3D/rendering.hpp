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
#include "AzCore/vk.hpp"

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

struct Material alignas(16) {
	// The following multiply with any texture bound (with default textures having a value of 1)
	vec4 color;
	vec3 emit;
	f32 normal;
	f32 metalness;
	f32 roughness;
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
			1, 1, 1, 1, 1};
	}
};

constexpr i32 texBlank = 1;

extern String error;

enum PipelineEnum {
	PIPELINE_NONE=0,
	PIPELINE_DEBUG_LINES,
	PIPELINE_BASIC_3D,
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
	Degrees32 fov = 120.0f;
};

// I fucking hate Microsoft and every decision they've ever made
// This should never be fucking necessary
#ifdef DrawText
#undef DrawText
#endif

// Contains all the info for a single indexed draw call
struct DrawCallInfo {
	mat4 transform;
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

struct ObjectShaderInfo alignas(16) {
	mat4 model;
	Material material;
};

struct DrawingContext {
	Array<DrawCallInfo> thingsToDraw;
	Array<DebugVertex> debugLines;
};

struct Manager {
	struct {
		vk::Instance instance;
		Ptr<vk::Device> device;
		Ptr<vk::Swapchain> swapchain;
		bool resized = false;
		// Indicates we've been told our extent is invalid, so wait until a resize
		bool zeroExtent = false;
		Ptr<vk::Framebuffer> framebuffer;
		Ptr<vk::RenderPass> renderPass;
		Ptr<vk::Queue> queueGraphics;
		Ptr<vk::Queue> queueTransfer;
		Ptr<vk::Queue> queuePresent;
		i32 concurrency = 1;
		Ptr<vk::CommandPool> commandPoolGraphics;
		bool buffer = false; // Which primary command buffer we're on. Switches every frame.
		Ptr<vk::CommandBuffer> commandBufferGraphics[2];
		Ptr<vk::CommandBuffer> commandBufferGraphicsTransfer;
		Ptr<vk::CommandPool> commandPoolTransfer;
		Ptr<vk::CommandBuffer> commandBufferTransfer;

		Ptr<vk::Semaphore> semaphoreRenderComplete;
		Ptr<vk::QueueSubmission> queueSubmission[2];
		// This can be used only for transfer. Generating mipmaps needs a GRAPHICS queue.
		Ptr<vk::QueueSubmission> queueSubmissionTransfer;
		// Use GraphicsTransfer if you're transferring images and generating mipmaps.
		Ptr<vk::QueueSubmission> queueSubmissionGraphicsTransfer;

		Ptr<vk::Sampler> textureSampler;

		Ptr<vk::Memory> stagingMemory;
		Ptr<vk::Memory> bufferMemory; // Uniform buffers, vertex buffers, index buffers
		Ptr<vk::Memory> textureMemory;

		Ptr<vk::Buffer> uniformStagingBuffer;
		Ptr<vk::Buffer> uniformBuffer;
		Ptr<vk::Buffer> objectStagingBuffer;
		Ptr<vk::Buffer> objectBuffer;
		Ptr<vk::Buffer> vertexStagingBuffer;
		Ptr<vk::Buffer> vertexBuffer;
		Ptr<vk::Buffer> indexStagingBuffer;
		Ptr<vk::Buffer> indexBuffer;
		
		// For debug lines
		Ptr<vk::Buffer> debugVertexStagingBuffer;
		Ptr<vk::Buffer> debugVertexBuffer;

		Ptr<vk::Memory> fontStagingMemory;
		Ptr<vk::Memory> fontBufferMemory;
		Ptr<vk::Memory> fontImageMemory;

		Ptr<vk::Buffer> fontStagingVertexBuffer;
		Range<vk::Buffer> fontStagingImageBuffers;
		Ptr<vk::Buffer> fontVertexBuffer;
		Range<vk::Image> fontImages;

		Array<Ptr<vk::Pipeline>> pipelines;
		Ptr<vk::Descriptors> descriptors;
		// Use one descriptor set for everything, bound once per frame
		Ptr<vk::DescriptorSet> descriptorSet;

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
	bool msaa = true;
	// Emptied at the beginning of every frame
	Array<Light> lights;
	UniformBuffer uniforms;
	Mutex lightsMutex;
	Camera camera;

	bool Init();
	bool Deinit();
	// void UpdateLights();
	bool UpdateFonts();
	bool UpdateUniforms();
	bool UpdateObjects();
	bool UpdateDebugLines(VkCommandBuffer cmdBuf);
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

void DrawMeshPart(DrawingContext &context, Assets::MeshPart *meshPart, const mat4 &transform, bool opaque, bool castsShadows);
void DrawMesh(DrawingContext &context, Assets::Mesh mesh, const mat4 &transform, bool opaque, bool castsShadows);

inline void DrawDebugLine(DrawingContext &context, DebugVertex point1, DebugVertex point2) {
	context.debugLines.Append(point1);
	context.debugLines.Append(point2);
}

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
