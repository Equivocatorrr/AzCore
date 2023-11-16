/*
	File: rendering.hpp
	Author: Philip Haynes
	Utilities and structures to aid in rendering.
*/

#ifndef AZ2D_RENDERING_HPP
#define AZ2D_RENDERING_HPP

#include "AzCore/memory.hpp"
#include "AzCore/vk.hpp"

namespace AzCore {
namespace io {
	struct Window;
}
}

namespace Az2D::Assets {
	struct Texture;
	struct Font;
}

namespace Az2D::Rendering {

using namespace AzCore;

void AddPointLight(vec3 pos, vec3 color, f32 distMin, f32 distMax, f32 attenuation=0.0f);
void AddLight(vec3 pos, vec3 color, vec3 direction, f32 angleMin, f32 angleMax, f32 distMin, f32 distMax, f32 attenuation=0.0f);

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

struct Vertex {
	vec2 pos;
	vec2 tex;
};

struct TexIndices {
	i32 albedo;
	i32 normal;
	i32 emit;
	TexIndices() = default;
	// Default to the blank textures on the other channels
	inline TexIndices(i32 _albedo, i32 _normal=2, i32 _emit=3) : albedo(_albedo), normal(_normal), emit(_emit) {};
};

struct Material {
	vec4 color;
	f32 emitStrength;
	f32 normalDepth;
	Material() = default;
	inline Material(vec4 _color, f32 _emitStrength=1.0f, f32 _normalDepth=1.0f) : color(_color), emitStrength(_emitStrength), normalDepth(_normalDepth) {}
};

struct PushConstants {
	struct vert_t {
		mat2 transform = mat2(1.0f);
		vec2 origin = vec2(0.0f);
		vec2 position = vec2(0.0f);
		vec2 texScale = vec2(1.0f);
		vec2 texOffset = vec2(0.0f);
		f32 z = 0.0f;
		f32 zShear = 0.0f;
		void Push(VkCommandBuffer commandBuffer, const Manager *rendering) const;
	} vert;
	int pad[2];
	struct frag_t {
		Material mat = {vec4(1.0f), 1.0f, 1.0f};
		TexIndices tex = {0, 2, 3};
		void Push(VkCommandBuffer commandBuffer, const Manager *rendering) const;
	} frag;
	union font_circle_t {
		struct font_t {
			f32 edge;
			f32 bounds;
			void Push(VkCommandBuffer commandBuffer, const Manager *rendering) const;
		} font = {0.1f, 0.5f};
		struct circle_t {
			f32 edge;
			void Push(VkCommandBuffer commandBuffer, const Manager *rendering) const;
		} circle;
	} font_circle;
	void Push2D(VkCommandBuffer commandBuffer, const Manager *rendering) const;
	void PushFont(VkCommandBuffer commandBuffer, const Manager *rendering) const;
	void PushCircle(VkCommandBuffer commandBuffer, const Manager *rendering) const;
};

constexpr i32 texBlank = 1;

extern String error;

enum PipelineEnum {
	PIPELINE_NONE=0,
	PIPELINE_BASIC_2D,
	PIPELINE_BASIC_2D_PIXEL,
	PIPELINE_FONT_2D,
	PIPELINE_CIRCLE_2D,
	PIPELINE_SHADED_2D,
	PIPELINE_SHADED_2D_PIXEL,
};
constexpr i32 PIPELINE_COUNT = PIPELINE_SHADED_2D_PIXEL+1;

typedef u32 PipelineIndex;

struct ScissorState {
	vec2i min;
	vec2i max;
};

struct DrawingContext {
	VkCommandBuffer commandBuffer;
	PipelineIndex currentPipeline;
	Array<ScissorState> scissorStack;
};

typedef void (*fpRenderCallback_t)(void*, struct Manager*, Array<DrawingContext>&);

struct RenderCallback {
	fpRenderCallback_t callback;
	void *userdata;
};

constexpr i32 MAX_LIGHTS = 256;
constexpr i32 MAX_LIGHTS_PER_BIN = 16;
constexpr i32 LIGHT_BIN_COUNT_X = 32;
constexpr i32 LIGHT_BIN_COUNT_Y = 18;
constexpr i32 LIGHT_BIN_COUNT = LIGHT_BIN_COUNT_X * LIGHT_BIN_COUNT_Y;

struct Light {
	// pixel-space position
	alignas(16) vec3 position;
	alignas(16) vec3 color;
	// How much light reaches the surface at a 90-degree angle of incidence in the range of 0.0 to 1.0
	f32 attenuation;
	// A normalized vector
	alignas(16) vec3 direction;
	// angular falloff in cos(radians) where < min is 100% brightness, between min and max blends, and > max is 0% brightness
	f32 angleMin;
	f32 angleMax;
	// distance-based falloff in pixel-space where < min is 100% brightness, between min and max blends, and > max is 0% brightness
	f32 distMin;
	f32 distMax;
};

struct LightBin {
	u8 lightIndices[MAX_LIGHTS_PER_BIN];
};

struct UniformBuffer {
	vec2 screenSize;
	alignas(16) vec3 ambientLight;
	LightBin lightBins[LIGHT_BIN_COUNT];
	// lights[0] is always a zero-brightness light
	Light lights[MAX_LIGHTS];
};

// I fucking hate Microsoft and every decision they've ever made
// This should never be fucking necessary
#ifdef DrawText
#undef DrawText
#endif

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
		Array<Ptr<vk::CommandPool>> commandPools;
		bool buffer = false; // Which primary command buffer we're on. Switches every frame.
		Ptr<vk::CommandBuffer> commandBufferPrimary[2]; // One for each buffer
		Ptr<vk::CommandBuffer> commandBufferGraphicsTransfer;
		Array<Ptr<vk::CommandBuffer>> commandBuffersSecondary[2];
		Ptr<vk::CommandPool> commandPoolTransfer;
		Ptr<vk::CommandBuffer> commandBufferTransfer;

