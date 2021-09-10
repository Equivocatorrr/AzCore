/*
	File: rendering.hpp
	Author: Philip Haynes
	Utilities and structures to aid in rendering.
*/

#ifndef RENDERING_HPP
#define RENDERING_HPP

#include "AzCore/memory.hpp"
#include "AzCore/vk.hpp"

namespace AzCore {
namespace io {
	struct Window;
}
}

namespace Assets {
	struct Texture;
	struct Font;
}

namespace Rendering {

using namespace AzCore;

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

struct PushConstants {
	struct vert_t {
		mat2 transform = mat2(1.0f);
		vec2 origin = vec2(0.0f);
		vec2 position = vec2(0.0f);
		void Push(VkCommandBuffer commandBuffer, const Manager *rendering) const;
	} vert;
	struct frag_t {
		vec4 color = vec4(1.0f);
		int texIndex = 0;
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
	PIPELINE_2D,
	PIPELINE_FONT,
	PIPELINE_CIRCLE
};

struct ScissorState {
	vec2i min;
	vec2i max;
};

struct DrawingContext {
	VkCommandBuffer commandBuffer;
	PipelineEnum currentPipeline;
	Array<ScissorState> scissorStack;
};

typedef void (*fpRenderCallback_t)(void*, struct Manager*, Array<DrawingContext>&);

struct RenderCallback {
	fpRenderCallback_t callback;
	void *userdata;
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
		Ptr<vk::Framebuffer> framebuffer;
		Ptr<vk::RenderPass> renderPass;
		Ptr<vk::Queue> queueGraphics;
		Ptr<vk::Queue> queuePresent;
		i32 concurrency = 1;
		Array<Ptr<vk::CommandPool>> commandPools;
		bool buffer = false; // Which primary command buffer we're on. Switches every frame.
		Ptr<vk::CommandBuffer> commandBufferPrimary[2]; // One for each buffer
		Array<Ptr<vk::CommandBuffer>> commandBuffersSecondary[2];

		Ptr<vk::Semaphore> semaphoreImageAvailable;
		Ptr<vk::Semaphore> semaphoreRenderComplete;
		Ptr<vk::QueueSubmission> queueSubmission[2]; // One for each buffer
		Ptr<vk::QueueSubmission> queueSubmissionTransfer;

		Ptr<vk::Sampler> textureSampler;

		Ptr<vk::Memory> stagingMemory;
		Ptr<vk::Memory> bufferMemory; // Uniform buffers, vertex buffers, index buffers
		Ptr<vk::Memory> textureMemory;

		Ptr<vk::Buffer> vertexBuffer;
		Ptr<vk::Buffer> indexBuffer;

		Ptr<vk::Memory> fontStagingMemory;
		Ptr<vk::Memory> fontBufferMemory;
		Ptr<vk::Memory> fontImageMemory;

		Ptr<vk::Buffer> fontStagingVertexBuffer;
		Range<vk::Buffer> fontStagingImageBuffers;
		Ptr<vk::Buffer> fontVertexBuffer;
		Range<vk::Image> fontImages;

		Ptr<vk::Pipeline> pipeline2D;
		Ptr<vk::Pipeline> pipelineFont;
		Ptr<vk::Pipeline> pipelineCircle;
		Ptr<vk::Descriptors> descriptors;
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
	FrametimeCounter frametimeCounter;

	inline void AddRenderCallback(fpRenderCallback_t callback, void* userdata) {
		data.renderCallbacks.Append({callback, userdata});
	}

	bool Init();
	bool Deinit();
	bool UpdateFonts();
	bool Draw();

	void BindPipeline2D(DrawingContext &context) const;
	void BindPipelineFont(DrawingContext &context) const;
	void BindPipelineCircle(DrawingContext &context) const;

	void PushScissor(DrawingContext &context, vec2i min, vec2i max);
	void PopScissor(DrawingContext &context);

	inline void UpdateBackground() {
		backgroundRGB = hsvToRgb(backgroundHSV);
	}

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
	void DrawQuadSS(DrawingContext &context, i32 texIndex, vec4 color, vec2 position, vec2 scalePre, vec2 scalePost, vec2 origin = vec2(0.0f), Radians32 rotation = 0.0f) const;
	void DrawCircleSS(DrawingContext &context, i32 texIndex, vec4 color, vec2 position, vec2 scalePre, vec2 scalePost, f32 edge, vec2 origin = vec2(0.0f), Radians32 rotation = 0.0f) const;
	// Units are in pixel space
	void DrawChar(DrawingContext &context, char32 character, i32 fontIndex, vec4 color, vec2 position, vec2 scale);
	void DrawText(DrawingContext &context, WString text, i32 fontIndex, vec4 color, vec2 position, vec2 scale, FontAlign alignH = LEFT, FontAlign alignV = BOTTOM, f32 maxWidth = 0.0f, f32 edge = 0.0f, f32 bounds = 0.5f);
	void DrawQuad(DrawingContext &context, i32 texIndex, vec4 color, vec2 position, vec2 scalePre, vec2 scalePost, vec2 origin = vec2(0.0f), Radians32 rotation = 0.0f) const;
	void DrawCircle(DrawingContext &context, i32 texIndex, vec4 color, vec2 position, vec2 scalePre, vec2 scalePost, vec2 origin = vec2(0.0f), Radians32 rotation = 0.0f) const;
};

f32 StringHeight(WString string);

}

#endif // RENDERING_HPP
