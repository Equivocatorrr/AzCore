/*
	File: gpu.hpp
	Author: Philip Haynes
	A high-level API for writing gpu-based applications that uses Vulkan as a backend.
*/

#ifndef AZCORE_GPU_HPP
#define AZCORE_GPU_HPP

#include "basictypes.hpp"
#include "Assert.hpp"
#include "Memory/String.hpp"
#include "Memory/Result.hpp"
#include "Memory/Array.hpp"
#include "Memory/ArrayWithBucket.hpp"
#include "Math/vec4_t.hpp"

#include "Time.hpp"
#include "IO/Window.hpp"

namespace AzCore::io {
	struct Window;
} // namespace AzCore::io

#define AZ_TRY(command) if (auto result = (command); result.isError)

namespace AzCore::GPU {

// Usable as an enum or as bitfields
enum class ShaderStage : u32 {
	VERTEX          = 0x01,
	TESS_CONTROL    = 0x02,
	TESS_EVALUATION = 0x04,
	GEOMETRY        = 0x08,
	FRAGMENT        = 0x10,
	COMPUTE         = 0x20,
	// TODO: Support VK_KHR_ray_tracing_pipeline
};
const Str ShaderStageString(ShaderStage shaderStage);

enum class ShaderValueType : u16 {
	U32=0,
	I32,
	IVEC2,
	IVEC3,
	IVEC4,
	F32,
	VEC2,
	VEC3,
	VEC4,
	MAT2,
	MAT2X3,
	MAT2X4,
	MAT3X2,
	MAT3,
	MAT3X4,
	MAT4X2,
	MAT4X3,
	MAT4,
	F64,
	DVEC2,
	DVEC3,
	DVEC4,
};

// Used to describe vertex inputs to pipelines
enum class Topology {
	POINT_LIST = 0,
	LINE_LIST = 1,
	LINE_STRIP = 2,
	TRIANGLE_LIST = 3,
	TRIANGLE_STRIP = 4,
	TRIANGLE_FAN = 5,
	LINE_LIST_WITH_ADJACENCY = 6,
	LINE_STRIP_WITH_ADJACENCY = 7,
	TRIANGLE_LIST_WITH_ADJACENCY = 8,
	TRIANGLE_STRIP_WITH_ADJACENCY = 9,
	PATCH_LIST = 10,
};

enum class CullingMode {
	NONE  = 0,
	FRONT = 1,
	BACK  = 2,
};

// Used to determine triangle front faces
enum class Winding {
	COUNTER_CLOCKWISE = 0,
	CLOCKWISE         = 1,
};

#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif

enum class BoolOrDefault {
	DEFAULT,
	TRUE,
	FALSE,
};

constexpr bool operator!=(BoolOrDefault lhs, bool rhs) {
	switch (lhs) {
		case BoolOrDefault::DEFAULT:
			return true;
		case BoolOrDefault::TRUE:
			return rhs != true;
		case BoolOrDefault::FALSE:
			return rhs != false;
	}
	return true;
}

inline bool ResolveBoolOrDefault(BoolOrDefault value, bool defaultValue) {
	switch (value) {
	case BoolOrDefault::DEFAULT:
		return defaultValue;
	case BoolOrDefault::TRUE:
		return true;
	case BoolOrDefault::FALSE:
		return false;
	}
	AzAssert(false, "BoolOrDefault value is not a valid enum value");
	return defaultValue;
}

enum class CompareOp {
	ALWAYS_FALSE = 0,
	LESS = 1,
	EQUAL = 2,
	LESS_OR_EQUAL = 3,
	GREATER = 4,
	NOT_EQUAL = 5,
	GREATER_OR_EQUAL = 6,
	ALWAYS_TRUE = 7,
};

enum class ImageComponentType : u16 {
	SRGB=0, // A UNORM in the sRGB color space (alpha channel is regular UNORM)
	UNORM,  // An unsigned integer representing values between 0 and 1 inclusive
	SNORM,  // A signed integer representing values between -1 and 1 inclusive (NOTE: the minimum int value, e.g. -128, is out of bounds and will be clamped to -1.0)
	USCALED,// An unsigned integer value that's converted to float by the sampler (i.e. 255 will be converted to 255.0f)
	SSCALED,// A signed integer value that's converted to float by the sampler (i.e. -128 will be converted to -128.0f)
	UINT,   // An unsigned integer
	SINT,   // A signed integer
	UFLOAT, // An unsigned floating point number
	SFLOAT, // A signed floating point number
};
extern Str imageComponentTypeStrings[9];

// TODO: Support more oddball bit layouts
enum class ImageBits : u16 {
	// Can be UNORM, SNORM, USCALED, SSCALED, UINT, SINT, SRGB
	R8,
	R8G8,
	R8G8B8,
	R8G8B8A8, // For SRGB, A is just regular UNORM
	B8G8R8,
	B8G8R8A8,
	// Can be UNORM, SNORM, USCALED, SSCALED, UINT, SINT, SFLOAT
	R16,
	R16G16,
	R16G16B16,
	R16G16B16A16,
	// Can be UINT, SINT, SFLOAT
	R32,
	R32G32,
	R32G32B32,
	R32G32B32A32,
	R64,
	R64G64,
	R64G64B64,
	R64G64B64A64,
	// Special UNORM-only
	R4G4,
	R4G4B4A4,
	R5G6B5,
	R5G5B5A1,
	// Depth
	D16, // Can only be UNORM
	D24, // Can only be UNORM
	D32, // Can only be SFLOAT
	// Special other
	A2R10G10B10, // Can be UNORM, SNORM, USCALED, SSCALED, UINT, SINT
	B10G11R11, // Can only be UFLOAT
	E5B9G9R9, // Can only be UFLOAT (E represents a shared exponent)
	// TODO: Compressed formats
};
extern Str imageBitsStrings[23];

enum class AddressMode : u16 {
	REPEAT,
	REPEAT_MIRRORED,
	CLAMP_TO_EDGE,
	CLAMP_TO_BORDER,
};
extern Str addressModeStrings[4];

enum class Filter : u16 {
	NEAREST,
	LINEAR,
	CUBIC,
};
extern Str filterStrings[3];

#ifdef OPAQUE
#undef OPAQUE
#endif
#ifdef TRANSPARENT
#undef TRANSPARENT
#endif

struct BlendMode {
	enum Kind {
		OPAQUE,
		TRANSPARENT,
		ADDITION,
		MIN,
		MAX,
	} kind = OPAQUE;
	// TODO: Add more common blend modes
	bool alphaPremult = false;
	BlendMode() = default;
	BlendMode(Kind _kind) : kind(_kind) {}
	BlendMode(Kind _kind, bool _alphaPremult) : kind(_kind), alphaPremult(_alphaPremult) {}
	constexpr bool operator==(const BlendMode &other) const {
		return kind == other.kind && alphaPremult == other.alphaPremult;
	}
	constexpr bool operator!=(const BlendMode &other) const {
		return !operator==(other);
	}
};
// NOTE: We probably want to support more specific stuff anyway, so probably make a mechanism for that.

enum class ImageLayout {
	// This is fine whenever you don't care about preserving the contents of the image.
	UNDEFINED,
	TRANSFER_SRC,
	TRANSFER_DST,
	ATTACHMENT,
	SHADER_READ,
};

// Base device from which everything else is allocated
struct Device;

// Single-thread context for commands
struct Context;

// Single shader module
struct Shader;

// Shaders, resource bindings, draw state, etc.
struct Pipeline;

// GPU-side buffer which can be memory mapped host-side and committed (transferred) on a Context
struct Buffer;

// GPU-side image which can be memory mapped host-side and committed (transferred) on a Context
struct Image;

// Used to determine how images are sampled in shaders.
struct Sampler;

// Describes the ability to render to an Image or a Window surface
struct Framebuffer;

// Describes a window with a surface
struct Window;

// Synchronization primitive for 1-way dependencies
struct Semaphore;


// Global settings


void SetAppName(Str appName);

void EnableValidationLayers();



// Initialize the API
[[nodiscard]] Result<VoidResult_t, String> Initialize();

void Deinitialize();


// Window


// Returns the index of the window, used for making a framebuffer, or an error message in a String
// This can only error if Initialize was called before AddWindow
[[nodiscard]] Result<Window*, String> AddWindow(io::Window *window, String tag = String());

/* Note on Framebuffer Attachments:
	If a Framebuffer contains a Window attachment, all other attachments will be automatically resized to match the Window's dimensions.
	This is necessary because with XCB, our surface can change size between io::Window::Update() calls, making it impossible to handle image resizing externally without a race condition.
	It's also just more convenient this way :)
	For more complicated rendering with multiple Framebuffers feeding into each other, there will always be the possibility of some undesired scaling for one frame when resizing the window, but at least it won't crash in that case.
*/

void FramebufferAddImage(Framebuffer *framebuffer, Image *image);
void FramebufferAddWindow(Framebuffer *framebuffer, Window *window);

// This will resolve our image into resolveImage after rendering is done.
// To do multiple passes before resolving, just use FramebufferAddImage and resolve manually.
void FramebufferAddImageMultisampled(Framebuffer *framebuffer, Image *image, Image *resolveImage);
// This will resolve our image into resolveWindow after rendering is done.
// To do multiple passes before resolving, just use FramebufferAddImage and resolve manually.
void FramebufferAddImageMultisampled(Framebuffer *framebuffer, Image *image, Window *resolveWindow);

// If there are any changes to attachments, you must recreate the framebuffer
[[nodiscard]] Result<VoidResult_t, String> FramebufferCreate(Framebuffer *framebuffer);

void SetVSync(Window *window, bool enable);

bool GetVSyncEnabled(Window *window);

[[nodiscard]] Result<VoidResult_t, String> WindowUpdate(Window *window);

// waitContexts are any contexts that the window's resulting image depend on. These only need to be populated if not using external synchronization like ContextWaitUntilFinished().
[[nodiscard]] Result<VoidResult_t, String> WindowPresent(Window *window, ArrayWithBucket<Semaphore*, 4> waitSemaphores={});


// Creating new objects


[[nodiscard]] Device* NewDevice(String tag = String());

[[nodiscard]] Context* NewContext(Device *device, String tag = String());

[[nodiscard]] Shader* NewShader(Device *device, String filename, ShaderStage stage, String tag = String());

[[nodiscard]] Pipeline* NewGraphicsPipeline(Device *device, String tag = String());

[[nodiscard]] Pipeline* NewComputePipeline(Device *device, String tag = String());

// Add a buffer that describes vertex data
[[nodiscard]] Buffer* NewVertexBuffer(Device *device, String tag = String());

// Add a buffer that describes indices into a vertex buffer
// bytesPerIndex determines whether we use u16 or u32 indices.
[[nodiscard]] Buffer* NewIndexBuffer(Device *device, String tag = String(), u32 bytesPerIndex=2);

// Add a buffer that can hold a large amount of memory, and supports read, write, and atomic operations
[[nodiscard]] Buffer* NewStorageBuffer(Device *device, String tag = String());

// Add a buffer that can hold a small amount of memory, and supports read operations
[[nodiscard]] Buffer* NewUniformBuffer(Device *device, String tag = String());

[[nodiscard]] Image* NewImage(Device *device, String tag = String());

[[nodiscard]] Sampler* NewSampler(Device *device, String tag = String());

[[nodiscard]] Framebuffer* NewFramebuffer(Device *device, String tag = String());


// Device


// feature names are as defined in the Vulkan Spec under Features
void DeviceRequireFeatures(Device *device, const ArrayWithBucket<Str, 8> &features);

void DeviceWaitIdle(Device *device);


// Buffer, Image

// If we're initted, the contents of the buffer are not preserved.
[[nodiscard]] Result<VoidResult_t, String> BufferSetSize(Buffer *buffer, i64 sizeBytes);
// If we're initted, the contents of the buffer are preserved.
[[nodiscard]] Result<VoidResult_t, String> BufferResize(Buffer *buffer, i64 sizeBytes, Context *copyContext);

// shaderStages is a bitmask of ShaderStage
void BufferSetShaderUsage(Buffer *buffer, u32 shaderStages);

i64 BufferGetSize(Buffer *buffer);

// returns true if the format changed and you need to call ImageRecreate
bool ImageSetFormat(Image *image, ImageBits imageBits, ImageComponentType componentType);

// returns true if the size changed and you need to call ImageRecreate
bool ImageSetSize(Image *image, i32 width, i32 height, u32 maxMipLevels=UINT32_MAX);

// returns true if the flag changed and you need to call ImageRecreate
bool ImageSetMipmapping(Image *image, bool enableMipmapping, u32 maxLevels=UINT32_MAX);

// shaderStages is a bitmask of ShaderStage
// returns true if the shader stages changed and you need to call ImageRecreate
bool ImageSetShaderUsage(Image *image, u32 shaderStages);

// sampleCount must be a power of 2
// returns true if the sample count changed and you need to call ImageRecreate
bool ImageSetSampleCount(Image *image, u32 sampleCount);

// If you change formats, size, mipmapping, shader usage, or sample count (or make a new image) after having called GPU::Initialize(), you must use this to recreate the image.
[[nodiscard]] Result<VoidResult_t, String> ImageRecreate(Image *image);


// Sampler


// When enabled, it linearly interpolates between miplevels.
void SamplerSetMipmapFiltering(Sampler *sampler, bool enabled);

void SamplerSetFiltering(Sampler *sampler, Filter magFilter, Filter minFilter);

void SamplerSetAddressMode(Sampler *sampler, AddressMode addressModeU, AddressMode addressModeV, AddressMode addressModeW=AddressMode::CLAMP_TO_BORDER);

void SamplerSetLod(Sampler *sampler, f32 bias, f32 minimum=0.0f, f32 maximum=1000.0f);

// anisotropy is the number of samples taken in anisotropic filtering (only relevant with mipmapped images)
void SamplerSetAnisotropy(Sampler *sampler, i32 anisotropy);

void SamplerSetCompare(Sampler *sampler, bool enable, CompareOp op=CompareOp::ALWAYS_TRUE);

// isFloat should be true when using with float images, false with integer images
// Border is white when white is true, black otherwise.
// Border is opaque when opaque is true, completely transparent otherwise.
void SamplerSetBorderColor(Sampler *sampler, bool isFloat, bool white, bool opaque);


// Pipeline


void PipelineAddShaders(Pipeline *pipeline, ArrayWithBucket<Shader*, 4> shaders);

void PipelineAddVertexInputs(Pipeline *pipeline, const ArrayWithBucket<ShaderValueType, 8> &inputs);

void PipelineSetTopology(Pipeline *pipeline, Topology topology);

void PipelineSetCullingMode(Pipeline *pipeline, CullingMode cullingMode);

void PipelineSetWinding(Pipeline *pipeline, Winding winding);

void PipelineSetDepthBias(Pipeline *pipeline, bool enable, f32 constant=0.0f, f32 slope=0.0f, f32 clampValue=0.0f);

void PipelineSetLineWidth(Pipeline *pipeline, f32 lineWidth);

void PipelineSetDepthTest(Pipeline *pipeline, bool enabled);

void PipelineSetDepthWrite(Pipeline *pipeline, bool enabled);

void PipelineSetDepthCompareOp(Pipeline *pipeline, CompareOp compareOp);

void PipelineSetBlendMode(Pipeline *pipeline, BlendMode blendMode, i32 attachment = 0);

// minFraction is shadedSamples / totalSamples
void PipelineSetMultisampleShading(Pipeline *pipeline, bool enabled, f32 minFraction=1.0f);

// shaderStages is a bitmask of ShaderStage values
void PipelineAddPushConstantRange(Pipeline *pipeline, u32 offset, u32 size, u32 shaderStages);


// Context, Commands


[[nodiscard]] Semaphore* ContextGetCurrentSemaphore(Context *context, i32 semaphoreIndex=0);
[[nodiscard]] Semaphore* ContextGetPreviousSemaphore(Context *context, i32 semaphoreIndex=0);

[[nodiscard]] Result<VoidResult_t, String> ContextBeginRecording(Context *context);

[[nodiscard]] Result<VoidResult_t, String> ContextBeginRecordingSecondary(Context *context, Framebuffer *framebuffer, i32 subpass);

[[nodiscard]] Result<VoidResult_t, String> ContextEndRecording(Context *context);

// numSemaphores is how many semaphores to signal
[[nodiscard]] Result<VoidResult_t, String> SubmitCommands(Context *context, i32 numSemaphores=0, ArrayWithBucket<Semaphore*, 4> waitSemaphores={});

// Returns true if Context is still executing
[[nodiscard]] Result<bool, String> ContextIsExecuting(Context *context);

// Returns true if we timed out
[[nodiscard]] Result<bool, String> ContextWaitUntilFinished(Context *context, Nanoseconds timeout=Nanoseconds(INT64_MAX));

[[nodiscard]] Result<VoidResult_t, String> CmdExecuteSecondary(Context *primary, Context *secondary);

// Combines the copy of data to the host buffer and from the host buffer to the device-local buffer
[[nodiscard]] Result<VoidResult_t, String> CmdCopyDataToBuffer(Context *context, Buffer *dst, void *src, i64 dstOffset=0, i64 size=0);

[[nodiscard]] Result<void*, String> BufferMapHostMemory(Buffer *buffer, i64 offset=0, i64 size=0);
void BufferUnmapHostMemory(Buffer *buffer);

[[nodiscard]] Result<VoidResult_t, String> CmdCopyHostBufferToDeviceBuffer(Context *context, Buffer *buffer, i64 dstOffset=0, i64 size=0);

void CmdCopyBufferToBuffer(Context *context, Buffer *dst, Buffer *src, i64 dstOffset=0, i64 srcOffset=0, i64 size=0);

[[nodiscard]] Result<VoidResult_t, String> CmdCopyDataToImage(Context *context, Image *dst, void *src);

// if buffer is nullptr, then it removes the framebuffer binding from context, and on commit it will complete the render pass and not begin another one.
void CmdBindFramebuffer(Context *context, Framebuffer *framebuffer);

void CmdBindPipeline(Context *context, Pipeline *pipeline);

// if buffer is nullptr, then it removes any vertex buffer binding from context, preventing it from getting bound automagically later
void CmdBindVertexBuffer(Context *context, Buffer *buffer);

// if buffer is nullptr, then it removes any index buffer binding from context, preventing it from getting bound automagically later
void CmdBindIndexBuffer(Context *context, Buffer *buffer);

// Clears all the descriptor bindings, allowing you to specify smaller descriptor sets. Without this, all previous bindings will be combined, even if they're not in use by previous pipelines.
void CmdClearDescriptors(Context *context);

void CmdBindUniformBuffer(Context *context, Buffer *buffer, i32 set, i32 binding);

void CmdBindUniformBufferArray(Context *context, const Array<Buffer*> &buffers, i32 set, i32 binding);

void CmdBindStorageBuffer(Context *context, Buffer *buffer, i32 set, i32 binding);

void CmdBindStorageBufferArray(Context *context, const Array<Buffer*> &buffer, i32 set, i32 binding);

void CmdBindImageSampler(Context *context, Image *image, Sampler *sampler, i32 set, i32 binding);

void CmdBindImageArraySampler(Context *context, const Array<Image*> &images, Sampler *sampler, i32 set, i32 binding);

// Before recording draw commands, you have to commit all your bindings at once
[[nodiscard]] Result<VoidResult_t, String> CmdCommitBindings(Context *context);

// Ends the render pass that was started with a CmdBindFramebuffer call followed by CmdCommitBindings.
// If you need to process the image drawn in the same context, you need to use this, otherwise it'll be automatically called in CmdEndRecording.
// doGenMipmaps only has an effect if images attached to the framebuffer have mipmaps enabled.
// Also resets all the bindings on the context, so you must bind things again if necessary.
void CmdFinishFramebuffer(Context *context, bool doGenMipmaps=true);

void CmdImageTransitionLayout(Context *context, Image *image, ImageLayout from, ImageLayout to, i32 baseMipLevel=0, i32 mipLevelCount=-1);

void CmdImageGenerateMipmaps(Context *context, Image *image, ImageLayout from, ImageLayout to);

void CmdPushConstants(Context *context, const void *src, u32 offset, u32 size);

void CmdSetViewport(Context *context, f32 width, f32 height, f32 minDepth=0.0f, f32 maxDepth=1.0f, f32 x=0.0f, f32 y=0.0f);

void CmdSetScissor(Context *context, u32 width, u32 height, i32 x=0, i32 y=0);

inline void CmdSetViewportAndScissor(Context *context, f32 width, f32 height, f32 minDepth=0.0f, f32 maxDepth=1.0f, f32 x=0.0f, f32 y=0.0f) {
	CmdSetViewport(context, width, height, minDepth, maxDepth, x, y);
	CmdSetScissor(context, (u32)width, (u32)height, (i32)x, (i32)y);
}

void CmdSetLineWidth(Context *context, f32 lineWidth);

void CmdSetDepthTestEnable(Context *context, bool enable);
void CmdSetDepthWriteEnable(Context *context, bool enable);
void CmdSetDepthCompareOp(Context *context, CompareOp op);

void CmdClearColorAttachment(Context *context, vec4 color, i32 attachment=0);
void CmdClearDepthAttachment(Context *context, f32 depth);

void CmdDraw(Context *context, i32 count, i32 vertexOffset, i32 instanceCount=1, i32 instanceOffset=0);

void CmdDrawIndexed(Context *context, i32 count, i32 indexOffset, i32 vertexOffset, i32 instanceCount=1, i32 instanceOffset=0);

} // namespace AzCore::GPU

namespace AzCore {

inline void AppendToString(String &string, GPU::ImageComponentType imageComponentType) {
	AppendToString(string, GPU::imageComponentTypeStrings[(u32)imageComponentType]);
}

inline void AppendToString(String &string, GPU::ImageBits imageBits) {
	AppendToString(string, GPU::imageBitsStrings[(u32)imageBits]);
}

};

#endif // AZCORE_GPU_HPP