		Ptr<vk::Semaphore> semaphoreRenderComplete;
		Ptr<vk::QueueSubmission> queueSubmission[2]; // One for each buffer
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
		Ptr<vk::Buffer> vertexBuffer;
		Ptr<vk::Buffer> indexBuffer;

		Ptr<vk::Memory> fontStagingMemory;
		Ptr<vk::Memory> fontBufferMemory;
		Ptr<vk::Memory> fontImageMemory;

		Ptr<vk::Buffer> fontStagingVertexBuffer;
		Range<vk::Buffer> fontStagingImageBuffers;
		Ptr<vk::Buffer> fontVertexBuffer;
		Range<vk::Image> fontImages;

		Array<Ptr<vk::Pipeline>> pipelines;
		Array<StaticArray<Ptr<vk::DescriptorSet>, 4>> pipelineDescriptorSets;
		Ptr<vk::Descriptors> descriptors;
		Ptr<vk::DescriptorSet> descriptorSetUniforms;
		Ptr<vk::DescriptorSet> descriptorSet2D;
		Ptr<vk::DescriptorSet> descriptorSetFont;

		// Functions to call every time Draw is called, in the order they're added.
		Array<RenderCallback> renderCallbacks;
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

	inline void AddRenderCallback(fpRenderCallback_t callback, void* userdata) {
		data.renderCallbacks.Append({callback, userdata});
	}

	bool Init();
	bool Deinit();
	void UpdateLights();
	bool UpdateFonts();
	bool UpdateUniforms();
	bool Draw();
	bool Present();

	void BindPipeline(DrawingContext &context, PipelineIndex pipeline) const;

	void SetScissor(DrawingContext &context, vec2i min, vec2i size);
	void PushScissor(DrawingContext &context, vec2i min, vec2i max);
	void PopScissor(DrawingContext &context);

	void UpdateBackground();

	f32 CharacterWidth(char32 character, const Assets::Font *fontDesired, const Assets::Font *fontFallback) const;
	f32 LineWidth(const char32 *string, i32 fontIndex) const;
	vec2 StringSize(WString string, i32 fontIndex) const;
	f32 StringWidth(WString string, i32 fontIndex) const;
	WString StringAddNewlines(WString string, i32 fontIndex, f32 maxWidth) const;
	void LineCursorStartAndSpaceScale(f32 &dstCursor, f32 &dstSpaceScale, f32 scale, f32 spaceWidth, i32 fontIndex, const char32 *string, f32 maxWidth, FontAlign alignH) const;

	// Units are in screen space
	// DrawChar assumes the font pipeline is bound
	void DrawCharSS(DrawingContext &context, char32 character,
					i32 fontIndex, vec4 color, vec2 position, vec2 scale);
	void DrawTextSS(DrawingContext &context, WString string,
					i32 fontIndex, vec4 color, vec2 position, vec2 scale,
					FontAlign alignH = LEFT, FontAlign alignV = TOP, f32 maxWidth = 0.0f, f32 edge = 0.5f, f32 bounds = 0.5f, Radians32 rotation = 0.0f);
	void DrawQuadSS(DrawingContext &context, vec2 position, vec2 scalePre, vec2 scalePost, vec2 origin = vec2(0.0f), Radians32 rotation = 0.0f, PipelineIndex pipeline=PIPELINE_BASIC_2D, Material material=Material(vec4(1.0f)), TexIndices texture=TexIndices(1), f32 zShear=0.0f, f32 zPos=0.0f, vec2 texScale=1.0f, vec2 texOffset=0.0f) const;
	void DrawCircleSS(DrawingContext &context, i32 texIndex, vec4 color, vec2 position, vec2 scalePre, vec2 scalePost, f32 edge, vec2 origin = vec2(0.0f), Radians32 rotation = 0.0f) const;
	// Units are in pixel space
	void DrawChar(DrawingContext &context, char32 character, i32 fontIndex, vec4 color, vec2 position, vec2 scale);
	void DrawText(DrawingContext &context, WString text, i32 fontIndex, vec4 color, vec2 position, vec2 scale, FontAlign alignH = LEFT, FontAlign alignV = BOTTOM, f32 maxWidth = 0.0f, f32 edge = 0.0f, f32 bounds = 0.5f);
	void DrawQuad(DrawingContext &context, vec2 position, vec2 scalePre, vec2 scalePost, vec2 origin = vec2(0.0f), Radians32 rotation = 0.0f, PipelineIndex pipeline=PIPELINE_BASIC_2D, Material material=Material(vec4(1.0f)), TexIndices texture=TexIndices(1), f32 zShear=0.0f, f32 zPos=0.0f, vec2 texScale=1.0f, vec2 texOffset=0.0f) const;
	void DrawCircle(DrawingContext &context, i32 texIndex, vec4 color, vec2 position, vec2 scalePre, vec2 scalePost, vec2 origin = vec2(0.0f), Radians32 rotation = 0.0f) const;
};

f32 StringHeight(WString string);

} // namespace Az2D::Rendering

#endif // AZ2D_RENDERING_HPP
