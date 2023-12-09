/*
	File: gpu.cpp
	Author: Philip Haynes
*/

#include "gpu.hpp"
#include "QuickSort.hpp"

namespace AzCore {

namespace GPU {

struct DescriptorSetLayout;
struct DescriptorBindings;

} // namespace GPU

template<u16 bounds>
constexpr i32 IndexHash(const GPU::DescriptorSetLayout&);
template<u16 bounds>
constexpr i32 IndexHash(const GPU::DescriptorBindings&);

} // namespace AzCore

#include "Memory/HashMap.hpp"
#include "Memory/BinaryMap.hpp"
#include "Memory/UniquePtr.hpp"
#include "Memory/Optional.hpp"

#ifdef __unix
	#define VK_USE_PLATFORM_XCB_KHR
	#define VK_USE_PLATFORM_WAYLAND_KHR
	#include "IO/Linux/WindowData.hpp"
#elif defined(_WIN32)
	#define VK_USE_PLATFORM_WIN32_KHR
	#include "IO/Win32/WindowData.hpp"
#endif
#include <vulkan/vulkan.h>

// More WinAPI bullshit
// These can't be removed because WindowData.hpp and vulkan.h both bring in WinAPI headers
#ifdef OPAQUE
#undef OPAQUE
#endif
#ifdef TRANSPARENT
#undef TRANSPARENT
#endif
#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

namespace AzCore::GPU {

#ifndef Utils

BoolOrDefault BoolOrDefaultFromBool(bool on) {
	return on ? BoolOrDefault::TRUE : BoolOrDefault::FALSE;
}

// This is a value that only measures order of events, not actual time between them.
u64 GetTimestamp() {
	static u64 nextVal = 0;
	return nextVal++;
}

const Str ShaderStageString(ShaderStage shaderStage) {
	switch (shaderStage) {
	case ShaderStage::VERTEX:
		return "VERTEX";
	case ShaderStage::TESS_CONTROL:
		return "TESS_CONTROL";
	case ShaderStage::TESS_EVALUATION:
		return "TESS_EVALUATION";
	case ShaderStage::GEOMETRY:
		return "GEOMETRY";
	case ShaderStage::FRAGMENT:
		return "FRAGMENT";
	case ShaderStage::COMPUTE:
		return "COMPUTE";
	default:
		return "INVALID";
	}
}

// Per-location stride, equal to alignment when not using the scalarBlockLayout device feature.
constexpr i64 ShaderValueTypeStride[] = {
	/* U32 */    4,
	/* I32 */    4,
	/* IVEC2 */  8,
	/* IVEC3 */  16,
	/* IVEC4 */  16,
	/* F32 */    4,
	/* VEC2 */   8,
	/* VEC3 */   16,
	/* VEC4 */   16,
	/* MAT2 */   8,
	/* MAT2X3 */ 16,
	/* MAT2X4 */ 16,
	/* MAT3X2 */ 8,
	/* MAT3 */   16,
	/* MAT3X4 */ 16,
	/* MAT4X2 */ 8,
	/* MAT4X3 */ 16,
	/* MAT4 */   16,
	/* F64 */    8,
	/* DVEC2 */  16,
	// A special exception must be made for DVEC3, as the second location's stride is half the first
	/* DVEC3 */  16,
	/* DVEC4 */  16,
};
// Per-location stride, used when the scalarBlockLayout device feature is used.
constexpr i64 ShaderValueTypeStrideScalarBlockLayout[] = {
	/* U32 */    4,
	/* I32 */    4,
	/* IVEC2 */  8,
	/* IVEC3 */  12,
	/* IVEC4 */  16,
	/* F32 */    4,
	/* VEC2 */   8,
	/* VEC3 */   12,
	/* VEC4 */   16,
	/* MAT2 */   8,
	/* MAT2X3 */ 12,
	/* MAT2X4 */ 16,
	/* MAT3X2 */ 8,
	/* MAT3 */   12,
	/* MAT3X4 */ 16,
	/* MAT4X2 */ 8,
	/* MAT4X3 */ 12,
	/* MAT4 */   16,
	/* F64 */    8,
	/* DVEC2 */  16,
	// A special exception must be made for DVEC3, as the second location's stride is half the first
	/* DVEC3 */  16,
	/* DVEC4 */  16,
};
// Per-location alignment, used when scalarBlockLayout device feature is used.
constexpr i64 ShaderValueTypeAlignmentScalarBlockLayout[] = {
	/* U32 */    4,
	/* I32 */    4,
	/* IVEC2 */  4,
	/* IVEC3 */  4,
	/* IVEC4 */  4,
	/* F32 */    4,
	/* VEC2 */   4,
	/* VEC3 */   4,
	/* VEC4 */   4,
	/* MAT2 */   4,
	/* MAT2X3 */ 4,
	/* MAT2X4 */ 4,
	/* MAT3X2 */ 4,
	/* MAT3 */   4,
	/* MAT3X4 */ 4,
	/* MAT4X2 */ 4,
	/* MAT4X3 */ 4,
	/* MAT4 */   4,
	/* F64 */    8,
	/* DVEC2 */  8,
	/* DVEC3 */  8,
	/* DVEC4 */  8,
};
// How many location bindings this value consumes
constexpr i64 ShaderValueNumLocations[] = {
	/* U32 */    1,
	/* I32 */    1,
	/* IVEC2 */  1,
	/* IVEC3 */  1,
	/* IVEC4 */  1,
	/* F32 */    1,
	/* VEC2 */   1,
	/* VEC3 */   1,
	/* VEC4 */   1,
	/* MAT2 */   2,
	/* MAT2X3 */ 2,
	/* MAT2X4 */ 2,
	/* MAT3X2 */ 3,
	/* MAT3 */   3,
	/* MAT3X4 */ 3,
	/* MAT4X2 */ 4,
	/* MAT4X3 */ 4,
	/* MAT4 */   4,
	/* F64 */    1,
	/* DVEC2 */  1,
	/* DVEC3 */  2,
	/* DVEC4 */  2,
};
// Format that describes a single location (gets duplicated ShaderValueNumLocations times)
constexpr VkFormat ShaderValueFormats[] = {
	/* U32 */    VK_FORMAT_R32_UINT,
	/* I32 */    VK_FORMAT_R32_SINT,
	/* IVEC2 */  VK_FORMAT_R32G32_SINT,
	/* IVEC3 */  VK_FORMAT_R32G32B32_SINT,
	/* IVEC4 */  VK_FORMAT_R32G32B32A32_SINT,
	/* F32 */    VK_FORMAT_R32_SFLOAT,
	/* VEC2 */   VK_FORMAT_R32G32_SFLOAT,
	/* VEC3 */   VK_FORMAT_R32G32B32_SFLOAT,
	/* VEC4 */   VK_FORMAT_R32G32B32A32_SFLOAT,
	/* MAT2 */   VK_FORMAT_R32G32_SFLOAT,
	/* MAT2X3 */ VK_FORMAT_R32G32B32_SFLOAT,
	/* MAT2X4 */ VK_FORMAT_R32G32B32A32_SFLOAT,
	/* MAT3X2 */ VK_FORMAT_R32G32_SFLOAT,
	/* MAT3 */   VK_FORMAT_R32G32B32_SFLOAT,
	/* MAT3X4 */ VK_FORMAT_R32G32B32A32_SFLOAT,
	/* MAT4X2 */ VK_FORMAT_R32G32_SFLOAT,
	/* MAT4X3 */ VK_FORMAT_R32G32B32_SFLOAT,
	/* MAT4 */   VK_FORMAT_R32G32B32A32_SFLOAT,
	/* F64 */    VK_FORMAT_R64_SFLOAT,
	/* DVEC2 */  VK_FORMAT_R64G64_SFLOAT,
	// A special exception must be made for DVEC3, as the second location's format is VK_FORMAT_R64_SFLOAT
	/* DVEC3 */  VK_FORMAT_R64G64_SFLOAT,
	/* DVEC4 */  VK_FORMAT_R64G64_SFLOAT,
};

Str imageComponentTypeStrings[9] = {
	"SRGB",
	"UNORM",
	"SNORM",
	"USCALED",
	"SSCALED",
	"UINT",
	"SINT",
	"UFLOAT",
	"SFLOAT",
};

Str imageBitsStrings[23] = {
	"R8",
	"R8G8",
	"R8G8B8",
	"R8G8B8A8",

	"R16",
	"R16G16",
	"R16G16B16",
	"R16G16B16A16",

	"R32",
	"R32G32",
	"R32G32B32",
	"R32G32B32A32",

	"R64",
	"R64G64",
	"R64G64B64",
	"R64G64B64A64",

	"R4G4",
	"R4G4B4A4",
	"R5G6B5",
	"R5G5B5A1",

	"A2R10G10B10",
	"B10G11R11",
	"E5B9G9R9",
};

String VkResultString(VkResult errorCode) {
	switch (errorCode) {
#define STR(r) case VK_ ##r: return #r
		STR(SUCCESS);
		STR(NOT_READY);
		STR(TIMEOUT);
		STR(EVENT_SET);
		STR(EVENT_RESET);
		STR(INCOMPLETE);
		STR(ERROR_OUT_OF_HOST_MEMORY);
		STR(ERROR_OUT_OF_DEVICE_MEMORY);
		STR(ERROR_INITIALIZATION_FAILED);
		STR(ERROR_DEVICE_LOST);
		STR(ERROR_MEMORY_MAP_FAILED);
		STR(ERROR_LAYER_NOT_PRESENT);
		STR(ERROR_EXTENSION_NOT_PRESENT);
		STR(ERROR_FEATURE_NOT_PRESENT);
		STR(ERROR_INCOMPATIBLE_DRIVER);
		STR(ERROR_TOO_MANY_OBJECTS);
		STR(ERROR_FORMAT_NOT_SUPPORTED);
		STR(ERROR_FRAGMENTED_POOL);
		STR(ERROR_UNKNOWN);
	#if VK_VERSION_1_1
		STR(ERROR_OUT_OF_POOL_MEMORY);
		STR(ERROR_INVALID_EXTERNAL_HANDLE);
	#endif
	#if VK_VERSION_1_2
		STR(ERROR_FRAGMENTATION);
		STR(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
	#endif
	#if VK_KHR_surface
		STR(ERROR_SURFACE_LOST_KHR);
		STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
	#endif
	#if VK_KHR_swapchain
		STR(SUBOPTIMAL_KHR);
		STR(ERROR_OUT_OF_DATE_KHR);
	#endif
	#if VK_KHR_display_swapchain
		STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
	#endif
	#if VK_EXT_debug_report
		STR(ERROR_VALIDATION_FAILED_EXT);
	#endif
	#if VK_NV_glsl_shader
		STR(ERROR_INVALID_SHADER_NV);
	#endif
	#if VK_EXT_image_drm_format_modifier
		STR(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
	#endif
	#if VK_EXT_global_priority
		STR(ERROR_NOT_PERMITTED_EXT);
	#endif
	#if VK_EXT_full_screen_exclusive
		STR(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
	#endif
	#if VK_KHR_deferred_host_operations
		STR(THREAD_IDLE_KHR);
	#endif
	#if VK_KHR_deferred_host_operations
		STR(THREAD_DONE_KHR);
	#endif
	#if VK_KHR_deferred_host_operations
		STR(OPERATION_DEFERRED_KHR);
	#endif
	#if VK_KHR_deferred_host_operations
		STR(OPERATION_NOT_DEFERRED_KHR);
	#endif
	#if VK_EXT_pipeline_creation_cache_control
		STR(PIPELINE_COMPILE_REQUIRED_EXT);
	#endif
#undef STR
	default:
		return Stringify("UNKNOWN_ERROR 0x", FormatInt((i64)errorCode, 16));
	}
}

String VkFormatString(VkFormat format) {
	switch (format) {
#define STR(r) case VK_FORMAT_ ##r: return #r
		STR(UNDEFINED);
		STR(R4G4_UNORM_PACK8);
		STR(R4G4B4A4_UNORM_PACK16);
		STR(B4G4R4A4_UNORM_PACK16);
		STR(R5G6B5_UNORM_PACK16);
		STR(B5G6R5_UNORM_PACK16);
		STR(R5G5B5A1_UNORM_PACK16);
		STR(B5G5R5A1_UNORM_PACK16);
		STR(A1R5G5B5_UNORM_PACK16);
		STR(R8_UNORM);
		STR(R8_SNORM);
		STR(R8_USCALED);
		STR(R8_SSCALED);
		STR(R8_UINT);
		STR(R8_SINT);
		STR(R8_SRGB);
		STR(R8G8_UNORM);
		STR(R8G8_SNORM);
		STR(R8G8_USCALED);
		STR(R8G8_SSCALED);
		STR(R8G8_UINT);
		STR(R8G8_SINT);
		STR(R8G8_SRGB);
		STR(R8G8B8_UNORM);
		STR(R8G8B8_SNORM);
		STR(R8G8B8_USCALED);
		STR(R8G8B8_SSCALED);
		STR(R8G8B8_UINT);
		STR(R8G8B8_SINT);
		STR(R8G8B8_SRGB);
		STR(B8G8R8_UNORM);
		STR(B8G8R8_SNORM);
		STR(B8G8R8_USCALED);
		STR(B8G8R8_SSCALED);
		STR(B8G8R8_UINT);
		STR(B8G8R8_SINT);
		STR(B8G8R8_SRGB);
		STR(R8G8B8A8_UNORM);
		STR(R8G8B8A8_SNORM);
		STR(R8G8B8A8_USCALED);
		STR(R8G8B8A8_SSCALED);
		STR(R8G8B8A8_UINT);
		STR(R8G8B8A8_SINT);
		STR(R8G8B8A8_SRGB);
		STR(B8G8R8A8_UNORM);
		STR(B8G8R8A8_SNORM);
		STR(B8G8R8A8_USCALED);
		STR(B8G8R8A8_SSCALED);
		STR(B8G8R8A8_UINT);
		STR(B8G8R8A8_SINT);
		STR(B8G8R8A8_SRGB);
		STR(A8B8G8R8_UNORM_PACK32);
		STR(A8B8G8R8_SNORM_PACK32);
		STR(A8B8G8R8_USCALED_PACK32);
		STR(A8B8G8R8_SSCALED_PACK32);
		STR(A8B8G8R8_UINT_PACK32);
		STR(A8B8G8R8_SINT_PACK32);
		STR(A8B8G8R8_SRGB_PACK32);
		STR(A2R10G10B10_UNORM_PACK32);
		STR(A2R10G10B10_SNORM_PACK32);
		STR(A2R10G10B10_USCALED_PACK32);
		STR(A2R10G10B10_SSCALED_PACK32);
		STR(A2R10G10B10_UINT_PACK32);
		STR(A2R10G10B10_SINT_PACK32);
		STR(A2B10G10R10_UNORM_PACK32);
		STR(A2B10G10R10_SNORM_PACK32);
		STR(A2B10G10R10_USCALED_PACK32);
		STR(A2B10G10R10_SSCALED_PACK32);
		STR(A2B10G10R10_UINT_PACK32);
		STR(A2B10G10R10_SINT_PACK32);
		STR(R16_UNORM);
		STR(R16_SNORM);
		STR(R16_USCALED);
		STR(R16_SSCALED);
		STR(R16_UINT);
		STR(R16_SINT);
		STR(R16_SFLOAT);
		STR(R16G16_UNORM);
		STR(R16G16_SNORM);
		STR(R16G16_USCALED);
		STR(R16G16_SSCALED);
		STR(R16G16_UINT);
		STR(R16G16_SINT);
		STR(R16G16_SFLOAT);
		STR(R16G16B16_UNORM);
		STR(R16G16B16_SNORM);
		STR(R16G16B16_USCALED);
		STR(R16G16B16_SSCALED);
		STR(R16G16B16_UINT);
		STR(R16G16B16_SINT);
		STR(R16G16B16_SFLOAT);
		STR(R16G16B16A16_UNORM);
		STR(R16G16B16A16_SNORM);
		STR(R16G16B16A16_USCALED);
		STR(R16G16B16A16_SSCALED);
		STR(R16G16B16A16_UINT);
		STR(R16G16B16A16_SINT);
		STR(R16G16B16A16_SFLOAT);
		STR(R32_UINT);
		STR(R32_SINT);
		STR(R32_SFLOAT);
		STR(R32G32_UINT);
		STR(R32G32_SINT);
		STR(R32G32_SFLOAT);
		STR(R32G32B32_UINT);
		STR(R32G32B32_SINT);
		STR(R32G32B32_SFLOAT);
		STR(R32G32B32A32_UINT);
		STR(R32G32B32A32_SINT);
		STR(R32G32B32A32_SFLOAT);
		STR(R64_UINT);
		STR(R64_SINT);
		STR(R64_SFLOAT);
		STR(R64G64_UINT);
		STR(R64G64_SINT);
		STR(R64G64_SFLOAT);
		STR(R64G64B64_UINT);
		STR(R64G64B64_SINT);
		STR(R64G64B64_SFLOAT);
		STR(R64G64B64A64_UINT);
		STR(R64G64B64A64_SINT);
		STR(R64G64B64A64_SFLOAT);
		STR(B10G11R11_UFLOAT_PACK32);
		STR(E5B9G9R9_UFLOAT_PACK32);
		STR(D16_UNORM);
		STR(X8_D24_UNORM_PACK32);
		STR(D32_SFLOAT);
		STR(S8_UINT);
		STR(D16_UNORM_S8_UINT);
		STR(D24_UNORM_S8_UINT);
		STR(D32_SFLOAT_S8_UINT);
		STR(BC1_RGB_UNORM_BLOCK);
		STR(BC1_RGB_SRGB_BLOCK);
		STR(BC1_RGBA_UNORM_BLOCK);
		STR(BC1_RGBA_SRGB_BLOCK);
		STR(BC2_UNORM_BLOCK);
		STR(BC2_SRGB_BLOCK);
		STR(BC3_UNORM_BLOCK);
		STR(BC3_SRGB_BLOCK);
		STR(BC4_UNORM_BLOCK);
		STR(BC4_SNORM_BLOCK);
		STR(BC5_UNORM_BLOCK);
		STR(BC5_SNORM_BLOCK);
		STR(BC6H_UFLOAT_BLOCK);
		STR(BC6H_SFLOAT_BLOCK);
		STR(BC7_UNORM_BLOCK);
		STR(BC7_SRGB_BLOCK);
		STR(ETC2_R8G8B8_UNORM_BLOCK);
		STR(ETC2_R8G8B8_SRGB_BLOCK);
		STR(ETC2_R8G8B8A1_UNORM_BLOCK);
		STR(ETC2_R8G8B8A1_SRGB_BLOCK);
		STR(ETC2_R8G8B8A8_UNORM_BLOCK);
		STR(ETC2_R8G8B8A8_SRGB_BLOCK);
		STR(EAC_R11_UNORM_BLOCK);
		STR(EAC_R11_SNORM_BLOCK);
		STR(EAC_R11G11_UNORM_BLOCK);
		STR(EAC_R11G11_SNORM_BLOCK);
		STR(ASTC_4x4_UNORM_BLOCK);
		STR(ASTC_4x4_SRGB_BLOCK);
		STR(ASTC_5x4_UNORM_BLOCK);
		STR(ASTC_5x4_SRGB_BLOCK);
		STR(ASTC_5x5_UNORM_BLOCK);
		STR(ASTC_5x5_SRGB_BLOCK);
		STR(ASTC_6x5_UNORM_BLOCK);
		STR(ASTC_6x5_SRGB_BLOCK);
		STR(ASTC_6x6_UNORM_BLOCK);
		STR(ASTC_6x6_SRGB_BLOCK);
		STR(ASTC_8x5_UNORM_BLOCK);
		STR(ASTC_8x5_SRGB_BLOCK);
		STR(ASTC_8x6_UNORM_BLOCK);
		STR(ASTC_8x6_SRGB_BLOCK);
		STR(ASTC_8x8_UNORM_BLOCK);
		STR(ASTC_8x8_SRGB_BLOCK);
		STR(ASTC_10x5_UNORM_BLOCK);
		STR(ASTC_10x5_SRGB_BLOCK);
		STR(ASTC_10x6_UNORM_BLOCK);
		STR(ASTC_10x6_SRGB_BLOCK);
		STR(ASTC_10x8_UNORM_BLOCK);
		STR(ASTC_10x8_SRGB_BLOCK);
		STR(ASTC_10x10_UNORM_BLOCK);
		STR(ASTC_10x10_SRGB_BLOCK);
		STR(ASTC_12x10_UNORM_BLOCK);
		STR(ASTC_12x10_SRGB_BLOCK);
		STR(ASTC_12x12_UNORM_BLOCK);
		STR(ASTC_12x12_SRGB_BLOCK);
	#if VK_VERSION_1_1
		STR(G8B8G8R8_422_UNORM);
		STR(B8G8R8G8_422_UNORM);
		STR(G8_B8_R8_3PLANE_420_UNORM);
		STR(G8_B8R8_2PLANE_420_UNORM);
		STR(G8_B8_R8_3PLANE_422_UNORM);
		STR(G8_B8R8_2PLANE_422_UNORM);
		STR(G8_B8_R8_3PLANE_444_UNORM);
		STR(R10X6_UNORM_PACK16);
		STR(R10X6G10X6_UNORM_2PACK16);
		STR(R10X6G10X6B10X6A10X6_UNORM_4PACK16);
		STR(G10X6B10X6G10X6R10X6_422_UNORM_4PACK16);
		STR(B10X6G10X6R10X6G10X6_422_UNORM_4PACK16);
		STR(G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16);
		STR(G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16);
		STR(G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16);
		STR(G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16);
		STR(G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16);
		STR(R12X4_UNORM_PACK16);
		STR(R12X4G12X4_UNORM_2PACK16);
		STR(R12X4G12X4B12X4A12X4_UNORM_4PACK16);
		STR(G12X4B12X4G12X4R12X4_422_UNORM_4PACK16);
		STR(B12X4G12X4R12X4G12X4_422_UNORM_4PACK16);
		STR(G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16);
		STR(G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16);
		STR(G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16);
		STR(G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16);
		STR(G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16);
		STR(G16B16G16R16_422_UNORM);
		STR(B16G16R16G16_422_UNORM);
		STR(G16_B16_R16_3PLANE_420_UNORM);
		STR(G16_B16R16_2PLANE_420_UNORM);
		STR(G16_B16_R16_3PLANE_422_UNORM);
		STR(G16_B16R16_2PLANE_422_UNORM);
		STR(G16_B16_R16_3PLANE_444_UNORM);
	#endif
	#if VK_VERSION_1_3
		STR(G8_B8R8_2PLANE_444_UNORM);
		STR(G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16);
		STR(G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16);
		STR(G16_B16R16_2PLANE_444_UNORM);
		STR(A4R4G4B4_UNORM_PACK16);
		STR(A4B4G4R4_UNORM_PACK16);
		STR(ASTC_4x4_SFLOAT_BLOCK);
		STR(ASTC_5x4_SFLOAT_BLOCK);
		STR(ASTC_5x5_SFLOAT_BLOCK);
		STR(ASTC_6x5_SFLOAT_BLOCK);
		STR(ASTC_6x6_SFLOAT_BLOCK);
		STR(ASTC_8x5_SFLOAT_BLOCK);
		STR(ASTC_8x6_SFLOAT_BLOCK);
		STR(ASTC_8x8_SFLOAT_BLOCK);
		STR(ASTC_10x5_SFLOAT_BLOCK);
		STR(ASTC_10x6_SFLOAT_BLOCK);
		STR(ASTC_10x8_SFLOAT_BLOCK);
		STR(ASTC_10x10_SFLOAT_BLOCK);
		STR(ASTC_12x10_SFLOAT_BLOCK);
		STR(ASTC_12x12_SFLOAT_BLOCK);
	#endif
	#if VK_IMG_format_pvrtc
		STR(PVRTC1_2BPP_UNORM_BLOCK_IMG);
		STR(PVRTC1_4BPP_UNORM_BLOCK_IMG);
		STR(PVRTC2_2BPP_UNORM_BLOCK_IMG);
		STR(PVRTC2_4BPP_UNORM_BLOCK_IMG);
		STR(PVRTC1_2BPP_SRGB_BLOCK_IMG);
		STR(PVRTC1_4BPP_SRGB_BLOCK_IMG);
		STR(PVRTC2_2BPP_SRGB_BLOCK_IMG);
		STR(PVRTC2_4BPP_SRGB_BLOCK_IMG);
	#endif
	#if VK_NV_optical_flow
		STR(R16G16_S10_5_NV);
	#endif
#undef STR
	default:
		return Stringify("UNKNOWN_FORMAT 0x", FormatInt((i64)format, 16));
	}
}

String FormatSize(u64 size) {
	String str;
	if (size > 1024*1024*1024) {
		AppendToString(str, size/(1024*1024*1024));
		AppendToString(str, " GiB");
		size %= (1024*1024*1024);
	}
	if (size > 1024*1024) {
		if (str.size) {
			AppendToString(str, ", ");
		}
		AppendToString(str, size/(1024*1024));
		AppendToString(str, " MiB");
		size %= (1024*1024);
	}
	if (size > 1024) {
		if (str.size) {
			AppendToString(str, ", ");
		}
		AppendToString(str, size/1024);
		AppendToString(str, " KiB");
		size %= 1024;
	}
	if (size > 0) {
		if (str.size) {
			AppendToString(str, ", ");
		}
		AppendToString(str, size);
		AppendToString(str, " B");
	}
	return str;
}

#define CHECK_INIT(obj) AzAssert((obj)->initted == false, "Trying to init a " #obj " that's already initted")
#define CHECK_DEINIT(obj) AzAssert((obj)->initted == true, "Trying to deinit a " #obj " that's not initted")
#define TRACE_INIT(obj) io::cout.PrintLnDebug("Initializing " #obj " \"", (obj)->tag, "\"");
#define TRACE_DEINIT(obj) io::cout.PrintLnDebug("Deinitializing " #obj " \"", (obj)->tag, "\"");

#define ERROR_RESULT(obj, ...) Stringify(#obj " \"", (obj)->tag, "\" error in ", __FUNCTION__, ":", Indent(), "\n", __VA_ARGS__)
#define WARNING(obj, ...) io::cout.PrintLn(#obj " \"", (obj)->tag, "\" warning in ", __FUNCTION__, ": ", __VA_ARGS__)

#define INIT_HEAD(obj) CHECK_INIT(obj); TRACE_INIT(obj)
#define DEINIT_HEAD(obj) CHECK_DEINIT(obj); TRACE_DEINIT(obj)

#endif

#ifndef Command_Recording

/*
	In order to enable a simple API, we record binding commands and have the user commit them all at once to create the renderpass, descriptors and pipelines. Naturally, we want to cache these, so a fast and robust way to detect existing configurations is necessary.
	Alternatively, we might choose auto-commit on a draw call, but that would increase overhead slightly, which would add up quickly with lots of draw calls.
*/

struct DescriptorIndex {
	i32 set, binding;
	DescriptorIndex() = default;
	constexpr DescriptorIndex(i32 _set, i32 _binding) : set(_set), binding(_binding) {}
	inline bool operator<(DescriptorIndex other) {
		if (set < other.set) return true;
		return binding < other.binding;
	}
	inline bool operator==(DescriptorIndex other) {
		return set == other.set && binding == other.binding;
	}
};

struct Binding {
	enum Kind {
		FRAMEBUFFER,
		PIPELINE,
		VERTEX_BUFFER,
		INDEX_BUFFER,
		UNIFORM_BUFFER,
		STORAGE_BUFFER,
		IMAGE_SAMPLER,
	} kind;
	union {
		struct {
			Framebuffer *object;
		} framebuffer;
		struct {
			Pipeline *object;
		} pipeline;
		struct {
			Buffer *object;
		} vertexBuffer;
		struct {
			Buffer *object;
		} indexBuffer;
		struct {
			Buffer *object;
			DescriptorIndex binding;
		} uniformBuffer;
		struct {
			Buffer *object;
			DescriptorIndex binding;
		} storageBuffer;
		struct {
			Sampler *sampler;
			DescriptorIndex binding;
			ArrayWithBucket<Image*, 8> images;
		} imageSampler;
		// For generic descriptor access to binding
		struct {
			void *object;
			DescriptorIndex binding;
		} anyDescriptor;
		// For generic buffer access
		struct {
			Buffer *object;
		} anyBuffer;
	};
	// Assumes kind matches other.kind
	inline void _Assign(const Binding &other) {
		switch (kind) {
			case FRAMEBUFFER:
				framebuffer.object = other.framebuffer.object;
				break;
			case PIPELINE:
				pipeline.object = other.pipeline.object;
				break;
			case VERTEX_BUFFER:
			case INDEX_BUFFER:
				anyBuffer.object = other.anyBuffer.object;
				break;
			case UNIFORM_BUFFER:
			case STORAGE_BUFFER:
				anyDescriptor.object = other.anyDescriptor.object;
				anyDescriptor.binding = other.anyDescriptor.binding;
				break;
			case IMAGE_SAMPLER:
				imageSampler.sampler = other.imageSampler.sampler;
				imageSampler.binding = other.imageSampler.binding;
				imageSampler.images = other.imageSampler.images;
				break;
			default:
				AzAssert(false, "Unreachable");
				break;
		}
	}
	// Assumes kind matches other kind
	inline void _Move(Binding &&other) {
		switch (kind) {
			case FRAMEBUFFER:
				framebuffer.object = other.framebuffer.object;
				break;
			case PIPELINE:
				pipeline.object = other.pipeline.object;
				break;
			case VERTEX_BUFFER:
			case INDEX_BUFFER:
				anyBuffer.object = other.anyBuffer.object;
				break;
			case UNIFORM_BUFFER:
			case STORAGE_BUFFER:
				anyDescriptor.object = other.anyDescriptor.object;
				anyDescriptor.binding = other.anyDescriptor.binding;
				break;
			case IMAGE_SAMPLER:
				imageSampler.sampler = other.imageSampler.sampler;
				imageSampler.binding = other.imageSampler.binding;
				imageSampler.images = std::move(other.imageSampler.images);
				break;
			default:
				AzAssert(false, "Unreachable");
				break;
		}
	}
	Binding() {
		AzPlacementNew(imageSampler.images);
	}
	Binding(const Binding &other) : kind(other.kind) {
		AzPlacementNew(imageSampler.images);
		_Assign(other);
	}
	Binding(Binding &&other) : kind(other.kind) {
		AzPlacementNew(imageSampler.images);
		_Move(std::move(other));
	}
	~Binding() {
		if (kind == IMAGE_SAMPLER) {
			imageSampler.images.~ArrayWithBucket();
		}
	}
	Binding& operator=(const Binding &other) {
		if (kind != other.kind) {
			this->~Binding();
			kind = other.kind;
			AzPlacementNew(imageSampler.images);
		}
		_Assign(other);
		return *this;
	}
	Binding& operator=(Binding &&other) {
		if (kind != other.kind) {
			this->~Binding();
			kind = other.kind;
			AzPlacementNew(imageSampler.images);
		}
		_Move(std::move(other));
		return *this;
	}
};

#endif

#ifndef Declarations_and_global_variables

template <typename T>
using List = Array<UniquePtr<T>>;

struct Fence {
	VkFence vkFence;

	Device *device;
	String tag;
	bool initted = false;

	Fence() = default;
	Fence(Device *_device, String _tag) : device(_device), tag(_tag) {}
};

struct Semaphore {
	VkSemaphore vkSemaphore;

	Device *device;
	String tag;
	bool initted = false;

	Semaphore() = default;
	Semaphore(Device *_device, String _tag) : device(_device), tag(_tag) {}
};

struct Window {
	bool vsync = false;
	bool attachment = true;
	bool transferDst = false;

	bool shouldReconfigure = false;

	io::Window *window;

	Framebuffer *framebuffer = nullptr;

	VkSurfaceCapabilitiesKHR surfaceCaps;
	Array<VkSurfaceFormatKHR> surfaceFormatsAvailable;
	Array<VkPresentModeKHR> presentModesAvailable;
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
	i32 numImages;
	struct SwapchainImage {
		VkImage vkImage;
		VkImageView vkImageView;
	};
	Array<SwapchainImage> swapchainImages;
	Array<Fence> acquireFences;
	Array<Semaphore> acquireSemaphores;
	// We get this one from vkAcquireNextImageKHR
	i32 currentImage;
	// We increment this one ourselves
	i32 currentSync;

	VkSurfaceKHR vkSurface;
	VkSwapchainKHR vkSwapchain;

	Device *device;
	String tag;
	bool initted = false;

	Window() = default;
	Window(io::Window *_window, String _tag) : window(_window), tag(_tag) {}
};

struct PhysicalDevice {
	VkPhysicalDeviceProperties2 properties;
	VkPhysicalDeviceFeatures2 vk10Features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	VkPhysicalDeviceVulkan11Features vk11Features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
	VkPhysicalDeviceMultiviewFeatures vkMultiviewFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES};
	VkPhysicalDeviceVulkan12Features vk12Features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
	VkPhysicalDeviceVulkan13Features vk13Features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
	Array<VkExtensionProperties> extensionsAvailable{};
	Array<VkQueueFamilyProperties2> queueFamiliesAvailable{};
	VkPhysicalDeviceMemoryProperties2 memoryProperties;

	VkPhysicalDevice vkPhysicalDevice;

	PhysicalDevice() = default;
	PhysicalDevice(VkPhysicalDevice _vkPhysicalDevice) : vkPhysicalDevice(_vkPhysicalDevice) {
		properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
		vkGetPhysicalDeviceProperties2(vkPhysicalDevice, &properties);

		io::cout.PrintLnDebug("Reading Physical Device Info for \"", properties.properties.deviceName, "\"");

		vk10Features.pNext = &vk11Features;
		vk11Features.pNext = &vkMultiviewFeatures;
		vkMultiviewFeatures.pNext = &vk12Features;
		vk12Features.pNext = &vk13Features;
		vkGetPhysicalDeviceFeatures2(vkPhysicalDevice, &vk10Features);

		u32 extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &extensionCount, nullptr);
		extensionsAvailable.Resize(extensionCount);
		vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &extensionCount, extensionsAvailable.data);

		u32 queueFamiliesCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties2(vkPhysicalDevice, &queueFamiliesCount, nullptr);
		queueFamiliesAvailable.Resize(queueFamiliesCount, {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
		vkGetPhysicalDeviceQueueFamilyProperties2(vkPhysicalDevice, &queueFamiliesCount, queueFamiliesAvailable.data);

		memoryProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
		vkGetPhysicalDeviceMemoryProperties2(vkPhysicalDevice, &memoryProperties);
	}
};

struct Instance {
	String appName = "AzCore::GPU App";
	bool enableValidationLayers = false;
	Array<PhysicalDevice> physicalDevices;
	Array<VkExtensionProperties> extensionsAvailable;
	Array<VkLayerProperties> layersAvailable;

	PFN_vkSetDebugUtilsObjectNameEXT fpSetDebugUtilsObjectNameEXT;
	VkInstance vkInstance;

	bool initted = false;

	Instance() {
		u32 extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		extensionsAvailable.Resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionsAvailable.data);
		u32 layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		layersAvailable.Resize(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, layersAvailable.data);
	}
};

struct Memory {
	struct Page {
		VkDeviceMemory vkMemory;
		struct Segment {
			u32 begin;
			u32 size;
			bool used;
		};
		Array<Segment> segments;
	};
	Array<Page> pages;
	// 64MiB sounds reasonable right?
	u32 pageSizeMin = 1024*1024*64;

	u32 memoryTypeIndex;
	Device *device = nullptr;
	String tag;

	Memory() = default;
	Memory(Device *_device, u32 _memoryTypeIndex, String _tag=Str()) : memoryTypeIndex(_memoryTypeIndex), device(_device), tag(_tag) {}
};

struct Allocation {
	Memory *memory;
	i32 page;
	u32 offset;
};

// Used to de-duplicate layouts
struct DescriptorSetLayout {
	VkDescriptorSetLayoutCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	Array<VkDescriptorSetLayoutBinding> bindings;
	
	// Some housekeeping, since createInfo must reference bindings, moving/copying etc would break createInfo.
	DescriptorSetLayout() = default;
	DescriptorSetLayout(const DescriptorSetLayout &other) : bindings(other.bindings) {
		createInfo.bindingCount = bindings.size;
		createInfo.pBindings = bindings.data;
	}
	DescriptorSetLayout(DescriptorSetLayout &&other) : bindings(std::move(other.bindings)) {
		createInfo.bindingCount = bindings.size;
		createInfo.pBindings = bindings.data;
	}
	DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
	DescriptorSetLayout& operator=(DescriptorSetLayout &&other) {
		bindings = std::move(other.bindings);
		createInfo.bindingCount = bindings.size;
		createInfo.pBindings = bindings.data;
		return *this;
	}
	bool operator==(const DescriptorSetLayout &other) const {
		return bindings == other.bindings;
	}
};

struct DescriptorBinding {
	enum Kind {
		UNIFORM_BUFFER,
		STORAGE_BUFFER,
		IMAGE_SAMPLER,
	} kind;
	ArrayWithBucket<void*, 8> objects;
	Sampler *sampler = nullptr;
	DescriptorBinding() = default;
	explicit DescriptorBinding(const ArrayWithBucket<Buffer*, 8> buffers);
	explicit DescriptorBinding(const ArrayWithBucket<Image*, 8> images, Sampler *_sampler) : kind(IMAGE_SAMPLER), objects(images.size), sampler(_sampler) {
		memcpy(objects.data, images.data, images.size * sizeof(void*));
	}
	bool operator!=(const DescriptorBinding &other) const {
		return kind != other.kind || objects != other.objects || sampler != other.sampler;
	}
};

// Used to de-duplicated the actual sets
struct DescriptorBindings {
	ArrayWithBucket<DescriptorBinding, 4> bindings;
	bool operator==(const DescriptorBindings &other) const {
		return bindings == other.bindings;
	}
};

struct DescriptorSet {
	VkDescriptorPool vkDescriptorPool;
	VkDescriptorSet vkDescriptorSet;
	Array<u64*> descriptorTimestamps;
	u64 timestamp;
};

struct Device {
	List<Context> contexts;
	List<Shader> shaders;
	List<Pipeline> pipelines;
	List<Buffer> buffers;
	List<Image> images;
	List<Sampler> samplers;
	List<Framebuffer> framebuffers;
	List<DescriptorSet> descriptorSets;
	// Map from memoryType to Memory
	HashMap<u32, Memory> memory;
	HashMap<DescriptorSetLayout, VkDescriptorSetLayout> vkDescriptorSetLayouts;
	HashMap<DescriptorBindings, DescriptorSet*> descriptorSetsMap;

	Ptr<PhysicalDevice> physicalDevice;
	
	VkPhysicalDeviceFeatures2 vk10Features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	VkPhysicalDeviceVulkan11Features vk11Features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
	VkPhysicalDeviceVulkan12Features vk12Features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
	VkPhysicalDeviceVulkan13Features vk13Features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
	
	VkDevice vkDevice;
	VkQueue vkQueue;
	i32 queueFamilyIndex;

	String tag;
	bool initted = false;

	Device() = default;
	Device(String _tag) : tag(_tag) {}
};

struct BoundDescriptorSet {
	VkDescriptorSetLayout layout;
	VkDescriptorSet set;
};

struct ContextFrame {
	VkCommandBuffer vkCommandBuffer;
	Fence fence;
	Array<Semaphore> semaphores;
	ArrayWithBucket<BoundDescriptorSet, 4> descriptorSetsBound;
};

struct Context {
	VkCommandPool vkCommandPool;
	Array<ContextFrame> frames;

	struct {
		Framebuffer *framebuffer = nullptr;
		Pipeline *pipeline = nullptr;
		Buffer *vertexBuffer = nullptr;
		Buffer *indexBuffer = nullptr;
		BinaryMap<DescriptorIndex, Binding> descriptors;
		bool damage = false;
	} bindings;
	Array<Binding> bindCommands;

	enum class State {
		NOT_RECORDING = 0,
		DONE_RECORDING = 1,
		RECORDING_PRIMARY = 2,
		RECORDING_SECONDARY = 3,
	} state = State::NOT_RECORDING;
	i32 numFrames = 3;
	i32 currentFrame = 0;

	Device *device;
	String tag;
	bool initted = false;

	Context() = default;
	Context(Device *_device, String _tag) : device(_device), tag(_tag) {}
};

inline bool ContextIsRecording(Context *context) {
	return (u32)context->state >= (u32)Context::State::RECORDING_PRIMARY;
}

struct Shader {
	String filename;
	ShaderStage stage;
	
	// TODO: Specialization constants
	
	VkShaderModule vkShaderModule;
	
	Device *device;
	String tag; // defaults to filename if not given
	bool initted = false;
	
	Shader() = default;
	Shader(Device *_device, String _filename, ShaderStage _stage, String _tag) : filename(_filename), stage(_stage), device(_device), tag(_tag) {}
};

struct Pipeline {
	Array<Shader*> shaders;
	ArrayWithBucket<ShaderValueType, 8> vertexInputs;
	
	Topology topology = Topology::TRIANGLE_LIST;
	
	CullingMode cullingMode = CullingMode::NONE;
	Winding winding = Winding::COUNTER_CLOCKWISE;
	// depthBias is calculated as (constant + slope*m) where m can either be sqrt(dFdx(z)^2 + dFdy(z)^2) or fwidth(z) depending on the implementation.
	// for clampValue > 0, depthBias = min(depthBias, clampValue)
	// for clampValue < 0, depthBias = max(depthBias, clampValue)
	// for clampValue == 0, depthBias is unchanged
	// The absolute bias depthBias represents depends on the depth buffer format. In general, a value of 1.0 corresponds to the minimum depth difference representable by the depth buffer.
	// TODO: Clarify the above statement.
	struct DepthBias {
		bool enable = false;
		f32 constant = 0.0f;
		f32 slope = 0.0f;
		f32 clampValue = 0.0f;
	} depthBias;
	f32 lineWidth = 1.0f;
	
	// DEFAULT means true if we have a depth buffer, else false
	BoolOrDefault depthTest = BoolOrDefault::DEFAULT;
	BoolOrDefault depthWrite = BoolOrDefault::DEFAULT;
	CompareOp depthCompareOp = CompareOp::LESS;
	
	// One for each possible color attachment
	BlendMode blendModes[8];
	
	struct {
		bool enabled = false;
		f32 minFraction = 1.0f;
	} multisampleShading;
	
	Array<VkPushConstantRange> pushConstantRanges;

	enum Kind {
		GRAPHICS,
		COMPUTE,
	} kind=GRAPHICS;

	// Keep track of current layout properties so we don't have to recreate everything all the time
	VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo{};
	VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
	VkPipeline vkPipeline = VK_NULL_HANDLE;
	// Used only to check if framebuffer changed
	struct {
		u32 sampleCount = 1;
		bool framebufferHasDepthBuffer = false;
		i32 numColorAttachments = 0;
	};
	
	Device *device;
	String tag;
	bool initted = false;

	Pipeline() = default;
	Pipeline(Device *_device, Kind _kind, String _tag) : kind(_kind), device(_device), tag(_tag) {}
};

struct Buffer {
	enum Kind {
		UNDEFINED=0,
		VERTEX_BUFFER,
		INDEX_BUFFER,
		STORAGE_BUFFER,
		UNIFORM_BUFFER,
	} kind=UNDEFINED;
	
	u32 shaderStages = 0;

	i64 size = 0;

	// Used only for index buffers
	VkIndexType indexType = VK_INDEX_TYPE_UINT16;
	
	VkBuffer vkBuffer;
	VkBuffer vkBufferHostVisible;
	VkMemoryRequirements memoryRequirements;
	Allocation alloc;
	Allocation allocHostVisible;

	// Used to determine when to update descriptors
	u64 timestamp;
	Device *device;
	String tag;
	bool initted = false;
	// Whether our host-visible buffer is active
	bool hostVisible = false;

	Buffer() = default;
	Buffer(Kind _kind, Device *_device, String _tag) : kind(_kind), device(_device), tag(_tag) {}
};

struct Image {
	// Usage flags
	u32 shaderStages = 0;
	bool attachment = false;
	bool transferSrc = false;
	bool transferDst = true;
	bool mipmapped = false;

	i32 width=1, height=1;
	i32 bytesPerPixel=4;

	u32 mipLevels = 1;
	u32 sampleCount = 1;

	VkImage vkImage;
	VkImageView vkImageView;
	VkImageView vkImageViewAttachment;
	VkBuffer vkBufferHostVisible;
	VkFormat vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageAspectFlags vkImageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
	VkMemoryRequirements memoryRequirements;
	VkMemoryRequirements memoryRequirementsHost;
	Allocation alloc;
	Allocation allocHostVisible;

	// Used to determine when to update descriptors
	u64 timestamp;
	Device *device;
	String tag;
	bool initted = false;
	// Whether our host-visible buffer is active
	bool hostVisible = false;

	Image() = default;
	Image(Device *_device, String _tag) : device(_device), tag(_tag) {}
};

struct Sampler {
	Filter magFilter = Filter::LINEAR;
	Filter minFilter = Filter::LINEAR;
	bool mipmapInterpolation = false;
	AddressMode addressModeU = AddressMode::CLAMP_TO_BORDER;
	AddressMode addressModeV = AddressMode::CLAMP_TO_BORDER;
	AddressMode addressModeW = AddressMode::CLAMP_TO_BORDER;
	f32 lodMin = 0.0f;
	f32 lodMax = VK_LOD_CLAMP_NONE;
	f32 lodBias = 0.0f;
	i32 anisotropy = 1;
	// Used for shadow maps
	struct {
		bool enable = false;
		CompareOp op = CompareOp::ALWAYS_TRUE;
	} compare;
	VkBorderColor borderColor=VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	
	VkSampler vkSampler;
	
	Device *device;
	String tag;
	bool initted = false;
	
	Sampler() = default;
	Sampler(Device *_device, String _tag) : device(_device), tag(_tag) {}
};

struct Attachment {
	enum Kind {
		WINDOW,
		IMAGE,
		DEPTH_BUFFER,
	} kind;
	union {
		Window *window;
		Image *image;
		Image *depthBuffer;
	};
	// Whether to load the existing data or leave it undefined
	bool load = false;
	// Whether to keep the data after rendering (depth buffers may not want to bother storing)
	bool store = true;
	Attachment() = default;
	Attachment(Window *_window) : kind(WINDOW), window(_window) {}
	Attachment(Image *_image, bool isDepth) : kind(isDepth ? DEPTH_BUFFER : IMAGE), image(_image) {}
};

struct AttachmentRef {
	Attachment attachment;
	Optional<Attachment> resolveAttachment;
	AttachmentRef() = default;
	AttachmentRef(Attachment _attachment) : attachment(_attachment) {}
	AttachmentRef(Attachment _attachment, Attachment _resolveAttachment) : attachment(_attachment), resolveAttachment(_resolveAttachment) {}
};

struct Framebuffer {
	Array<AttachmentRef> attachmentRefs;
	// Used to determine whether we need to recreate renderpass
	bool attachmentsDirty = false;

	// If we have a WINDOW attachment, this will match the number of swapchain images, else it will just be size 1
	Array<VkFramebuffer> vkFramebuffers;
	VkRenderPass vkRenderPass;
	
	// width and height will be set automagically, just used for easy access
	i32 width, height;
	u32 sampleCount = 1;

	Device *device;
	String tag;
	bool initted = false;

	Framebuffer() = default;
	Framebuffer(Device *_device, String _tag) : device(_device), tag(_tag) {}
};

Instance instance;
List<Device> devices;
List<Window> windows;


DescriptorBinding::DescriptorBinding(const ArrayWithBucket<Buffer*, 8> buffers) : objects(buffers.size) {
	AzAssert(buffers.size > 0, "Cannot create a DescriptorBinding with zero descriptors");
	switch (buffers[0]->kind) {
	case Buffer::UNIFORM_BUFFER:
		kind = UNIFORM_BUFFER;
		break;
	case Buffer::STORAGE_BUFFER:
		kind = STORAGE_BUFFER;
		break;
	default:
		AzAssert(false, Stringify("Invalid buffer type for descriptor: ", (u32)buffers[0]->kind));
		break;
	}
	memcpy(objects.data, buffers.data, buffers.size * sizeof(void*));
}

} // namespace AzCore::GPU

namespace AzCore {

constexpr u64 PRIME1 = 123456789133;
constexpr u64 PRIME2 = 456789123499;

// TODO: Is this overkill? Is it even good?
constexpr void ProgressiveHash(u64 &dst, u64 value) {
	dst += value + PRIME2;
	dst *= PRIME1;
	dst ^= dst >> 31;
	dst ^= dst << 21;
	dst ^= dst >> 13;
}

template<u16 bounds>
constexpr i32 IndexHash(const GPU::DescriptorSetLayout &in) {
	u64 hash = 0;
	for (const VkDescriptorSetLayoutBinding &binding : in.bindings) {
		ProgressiveHash(hash, (u64)binding.binding);
		ProgressiveHash(hash, (u64)binding.descriptorType);
		ProgressiveHash(hash, (u64)binding.descriptorCount);
		ProgressiveHash(hash, (u64)binding.stageFlags);
	}
	return hash % bounds;
}

template<u16 bounds>
constexpr i32 IndexHash(const GPU::DescriptorBindings &in) {
	u64 hash = 0;
	for (const GPU::DescriptorBinding &binding : in.bindings) {
		ProgressiveHash(hash, (u64)binding.kind);
		for (const void *ptr : binding.objects) {
			ProgressiveHash(hash, (u64)ptr);
		}
	}
	return hash % bounds;
}

} // namespace AzCore

namespace AzCore::GPU {

static void SetDebugMarker(Device *device, const String &debugMarker, VkObjectType objectType, u64 objectHandle) {
	if (instance.enableValidationLayers && debugMarker.size != 0) {
		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = objectType;
		nameInfo.objectHandle = objectHandle;
		nameInfo.pObjectName = debugMarker.data;
		instance.fpSetDebugUtilsObjectNameEXT(device->vkDevice, &nameInfo);
	}
}


[[nodiscard]] Result<VoidResult_t, String> FenceInit(Fence *fence, bool startSignaled=false);
void FenceDeinit(Fence *fence);
// VK_SUCCESS indicates it's signaled
// VK_NOT_READY indicates it's not signaled
// VK_ERROR_DEVICE_LOST may also be returned.
[[nodiscard]] VkResult FenceGetStatus(Fence *fence);
// Sets fence state to not signaled
[[nodiscard]] Result<VoidResult_t, String> FenceResetSignaled(Fence *fence);
// dstWasTimout will be set to whether the signal timed out
[[nodiscard]] Result<VoidResult_t, String> FenceWaitForSignal(Fence *fence, u64 timeout=UINT64_MAX, bool *dstWasTimeout=nullptr);

[[nodiscard]] Result<VoidResult_t, String> SemaphoreInit(Semaphore *semaphore);
void SemaphoreDeinit(Semaphore *semaphore);


[[nodiscard]] Result<VoidResult_t, String> WindowSurfaceInit(Window *window);
void WindowSurfaceDeinit(Window *window);

[[nodiscard]] Result<VoidResult_t, String> WindowInit(Window *window);
void WindowDeinit(Window *window);


[[nodiscard]] Result<VoidResult_t, String> DeviceInit(Device *device);
void DeviceDeinit(Device *device);

[[nodiscard]] Result<Allocation, String> MemoryAllocate(Memory *memory, u32 size, u32 alignment);
void MemoryFree(Allocation allocation);


[[nodiscard]] Result<VoidResult_t, String> ContextInit(Context *context);
void ContextDeinit(Context *context);

[[nodiscard]] Result<VoidResult_t, String> ContextDescriptorsCompose(Context *context);


[[nodiscard]] Result<VoidResult_t, String> ShaderInit(Shader *shader);
void ShaderDeinit(Shader *shader);


[[nodiscard]] Result<VoidResult_t, String> PipelineInit(Pipeline *pipeline);
void PipelineDeinit(Pipeline *pipeline);

[[nodiscard]] Result<VoidResult_t, String> BufferInit(Buffer *buffer);
void BufferDeinit(Buffer *buffer);
[[nodiscard]] Result<VoidResult_t, String> BufferHostInit(Buffer *buffer);
void BufferHostDeinit(Buffer *buffer);


[[nodiscard]] Result<VoidResult_t, String> ImageInit(Image *image);
void ImageDeinit(Image *image);
[[nodiscard]] Result<VoidResult_t, String> ImageHostInit(Image *image);
void ImageHostDeinit(Image *image);

[[nodiscard]] Result<VoidResult_t, String> SamplerInit(Sampler *sampler);
void SamplerDeinit(Sampler *sampler);

[[nodiscard]] Result<VoidResult_t, String> FramebufferInit(Framebuffer *framebuffer);
void FramebufferDeinit(Framebuffer *framebuffer);
[[nodiscard]] Result<VoidResult_t, String> FramebufferCreate(Framebuffer *framebuffer);
[[nodiscard]] VkFramebuffer FramebufferGetCurrentVkFramebuffer(Framebuffer *framebuffer);
[[nodiscard]] bool FramebufferHasDepthBuffer(Framebuffer *framebuffer);
// Will return nullptr if there is no Window attachment.
[[nodiscard]] Window* FramebufferGetWindowAttachment(Framebuffer *framebuffer);

#endif

#ifndef Global_settings

void SetAppName(Str appName) {
	instance.appName = appName;
}

void EnableValidationLayers() {
	instance.enableValidationLayers = true;
}

#endif

#ifndef API_Initialization

Result<VoidResult_t, String> Initialize() {
	AzAssert(instance.initted == false, "Initializing an instance that's already initialized");
	VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
	appInfo.pApplicationName = instance.appName.data;
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = "AzCore::GPU";
	appInfo.engineVersion = VK_MAKE_VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	Array<const char*> extensions;
	{ // Add and check availability of extensions
		if (instance.enableValidationLayers) {
			extensions.Append(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		if (windows.size) {
			extensions.Append("VK_KHR_surface");
	#ifdef __unix
			if (windows[0]->window->data->useWayland) {
				extensions.Append("VK_KHR_wayland_surface");
			} else {
				extensions.Append("VK_KHR_xcb_surface");
			}
	#elif defined(_WIN32)
			extensions.Append("VK_KHR_win32_surface");
	#endif
		}
		Array<const char*> extensionsUnavailable(extensions);
		for (i32 i = 0; i < extensionsUnavailable.size; i++) {
			for (i32 j = 0; j < instance.extensionsAvailable.size; j++) {
				if (equals(extensionsUnavailable[i], instance.extensionsAvailable[j].extensionName)) {
					extensionsUnavailable.Erase(i);
					i--;
					break;
				}
			}
		}
		if (extensionsUnavailable.size > 0) {
			String error = "Instance extensions unavailable:";
			for (const char *extension : extensionsUnavailable) {
				error += "\n\t";
				error += extension;
			}
			return error;
		}
	}

	Array<const char*> layers;
	{ // Add and check availablility of layers
		if (instance.enableValidationLayers) {
			io::cout.PrintLn("Enabling validation layers");
			layers = {
				"VK_LAYER_KHRONOS_validation",
			};
		}
		Array<const char*> layersUnavailable(layers);
		for (i32 i = 0; i < layersUnavailable.size; i++) {
			for (i32 j = 0; j < instance.layersAvailable.size; j++) {
				if (equals(layersUnavailable[i], instance.layersAvailable[j].layerName)) {
					layersUnavailable.Erase(i);
					i--;
					break;
				}
			}
		}
		if (layersUnavailable.size > 0) {
			String error = "Instance layers unavailable:";
			for (const char *layer : layersUnavailable) {
				error += "\n\t";
				error += layer;
			}
			return error;
		}
	}

	VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = extensions.size;
	createInfo.ppEnabledExtensionNames = extensions.data;
	createInfo.enabledLayerCount = layers.size;
	createInfo.ppEnabledLayerNames = layers.data;

	VkResult vkResult = vkCreateInstance(&createInfo, nullptr, &instance.vkInstance);
	if (vkResult != VK_SUCCESS) {
		return Stringify("vkCreateInstance failed with ", VkResultString(vkResult));
	}
	instance.initted = true;
	
	if (instance.enableValidationLayers) {
		instance.fpSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance.vkInstance, "vkSetDebugUtilsObjectNameEXT");
		if (instance.fpSetDebugUtilsObjectNameEXT == nullptr) {
			return String("vkGetInstanceProcAddr failed to get vkSetDebugUtilsObjectNameEXT");
		}
	}

	u32 physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance.vkInstance, &physicalDeviceCount, nullptr);
	if (physicalDeviceCount == 0) {
		Deinitialize();
		return String("No GPUs available with Vulkan support");
	}
	Array<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance.vkInstance, &physicalDeviceCount, physicalDevices.data);
	instance.physicalDevices.ClearSoft();
	for (i32 i = 0; i < physicalDevices.size; i++) {
		instance.physicalDevices.Append(PhysicalDevice(physicalDevices[i]));
	}

	for (auto &window : windows) {
		auto result = WindowSurfaceInit(window.RawPtr());
		if (result.isError) return result.error;
	}

	for (auto &device : devices) {
		auto result = DeviceInit(device.RawPtr());
		if (result.isError) return result.error;
	}

	return VoidResult_t();
}

void Deinitialize() {
	AzAssert(instance.initted, "Deinitializing an instance that wasn't Initialized");
	for (auto &device : devices) {
		DeviceDeinit(device.RawPtr());
	}
	for (auto &window : windows) {
		WindowSurfaceDeinit(window.RawPtr());
	}
	vkDestroyInstance(instance.vkInstance, nullptr);
	instance.initted = false;
}

#endif

#ifndef Synchronization_Primitives

Result<VoidResult_t, String> FenceInit(Fence *fence, bool startSignaled) {
	INIT_HEAD(fence);
	VkFenceCreateInfo createInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	if (startSignaled) {
		createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	}
	if (VkResult result = vkCreateFence(fence->device->vkDevice, &createInfo, nullptr, &fence->vkFence); result != VK_SUCCESS) {
		return ERROR_RESULT(fence, "Failed to create Fence: ", VkResultString(result));
	}
	SetDebugMarker(fence->device, fence->tag, VK_OBJECT_TYPE_FENCE, (u64)fence->vkFence);
	fence->initted = true;
	return VoidResult_t();
}

void FenceDeinit(Fence *fence) {
	DEINIT_HEAD(fence);
	vkDestroyFence(fence->device->vkDevice, fence->vkFence, nullptr);
	fence->initted = false;
}

VkResult FenceGetStatus(Fence *fence) {
	return vkGetFenceStatus(fence->device->vkDevice, fence->vkFence);
}

Result<VoidResult_t, String> FenceResetSignaled(Fence *fence) {
	if (VkResult result = vkResetFences(fence->device->vkDevice, 1, &fence->vkFence); result != VK_SUCCESS) {
		return ERROR_RESULT(fence, "vkResetFences failed with ", VkResultString(result));
	}
	return VoidResult_t();
}

Result<VoidResult_t, String> FenceWaitForSignal(Fence *fence, u64 timeout, bool *dstWasTimeout) {
	VkResult result = vkWaitForFences(fence->device->vkDevice, 1, &fence->vkFence, VK_TRUE, timeout);
	bool wasTimeout;
	if (result == VK_SUCCESS) {
		wasTimeout = false;
	} else if (result == VK_TIMEOUT) {
		wasTimeout = true;
	} else {
		return ERROR_RESULT(fence, "vkWaitForFences failed with ", VkResultString(result));
	}
	if (dstWasTimeout) *dstWasTimeout = wasTimeout;
	return VoidResult_t();
}



Result<VoidResult_t, String> SemaphoreInit(Semaphore *semaphore) {
	INIT_HEAD(semaphore);
	VkSemaphoreCreateInfo createInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	if (VkResult result = vkCreateSemaphore(semaphore->device->vkDevice, &createInfo, nullptr, &semaphore->vkSemaphore); result != VK_SUCCESS) {
		return ERROR_RESULT(semaphore, "Failed to create semaphore: ", VkResultString(result));
	}
	SetDebugMarker(semaphore->device, semaphore->tag, VK_OBJECT_TYPE_SEMAPHORE, (u64)semaphore->vkSemaphore);
	semaphore->initted = true;
	return VoidResult_t();
}

void SemaphoreDeinit(Semaphore *semaphore) {
	DEINIT_HEAD(semaphore);
	vkDestroySemaphore(semaphore->device->vkDevice, semaphore->vkSemaphore, nullptr);
	semaphore->initted = false;
}

#endif

#ifndef Window

Result<Window*, String> AddWindow(io::Window *window, String tag) {
	Window *result = windows.Append(new Window(window, tag)).RawPtr();
	if (windows.size == 1 && instance.initted) {
		// To add window surface extensions
		Deinitialize();
		auto initResult = Initialize();
		if (initResult.isError) {
			windows.ClearSoft();
			return initResult.error;
		}
	}
	return result;
}

static bool FormatIsDepth(VkFormat format) {
	switch (format) {
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
	case VK_FORMAT_D32_SFLOAT:
	case VK_FORMAT_D16_UNORM_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		return true;
	default: return false;
	}
}

void FramebufferAddImage(Framebuffer *framebuffer, Image *image) {
	framebuffer->attachmentRefs.Append(AttachmentRef(Attachment(image, FormatIsDepth(image->vkFormat))));
	image->attachment = true;
	image->transferDst = false;
	if (image->sampleCount != 1) {
		image->transferSrc = true;
	}
}

void FramebufferAddWindow(Framebuffer *framebuffer, Window *window) {
	framebuffer->attachmentRefs.Append(AttachmentRef(Attachment(window)));
	// TODO: Probably allow multiple framebuffers for the same window
	AzAssert(window->framebuffer == nullptr, "Windows can only have 1 framebuffer assocation");
	window->framebuffer = framebuffer;
	window->attachment = true;
	window->transferDst = false;
}

void FramebufferAddImageMultisampled(Framebuffer *framebuffer, Image *image, Image *resolveImage) {
	AzAssert(image->vkFormat == resolveImage->vkFormat, "Resolving multisampled images requires both images to be the same format");
	AzAssert(image->sampleCount != 1, "Expected image to have a sample count != 1");
	AzAssert(resolveImage->sampleCount == 1, "Expected resolveImage to have a sample count == 1");
	framebuffer->attachmentRefs.Append(AttachmentRef(Attachment(image, false), Attachment(resolveImage, false)));
	image->attachment = true;
	image->transferDst = false;
	image->transferSrc = true;
	resolveImage->attachment = true;
	resolveImage->transferDst = true;
}

void FramebufferAddImageMultisampled(Framebuffer *framebuffer, Image *image, Window *resolveWindow) {
	AzAssert(image->sampleCount != 1, "Expected image to have a sample count != 1");
	framebuffer->attachmentRefs.Append(AttachmentRef(Attachment(image, false), Attachment(resolveWindow)));
	// TODO: Probably allow multiple framebuffers for the same window
	AzAssert(resolveWindow->framebuffer == nullptr, "Windows can only have 1 framebuffer assocation");
	image->attachment = true;
	image->transferDst = false;
	image->transferSrc = true;
	resolveWindow->framebuffer = framebuffer;
	resolveWindow->attachment = true;
	resolveWindow->transferDst = true;
}

void SetVSync(Window *window, bool enable) {
	if (window->initted) {
		window->shouldReconfigure = enable != window->vsync;
	}
	window->vsync = enable;
}

bool GetVSyncEnabled(Window *window) {
	return window->vsync;
}

Result<VoidResult_t, String> WindowSurfaceInit(Window *window) {
	if (!window->window->open) {
		return String("InitWindowSurface was called before the window was created!");
	}
#ifdef __unix
	if (window->window->data->useWayland) {
		VkWaylandSurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
		createInfo.display = window->window->data->wayland.display;
		createInfo.surface = window->window->data->wayland.surface;
		VkResult result = vkCreateWaylandSurfaceKHR(instance.vkInstance, &createInfo, nullptr, &window->vkSurface);
		if (result != VK_SUCCESS) {
			return ERROR_RESULT(window, "Failed to create Vulkan Wayland surface: ", VkResultString(result));
		}
	} else {
		VkXcbSurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
		createInfo.connection = window->window->data->x11.connection;
		createInfo.window = window->window->data->x11.window;
		VkResult result = vkCreateXcbSurfaceKHR(instance.vkInstance, &createInfo, nullptr, &window->vkSurface);
		if (result != VK_SUCCESS) {
			return ERROR_RESULT(window, "Failed to create Vulkan XCB surface: ", VkResultString(result));
		}
	}
#elif defined(_WIN32)
	VkWin32SurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	createInfo.hinstance = window->window->data->instance;
	createInfo.hwnd = window->window->data->window;
	VkResult result = vkCreateWin32SurfaceKHR(instance.vkInstance, &createInfo, nullptr, &window->vkSurface);
	if (result != VK_SUCCESS) {
		return ERROR_RESULT(window, "Failed to create Win32 Surface: ", VkResultString(result));
	}
#endif
	return VoidResult_t();
}



void WindowSurfaceDeinit(Window *window) {
	vkDestroySurfaceKHR(instance.vkInstance, window->vkSurface, nullptr);
}

Result<VoidResult_t, String> WindowInit(Window *window) {
	TRACE_INIT(window);
	vkQueueWaitIdle(window->device->vkQueue);
	SetDebugMarker(window->device, window->tag, VK_OBJECT_TYPE_SURFACE_KHR, (u64)window->vkSurface);
	{ // Query surface capabilities
		VkPhysicalDevice vkPhysicalDevice = window->device->physicalDevice->vkPhysicalDevice;
		VkSurfaceKHR vkSurface = window->vkSurface;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, vkSurface, &window->surfaceCaps);
		u32 count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurface, &count, nullptr);
		AzAssertRel(count > 0, "Vulkan Spec violation: vkGetPhysicalDeviceSurfaceFormatsKHR must support >= 1 surface formats.");
		window->surfaceFormatsAvailable.Resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurface, &count, window->surfaceFormatsAvailable.data);
		vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurface, &count, nullptr);
		AzAssertRel(count > 0, "Vulkan Spec violation: vkGetPhysicalDeviceSurfacePresentModesKHR must support >= 1 present modes.");
		window->presentModesAvailable.Resize(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurface, &count, window->presentModesAvailable.data);
	}
	{ // Choose surface format
		bool found = false;
		for (const VkSurfaceFormatKHR& fmt : window->surfaceFormatsAvailable) {
			if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				window->surfaceFormat = fmt;
				found = true;
			}
		}
		if (!found) {
			WARNING(window, "Desired Window surface format unavailable, falling back to what is.");
			window->surfaceFormat = window->surfaceFormatsAvailable[0];
		}
	}
	u32 imageCountPreferred = 3;
	{ // Choose present mode
		bool found = false;
		if (window->vsync) {
			// The Vulkan Spec requires this present mode to exist
			window->presentMode = VK_PRESENT_MODE_FIFO_KHR;
			imageCountPreferred = 3;
			found = true;
		} else {
			for (const VkPresentModeKHR& mode : window->presentModesAvailable) {
				if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
					window->presentMode = mode;
					found = true;
					imageCountPreferred = 2;
					// Acceptable choice, but keep looking
				} else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
					window->presentMode = mode;
					found = true;
					imageCountPreferred = 3;
					break; // Ideal choice, don't keep looking
				}
			}
		}
		if (!found) {
			WARNING(window, "Defaulting to FIFO present mode since we don't have a choice.");
			window->presentMode = VK_PRESENT_MODE_FIFO_KHR;
		} else {
			io::cout.PrintDebug("Present Mode: ");
			switch(window->presentMode) {
				case VK_PRESENT_MODE_FIFO_KHR:
					io::cout.PrintLnDebug("VK_PRESENT_MODE_FIFO_KHR");
					break;
				case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
					io::cout.PrintLnDebug("VK_PRESENT_MODE_FIFO_RELAXED_KHR");
					break;
				case VK_PRESENT_MODE_MAILBOX_KHR:
					io::cout.PrintLnDebug("VK_PRESENT_MODE_MAILBOX_KHR");
					break;
				case VK_PRESENT_MODE_IMMEDIATE_KHR:
					io::cout.PrintLnDebug("VK_PRESENT_MODE_IMMEDIATE_KHR");
					break;
				default:
					io::cout.PrintLnDebug("Unknown present mode 0x", FormatInt((u32)window->presentMode, 16));
					break;
			}
		}
	}
	if (window->surfaceCaps.currentExtent.width != UINT32_MAX) {
		window->extent = window->surfaceCaps.currentExtent;
	} else {
		window->extent.width = clamp((u32)window->window->width, window->surfaceCaps.minImageExtent.width, window->surfaceCaps.maxImageExtent.width);
		window->extent.height = clamp((u32)window->window->height, window->surfaceCaps.minImageExtent.height, window->surfaceCaps.maxImageExtent.height);
	}
	io::cout.PrintLnDebug("Extent: ", window->extent.width, "x", window->extent.height);
	window->numImages = (i32)clamp(imageCountPreferred, window->surfaceCaps.minImageCount, window->surfaceCaps.maxImageCount != 0 ? window->surfaceCaps.maxImageCount : UINT32_MAX);
	{ // Create the swapchain
		VkSwapchainCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
		createInfo.surface = window->vkSurface;
		createInfo.minImageCount = window->numImages;
		createInfo.imageFormat = window->surfaceFormat.format;
		createInfo.imageColorSpace = window->surfaceFormat.colorSpace;
		createInfo.imageExtent = window->extent;
		createInfo.imageArrayLayers = 1;
		if (window->attachment) {
			createInfo.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		if (window->transferDst) {
			createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		// TODO: If we need to use multiple queues, we need to be smarter about this.
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.preTransform = window->surfaceCaps.currentTransform;
		// TODO: Maybe support transparent windows
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = window->presentMode;
		// TODO: This may not play nicely with window capture software?
		createInfo.clipped = VK_TRUE;
		if (window->initted) {
			createInfo.oldSwapchain = window->vkSwapchain;
		}
		VkSwapchainKHR newSwapchain;
		if (VkResult result = vkCreateSwapchainKHR(window->device->vkDevice, &createInfo, nullptr, &newSwapchain); result != VK_SUCCESS) {
			window->initted = false;
			return ERROR_RESULT(window, "Failed to create swapchain: ", VkResultString(result));
		}
		if (window->initted) {
			vkDestroySwapchainKHR(window->device->vkDevice, window->vkSwapchain, nullptr);
		}
		window->vkSwapchain = newSwapchain;
		SetDebugMarker(window->device, Stringify(window->tag, " swapchain"), VK_OBJECT_TYPE_SWAPCHAIN_KHR, (u64)window->vkSwapchain);
	}
	{ // Get Images and create Image Views
		if (window->initted) {
			for (Window::SwapchainImage &image : window->swapchainImages) {
				vkDestroyImageView(window->device->vkDevice, image.vkImageView, nullptr);
			}
		}
		Array<VkImage> images;
		u32 numImages;
		vkGetSwapchainImagesKHR(window->device->vkDevice, window->vkSwapchain, &numImages, nullptr);
		images.Resize(numImages);
		vkGetSwapchainImagesKHR(window->device->vkDevice, window->vkSwapchain, &numImages, images.data);
		window->numImages = (i32)numImages;
		window->swapchainImages.Resize(images.size);
		VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = window->surfaceFormat.format;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.layerCount = 1;
		for (i32 i = 0; i < images.size; i++) {
			window->swapchainImages[i].vkImage = images[i];
			createInfo.image = images[i];
			SetDebugMarker(window->device, Stringify(window->tag, " swapchain image ", i), VK_OBJECT_TYPE_IMAGE, (u64)window->swapchainImages[i].vkImage);

			if (VkResult result = vkCreateImageView(window->device->vkDevice, &createInfo, nullptr, &window->swapchainImages[i].vkImageView); result != VK_SUCCESS) {
				return ERROR_RESULT(window, "Failed to create Image View for Swapchain image ", i, ":", VkResultString(result));
			}
			SetDebugMarker(window->device, Stringify(window->tag, " swapchain image view ", i), VK_OBJECT_TYPE_IMAGE_VIEW, (u64)window->swapchainImages[i].vkImageView);
		}
	}
	io::cout.PrintLnDebug("Number of images: ", window->numImages);
	if (window->acquireFences.size > window->numImages) {
		for (i32 i = window->acquireFences.size-1; i >= window->numImages; i--) {
			// Calling vkQueueWaitIdle does nothing for swapchain image acquisition, so we need to wait on the fence.
			// This is okay to call on all of them because we only set unsignaled right before asking for the image.
			if (auto result = FenceWaitForSignal(&window->acquireFences[i]); result.isError) return result.error;
			FenceDeinit(&window->acquireFences[i]);
			SemaphoreDeinit(&window->acquireSemaphores[i]);
		}
		window->acquireFences.Resize(window->numImages);
		window->acquireSemaphores.Resize(window->numImages);
	} else if (window->acquireFences.size < window->numImages) {
		i32 previousSize = window->acquireFences.size;
		window->acquireFences.Resize(window->numImages, Fence(window->device, Stringify(window->tag, " Fence")));
		window->acquireSemaphores.Resize(window->numImages, Semaphore(window->device, Stringify(window->tag, " Semaphore")));
		for (i32 i = previousSize; i < window->numImages; i++) {
			if (auto result = FenceInit(&window->acquireFences[i], true); result.isError) {
				return ERROR_RESULT(window, result.error);
			}
			if (auto result = SemaphoreInit(&window->acquireSemaphores[i]); result.isError) {
				return ERROR_RESULT(window, result.error);
			}
		}
	}
	if (window->initted && window->framebuffer) {
		if (auto result = FramebufferCreate(window->framebuffer); result.isError) {
			return ERROR_RESULT(window, "Failed to recreate Framebuffer: ", result.error);
		}
	}
	window->currentSync = 0;
	window->initted = true;
	return VoidResult_t();
}

void WindowDeinit(Window *window) {
	DEINIT_HEAD(window);
	for (Fence &fence : window->acquireFences) {
		FenceWaitForSignal(&fence).AzUnwrap();
		FenceDeinit(&fence);
	}
	for (Semaphore &semaphore : window->acquireSemaphores) {
		SemaphoreDeinit(&semaphore);
	}
	for (Window::SwapchainImage &image : window->swapchainImages) {
		vkDestroyImageView(window->device->vkDevice, image.vkImageView, nullptr);
	}
	vkDestroySwapchainKHR(window->device->vkDevice, window->vkSwapchain, nullptr);
	window->initted = false;
}


Result<VoidResult_t, String> WindowUpdate(Window *window) {
	bool resize = false;
	if (window->window->width != window->extent.width
	 || window->window->height != window->extent.height) {
		resize = true;
	}
	if (resize || window->shouldReconfigure) {
reconfigure:
		if (auto result = WindowInit(window); result.isError) {
			return ERROR_RESULT(window, "Failed to reconfigure window: ", result.error);
		}
		window->shouldReconfigure = false;
	}

	// Swapchain::AcquireNextImage
	window->currentSync = (window->currentSync + 1) % window->numImages;
	Fence *fence = &window->acquireFences[window->currentSync];
	if (auto result = FenceWaitForSignal(fence); result.isError) {
		return ERROR_RESULT(window, result.error);
	}
	if (auto result = FenceResetSignaled(fence); result.isError) {
		return ERROR_RESULT(window, result.error);
	}
	Semaphore *semaphore = &window->acquireSemaphores[window->currentSync];
	u32 currentImage;
	VkResult result = vkAcquireNextImageKHR(window->device->vkDevice, window->vkSwapchain, UINT64_MAX, semaphore->vkSemaphore, fence->vkFence, &currentImage);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		goto reconfigure;
	} else if (result == VK_TIMEOUT || result == VK_NOT_READY) {
		// This shouldn't happen with a timeout of UINT64_MAX
		return ERROR_RESULT(window, "Unreachable");
	} else if (result == VK_SUBOPTIMAL_KHR) {
		// Let it go, we'll resize next time
		window->shouldReconfigure = true;
	} else if (result != VK_SUCCESS) {
		return ERROR_RESULT(window, "Failed to acquire swapchain image: ", VkResultString(result));
	}
	window->currentImage = (i32)currentImage;
	return VoidResult_t();
}

Result<VoidResult_t, String> WindowPresent(Window *window, ArrayWithBucket<Semaphore*, 4> waitSemaphores) {
	ArrayWithBucket<VkSemaphore, 4> waitVkSemaphores(waitSemaphores.size);
	for (i32 i = 0; i < waitVkSemaphores.size; i++) {
		waitVkSemaphores[i] = waitSemaphores[i]->vkSemaphore;
	}
	VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	presentInfo.waitSemaphoreCount = waitVkSemaphores.size;
	presentInfo.pWaitSemaphores = waitVkSemaphores.data;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &window->vkSwapchain;
	presentInfo.pImageIndices = (u32*)&window->currentImage;
	
	VkResult result = vkQueuePresentKHR(window->device->vkQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		io::cout.PrintLnDebug("WindowPresent got ", VkResultString(result));
	} else if (result != VK_SUCCESS) {
		return ERROR_RESULT(window, "Failed to Queue Present: ", VkResultString(result));
	}
	return VoidResult_t();
}

#endif

#ifndef Creating_new_objects

Device* NewDevice(String tag) {
	return devices.Append(new Device(tag)).RawPtr();
}

Context* NewContext(Device *device, String tag) {
	return device->contexts.Append(new Context(device, tag)).RawPtr();
}

Shader* NewShader(Device *device, String filename, ShaderStage stage, String tag) {
	return device->shaders.Append(new Shader(device, filename, stage, tag)).RawPtr();
}

Pipeline* NewGraphicsPipeline(Device *device, String tag) {
	return device->pipelines.Append(new Pipeline(device, Pipeline::GRAPHICS, tag)).RawPtr();
}

Pipeline* NewComputePipeline(Device *device, String tag) {
	return device->pipelines.Append(new Pipeline(device, Pipeline::COMPUTE, tag)).RawPtr();
}

Buffer* NewVertexBuffer(Device *device, String tag) {
	return device->buffers.Append(new Buffer(Buffer::VERTEX_BUFFER, device, tag)).RawPtr();
}

Buffer* NewIndexBuffer(Device *device, String tag, u32 bytesPerIndex) {
	Buffer *result = device->buffers.Append(new Buffer(Buffer::INDEX_BUFFER, device, tag)).RawPtr();
	switch (bytesPerIndex) {
	// TODO: Probably support 8-bit indices
	case 2:
		result->indexType = VK_INDEX_TYPE_UINT16;
		break;
	case 4:
		result->indexType = VK_INDEX_TYPE_UINT32;
		break;
	default:
		AzAssert(false, Stringify("Can only have 2 or 4 byte indices in an index buffer (had ", bytesPerIndex, ")"));
	}
	return result;
}

Buffer* NewStorageBuffer(Device *device, String tag) {
	return device->buffers.Append(new Buffer(Buffer::STORAGE_BUFFER, device, tag)).RawPtr();
}

Buffer* NewUniformBuffer(Device *device, String tag) {
	return device->buffers.Append(new Buffer(Buffer::UNIFORM_BUFFER, device, tag)).RawPtr();
}

Image* NewImage(Device *device, String tag) {
	return device->images.Append(new Image(device, tag)).RawPtr();
}

Sampler* NewSampler(Device *device, String tag) {
	return device->samplers.Append(new Sampler(device, tag)).RawPtr();
}

Framebuffer* NewFramebuffer(Device *device, String tag) {
	return device->framebuffers.Append(new Framebuffer(device, tag)).RawPtr();
}

#endif

#ifndef Physical_Device

void PrintPhysicalDeviceInfo(PhysicalDevice *physicalDevice);

i32 RatePhysicalDevice(PhysicalDevice *device) {
	i32 score = 0;
	switch (device->properties.properties.deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			score += 2000;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			score += 1000;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			score += 500;
			break;
		default:
		// case VK_PHYSICAL_DEVICE_TYPE_CPU:
		// case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			break;
	}
	score += min(device->properties.properties.limits.maxImageDimension2D, 16384u)/10;
	return score;
}

Result<Ptr<PhysicalDevice>, String> FindBestPhysicalDeviceWithExtensions(Array<const char*> extensions) {
	AzAssert(instance.initted, "Trying to use Instance when it's not initted");
	struct Rating {
		Ptr<PhysicalDevice> dev;
		Array<const char*> extensionsUnavailable;
		i32 rating;
	};
	Array<Rating> ratings(instance.physicalDevices.size);
	for (i32 i = 0; i < ratings.size; i++) {
		Ptr<PhysicalDevice> physicalDevice = instance.physicalDevices.GetPtr(i);
		ratings[i].dev = physicalDevice;
		ratings[i].rating = RatePhysicalDevice(&instance.physicalDevices[i]);
		ratings[i].extensionsUnavailable = extensions;
		for (i32 k = 0; k < ratings[i].extensionsUnavailable.size; k++) {
			for (i32 j = 0; j < physicalDevice->extensionsAvailable.size; j++) {
				if (equals(ratings[i].extensionsUnavailable[k], physicalDevice->extensionsAvailable[j].extensionName)) {
					ratings[i].extensionsUnavailable.Erase(k);
					k--;
					break;
				}
			}
		}
		if (ratings[i].extensionsUnavailable.size) {
			ratings[i].rating -= 100000000;
		}
	}
	QuickSort(ratings, [](Rating &lhs, Rating &rhs) -> bool {
		return rhs.rating < lhs.rating;
	});
	if ((u32)io::logLevel >= (u32)io::LogLevel::DEBUG) {
		for (i32 i = 0; i < ratings.size; i++) {
			io::cout.PrintLn("Device ", i, " with rating ", ratings[i].rating, ":");
			io::cout.IndentMore();
			PrintPhysicalDeviceInfo(ratings[i].dev.RawPtr());
			io::cout.IndentLess();
		}
	}
	if (ratings[0].rating < 0) {
		String error = Stringify("All physical device candidates lacked extensions. The best one (", ratings[0].dev->properties.properties.deviceName, ") was missing:");
		for (const char *extension : ratings[0].extensionsUnavailable) {
			error += "\n\t";
			error += extension;
		}
		return error;
	}
	return ratings[0].dev;
}

void PrintPhysicalDeviceInfo(PhysicalDevice *physicalDevice) {
	// Basic info
	const VkPhysicalDeviceProperties2 &properties = physicalDevice->properties;
	io::cout.PrintLn("Name: ", properties.properties.deviceName, "\nVulkan Version: ", VK_VERSION_MAJOR(properties.properties.apiVersion), ".", VK_VERSION_MINOR(properties.properties.apiVersion), ".", VK_VERSION_PATCH(properties.properties.apiVersion));
	// Memory
	const VkPhysicalDeviceMemoryProperties2 &memoryProperties = physicalDevice->memoryProperties;
	u64 deviceLocalMemory = 0;
	for (u32 i = 0; i < memoryProperties.memoryProperties.memoryHeapCount; i++) {
		if (memoryProperties.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			deviceLocalMemory += memoryProperties.memoryProperties.memoryHeaps[i].size;
	}
	io::cout.PrintLn("Memory: ", FormatSize(deviceLocalMemory));
	// Queue families
	io::cout.Print("Queue Families:");
	for (i32 i = 0; i < physicalDevice->queueFamiliesAvailable.size; i++) {
		const VkQueueFamilyProperties2 &props = physicalDevice->queueFamiliesAvailable[i];
		VkQueueFlags queueFlags = props.queueFamilyProperties.queueFlags;
		io::cout.Print("\n\tFamily[", i, "] Queue count: ", props.queueFamilyProperties.queueCount, "\tSupports: ", ((queueFlags & VK_QUEUE_COMPUTE_BIT) ? "COMPUTE " : ""), ((queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "GRAPHICS " : ""), ((queueFlags & VK_QUEUE_TRANSFER_BIT) ? "TRANSFER " : ""));
		String presentString = "PRESENT on windows {";
		VkBool32 presentSupport = false;
		bool first = true;
		for (i32 j = 0; j < windows.size; j++) {
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice->vkPhysicalDevice, i, windows[j]->vkSurface, &presentSupport);
			if (presentSupport) {
				if (!first)
					presentString += ", ";
				presentString += ToString(j);
				first = false;
				break;
			}
		}
		presentString += "}";
		if (!first)
			io::cout.Print(presentString);
	}
	io::cout.Newline();
}

#endif

#ifndef Memory_Operations

// linear=true indicates whether we're buffers and images with VK_IMAGE_TILING_LINEAR
// linear=false is for images with VK_IMAGE_TILING_OPTIMAL
Memory* DeviceGetMemory(Device *device, u32 memoryType, bool linear) {
	Memory *result;
	u32 key = memoryType;
	if (!linear) {
		key |= 0x10000;
	}
	if (auto *node = device->memory.Find(key); node == nullptr) {
		result = &device->memory.Emplace(key, Memory(device, memoryType, Stringify("Memory (type ", memoryType, linear? " linear" : " non-linear", ")")));
	} else {
		result = &node->value;
	}
	return result;
}

Result<u32, String> FindMemoryType(u32 memoryTypeBits, VkMemoryPropertyFlags propertyFlags, VkPhysicalDeviceMemoryProperties memoryProperties) {
	for (u32 i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
			return i;
		}
	}
	return String("Failed to find a suitable memory type!");
}

Result<VoidResult_t, String> MemoryAddPage(Memory *memory, u32 minSize) {
	AzAssert(memory->device->initted, "Device not initted!");
	minSize = max(minSize, memory->pageSizeMin);
	Memory::Page &newPage = memory->pages.Append(Memory::Page());
	VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.memoryTypeIndex = memory->memoryTypeIndex;
	allocInfo.allocationSize = minSize;
	if (VkResult result = vkAllocateMemory(memory->device->vkDevice, &allocInfo, nullptr, &newPage.vkMemory); result != VK_SUCCESS) {
		return ERROR_RESULT(memory, "Failed to allocate a new page: ", VkResultString(result));
	}
	SetDebugMarker(memory->device, Stringify(memory->tag, " page ", memory->pages.size-1), VK_OBJECT_TYPE_DEVICE_MEMORY, (u64)newPage.vkMemory);
	newPage.segments.Append(Memory::Page::Segment{0, minSize, false});
	return VoidResult_t();
}

// Cleans up and destroys all memory pages
void MemoryClear(Memory *memory) {
	for (Memory::Page &page : memory->pages) {
		vkFreeMemory(memory->device->vkDevice, page.vkMemory, nullptr);
	}
}

u32 alignedSize(u32 offset, u32 size, u32 alignment) {
	return (u32)max((i64)0, (i64)size - (i64)(align(offset, alignment) - offset));
}

i32 PageFindSegment(Memory::Page *page, u32 size, u32 alignment) {
	for (i32 i = 0; i < page->segments.size; i++) {
		if (page->segments[i].used) continue;
		if (alignedSize(page->segments[i].begin, page->segments[i].size, alignment) >= size) {
			return i;
		}
	}
	return -1;
}

Allocation PageAllocInSegment(Memory *memory, i32 pageIndex, i32 segmentIndex, u32 size, u32 alignment) {
	Allocation result;
	Memory::Page &page = memory->pages[pageIndex];
	AzAssert(size <= page.segments[segmentIndex].size, "segment is too small for alloc");
	AzAssert(page.segments[segmentIndex].used == false, "Trying to allocate in a segment that's already in use!");
	using Segment = Memory::Page::Segment;
	u32 alignedBegin = align(page.segments[segmentIndex].begin, alignment);
	u32 availableSize = alignedSize(page.segments[segmentIndex].begin, page.segments[segmentIndex].size, alignment);
	if (page.segments[segmentIndex].begin != alignedBegin) {
		Segment &preSegment = page.segments.Insert(segmentIndex, Segment());
		Segment &ourSegment = page.segments[segmentIndex+1];
		preSegment.begin = ourSegment.begin;
		preSegment.size = alignedBegin - ourSegment.begin;
		ourSegment.begin = alignedBegin;
		ourSegment.size = availableSize;
		segmentIndex++;
	}
	if (availableSize > size) {
		Segment &newSegment = page.segments.Insert(segmentIndex+1, Segment());
		Segment &ourSegment = page.segments[segmentIndex];
		newSegment.begin = ourSegment.begin + size;
		newSegment.size = ourSegment.size - size;
		newSegment.used = false;
		ourSegment.size = size;
		ourSegment.used = true;
	} else {
		Segment &segment = page.segments[segmentIndex];
		segment.used = true;
	}
	result.memory = memory;
	result.page = pageIndex;
	result.offset = page.segments[segmentIndex].begin;
	return result;
}

Result<Allocation, String> MemoryAllocate(Memory *memory, u32 size, u32 alignment) {
	i32 page = 0;
	i32 segment = -1;
	for (; page < memory->pages.size; page++) {
		segment = PageFindSegment(&memory->pages[page], size, alignment);
		if (segment != -1) break;
	}
	if (page == memory->pages.size) {
		if (auto result = MemoryAddPage(memory, size); result.isError) {
			return result.error;
		}
		segment = 0;
	}
	return PageAllocInSegment(memory, page, segment, size, alignment);
}

void MemoryFree(Allocation allocation) {
	Memory::Page &page = allocation.memory->pages[allocation.page];
	i32 segment = -1;
	for (i32 i = 0; i < page.segments.size; i++) {
		if (page.segments[i].begin == allocation.offset) {
			segment = i;
			break;
		}
	}
	AzAssertRel(segment != -1, "Bad Free");
	page.segments[segment].used = false;
	// Combine adjacent unused segments
	if (segment < page.segments.size-1 && page.segments[segment+1].used == false) {
		page.segments[segment].size += page.segments[segment+1].size;
		page.segments.Erase(segment+1);
	}
	if (segment > 0 && page.segments[segment-1].used == false) {
		page.segments[segment-1].size += page.segments[segment].size;
		page.segments.Erase(segment);
	}
}

// Allocates memory and binds it to the buffer
Result<Allocation, String> AllocateBuffer(Device *device, VkBuffer buffer, VkMemoryRequirements memoryRequirements, VkMemoryPropertyFlags memoryPropertyFlags) {
	u32 memoryType;
	if (auto result = FindMemoryType(memoryRequirements.memoryTypeBits, memoryPropertyFlags, device->physicalDevice->memoryProperties.memoryProperties); result.isError) {
		return result.error;
	} else {
		memoryType = result.value;
	}
	Memory *memory = DeviceGetMemory(device, memoryType, true);
	Allocation alloc;
	if (auto result = MemoryAllocate(memory, memoryRequirements.size, memoryRequirements.alignment); result.isError) {
		return result.error;
	} else {
		alloc = result.value;
	}
	if (VkResult result = vkBindBufferMemory(device->vkDevice, buffer, memory->pages[alloc.page].vkMemory, align(alloc.offset, memoryRequirements.alignment)); result != VK_SUCCESS) {
		return ERROR_RESULT(memory, "Failed to bind Buffer to Memory: ", VkResultString(result));
	}
	return alloc;
}

Result<Allocation, String> AllocateImage(Device *device, VkImage image, VkMemoryRequirements memoryRequirements, VkMemoryPropertyFlags memoryPropertyFlags, bool linear) {
	u32 memoryType;
	if (auto result = FindMemoryType(memoryRequirements.memoryTypeBits, memoryPropertyFlags, device->physicalDevice->memoryProperties.memoryProperties); result.isError) {
		return result.error;
	} else {
		memoryType = result.value;
	}
	Memory *memory = DeviceGetMemory(device, memoryType, linear);
	Allocation alloc;
	if (auto result = MemoryAllocate(memory, memoryRequirements.size, memoryRequirements.alignment); result.isError) {
		return result.error;
	} else {
		alloc = result.value;
	}
	if (VkResult result = vkBindImageMemory(device->vkDevice, image, memory->pages[alloc.page].vkMemory, align(alloc.offset, memoryRequirements.alignment)); result != VK_SUCCESS) {
		return ERROR_RESULT(memory, "Failed to bind Image to Memory: ", VkResultString(result));
	}
	return alloc;
}

#endif

#ifndef Device

Result<VoidResult_t, String> DeviceInit(Device *device) {
	INIT_HEAD(device);

	bool needsPresent = false;
	bool needsGraphics = false;
	bool needsCompute = false;
	for (auto &pipeline : device->pipelines) {
		switch (pipeline->kind) {
			case Pipeline::GRAPHICS:
				needsGraphics = true;
				break;
			case Pipeline::COMPUTE:
				needsCompute = true;
				break;
		}
	}
	Array<const char*> extensions;
	{ // Add and check availability of extensions to pick a physical device
		for (auto &fb : device->framebuffers) {
			for (AttachmentRef &attachmentRef : fb->attachmentRefs) {
				if (attachmentRef.attachment.kind == Attachment::WINDOW || (attachmentRef.resolveAttachment.Exists() && attachmentRef.resolveAttachment.ValueOrAssert().kind == Attachment::WINDOW)) {
					// If even one framebuffer outputs to a Window, we use a Swapchain
					extensions.Append(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
					needsPresent = true;
					goto breakout;
				}
			}
		}
breakout:
		if (device->pipelines.size) {
			// For VK_DYNAMIC_STATE_VERTEX_INPUT_EXT
			// extensions.Append("VK_EXT_vertex_input_dynamic_state");
		}
		auto physicalDevice = FindBestPhysicalDeviceWithExtensions(extensions);
		if (physicalDevice.isError) return physicalDevice.error;
		device->physicalDevice = physicalDevice.value;
		if (device->tag.size == 0) {
			device->tag = device->physicalDevice->properties.properties.deviceName;
		}
	}
	VkPhysicalDeviceFeatures2 featuresAvailable = device->physicalDevice->vk10Features;
	VkPhysicalDeviceFeatures2 featuresEnabled = device->vk10Features;
	{ // Select needed features based on what we use
		bool anisotropyAvailable = featuresAvailable.features.samplerAnisotropy;
		if (!anisotropyAvailable) {
			for (auto &sampler : device->samplers) {
				if (sampler->anisotropy != 1) {
					WARNING(sampler, "Sampler Anisotropy unavailable, so anisotropy is being reset to 1");
					sampler->anisotropy = 1;
				}
			}
		} else {
			for (auto &sampler : device->samplers) {
				if (sampler->anisotropy != 1) {
					featuresEnabled.features.samplerAnisotropy = VK_TRUE;
					io::cout.PrintLnDebug("Enabling Sampler Anisotropy");
					break;
				}
			}
		}
		bool wideLinesAvailable = featuresAvailable.features.wideLines;
		if (!wideLinesAvailable) {
			for (auto &pipeline : device->pipelines) {
				if (pipeline->lineWidth != 1.0f) {
					WARNING(pipeline, "Wide lines unavailable, so lineWidth is being reset to 1.0f");
					pipeline->lineWidth = 1.0f;
				}
			}
		} else {
			for (auto &pipeline : device->pipelines) {
				if (pipeline->lineWidth != 1.0f) {
					featuresEnabled.features.wideLines = VK_TRUE;
					io::cout.PrintLnDebug("Enabling Wide Lines");
					break;
				}
			}
		}
		bool sampleRateShadingAvailable = featuresAvailable.features.sampleRateShading;
		if (!sampleRateShadingAvailable) {
			for (auto &pipeline : device->pipelines) {
				if (pipeline->multisampleShading.enabled) {
					WARNING(pipeline, "Multisample Shading unavailable, disabling");
					pipeline->multisampleShading.enabled = false;
				}
			}
		} else {
			for (auto &pipeline : device->pipelines) {
				if (pipeline->multisampleShading.enabled) {
					featuresEnabled.features.sampleRateShading = VK_TRUE;
					io::cout.PrintLnDebug("Enabling Multisample Shading");
					break;
				}
			}
		}
	}
	featuresEnabled.pNext = &device->vk11Features;
	device->vk11Features.pNext = &device->vk12Features;
	device->vk12Features.pNext = &device->vk13Features;
	if ((u32)io::logLevel >= (u32)io::LogLevel::DEBUG) {
		PrintPhysicalDeviceInfo(device->physicalDevice.RawPtr());
	}
	// NOTE: This is stupid and probably won't work in the general case, but let's see.
	VkDeviceQueueCreateInfo queueInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queueInfo.queueCount = 1;
	f32 one = 1.0f;
	queueInfo.pQueuePriorities = &one;
	bool found = false;
	for (i32 i = 0; i < device->physicalDevice->queueFamiliesAvailable.size; i++) {
		VkQueueFamilyProperties2 props = device->physicalDevice->queueFamiliesAvailable[i];
		if (props.queueFamilyProperties.queueCount == 0) continue;
		if (needsPresent) {
			VkBool32 supportsPresent = VK_FALSE;
			for (auto &framebuffer : device->framebuffers) {
				for (AttachmentRef &attachmentRef : framebuffer->attachmentRefs) {
					if (attachmentRef.attachment.kind == Attachment::WINDOW) {
						vkGetPhysicalDeviceSurfaceSupportKHR(device->physicalDevice->vkPhysicalDevice, i, attachmentRef.attachment.window->vkSurface, &supportsPresent);
						if (!supportsPresent) goto breakout2;
					}
					if (attachmentRef.resolveAttachment.Exists()) {
						Attachment &attachment = attachmentRef.resolveAttachment.ValueOrAssert();
						if (attachment.kind == Attachment::WINDOW) {
							vkGetPhysicalDeviceSurfaceSupportKHR(device->physicalDevice->vkPhysicalDevice, i, attachment.window->vkSurface, &supportsPresent);
							if (!supportsPresent) goto breakout2;
						}
					}
				}
			}
breakout2:
			if (!supportsPresent) continue;
		}
		if (needsGraphics) {
			if (!(props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)) continue;
		}
		if (needsCompute) {
			if (!(props.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)) continue;
		}
		if (!(props.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)) continue;
		device->queueFamilyIndex = i;
		found = true;
		break;
	}
	if (!found) {
		// NOTE: If we ever see this, we probably need to break up our single queue into multiple specialized queues.
		return ERROR_RESULT(device, "There were no queues available that had everything we needed");
	}
	queueInfo.queueFamilyIndex = device->queueFamilyIndex;

	VkDeviceCreateInfo createInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	createInfo.pQueueCreateInfos = &queueInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pNext = &featuresEnabled;
	createInfo.enabledExtensionCount = extensions.size;
	createInfo.ppEnabledExtensionNames = extensions.data;

	VkResult result = vkCreateDevice(device->physicalDevice->vkPhysicalDevice, &createInfo, nullptr, &device->vkDevice);
	if (result != VK_SUCCESS) {
		return ERROR_RESULT(device, "Failed to create Device: ", VkResultString(result));
	}
	SetDebugMarker(device, device->tag, VK_OBJECT_TYPE_DEVICE, (u64)device->vkDevice);
	device->initted = true;

	vkGetDeviceQueue(device->vkDevice, device->queueFamilyIndex, 0, &device->vkQueue);

	for (auto &window : windows) {
		window->device = device;
		if (auto result = WindowInit(window.RawPtr()); result.isError) {
			return ERROR_RESULT(device, result.error);
		}
	}
	for (auto &buffer : device->buffers) {
		if (auto result = BufferInit(buffer.RawPtr()); result.isError) {
			return ERROR_RESULT(device, result.error);
		}
	}
	for (auto &image : device->images) {
		if (auto result = ImageInit(image.RawPtr()); result.isError) {
			return ERROR_RESULT(device, result.error);
		}
	}
	for (auto &sampler : device->samplers) {
		if (auto result = SamplerInit(sampler.RawPtr()); result.isError) {
			return ERROR_RESULT(device, result.error);
		}
	}
	for (auto &framebuffer : device->framebuffers) {
		if (auto result = FramebufferInit(framebuffer.RawPtr()); result.isError) {
			return ERROR_RESULT(device, result.error);
		}
	}
	for (auto &context : device->contexts) {
		if (auto result = ContextInit(context.RawPtr()); result.isError) {
			return ERROR_RESULT(device, result.error);
		}
	}
	for (auto &shader : device->shaders) {
		if (auto result = ShaderInit(shader.RawPtr()); result.isError) {
			return ERROR_RESULT(device, result.error);
		}
	}
	for (auto &pipeline : device->pipelines) {
		if (auto result = PipelineInit(pipeline.RawPtr()); result.isError) {
			return ERROR_RESULT(device, result.error);
		}
	}
	// TODO: Init everything else

	return VoidResult_t();
}

void DeviceDeinit(Device *device) {
	AzAssert(device->initted, "Trying to Deinit a Device that isn't initted");
	vkDeviceWaitIdle(device->vkDevice);
	io::cout.PrintLnTrace("Deinitializing Device \"", device->tag, "\"");
	for (auto &window : windows) {
		WindowDeinit(window.RawPtr());
	}
	for (auto &framebuffer : device->framebuffers) {
		FramebufferDeinit(framebuffer.RawPtr());
	}
	for (auto &context : device->contexts) {
		ContextDeinit(context.RawPtr());
	}
	for (auto &buffer : device->buffers) {
		BufferDeinit(buffer.RawPtr());
	}
	for (auto &image : device->images) {
		ImageDeinit(image.RawPtr());
	}
	for (auto &sampler : device->samplers) {
		SamplerDeinit(sampler.RawPtr());
	}
	for (auto &shader : device->shaders) {
		ShaderDeinit(shader.RawPtr());
	}
	for (auto &pipeline : device->pipelines) {
		PipelineDeinit(pipeline.RawPtr());
	}
	for (auto &descriptorSet : device->descriptorSets) {
		vkDestroyDescriptorPool(device->vkDevice, descriptorSet->vkDescriptorPool, nullptr);
	}
	for (auto &vkDescriptorSetLayout : device->vkDescriptorSetLayouts) {
		vkDestroyDescriptorSetLayout(device->vkDevice, vkDescriptorSetLayout.value, nullptr);
	}
	for (auto node : device->memory) {
		Memory &memory = node.value;
		MemoryClear(&memory);
	}
	vkDestroyDevice(device->vkDevice, nullptr);
}

void DeviceWaitIdle(Device *device) {
	if (!device->initted) return;
	vkDeviceWaitIdle(device->vkDevice);
}

void DeviceRequireFeatures(Device *device, const ArrayWithBucket<Str, 8> &features) {
	static BinaryMap<Str, u64> featureByteOffsets = {
		// Vulkan 1.0 Features
		{"robustBufferAccess", (u64)&device->vk10Features.features.robustBufferAccess - (u64)&device},
		{"fullDrawIndexUint32", (u64)&device->vk10Features.features.fullDrawIndexUint32 - (u64)&device},
		{"imageCubeArray", (u64)&device->vk10Features.features.imageCubeArray - (u64)&device},
		{"independentBlend", (u64)&device->vk10Features.features.independentBlend - (u64)&device},
		{"geometryShader", (u64)&device->vk10Features.features.geometryShader - (u64)&device},
		{"tessellationShader", (u64)&device->vk10Features.features.tessellationShader - (u64)&device},
		{"sampleRateShading", (u64)&device->vk10Features.features.sampleRateShading - (u64)&device},
		{"dualSrcBlend", (u64)&device->vk10Features.features.dualSrcBlend - (u64)&device},
		{"logicOp", (u64)&device->vk10Features.features.logicOp - (u64)&device},
		{"multiDrawIndirect", (u64)&device->vk10Features.features.multiDrawIndirect - (u64)&device},
		{"drawIndirectFirstInstance", (u64)&device->vk10Features.features.drawIndirectFirstInstance - (u64)&device},
		{"depthClamp", (u64)&device->vk10Features.features.depthClamp - (u64)&device},
		{"depthBiasClamp", (u64)&device->vk10Features.features.depthBiasClamp - (u64)&device},
		{"fillModeNonSolid", (u64)&device->vk10Features.features.fillModeNonSolid - (u64)&device},
		{"depthBounds", (u64)&device->vk10Features.features.depthBounds - (u64)&device},
		{"wideLines", (u64)&device->vk10Features.features.wideLines - (u64)&device},
		{"largePoints", (u64)&device->vk10Features.features.largePoints - (u64)&device},
		{"alphaToOne", (u64)&device->vk10Features.features.alphaToOne - (u64)&device},
		{"multiViewport", (u64)&device->vk10Features.features.multiViewport - (u64)&device},
		{"samplerAnisotropy", (u64)&device->vk10Features.features.samplerAnisotropy - (u64)&device},
		{"textureCompressionETC2", (u64)&device->vk10Features.features.textureCompressionETC2 - (u64)&device},
		{"textureCompressionASTC_LDR", (u64)&device->vk10Features.features.textureCompressionASTC_LDR - (u64)&device},
		{"textureCompressionBC", (u64)&device->vk10Features.features.textureCompressionBC - (u64)&device},
		{"occlusionQueryPrecise", (u64)&device->vk10Features.features.occlusionQueryPrecise - (u64)&device},
		{"pipelineStatisticsQuery", (u64)&device->vk10Features.features.pipelineStatisticsQuery - (u64)&device},
		{"vertexPipelineStoresAndAtomics", (u64)&device->vk10Features.features.vertexPipelineStoresAndAtomics - (u64)&device},
		{"fragmentStoresAndAtomics", (u64)&device->vk10Features.features.fragmentStoresAndAtomics - (u64)&device},
		{"shaderTessellationAndGeometryPointSize", (u64)&device->vk10Features.features.shaderTessellationAndGeometryPointSize - (u64)&device},
		{"shaderImageGatherExtended", (u64)&device->vk10Features.features.shaderImageGatherExtended - (u64)&device},
		{"shaderStorageImageExtendedFormats", (u64)&device->vk10Features.features.shaderStorageImageExtendedFormats - (u64)&device},
		{"shaderStorageImageMultisample", (u64)&device->vk10Features.features.shaderStorageImageMultisample - (u64)&device},
		{"shaderStorageImageReadWithoutFormat", (u64)&device->vk10Features.features.shaderStorageImageReadWithoutFormat - (u64)&device},
		{"shaderStorageImageWriteWithoutFormat", (u64)&device->vk10Features.features.shaderStorageImageWriteWithoutFormat - (u64)&device},
		{"shaderUniformBufferArrayDynamicIndexing", (u64)&device->vk10Features.features.shaderUniformBufferArrayDynamicIndexing - (u64)&device},
		{"shaderSampledImageArrayDynamicIndexing", (u64)&device->vk10Features.features.shaderSampledImageArrayDynamicIndexing - (u64)&device},
		{"shaderStorageBufferArrayDynamicIndexing", (u64)&device->vk10Features.features.shaderStorageBufferArrayDynamicIndexing - (u64)&device},
		{"shaderStorageImageArrayDynamicIndexing", (u64)&device->vk10Features.features.shaderStorageImageArrayDynamicIndexing - (u64)&device},
		{"shaderClipDistance", (u64)&device->vk10Features.features.shaderClipDistance - (u64)&device},
		{"shaderCullDistance", (u64)&device->vk10Features.features.shaderCullDistance - (u64)&device},
		{"shaderFloat64", (u64)&device->vk10Features.features.shaderFloat64 - (u64)&device},
		{"shaderInt64", (u64)&device->vk10Features.features.shaderInt64 - (u64)&device},
		{"shaderInt16", (u64)&device->vk10Features.features.shaderInt16 - (u64)&device},
		{"shaderResourceResidency", (u64)&device->vk10Features.features.shaderResourceResidency - (u64)&device},
		{"shaderResourceMinLod", (u64)&device->vk10Features.features.shaderResourceMinLod - (u64)&device},
		{"sparseBinding", (u64)&device->vk10Features.features.sparseBinding - (u64)&device},
		{"sparseResidencyBuffer", (u64)&device->vk10Features.features.sparseResidencyBuffer - (u64)&device},
		{"sparseResidencyImage2D", (u64)&device->vk10Features.features.sparseResidencyImage2D - (u64)&device},
		{"sparseResidencyImage3D", (u64)&device->vk10Features.features.sparseResidencyImage3D - (u64)&device},
		{"sparseResidency2Samples", (u64)&device->vk10Features.features.sparseResidency2Samples - (u64)&device},
		{"sparseResidency4Samples", (u64)&device->vk10Features.features.sparseResidency4Samples - (u64)&device},
		{"sparseResidency8Samples", (u64)&device->vk10Features.features.sparseResidency8Samples - (u64)&device},
		{"sparseResidency16Samples", (u64)&device->vk10Features.features.sparseResidency16Samples - (u64)&device},
		{"sparseResidencyAliased", (u64)&device->vk10Features.features.sparseResidencyAliased - (u64)&device},
		{"variableMultisampleRate", (u64)&device->vk10Features.features.variableMultisampleRate - (u64)&device},
		{"inheritedQueries", (u64)&device->vk10Features.features.inheritedQueries - (u64)&device},
		// Vulkan 1.1 Features
		{"storageBuffer16BitAccess", (u64)&device->vk11Features.storageBuffer16BitAccess - (u64)&device},
		{"uniformAndStorageBuffer16BitAccess", (u64)&device->vk11Features.uniformAndStorageBuffer16BitAccess - (u64)&device},
		{"storagePushConstant16", (u64)&device->vk11Features.storagePushConstant16 - (u64)&device},
		{"storageInputOutput16", (u64)&device->vk11Features.storageInputOutput16 - (u64)&device},
		{"multiview", (u64)&device->vk11Features.multiview - (u64)&device},
		{"multiviewGeometryShader", (u64)&device->vk11Features.multiviewGeometryShader - (u64)&device},
		{"multiviewTessellationShader", (u64)&device->vk11Features.multiviewTessellationShader - (u64)&device},
		{"variablePointersStorageBuffer", (u64)&device->vk11Features.variablePointersStorageBuffer - (u64)&device},
		{"variablePointers", (u64)&device->vk11Features.variablePointers - (u64)&device},
		{"protectedMemory", (u64)&device->vk11Features.protectedMemory - (u64)&device},
		{"samplerYcbcrConversion", (u64)&device->vk11Features.samplerYcbcrConversion - (u64)&device},
		{"shaderDrawParameters", (u64)&device->vk11Features.shaderDrawParameters - (u64)&device},
		// Vulkan 1.2 Features
		{"samplerMirrorClampToEdge", (u64)&device->vk12Features.samplerMirrorClampToEdge - (u64)&device},
		{"drawIndirectCount", (u64)&device->vk12Features.drawIndirectCount - (u64)&device},
		{"storageBuffer8BitAccess", (u64)&device->vk12Features.storageBuffer8BitAccess - (u64)&device},
		{"uniformAndStorageBuffer8BitAccess", (u64)&device->vk12Features.uniformAndStorageBuffer8BitAccess - (u64)&device},
		{"storagePushConstant8", (u64)&device->vk12Features.storagePushConstant8 - (u64)&device},
		{"shaderBufferInt64Atomics", (u64)&device->vk12Features.shaderBufferInt64Atomics - (u64)&device},
		{"shaderSharedInt64Atomics", (u64)&device->vk12Features.shaderSharedInt64Atomics - (u64)&device},
		{"shaderFloat16", (u64)&device->vk12Features.shaderFloat16 - (u64)&device},
		{"shaderInt8", (u64)&device->vk12Features.shaderInt8 - (u64)&device},
		{"descriptorIndexing", (u64)&device->vk12Features.descriptorIndexing - (u64)&device},
		{"shaderInputAttachmentArrayDynamicIndexing", (u64)&device->vk12Features.shaderInputAttachmentArrayDynamicIndexing - (u64)&device},
		{"shaderUniformTexelBufferArrayDynamicIndexing", (u64)&device->vk12Features.shaderUniformTexelBufferArrayDynamicIndexing - (u64)&device},
		{"shaderStorageTexelBufferArrayDynamicIndexing", (u64)&device->vk12Features.shaderStorageTexelBufferArrayDynamicIndexing - (u64)&device},
		{"shaderUniformBufferArrayNonUniformIndexing", (u64)&device->vk12Features.shaderUniformBufferArrayNonUniformIndexing - (u64)&device},
		{"shaderSampledImageArrayNonUniformIndexing", (u64)&device->vk12Features.shaderSampledImageArrayNonUniformIndexing - (u64)&device},
		{"shaderStorageBufferArrayNonUniformIndexing", (u64)&device->vk12Features.shaderStorageBufferArrayNonUniformIndexing - (u64)&device},
		{"shaderStorageImageArrayNonUniformIndexing", (u64)&device->vk12Features.shaderStorageImageArrayNonUniformIndexing - (u64)&device},
		{"shaderInputAttachmentArrayNonUniformIndexing", (u64)&device->vk12Features.shaderInputAttachmentArrayNonUniformIndexing - (u64)&device},
		{"shaderUniformTexelBufferArrayNonUniformIndexing", (u64)&device->vk12Features.shaderUniformTexelBufferArrayNonUniformIndexing - (u64)&device},
		{"shaderStorageTexelBufferArrayNonUniformIndexing", (u64)&device->vk12Features.shaderStorageTexelBufferArrayNonUniformIndexing - (u64)&device},
		{"descriptorBindingUniformBufferUpdateAfterBind", (u64)&device->vk12Features.descriptorBindingUniformBufferUpdateAfterBind - (u64)&device},
		{"descriptorBindingSampledImageUpdateAfterBind", (u64)&device->vk12Features.descriptorBindingSampledImageUpdateAfterBind - (u64)&device},
		{"descriptorBindingStorageImageUpdateAfterBind", (u64)&device->vk12Features.descriptorBindingStorageImageUpdateAfterBind - (u64)&device},
		{"descriptorBindingStorageBufferUpdateAfterBind", (u64)&device->vk12Features.descriptorBindingStorageBufferUpdateAfterBind - (u64)&device},
		{"descriptorBindingUniformTexelBufferUpdateAfterBind", (u64)&device->vk12Features.descriptorBindingUniformTexelBufferUpdateAfterBind - (u64)&device},
		{"descriptorBindingStorageTexelBufferUpdateAfterBind", (u64)&device->vk12Features.descriptorBindingStorageTexelBufferUpdateAfterBind - (u64)&device},
		{"descriptorBindingUpdateUnusedWhilePending", (u64)&device->vk12Features.descriptorBindingUpdateUnusedWhilePending - (u64)&device},
		{"descriptorBindingPartiallyBound", (u64)&device->vk12Features.descriptorBindingPartiallyBound - (u64)&device},
		{"descriptorBindingVariableDescriptorCount", (u64)&device->vk12Features.descriptorBindingVariableDescriptorCount - (u64)&device},
		{"runtimeDescriptorArray", (u64)&device->vk12Features.runtimeDescriptorArray - (u64)&device},
		{"samplerFilterMinmax", (u64)&device->vk12Features.samplerFilterMinmax - (u64)&device},
		{"scalarBlockLayout", (u64)&device->vk12Features.scalarBlockLayout - (u64)&device},
		{"imagelessFramebuffer", (u64)&device->vk12Features.imagelessFramebuffer - (u64)&device},
		{"uniformBufferStandardLayout", (u64)&device->vk12Features.uniformBufferStandardLayout - (u64)&device},
		{"shaderSubgroupExtendedTypes", (u64)&device->vk12Features.shaderSubgroupExtendedTypes - (u64)&device},
		{"separateDepthStencilLayouts", (u64)&device->vk12Features.separateDepthStencilLayouts - (u64)&device},
		{"hostQueryReset", (u64)&device->vk12Features.hostQueryReset - (u64)&device},
		{"timelineSemaphore", (u64)&device->vk12Features.timelineSemaphore - (u64)&device},
		{"bufferDeviceAddress", (u64)&device->vk12Features.bufferDeviceAddress - (u64)&device},
		{"bufferDeviceAddressCaptureReplay", (u64)&device->vk12Features.bufferDeviceAddressCaptureReplay - (u64)&device},
		{"bufferDeviceAddressMultiDevice", (u64)&device->vk12Features.bufferDeviceAddressMultiDevice - (u64)&device},
		{"vulkanMemoryModel", (u64)&device->vk12Features.vulkanMemoryModel - (u64)&device},
		{"vulkanMemoryModelDeviceScope", (u64)&device->vk12Features.vulkanMemoryModelDeviceScope - (u64)&device},
		{"vulkanMemoryModelAvailabilityVisibilityChains", (u64)&device->vk12Features.vulkanMemoryModelAvailabilityVisibilityChains - (u64)&device},
		{"shaderOutputViewportIndex", (u64)&device->vk12Features.shaderOutputViewportIndex - (u64)&device},
		{"shaderOutputLayer", (u64)&device->vk12Features.shaderOutputLayer - (u64)&device},
		{"subgroupBroadcastDynamicId", (u64)&device->vk12Features.subgroupBroadcastDynamicId - (u64)&device},
		// Vulkan 1.3 Features
		{"robustImageAccess", (u64)&device->vk13Features.robustImageAccess - (u64)&device},
		{"inlineUniformBlock", (u64)&device->vk13Features.inlineUniformBlock - (u64)&device},
		{"descriptorBindingInlineUniformBlockUpdateAfterBind", (u64)&device->vk13Features.descriptorBindingInlineUniformBlockUpdateAfterBind - (u64)&device},
		{"pipelineCreationCacheControl", (u64)&device->vk13Features.pipelineCreationCacheControl - (u64)&device},
		{"privateData", (u64)&device->vk13Features.privateData - (u64)&device},
		{"shaderDemoteToHelperInvocation", (u64)&device->vk13Features.shaderDemoteToHelperInvocation - (u64)&device},
		{"shaderTerminateInvocation", (u64)&device->vk13Features.shaderTerminateInvocation - (u64)&device},
		{"subgroupSizeControl", (u64)&device->vk13Features.subgroupSizeControl - (u64)&device},
		{"computeFullSubgroups", (u64)&device->vk13Features.computeFullSubgroups - (u64)&device},
		{"synchronization2", (u64)&device->vk13Features.synchronization2 - (u64)&device},
		{"textureCompressionASTC_HDR", (u64)&device->vk13Features.textureCompressionASTC_HDR - (u64)&device},
		{"shaderZeroInitializeWorkgroupMemory", (u64)&device->vk13Features.shaderZeroInitializeWorkgroupMemory - (u64)&device},
		{"dynamicRendering", (u64)&device->vk13Features.dynamicRendering - (u64)&device},
		{"shaderIntegerDotProduct", (u64)&device->vk13Features.shaderIntegerDotProduct - (u64)&device},
		{"maintenance4", (u64)&device->vk13Features.maintenance4 - (u64)&device},
	};
	for (Str feature : features) {
		if (auto *node = featureByteOffsets.Find(feature)) {
			*(VkBool32*)((u64)&device + node->value) = VK_TRUE;
		} else {
			AzAssert(false, Stringify("Feature string \"", feature, "\" is unrecognized"));
		}
	}
}

#endif

#ifndef Resources

Result<VoidResult_t, String> BufferInit(Buffer *buffer) {
	// AzAssert(buffer->size > 0, "Cannot allocate a buffer with size <= 0");
	INIT_HEAD(buffer);
	VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	createInfo.size = buffer->size;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	switch (buffer->kind) {
		case Buffer::VERTEX_BUFFER:
			createInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case Buffer::INDEX_BUFFER:
			createInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;
		case Buffer::STORAGE_BUFFER:
			createInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			break;
		case Buffer::UNIFORM_BUFFER:
			createInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;
		default:
			return ERROR_RESULT(buffer, "Cannot initialize buffer with undefined Kind");
	}
	if (VkResult result = vkCreateBuffer(buffer->device->vkDevice, &createInfo, nullptr, &buffer->vkBuffer); result != VK_SUCCESS) {
		return ERROR_RESULT(buffer, "Failed to create buffer: ", VkResultString(result));
	}
	SetDebugMarker(buffer->device, buffer->tag, VK_OBJECT_TYPE_BUFFER, (u64)buffer->vkBuffer);
	vkGetBufferMemoryRequirements(buffer->device->vkDevice, buffer->vkBuffer, &buffer->memoryRequirements);
	if (auto result = AllocateBuffer(buffer->device, buffer->vkBuffer, buffer->memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); result.isError) {
		return result.error;
	} else {
		buffer->alloc = result.value;
	}
	buffer->initted = true;
	buffer->timestamp = GetTimestamp();
	return VoidResult_t();
}

void BufferDeinit(Buffer *buffer) {
	DEINIT_HEAD(buffer);
	vkDestroyBuffer(buffer->device->vkDevice, buffer->vkBuffer, nullptr);
	MemoryFree(buffer->alloc);
	if (buffer->hostVisible) {
		vkDestroyBuffer(buffer->device->vkDevice, buffer->vkBufferHostVisible, nullptr);
		MemoryFree(buffer->allocHostVisible);
		buffer->hostVisible = false;
	}
	buffer->initted = false;
}

Result<VoidResult_t, String> BufferHostInit(Buffer *buffer) {
	AzAssert(buffer->initted == true, "Trying to init staging buffer for buffer that's not initted");
	AzAssert(buffer->hostVisible == false, "Trying to init staging buffer that's already initted");
	TRACE_INIT(buffer);
	VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	createInfo.size = buffer->size;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (VkResult result = vkCreateBuffer(buffer->device->vkDevice, &createInfo, nullptr, &buffer->vkBufferHostVisible); result != VK_SUCCESS) {
		return ERROR_RESULT(buffer, "Failed to create staging buffer: ", VkResultString(result));
	}
	SetDebugMarker(buffer->device, Stringify(buffer->tag, " host-visible buffer"), VK_OBJECT_TYPE_BUFFER, (u64)buffer->vkBufferHostVisible);
	if (auto result = AllocateBuffer(buffer->device, buffer->vkBufferHostVisible, buffer->memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT); result.isError) {
		return result.error;
	} else {
		buffer->allocHostVisible = result.value;
	}
	buffer->hostVisible = true;
	return VoidResult_t();
}

void BufferHostDeinit(Buffer *buffer) {
	AzAssert(buffer->initted == true, "Trying to deinit staging buffer for buffer that's not initted");
	AzAssert(buffer->hostVisible == true, "Trying to deinit staging buffer that's not initted");
	TRACE_DEINIT(buffer);
	vkDestroyBuffer(buffer->device->vkDevice, buffer->vkBufferHostVisible, nullptr);
	MemoryFree(buffer->allocHostVisible);
	buffer->hostVisible = false;
}

void BufferSetSize(Buffer *buffer, i64 sizeBytes) {
	bool initted = buffer->initted;
	buffer->size = sizeBytes;
	if (initted) {
		// Reinit
		AzAssertRel(false, "Unimplemented");
	}
}

void BufferSetShaderUsage(Buffer *buffer, u32 shaderStages) {
	buffer->shaderStages = shaderStages;
}

Result<VoidResult_t, String> ImageInit(Image *image) {
	INIT_HEAD(image);
	VkImageCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.format = image->vkFormat;
	createInfo.extent.width = image->width;
	createInfo.extent.height = image->height;
	createInfo.extent.depth = 1;
	createInfo.mipLevels = image->mipLevels;
	createInfo.arrayLayers = 1;
	createInfo.samples = (VkSampleCountFlagBits)image->sampleCount;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	if (image->transferSrc) {
		createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	if (image->transferDst) {
		createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	if (image->mipmapped) {
		createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	if (image->shaderStages) {
		createInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	if (image->attachment) {
		if (FormatIsDepth(image->vkFormat)) {
			createInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		} else {
			createInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
	}
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

	if (VkResult result = vkCreateImage(image->device->vkDevice, &createInfo, nullptr, &image->vkImage); result != VK_SUCCESS) {
		return ERROR_RESULT(image, "Failed to create image: ", VkResultString(result));
	}
	SetDebugMarker(image->device, image->tag, VK_OBJECT_TYPE_IMAGE, (u64)image->vkImage);
	vkGetImageMemoryRequirements(image->device->vkDevice, image->vkImage, &image->memoryRequirements);
	if (auto result = AllocateImage(image->device, image->vkImage, image->memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false); result.isError) {
		return result.error;
	} else {
		image->alloc = result.value;
	}
	VkImageViewCreateInfo viewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	viewCreateInfo.image = image->vkImage;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = image->vkFormat;
	viewCreateInfo.subresourceRange.aspectMask = image->vkImageAspect;
	viewCreateInfo.subresourceRange.levelCount = image->mipLevels;
	viewCreateInfo.subresourceRange.layerCount = 1;

	if (VkResult result = vkCreateImageView(image->device->vkDevice, &viewCreateInfo, nullptr, &image->vkImageView); result != VK_SUCCESS) {
		return ERROR_RESULT(image, "Failed to create image view: ", VkResultString(result));
	}
	if (image->attachment && image->mipmapped && image->mipLevels > 1) {
		viewCreateInfo.subresourceRange.levelCount = 1;
		if (VkResult result = vkCreateImageView(image->device->vkDevice, &viewCreateInfo, nullptr, &image->vkImageViewAttachment); result != VK_SUCCESS) {
			return ERROR_RESULT(image, "Failed to create image view: ", VkResultString(result));
		}
	} else {
		image->vkImageViewAttachment = image->vkImageView;
	}
	SetDebugMarker(image->device, Stringify(image->tag, " image view"), VK_OBJECT_TYPE_IMAGE_VIEW, (u64)image->vkImageView);
	image->initted = true;
	image->timestamp = GetTimestamp();
	return VoidResult_t();
}

void ImageDeinit(Image *image) {
	DEINIT_HEAD(image);
	vkDestroyImageView(image->device->vkDevice, image->vkImageView, nullptr);
	if (image->vkImageViewAttachment != image->vkImageView) {
		vkDestroyImageView(image->device->vkDevice, image->vkImageViewAttachment, nullptr);
	}
	vkDestroyImage(image->device->vkDevice, image->vkImage, nullptr);
	MemoryFree(image->alloc);
	if (image->hostVisible) {
		vkDestroyBuffer(image->device->vkDevice, image->vkBufferHostVisible, nullptr);
		MemoryFree(image->allocHostVisible);
		image->hostVisible = false;
	}
	image->initted = false;
}

Result<VoidResult_t, String> ImageRecreate(Image *image) {
	AzAssert(image->initted, "Cannot Recreate an image that's not initialized");
	vkDeviceWaitIdle(image->device->vkDevice);
	ImageDeinit(image);
	return ImageInit(image);
}

Result<VoidResult_t, String> ImageHostInit(Image *image) {
	AzAssert(image->initted == true, "Trying to init image staging buffer that's not initted");
	AzAssert(image->hostVisible == false, "Trying to init image staging buffer that's already initted");
	TRACE_INIT(image);
	VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	createInfo.size = image->width * image->height * image->bytesPerPixel;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (VkResult result = vkCreateBuffer(image->device->vkDevice, &createInfo, nullptr, &image->vkBufferHostVisible); result != VK_SUCCESS) {
		return ERROR_RESULT(image, "Failed to create image staging buffer: ", VkResultString(result));
	}
	SetDebugMarker(image->device, Stringify(image->tag, " host-visible buffer"), VK_OBJECT_TYPE_BUFFER, (u64)image->vkBufferHostVisible);
	vkGetBufferMemoryRequirements(image->device->vkDevice, image->vkBufferHostVisible, &image->memoryRequirementsHost);
	if (auto result = AllocateBuffer(image->device, image->vkBufferHostVisible, image->memoryRequirementsHost, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT); result.isError) {
		return result.error;
	} else {
		image->allocHostVisible = result.value;
	}
	image->hostVisible = true;
	return VoidResult_t();
}

void ImageHostDeinit(Image *image) {
	AzAssert(image->initted == true, "Trying to deinit image staging buffer that's not initted");
	AzAssert(image->hostVisible == true, "Trying to deinit image staging buffer that's not initted");
	TRACE_DEINIT(image);
	vkDestroyBuffer(image->device->vkDevice, image->vkBufferHostVisible, nullptr);
	MemoryFree(image->allocHostVisible);
	image->hostVisible = false;
}

void ImageSetFormat(Image *image, ImageBits imageBits, ImageComponentType componentType) {
	switch (imageBits) {
		case ImageBits::D16:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_D16_UNORM;   break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 2;
			break;
		case ImageBits::D24:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_X8_D24_UNORM_PACK32;   break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 4;
			break;
		case ImageBits::D32:
			switch (componentType) {
				case ImageComponentType::SFLOAT:   image->vkFormat = VK_FORMAT_D32_SFLOAT; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 4;
			break;
		case ImageBits::R8:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_R8_UNORM;   break;
				case ImageComponentType::SNORM:   image->vkFormat = VK_FORMAT_R8_SNORM;   break;
				case ImageComponentType::USCALED: image->vkFormat = VK_FORMAT_R8_USCALED; break;
				case ImageComponentType::SSCALED: image->vkFormat = VK_FORMAT_R8_SSCALED; break;
				case ImageComponentType::UINT:    image->vkFormat = VK_FORMAT_R8_UINT;    break;
				case ImageComponentType::SINT:    image->vkFormat = VK_FORMAT_R8_SINT;    break;
				case ImageComponentType::SRGB:    image->vkFormat = VK_FORMAT_R8_SRGB;    break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 1;
			break;
		case ImageBits::R8G8:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_R8G8_UNORM;   break;
				case ImageComponentType::SNORM:   image->vkFormat = VK_FORMAT_R8G8_SNORM;   break;
				case ImageComponentType::USCALED: image->vkFormat = VK_FORMAT_R8G8_USCALED; break;
				case ImageComponentType::SSCALED: image->vkFormat = VK_FORMAT_R8G8_SSCALED; break;
				case ImageComponentType::UINT:    image->vkFormat = VK_FORMAT_R8G8_UINT;    break;
				case ImageComponentType::SINT:    image->vkFormat = VK_FORMAT_R8G8_SINT;    break;
				case ImageComponentType::SRGB:    image->vkFormat = VK_FORMAT_R8G8_SRGB;    break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 2;
			break;
		case ImageBits::R8G8B8:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_R8G8B8_UNORM;   break;
				case ImageComponentType::SNORM:   image->vkFormat = VK_FORMAT_R8G8B8_SNORM;   break;
				case ImageComponentType::USCALED: image->vkFormat = VK_FORMAT_R8G8B8_USCALED; break;
				case ImageComponentType::SSCALED: image->vkFormat = VK_FORMAT_R8G8B8_SSCALED; break;
				case ImageComponentType::UINT:    image->vkFormat = VK_FORMAT_R8G8B8_UINT;    break;
				case ImageComponentType::SINT:    image->vkFormat = VK_FORMAT_R8G8B8_SINT;    break;
				case ImageComponentType::SRGB:    image->vkFormat = VK_FORMAT_R8G8B8_SRGB;    break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 3;
			break;
		case ImageBits::R8G8B8A8:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_R8G8B8A8_UNORM;   break;
				case ImageComponentType::SNORM:   image->vkFormat = VK_FORMAT_R8G8B8A8_SNORM;   break;
				case ImageComponentType::USCALED: image->vkFormat = VK_FORMAT_R8G8B8A8_USCALED; break;
				case ImageComponentType::SSCALED: image->vkFormat = VK_FORMAT_R8G8B8A8_SSCALED; break;
				case ImageComponentType::UINT:    image->vkFormat = VK_FORMAT_R8G8B8A8_UINT;    break;
				case ImageComponentType::SINT:    image->vkFormat = VK_FORMAT_R8G8B8A8_SINT;    break;
				case ImageComponentType::SRGB:    image->vkFormat = VK_FORMAT_R8G8B8A8_SRGB;    break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 4;
			break;
		case ImageBits::B8G8R8:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_B8G8R8_UNORM;   break;
				case ImageComponentType::SNORM:   image->vkFormat = VK_FORMAT_B8G8R8_SNORM;   break;
				case ImageComponentType::USCALED: image->vkFormat = VK_FORMAT_B8G8R8_USCALED; break;
				case ImageComponentType::SSCALED: image->vkFormat = VK_FORMAT_B8G8R8_SSCALED; break;
				case ImageComponentType::UINT:    image->vkFormat = VK_FORMAT_B8G8R8_UINT;    break;
				case ImageComponentType::SINT:    image->vkFormat = VK_FORMAT_B8G8R8_SINT;    break;
				case ImageComponentType::SRGB:    image->vkFormat = VK_FORMAT_B8G8R8_SRGB;    break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 3;
			break;
		case ImageBits::B8G8R8A8:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_B8G8R8A8_UNORM;   break;
				case ImageComponentType::SNORM:   image->vkFormat = VK_FORMAT_B8G8R8A8_SNORM;   break;
				case ImageComponentType::USCALED: image->vkFormat = VK_FORMAT_B8G8R8A8_USCALED; break;
				case ImageComponentType::SSCALED: image->vkFormat = VK_FORMAT_B8G8R8A8_SSCALED; break;
				case ImageComponentType::UINT:    image->vkFormat = VK_FORMAT_B8G8R8A8_UINT;    break;
				case ImageComponentType::SINT:    image->vkFormat = VK_FORMAT_B8G8R8A8_SINT;    break;
				case ImageComponentType::SRGB:    image->vkFormat = VK_FORMAT_B8G8R8A8_SRGB;    break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 4;
			break;
		case ImageBits::R16:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_R16_UNORM;   break;
				case ImageComponentType::SNORM:   image->vkFormat = VK_FORMAT_R16_SNORM;   break;
				case ImageComponentType::USCALED: image->vkFormat = VK_FORMAT_R16_USCALED; break;
				case ImageComponentType::SSCALED: image->vkFormat = VK_FORMAT_R16_SSCALED; break;
				case ImageComponentType::UINT:    image->vkFormat = VK_FORMAT_R16_UINT;    break;
				case ImageComponentType::SINT:    image->vkFormat = VK_FORMAT_R16_SINT;    break;
				case ImageComponentType::SFLOAT:  image->vkFormat = VK_FORMAT_R16_SFLOAT;  break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 2;
			break;
		case ImageBits::R16G16:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_R16G16_UNORM;   break;
				case ImageComponentType::SNORM:   image->vkFormat = VK_FORMAT_R16G16_SNORM;   break;
				case ImageComponentType::USCALED: image->vkFormat = VK_FORMAT_R16G16_USCALED; break;
				case ImageComponentType::SSCALED: image->vkFormat = VK_FORMAT_R16G16_SSCALED; break;
				case ImageComponentType::UINT:    image->vkFormat = VK_FORMAT_R16G16_UINT;    break;
				case ImageComponentType::SINT:    image->vkFormat = VK_FORMAT_R16G16_SINT;    break;
				case ImageComponentType::SFLOAT:  image->vkFormat = VK_FORMAT_R16G16_SFLOAT;  break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 4;
			break;
		case ImageBits::R16G16B16:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_R16G16B16_UNORM;   break;
				case ImageComponentType::SNORM:   image->vkFormat = VK_FORMAT_R16G16B16_SNORM;   break;
				case ImageComponentType::USCALED: image->vkFormat = VK_FORMAT_R16G16B16_USCALED; break;
				case ImageComponentType::SSCALED: image->vkFormat = VK_FORMAT_R16G16B16_SSCALED; break;
				case ImageComponentType::UINT:    image->vkFormat = VK_FORMAT_R16G16B16_UINT;    break;
				case ImageComponentType::SINT:    image->vkFormat = VK_FORMAT_R16G16B16_SINT;    break;
				case ImageComponentType::SFLOAT:  image->vkFormat = VK_FORMAT_R16G16B16_SFLOAT;  break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 6;
			break;
		case ImageBits::R16G16B16A16:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_R16G16B16A16_UNORM;   break;
				case ImageComponentType::SNORM:   image->vkFormat = VK_FORMAT_R16G16B16A16_SNORM;   break;
				case ImageComponentType::USCALED: image->vkFormat = VK_FORMAT_R16G16B16A16_USCALED; break;
				case ImageComponentType::SSCALED: image->vkFormat = VK_FORMAT_R16G16B16A16_SSCALED; break;
				case ImageComponentType::UINT:    image->vkFormat = VK_FORMAT_R16G16B16A16_UINT;    break;
				case ImageComponentType::SINT:    image->vkFormat = VK_FORMAT_R16G16B16A16_SINT;    break;
				case ImageComponentType::SFLOAT:  image->vkFormat = VK_FORMAT_R16G16B16A16_SFLOAT;  break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 8;
			break;
		case ImageBits::R32:
			switch (componentType) {
				case ImageComponentType::UINT:   image->vkFormat = VK_FORMAT_R32_UINT;   break;
				case ImageComponentType::SINT:   image->vkFormat = VK_FORMAT_R32_SINT;   break;
				case ImageComponentType::SFLOAT: image->vkFormat = VK_FORMAT_R32_SFLOAT; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 4;
			break;
		case ImageBits::R32G32:
			switch (componentType) {
				case ImageComponentType::UINT:   image->vkFormat = VK_FORMAT_R32G32_UINT;   break;
				case ImageComponentType::SINT:   image->vkFormat = VK_FORMAT_R32G32_SINT;   break;
				case ImageComponentType::SFLOAT: image->vkFormat = VK_FORMAT_R32G32_SFLOAT; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 8;
			break;
		case ImageBits::R32G32B32:
			switch (componentType) {
				case ImageComponentType::UINT:   image->vkFormat = VK_FORMAT_R32G32B32_UINT;   break;
				case ImageComponentType::SINT:   image->vkFormat = VK_FORMAT_R32G32B32_SINT;   break;
				case ImageComponentType::SFLOAT: image->vkFormat = VK_FORMAT_R32G32B32_SFLOAT; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 12;
			break;
		case ImageBits::R32G32B32A32:
			switch (componentType) {
				case ImageComponentType::UINT:   image->vkFormat = VK_FORMAT_R32G32B32A32_UINT;   break;
				case ImageComponentType::SINT:   image->vkFormat = VK_FORMAT_R32G32B32A32_SINT;   break;
				case ImageComponentType::SFLOAT: image->vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 16;
			break;
		case ImageBits::R64:
			switch (componentType) {
				case ImageComponentType::UINT:   image->vkFormat = VK_FORMAT_R64_UINT;   break;
				case ImageComponentType::SINT:   image->vkFormat = VK_FORMAT_R64_SINT;   break;
				case ImageComponentType::SFLOAT: image->vkFormat = VK_FORMAT_R64_SFLOAT; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 8;
			break;
		case ImageBits::R64G64:
			switch (componentType) {
				case ImageComponentType::UINT:   image->vkFormat = VK_FORMAT_R64G64_UINT;   break;
				case ImageComponentType::SINT:   image->vkFormat = VK_FORMAT_R64G64_SINT;   break;
				case ImageComponentType::SFLOAT: image->vkFormat = VK_FORMAT_R64G64_SFLOAT; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 16;
			break;
		case ImageBits::R64G64B64:
			switch (componentType) {
				case ImageComponentType::UINT:   image->vkFormat = VK_FORMAT_R64G64B64_UINT;   break;
				case ImageComponentType::SINT:   image->vkFormat = VK_FORMAT_R64G64B64_SINT;   break;
				case ImageComponentType::SFLOAT: image->vkFormat = VK_FORMAT_R64G64B64_SFLOAT; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 24;
			break;
		case ImageBits::R64G64B64A64:
			switch (componentType) {
				case ImageComponentType::UINT:   image->vkFormat = VK_FORMAT_R64G64B64A64_UINT;   break;
				case ImageComponentType::SINT:   image->vkFormat = VK_FORMAT_R64G64B64A64_SINT;   break;
				case ImageComponentType::SFLOAT: image->vkFormat = VK_FORMAT_R64G64B64A64_SFLOAT; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 32;
			break;
		case ImageBits::R4G4:
			switch (componentType) {
				case ImageComponentType::UNORM: image->vkFormat = VK_FORMAT_R4G4_UNORM_PACK8; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 1;
			break;
		case ImageBits::R4G4B4A4:
			switch (componentType) {
				case ImageComponentType::UNORM: image->vkFormat = VK_FORMAT_R4G4B4A4_UNORM_PACK16; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 2;
			break;
		case ImageBits::R5G6B5:
			switch (componentType) {
				case ImageComponentType::UNORM: image->vkFormat = VK_FORMAT_R5G6B5_UNORM_PACK16; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 2;
			break;
		case ImageBits::R5G5B5A1:
			switch (componentType) {
				case ImageComponentType::UNORM: image->vkFormat = VK_FORMAT_R5G5B5A1_UNORM_PACK16; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 2;
			break;
		case ImageBits::A2R10G10B10:
			switch (componentType) {
				case ImageComponentType::UNORM:   image->vkFormat = VK_FORMAT_A2R10G10B10_UNORM_PACK32;   break;
				case ImageComponentType::SNORM:   image->vkFormat = VK_FORMAT_A2R10G10B10_SNORM_PACK32;   break;
				case ImageComponentType::USCALED: image->vkFormat = VK_FORMAT_A2R10G10B10_USCALED_PACK32; break;
				case ImageComponentType::SSCALED: image->vkFormat = VK_FORMAT_A2R10G10B10_SSCALED_PACK32; break;
				case ImageComponentType::UINT:    image->vkFormat = VK_FORMAT_A2R10G10B10_UINT_PACK32;    break;
				case ImageComponentType::SINT:    image->vkFormat = VK_FORMAT_A2R10G10B10_SINT_PACK32;    break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 4;
			break;
		case ImageBits::B10G11R11:
			switch (componentType) {
				case ImageComponentType::UFLOAT: image->vkFormat = VK_FORMAT_B10G11R11_UFLOAT_PACK32; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 4;
			break;
		case ImageBits::E5B9G9R9:
			switch (componentType) {
				case ImageComponentType::UFLOAT: image->vkFormat = VK_FORMAT_E5B9G9R9_UFLOAT_PACK32; break;
				default: goto bad_format;
			}
			image->bytesPerPixel = 4;
			break;
		default: goto bad_format;
	}
	if (FormatIsDepth(image->vkFormat)) {
		image->vkImageAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	} else {
		image->vkImageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	return;
bad_format:
	AzAssertRel(false, Stringify("Cannot match ", imageBits, " bit layout and component type ", componentType));
}

void ImageSetSize(Image *image, i32 width, i32 height) {
	image->width = width;
	image->height = height;
	if (image->mipmapped) {
		image->mipLevels = (u32)ceil(log2((f64)max(image->width, image->height)));
	}
}

void ImageSetMipmapping(Image *image, bool enableMipmapping) {
	image->mipmapped = enableMipmapping;
	if (image->mipmapped) {
		if (image->width == 1 && image->height == 1) {
			image->mipLevels = 1;
			image->mipmapped = false;
			WARNING(image, "Image is too small to use mipmaps (1x1). Ignoring.");
		} else {
			image->mipLevels = (u32)ceil(log2((f64)max(image->width, image->height)));
		}
	} else {
		image->mipLevels = 1;
	}
}

void ImageSetShaderUsage(Image *image, u32 shaderStages) {
	image->shaderStages = shaderStages;
}

void ImageSetSampleCount(Image *image, u32 sampleCount) {
	AzAssert(IsPowerOfTwo(sampleCount), "sampleCount must be a power of 2");
	AzAssert(sampleCount <= 64, "sampleCount must not be > 64");
	AzAssert(sampleCount > 0, "sampleCount must be > 0");
	image->sampleCount = sampleCount;
}

static VkFilter GetVkFilter(Filter filter) {
	switch (filter) {
		case Filter::NEAREST:
			return VK_FILTER_NEAREST;
		case Filter::LINEAR:
			return VK_FILTER_LINEAR;
		case Filter::CUBIC:
			return VK_FILTER_CUBIC_EXT;
		default:
			AzAssert(false, "Unreachable");
			return VK_FILTER_NEAREST;
	}
}

Result<VoidResult_t, String> SamplerInit(Sampler *sampler) {
	INIT_HEAD(sampler);
	VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	// TODO: Make controls for all of these
	samplerCreateInfo.magFilter = GetVkFilter(sampler->magFilter);
	samplerCreateInfo.minFilter = GetVkFilter(sampler->minFilter);
	// TODO: Support trilinear filtering
	samplerCreateInfo.mipmapMode = sampler->mipmapInterpolation ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerCreateInfo.addressModeU = (VkSamplerAddressMode)sampler->addressModeU;
	samplerCreateInfo.addressModeV = (VkSamplerAddressMode)sampler->addressModeV;
	samplerCreateInfo.addressModeW = (VkSamplerAddressMode)sampler->addressModeW;
	samplerCreateInfo.mipLodBias = sampler->lodBias;
	samplerCreateInfo.minLod = sampler->lodMin;
	samplerCreateInfo.maxLod = sampler->lodMax;
	samplerCreateInfo.anisotropyEnable = sampler->anisotropy != 1 ? VK_TRUE : VK_FALSE;
	samplerCreateInfo.maxAnisotropy = sampler->anisotropy;
	samplerCreateInfo.compareEnable = sampler->compare.enable;
	samplerCreateInfo.compareOp = (VkCompareOp)sampler->compare.op;
	samplerCreateInfo.borderColor = sampler->borderColor;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	if (VkResult result = vkCreateSampler(sampler->device->vkDevice, &samplerCreateInfo, nullptr, &sampler->vkSampler); result != VK_SUCCESS) {
		return ERROR_RESULT(sampler, "Failed to create sampler: ", VkResultString(result));
	}
	sampler->initted = true;
	return VoidResult_t();
}

void SamplerDeinit(Sampler *sampler) {
	DEINIT_HEAD(sampler);
	vkDestroySampler(sampler->device->vkDevice, sampler->vkSampler, nullptr);
	sampler->initted = false;
}

void SamplerSetMipmapFiltering(Sampler *sampler, bool enabled) {
	sampler->mipmapInterpolation = enabled;
}

void SamplerSetFiltering(Sampler *sampler, Filter magFilter, Filter minFilter) {
	sampler->magFilter = magFilter;
	sampler->minFilter = minFilter;
}

void SamplerSetAddressMode(Sampler *sampler, AddressMode addressModeU, AddressMode addressModeV, AddressMode addressModeW) {
	sampler->addressModeU = addressModeU;
	sampler->addressModeV = addressModeV;
	sampler->addressModeW = addressModeW;
}

void SamplerSetLod(Sampler *sampler, f32 bias, f32 minimum, f32 maximum) {
	sampler->lodBias = bias;
	sampler->lodMin = minimum;
	sampler->lodMax = maximum;
}

void SamplerSetAnisotropy(Sampler *sampler, i32 anisotropy) {
	sampler->anisotropy = anisotropy;
}

void SamplerSetCompare(Sampler *sampler, bool enable, CompareOp op) {
	sampler->compare.enable = enable;
	sampler->compare.op = op;
}

void SamplerSetBorderColor(Sampler *sampler, bool isFloat, bool white, bool opaque) {
	if (isFloat) {
		if (white) {
			AzAssert(opaque, "Cannot have transparent white");
			sampler->borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		} else {
			if (opaque) {
				sampler->borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			} else {
				sampler->borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			}
		}
	} else {
		if (white) {
			AzAssert(opaque, "Cannot have transparent white");
			sampler->borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		} else {
			if (opaque) {
				sampler->borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			} else {
				sampler->borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
			}
		}
	}
}

#endif

#ifndef Framebuffer

static Result<VoidResult_t, String> EnsureAttachmentIsInitted(Framebuffer *framebuffer, Attachment &attachment, bool isResolve, i32 index) {
	switch (attachment.kind) {
	case Attachment::WINDOW:
		if (!attachment.window->initted) return ERROR_RESULT(framebuffer, "Cannot init Framebuffer when ", isResolve ? "resolve attachment " : "attachment ", index, " (Window) is not initialized");
		break;
	case Attachment::IMAGE:
		if (!attachment.image->initted) return ERROR_RESULT(framebuffer, "Cannot init Framebuffer when ", isResolve ? "resolve attachment " : "attachment ", index, " (Image) is not initialized");
		break;
	case Attachment::DEPTH_BUFFER:
		if (!attachment.depthBuffer->initted) return ERROR_RESULT(framebuffer, "Cannot init Framebuffer when ", isResolve ? "resolve attachment " : "attachment ", index, " (depth buffer Image) is not initialized");
		break;
	}
	return VoidResult_t();
}

static VkAttachmentDescription GetAttachmentDescription(Attachment &attachment, bool willBeResolved) {
	VkAttachmentDescription desc{};
	if (attachment.kind == Attachment::WINDOW) {
		desc.format = attachment.window->surfaceFormat.format;
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
	} else {
		desc.format = attachment.image->vkFormat;
		desc.samples = (VkSampleCountFlagBits)attachment.image->sampleCount;
	}
	desc.storeOp = attachment.store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
	switch (attachment.kind) {
	case Attachment::WINDOW:
		desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		break;
	case Attachment::IMAGE:
		desc.finalLayout = willBeResolved ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		break;
	case Attachment::DEPTH_BUFFER:
		desc.finalLayout = willBeResolved ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		break;
	}
	if (attachment.load) {
		desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		desc.initialLayout = attachment.kind == Attachment::DEPTH_BUFFER ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	} else {
		desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}
	desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	return desc;
}

static VkFormat GetAttachmentFormat(Attachment &attachment) {
	switch (attachment.kind) {
	case Attachment::WINDOW:
		AzAssert(attachment.window->initted, "Cannot get format from an uninitialized Window");
		return attachment.window->surfaceFormat.format;
	case Attachment::IMAGE:
	case Attachment::DEPTH_BUFFER:
		AzAssert(attachment.image->initted, "Cannot get format from an uninitialized Image");
		return attachment.image->vkFormat;
	}
	AzAssert(false, "Unreachable");
	return (VkFormat)0;
}

Result<VoidResult_t, String> FramebufferInit(Framebuffer *framebuffer) {
	INIT_HEAD(framebuffer);
	if (framebuffer->attachmentRefs.size == 0) {
		return ERROR_RESULT(framebuffer, "We have no attachments!");
	}
	{ // RenderPass
		bool hasDepth = false;
		Array<VkAttachmentDescription> attachments;
		Array<VkAttachmentReference> attachmentRefsColor;
		Array<VkAttachmentReference> attachmentRefsResolve;
		VkAttachmentReference attachmentRefDepth;
		Array<u32> preserveAttachments;
		i32 currentAttachment = 0;
		for (i32 i = 0; i < framebuffer->attachmentRefs.size; i++) {
			AttachmentRef &attachmentRef = framebuffer->attachmentRefs[i];
			Attachment &attachment = attachmentRef.attachment;
			if (auto result = EnsureAttachmentIsInitted(framebuffer, attachment, false, i); result.isError) {
				return result.error;
			}
			bool hasResolve = attachmentRef.resolveAttachment.Exists();
			VkAttachmentReference ref;
			ref.attachment = currentAttachment++;
			attachments.Append(GetAttachmentDescription(attachment, hasResolve));
			switch (attachment.kind) {
			case Attachment::WINDOW:
				ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachmentRefsColor.Append(ref);
				break;
			case Attachment::IMAGE:
				ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachmentRefsColor.Append(ref);
				break;
			case Attachment::DEPTH_BUFFER:
				if (hasResolve) {
					return ERROR_RESULT(framebuffer, "Cannot resolve depth attachments");
				}
				if (hasDepth) {
					return ERROR_RESULT(framebuffer, "Cannot have more than one depth attachment");
				}
				hasDepth = true;
				ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				attachmentRefDepth = ref;
				break;
			}
			if (attachment.load && attachment.store) {
				preserveAttachments.Append(currentAttachment);
			}
			if (hasResolve) {
				// By god we're gonna make this happen
				Attachment &resolveAttachment = attachmentRef.resolveAttachment.ValueOrAssert();
				VkFormat baseFormat = GetAttachmentFormat(attachment);
				VkFormat resolveFormat = GetAttachmentFormat(resolveAttachment);
				if (baseFormat != resolveFormat) {
					return ERROR_RESULT(framebuffer, "Multisampled attachment ", i, " format (", VkFormatString(baseFormat), ") doesn't match resolve format (", VkFormatString(resolveFormat), ")");
				}
				if (auto result = EnsureAttachmentIsInitted(framebuffer, resolveAttachment, true, i); result.isError) {
					return result.error;
				}
				ref.attachment = currentAttachment++;
				attachments.Append(GetAttachmentDescription(resolveAttachment, false));
				ref.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				attachmentRefsResolve.Append(ref);
			}
		}
		AzAssert(attachmentRefsResolve.size == 0 || attachmentRefsColor.size == attachmentRefsResolve.size, "Either all color attachments must be resolved, or none of them.");
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = attachmentRefsColor.size;
		subpass.pColorAttachments = attachmentRefsColor.data;
		subpass.pResolveAttachments = attachmentRefsResolve.data;
		subpass.pDepthStencilAttachment = hasDepth ? &attachmentRefDepth : nullptr;
		subpass.preserveAttachmentCount = preserveAttachments.size;
		subpass.pPreserveAttachments = preserveAttachments.data;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		
		VkRenderPassCreateInfo createInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
		createInfo.attachmentCount = attachments.size;
		createInfo.pAttachments = attachments.data;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		// We'll just use barriers to transition layouts
		createInfo.dependencyCount = 0;
		createInfo.pDependencies = nullptr;
		
		if (VkResult result = vkCreateRenderPass(framebuffer->device->vkDevice, &createInfo, nullptr, &framebuffer->vkRenderPass); result != VK_SUCCESS) {
			return ERROR_RESULT(framebuffer, "Failed to create RenderPass: ", VkResultString(result));
		}
		SetDebugMarker(framebuffer->device, Stringify(framebuffer->tag, " render pass"), VK_OBJECT_TYPE_RENDER_PASS, (u64)framebuffer->vkRenderPass);
	}
	framebuffer->initted = true;
	framebuffer->attachmentsDirty = false;
	return FramebufferCreate(framebuffer);
}

void FramebufferDeinit(Framebuffer *framebuffer) {
	DEINIT_HEAD(framebuffer);
	vkDestroyRenderPass(framebuffer->device->vkDevice, framebuffer->vkRenderPass, nullptr);
	for (VkFramebuffer fb : framebuffer->vkFramebuffers) {
		vkDestroyFramebuffer(framebuffer->device->vkDevice, fb, nullptr);
	}
}

static void GetAttachmentDimensions(Attachment &attachment, i32 &dstWidth, i32 &dstHeight, u32 &dstSampleCount, i32 &dstNumFramebuffers) {
	switch (attachment.kind) {
	case Attachment::WINDOW:
		dstNumFramebuffers = attachment.window->numImages;
		dstWidth = attachment.window->extent.width;
		dstHeight = attachment.window->extent.height;
		dstSampleCount = 1;
		break;
	case Attachment::IMAGE:
	case Attachment::DEPTH_BUFFER:
		dstWidth = attachment.image->width;
		dstHeight = attachment.image->height;
		dstSampleCount = attachment.image->sampleCount;
		break;
	}
}

static VkImageView GetAttachmentImageView(Attachment &attachment, i32 framebufferIndex) {
	switch (attachment.kind) {
	case Attachment::WINDOW:
		return attachment.window->swapchainImages[framebufferIndex].vkImageView;
		break;
	case Attachment::IMAGE:
		return attachment.image->vkImageViewAttachment;
		break;
	case Attachment::DEPTH_BUFFER:
		return attachment.depthBuffer->vkImageViewAttachment;
		break;
	}
	AzAssert(false, "Unreachable");
	return VK_NULL_HANDLE;
}

Result<VoidResult_t, String> FramebufferCreate(Framebuffer *framebuffer) {
	AzAssert(framebuffer->initted, "Framebuffer is not initialized");
	if (framebuffer->attachmentsDirty) {
		FramebufferDeinit(framebuffer);
		if (auto result = FramebufferInit(framebuffer); result.isError) {
			return result.error;
		}
	}
	i32 numFramebuffers = 1;
	bool resizeAttachmentsAsNeeded = false;
	for (i32 i = 0; i < framebuffer->attachmentRefs.size; i++) {
		AttachmentRef &attachmentRef = framebuffer->attachmentRefs[i];
		Attachment &attachment = attachmentRef.attachment;
		if (attachment.kind == Attachment::WINDOW) {
			framebuffer->width = attachment.window->extent.width;
			framebuffer->height = attachment.window->extent.height;
			resizeAttachmentsAsNeeded = true;
			break;
		}
		if (attachmentRef.resolveAttachment.Exists()) {
			Attachment &resolveAttachment = attachmentRef.resolveAttachment.ValueOrAssert();
			if (resolveAttachment.kind == Attachment::WINDOW) {
				framebuffer->width = resolveAttachment.window->extent.width;
				framebuffer->height = resolveAttachment.window->extent.height;
				resizeAttachmentsAsNeeded = true;
				break;
			}
		}
	}
	for (i32 i = 0; i < framebuffer->attachmentRefs.size; i++) {
		AttachmentRef &attachmentRef = framebuffer->attachmentRefs[i];
		Attachment &attachment = attachmentRef.attachment;
		i32 ourWidth = 1;
		i32 ourHeight = 1;
		u32 ourSampleCount = 1;
		GetAttachmentDimensions(attachment, ourWidth, ourHeight, ourSampleCount, numFramebuffers);
		if (i == 0) {
			framebuffer->sampleCount = ourSampleCount;
		} else {
			if (framebuffer->sampleCount != ourSampleCount) {
				return ERROR_RESULT(framebuffer, "Attachment ", i, " sample count mismatch. Expected ", framebuffer->sampleCount, ", but got ", ourSampleCount);
			}
		}
		if (framebuffer->width != ourWidth || framebuffer->height != ourHeight) {
			if (resizeAttachmentsAsNeeded) {
				AzAssert(attachment.kind != Attachment::WINDOW, "This shouldn't be possible");
				ImageSetSize(attachment.image, framebuffer->width, framebuffer->height);
				if (auto result = ImageRecreate(attachment.image); result.isError) {
					return ERROR_RESULT(framebuffer, "Attachment ", i, " attempted to resize, but failed: ", result.error);
				}
			} else if (i == 0) {
				framebuffer->width = ourWidth;
				framebuffer->height = ourHeight;
			} else {
				return ERROR_RESULT(framebuffer, "Attachment ", i, " dimensions mismatch. Expected ", framebuffer->width, "x", framebuffer->height, ", but got ", ourWidth, "x", ourHeight);
			}
		}
		if (attachmentRef.resolveAttachment.Exists()) {
			Attachment &resolveAttachment = attachmentRef.resolveAttachment.ValueOrAssert();
			GetAttachmentDimensions(resolveAttachment, ourWidth, ourHeight, ourSampleCount, numFramebuffers);
			if (framebuffer->width != ourWidth || framebuffer->height != ourHeight) {
				if (resizeAttachmentsAsNeeded) {
					AzAssert(resolveAttachment.kind != Attachment::WINDOW, "This shouldn't be possible");
					ImageSetSize(resolveAttachment.image, framebuffer->width, framebuffer->height);
					if (auto result = ImageRecreate(resolveAttachment.image); result.isError) {
						return ERROR_RESULT(framebuffer, "Resolve Attachment ", i, " attempted to resize, but failed: ", result.error);
					}
				} else {
					return ERROR_RESULT(framebuffer, "Resolve Attachment ", i, " dimensions mismatch. Expected ", framebuffer->width, "x", framebuffer->height, ", but got ", ourWidth, "x", ourHeight);
				}
			}
		}
	}
	for (VkFramebuffer fb : framebuffer->vkFramebuffers) {
		vkDestroyFramebuffer(framebuffer->device->vkDevice, fb, nullptr);
	}
	framebuffer->vkFramebuffers.Resize(numFramebuffers);
	Array<VkImageView> imageViews;
	for (i32 i = 0; i < numFramebuffers; i++) {
		VkFramebuffer &vkFramebuffer = framebuffer->vkFramebuffers[i];
		imageViews.ClearSoft();
		for (i32 j = 0; j < framebuffer->attachmentRefs.size; j++) {
			AttachmentRef &attachmentRef = framebuffer->attachmentRefs[j];
			imageViews.Append(GetAttachmentImageView(attachmentRef.attachment, i));
			if (attachmentRef.resolveAttachment.Exists()) {
				imageViews.Append(GetAttachmentImageView(attachmentRef.resolveAttachment.ValueOrAssert(), i));
			}
		}
		VkFramebufferCreateInfo createInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		createInfo.renderPass = framebuffer->vkRenderPass;
		createInfo.width = framebuffer->width;
		createInfo.height = framebuffer->height;
		createInfo.layers = 1;
		createInfo.attachmentCount = imageViews.size;
		createInfo.pAttachments = imageViews.data;
		if (VkResult result = vkCreateFramebuffer(framebuffer->device->vkDevice, &createInfo, nullptr, &vkFramebuffer); result != VK_SUCCESS) {
			return ERROR_RESULT(framebuffer, "Failed to create framebuffer ", i, "/", numFramebuffers, ": ", VkResultString(result));
		}
		SetDebugMarker(framebuffer->device, Stringify(framebuffer->tag, " framebuffer"), VK_OBJECT_TYPE_FRAMEBUFFER, (u64)vkFramebuffer);
	}
	return VoidResult_t();
}

VkFramebuffer FramebufferGetCurrentVkFramebuffer(Framebuffer *framebuffer) {
	AzAssert(framebuffer->vkFramebuffers.size >= 1, "Didn't have any framebuffers???");
	if (framebuffer->vkFramebuffers.size == 1) {
		return framebuffer->vkFramebuffers[0];
	} else if (framebuffer->vkFramebuffers.size > 1) {
		i32 currentFramebuffer = -1;
		for (i32 i = 0; i < framebuffer->attachmentRefs.size; i++) {
			AttachmentRef &attachmentRef = framebuffer->attachmentRefs[i];
			if (attachmentRef.attachment.kind == Attachment::WINDOW) {
				currentFramebuffer = attachmentRef.attachment.window->currentImage;
				break;
			}
			if (attachmentRef.resolveAttachment.Exists()) {
				Attachment &attachment = attachmentRef.resolveAttachment.ValueOrAssert();
				if (attachment.kind == Attachment::WINDOW) {
					currentFramebuffer = attachment.window->currentImage;
					break;
				}
			}
		}
		AzAssert(currentFramebuffer != -1, "Unreachable");
		return framebuffer->vkFramebuffers[currentFramebuffer];
	}
	return 0; // Unnecessary, but shushes the warning
}

bool FramebufferHasDepthBuffer(Framebuffer *framebuffer) {
	for (AttachmentRef &attachmentRef : framebuffer->attachmentRefs) {
		if (attachmentRef.attachment.kind == Attachment::DEPTH_BUFFER) return true;
	}
	return false;
}

Window* FramebufferGetWindowAttachment(Framebuffer *framebuffer) {
	for (AttachmentRef &attachmentRef : framebuffer->attachmentRefs) {
		if (attachmentRef.attachment.kind == Attachment::WINDOW) return attachmentRef.attachment.window;
		if (attachmentRef.resolveAttachment.Exists()) {
			Attachment &attachment = attachmentRef.resolveAttachment.ValueOrAssert();
			if (attachment.kind == Attachment::WINDOW) return attachment.window;
		}
	}
	return nullptr;
}

#endif

#ifndef Pipeline

void PipelineAddShaders(Pipeline *pipeline, ArrayWithBucket<Shader*, 4> shaders) {
	pipeline->shaders.Append(shaders);
}

void PipelineAddVertexInputs(Pipeline *pipeline, const ArrayWithBucket<ShaderValueType, 8> &inputs) {
	pipeline->vertexInputs.Append(inputs);
}

void PipelineSetBlendMode(Pipeline *pipeline, BlendMode blendMode, i32 attachment) {
	pipeline->blendModes[attachment] = blendMode;
}

void PipelineSetTopology(Pipeline *pipeline, Topology topology) {
	pipeline->topology = topology;
}

void PipelineSetCullingMode(Pipeline *pipeline, CullingMode cullingMode) {
	pipeline->cullingMode = cullingMode;
}

void PipelineSetWinding(Pipeline *pipeline, Winding winding) {
	pipeline->winding = winding;
}

void PipelineSetDepthBias(Pipeline *pipeline, bool enable, f32 constant, f32 slope, f32 clampValue) {
	pipeline->depthBias.enable = enable;
	pipeline->depthBias.constant = constant;
	pipeline->depthBias.slope = slope;
	pipeline->depthBias.clampValue = clampValue;
}

void PipelineSetLineWidth(Pipeline *pipeline, f32 lineWidth) {
	pipeline->lineWidth = lineWidth;
}

void PipelineSetDepthTest(Pipeline *pipeline, bool enabled) {
	pipeline->depthTest = BoolOrDefaultFromBool(enabled);
}

void PipelineSetDepthWrite(Pipeline *pipeline, bool enabled) {
	pipeline->depthWrite = BoolOrDefaultFromBool(enabled);
}

void PipelineSetDepthCompareOp(Pipeline *pipeline, CompareOp compareOp) {
	pipeline->depthCompareOp = compareOp;
}

void PipelineSetMultisampleShading(Pipeline *pipeline, bool enabled, f32 minFraction) {
	pipeline->multisampleShading.enabled = enabled;
	pipeline->multisampleShading.minFraction = minFraction;
}

void PipelineAddPushConstantRange(Pipeline *pipeline, u32 offset, u32 size, u32 shaderStages) {
#ifndef NDEBUG
	for (VkPushConstantRange &range : pipeline->pushConstantRanges) {
		if (((u32)range.stageFlags & shaderStages) == 0) continue; // Allow overlapping ranges in different stages
		AzAssert(range.offset > offset+size || range.offset+range.size <= offset, Stringify("Found an overlapping Push Constant Range: [ ", offset, "...", offset+size, "] incoming, [", range.offset, "...", range.offset+range.size, "] existing"));
	}
#endif
	VkPushConstantRange range;
	range.offset = offset;
	range.size = size;
	range.stageFlags = (VkShaderStageFlags)shaderStages;
	pipeline->pushConstantRanges.Append(range);
}

bool VkPipelineLayoutCreateInfoMatches(VkPipelineLayoutCreateInfo a, VkPipelineLayoutCreateInfo b) {
	if (a.sType != b.sType) return false;
	if (a.flags != b.flags) return false;
	if (a.setLayoutCount != b.setLayoutCount) return false;
	// We can't compare these because pSetLayouts is a dangling pointer
	// for (i32 i = 0; i < (i32)a.setLayoutCount; i++) {
	// 	if (a.pSetLayouts[i] != b.pSetLayouts[i]) return false;
	// }
	if (a.pushConstantRangeCount != b.pushConstantRangeCount) return false;
	for (i32 i = 0; i < (i32)a.pushConstantRangeCount; i++) {
		if (a.pPushConstantRanges[i].offset != b.pPushConstantRanges[i].offset) return false;
		if (a.pPushConstantRanges[i].size != b.pPushConstantRanges[i].size) return false;
		if (a.pPushConstantRanges[i].stageFlags != b.pPushConstantRanges[i].stageFlags) return false;
	}
	return true;
}

Result<VoidResult_t, String> ShaderInit(Shader *shader) {
	INIT_HEAD(shader);
	Array<char> code = FileContents(shader->filename);
	if (code.size == 0) {
		return ERROR_RESULT(shader, "Failed to open shader source \"", shader->filename, "\"");
	}
	VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	createInfo.codeSize = code.size;
	createInfo.pCode = (u32*)code.data;
	if (VkResult result = vkCreateShaderModule(shader->device->vkDevice, &createInfo, nullptr, &shader->vkShaderModule); result != VK_SUCCESS) {
		return ERROR_RESULT(shader, "Failed to create shader module for \"", shader->filename, "\": ", VkResultString(result));
	}
	if (shader->tag.size == 0) {
		shader->tag = Stringify(ShaderStageString(shader->stage), " shader \"", shader->filename, "\"");
	}
	SetDebugMarker(shader->device, shader->tag, VK_OBJECT_TYPE_SHADER_MODULE, (u64)shader->vkShaderModule);
	shader->initted = true;
	return VoidResult_t();
}

void ShaderDeinit(Shader *shader) {
	DEINIT_HEAD(shader);
	vkDestroyShaderModule(shader->device->vkDevice, shader->vkShaderModule, nullptr);
	shader->initted = false;
}

Result<VoidResult_t, String> PipelineInit(Pipeline *pipeline) {
	// TODO: Maybe just delete this
	INIT_HEAD(pipeline);
	pipeline->initted = true;
	return VoidResult_t();
}

void PipelineDeinit(Pipeline *pipeline) {
	DEINIT_HEAD(pipeline);
	if (pipeline->vkPipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(pipeline->device->vkDevice, pipeline->vkPipelineLayout, nullptr);
		pipeline->vkPipelineLayout = VK_NULL_HANDLE;
	}
	if (pipeline->vkPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(pipeline->device->vkDevice, pipeline->vkPipeline, nullptr);
		pipeline->vkPipeline = VK_NULL_HANDLE;
	}
	pipeline->vkPipelineLayoutCreateInfo = {};
	pipeline->initted = false;
}

Result<VoidResult_t, String> PipelineCompose(Pipeline *pipeline, Context *context) {
	ContextFrame &frame = context->frames[context->currentFrame];
	VkPipelineLayoutCreateInfo layoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	Array<VkDescriptorSetLayout> vkDescriptorSetLayouts(frame.descriptorSetsBound.size);
	for (i32 i = 0; i < vkDescriptorSetLayouts.size; i++) {
		vkDescriptorSetLayouts[i] = frame.descriptorSetsBound[i].layout;
	}
	layoutCreateInfo.setLayoutCount = vkDescriptorSetLayouts.size;
	layoutCreateInfo.pSetLayouts = vkDescriptorSetLayouts.data;
	layoutCreateInfo.pPushConstantRanges = pipeline->pushConstantRanges.data;
	layoutCreateInfo.pushConstantRangeCount = pipeline->pushConstantRanges.size;
	
	bool create = false;
	
	if (context->bindings.framebuffer->sampleCount != pipeline->sampleCount) {
		pipeline->sampleCount = context->bindings.framebuffer->sampleCount;
		create = true;
	}
	bool framebufferHasDepthBuffer = FramebufferHasDepthBuffer(context->bindings.framebuffer);
	if (framebufferHasDepthBuffer != pipeline->framebufferHasDepthBuffer) {
		pipeline->framebufferHasDepthBuffer = framebufferHasDepthBuffer;
		create = true;
	}
	{
		i32 numColorAttachments = 0;
		for (AttachmentRef &attachmentRef : context->bindings.framebuffer->attachmentRefs) {
			if (attachmentRef.attachment.kind != Attachment::DEPTH_BUFFER) {
				numColorAttachments++;
				// We don't care about resolveAttachments because we don't draw into them
			}
		}
		if (numColorAttachments != pipeline->numColorAttachments) {
			pipeline->numColorAttachments = numColorAttachments;
			create = true;
		}
	}
	
	if (!VkPipelineLayoutCreateInfoMatches(layoutCreateInfo, pipeline->vkPipelineLayoutCreateInfo)) {
		pipeline->vkPipelineLayoutCreateInfo = layoutCreateInfo;
		create = true;
		if (pipeline->vkPipelineLayout != VK_NULL_HANDLE) {
			// TODO: Probably just cache it
			vkDestroyPipelineLayout(pipeline->device->vkDevice, pipeline->vkPipelineLayout, nullptr);
		}
		if (VkResult result = vkCreatePipelineLayout(pipeline->device->vkDevice, &layoutCreateInfo, nullptr, &pipeline->vkPipelineLayout); result != VK_SUCCESS) {
			return ERROR_RESULT(pipeline, "Failed to create pipeline layout: ", VkResultString(result));
		}
		SetDebugMarker(pipeline->device, Stringify(pipeline->tag, " pipeline layout"), VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipeline->vkPipelineLayout);
	}
	if (create) {
		Array<VkPipelineShaderStageCreateInfo> shaderStages(pipeline->shaders.size);
		io::cout.PrintLnDebug("Composing Pipeline with ", shaderStages.size, " shader", shaderStages.size != 1 ? "s:" : ":");
		io::cout.IndentMore();
		for (i32 i = 0; i < pipeline->shaders.size; i++) {
			Shader *shader = pipeline->shaders[i];
			AzAssert(shader->initted, "Expected Shader to be initted");
			VkPipelineShaderStageCreateInfo &createInfo = shaderStages[i];
			createInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
			switch (shader->stage) {
			case ShaderStage::COMPUTE:
				io::cout.PrintLnDebug("Compute shader \"", shader->filename, "\"");
				createInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
				break;
			case ShaderStage::VERTEX:
				io::cout.PrintLnDebug("Vertex shader \"", shader->filename, "\"");
				createInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case ShaderStage::FRAGMENT:
				io::cout.PrintLnDebug("Fragment shader \"", shader->filename, "\"");
				createInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			default: return ERROR_RESULT(pipeline, "Unimplemented");
			}
			createInfo.module = shader->vkShaderModule;
			// The Vulkan API pretends we can use something other than "main", but we really can't :(
			createInfo.pName = "main";
		}
		io::cout.IndentLess();
		if (pipeline->kind == Pipeline::GRAPHICS) {
			VkPipelineVertexInputStateCreateInfo vertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
			Array<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
			VkVertexInputBindingDescription vertexInputBindingDescription;
			{ // Vertex Inputs
				vertexInputBindingDescription.binding = 0;
				vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
				u32 offset = 0;
				i32 location = 0;
				for (i32 i = 0; i < pipeline->vertexInputs.size; i++) {
					ShaderValueType inputType = pipeline->vertexInputs[i];
					i32 numLocations = ShaderValueNumLocations[(u16)inputType];
					for (i32 j = 0; j < numLocations; j++) {
						VkVertexInputAttributeDescription attributeDescription;
						attributeDescription.binding = 0;
						attributeDescription.location = location++;
						i64 myStride, myAlignment;
						if (inputType == ShaderValueType::DVEC3 && j == 1) {
							// Handle our special case, as DVEC3 is the only input type that takes multiple locations with different strides/formats
							if (pipeline->device->vk12Features.scalarBlockLayout) {
								myStride = ShaderValueTypeStrideScalarBlockLayout[(u16)inputType]/2;
								myAlignment = ShaderValueTypeAlignmentScalarBlockLayout[(u16)inputType];
							} else {
								myStride = ShaderValueTypeStride[(u16)inputType]/2;
								myAlignment = myStride;
							}
							attributeDescription.format = VK_FORMAT_R64_SFLOAT;
						} else {
							if (pipeline->device->vk12Features.scalarBlockLayout) {
								myStride = ShaderValueTypeStrideScalarBlockLayout[(u16)inputType];
								myAlignment = ShaderValueTypeAlignmentScalarBlockLayout[(u16)inputType];
							} else {
								myStride = ShaderValueTypeStride[(u16)inputType];
								myAlignment = myStride;
							}
							attributeDescription.format = ShaderValueFormats[(u16)inputType];
						}
						attributeDescription.offset = align(offset, myAlignment);
						offset += myStride;
						vertexInputAttributeDescriptions.Append(attributeDescription);
					}
				}
				if (pipeline->vertexInputs.size == 0) {
					vertexInputBindingDescription.stride = 0;
				} else {
					// vertexInputBindingDescription.stride = align(offset, ShaderValueTypeStride[(u16)pipeline->vertexInputs[0]]);
					// Vertex buffers can be densely-packed I guess
					vertexInputBindingDescription.stride = offset;
				}
				// TODO: It could be nice to print out the final bindings, along with alignment, as working these things out for writing shaders and packing data in our vertex buffer might be annoying.
			}
			// TODO: Support multiple simultaneous bindings
			vertexInputState.vertexBindingDescriptionCount = 1;
			vertexInputState.pVertexBindingDescriptions = &vertexInputBindingDescription;
			vertexInputState.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size;
			vertexInputState.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data;
			
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
			// This is a 1-to-1 mapping
			inputAssemblyState.topology = (VkPrimitiveTopology)pipeline->topology;
			// TODO: We could use this
			inputAssemblyState.primitiveRestartEnable = VK_FALSE;
			
			VkPipelineViewportStateCreateInfo viewportState={VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
			VkViewport viewport;
			viewport.width = (f32)context->bindings.framebuffer->width;
			viewport.height = (f32)context->bindings.framebuffer->height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;
			VkRect2D scissor;
			scissor.offset = {0, 0};
			scissor.extent.width = context->bindings.framebuffer->width;
			scissor.extent.height = context->bindings.framebuffer->height;
			viewportState.scissorCount = 1;
			viewportState.pScissors = &scissor;
			
			VkPipelineRasterizationStateCreateInfo rasterizerState{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
			rasterizerState.depthClampEnable = VK_FALSE;
			rasterizerState.rasterizerDiscardEnable = VK_FALSE;
			rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizerState.cullMode = (VkCullModeFlagBits)pipeline->cullingMode;
			rasterizerState.frontFace = (VkFrontFace)pipeline->winding;
			rasterizerState.depthBiasEnable = pipeline->depthBias.enable;
			rasterizerState.depthBiasConstantFactor = pipeline->depthBias.constant;
			rasterizerState.depthBiasSlopeFactor = pipeline->depthBias.slope;
			rasterizerState.depthBiasClamp = pipeline->depthBias.clampValue;
			rasterizerState.lineWidth = pipeline->lineWidth;
			
			VkPipelineMultisampleStateCreateInfo multisampleState{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
			multisampleState.rasterizationSamples = (VkSampleCountFlagBits)pipeline->sampleCount;
			multisampleState.sampleShadingEnable = pipeline->multisampleShading.enabled;
			// Controls what fraction of samples get shaded with the above turned on. No effect otherwise.
			multisampleState.minSampleShading = pipeline->multisampleShading.minFraction;
			multisampleState.pSampleMask = nullptr;
			multisampleState.alphaToCoverageEnable = VK_FALSE;
			multisampleState.alphaToOneEnable = VK_FALSE;
			
			VkPipelineDepthStencilStateCreateInfo depthStencilState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
			// depthStencilState.flags; // VkPipelineDepthStencilStateCreateFlags
			
			if (pipeline->depthTest == BoolOrDefault::TRUE && !framebufferHasDepthBuffer) {
				return ERROR_RESULT(pipeline, "Depth test is enabled, but framebuffer doesn't have a depth buffer");
			}
			depthStencilState.depthTestEnable = ResolveBoolOrDefault(pipeline->depthTest, framebufferHasDepthBuffer);
			if (pipeline->depthWrite == BoolOrDefault::TRUE && !framebufferHasDepthBuffer) {
				return ERROR_RESULT(pipeline, "Depth write is enabled, but framebuffer doesn't have a depth buffer");
			}
			depthStencilState.depthWriteEnable = ResolveBoolOrDefault(pipeline->depthWrite, framebufferHasDepthBuffer);
			depthStencilState.depthCompareOp = (VkCompareOp)pipeline->depthCompareOp;
			depthStencilState.depthBoundsTestEnable = VK_FALSE;
			// TODO: Support stencil buffers
			depthStencilState.stencilTestEnable = VK_FALSE;
			// depthStencilState.front; // VkStencilOpState
			// depthStencilState.back; // VkStencilOpState
			depthStencilState.minDepthBounds = 0.0f;
			depthStencilState.maxDepthBounds = 1.0f;
			
			VkPipelineColorBlendStateCreateInfo colorBlendState{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
			colorBlendState.logicOpEnable = VK_FALSE;
			colorBlendState.logicOp = VK_LOGIC_OP_COPY;
			colorBlendState.blendConstants[0] = 1.0f;
			colorBlendState.blendConstants[1] = 1.0f;
			colorBlendState.blendConstants[2] = 1.0f;
			colorBlendState.blendConstants[3] = 1.0f;
			Array<VkPipelineColorBlendAttachmentState> blendModes;
			{ // Attachment blend modes
				for (i32 i = 0; i < context->bindings.framebuffer->attachmentRefs.size; i++) {
					Attachment &attachment = context->bindings.framebuffer->attachmentRefs[i].attachment;
					if (attachment.kind == Attachment::IMAGE || attachment.kind == Attachment::WINDOW) {
						VkPipelineColorBlendAttachmentState state;
						state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
						state.blendEnable = VK_TRUE;
						state.colorBlendOp = VK_BLEND_OP_ADD;
						state.alphaBlendOp = VK_BLEND_OP_ADD;
						BlendMode blendMode = pipeline->blendModes[blendModes.size];
						switch (blendMode.kind) {
						case BlendMode::OPAQUE:
							state.blendEnable = VK_FALSE;
							state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
							state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
							state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
							state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
							break;
						case BlendMode::TRANSPARENT:
							state.srcColorBlendFactor = blendMode.alphaPremult ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_SRC_ALPHA;
							state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
							state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
							state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
							break;
						case BlendMode::ADDITION:
							state.srcColorBlendFactor = blendMode.alphaPremult ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_SRC_ALPHA;
							state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
							state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
							state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
							break;
						// TODO: Is there a scenario where you'd want alpha to act like alpha when using MIN or MAX?
						case BlendMode::MIN:
							state.colorBlendOp = VK_BLEND_OP_MIN;
							state.alphaBlendOp = VK_BLEND_OP_MIN;
							state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
							state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
							state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
							state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
							break;
						case BlendMode::MAX:
							state.colorBlendOp = VK_BLEND_OP_MAX;
							state.alphaBlendOp = VK_BLEND_OP_MAX;
							state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
							state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
							state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
							state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
							break;
						default:
							AzAssert(false, "Unreachable");
						}
						blendModes.Append(state);
					}
				}
			}
			// TODO: Find the real upper limit
			if (blendModes.size > 8) {
				return ERROR_RESULT(pipeline, "Pipelines don't support more than 8 color attachments right now (had ", blendModes.size, ")");
			}
			colorBlendState.attachmentCount = blendModes.size;
			colorBlendState.pAttachments = blendModes.data;
			
			VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
			Array<VkDynamicState> dynamicStates = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR,
				// VK_DYNAMIC_STATE_LINE_WIDTH,
				// VK_DYNAMIC_STATE_DEPTH_BIAS,
				// VK_DYNAMIC_STATE_DEPTH_BOUNDS,
				// VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE,
				// VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
				// VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
				// VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
				// VK_DYNAMIC_STATE_CULL_MODE,
				// VK_DYNAMIC_STATE_FRONT_FACE,
				// VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY,
				// // Provided by VK_EXT_vertex_input_dynamic_state
				// VK_DYNAMIC_STATE_VERTEX_INPUT_EXT,
			};
			dynamicState.dynamicStateCount = dynamicStates.size;
			dynamicState.pDynamicStates = dynamicStates.data;
			
			if (pipeline->vkPipeline != VK_NULL_HANDLE) {
				// TODO: Probably cache
				vkDestroyPipeline(pipeline->device->vkDevice, pipeline->vkPipeline, nullptr);
			}
			VkGraphicsPipelineCreateInfo createInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
			createInfo.stageCount = shaderStages.size;
			createInfo.pStages = shaderStages.data;
			createInfo.pVertexInputState = &vertexInputState;
			createInfo.pInputAssemblyState = &inputAssemblyState;
			createInfo.pViewportState = &viewportState;
			createInfo.pRasterizationState = &rasterizerState;
			createInfo.pMultisampleState = &multisampleState;
			createInfo.pDepthStencilState = &depthStencilState;
			createInfo.pColorBlendState = &colorBlendState;
			createInfo.pDynamicState = &dynamicState;
			createInfo.layout = pipeline->vkPipelineLayout;
			if (context->bindings.framebuffer == nullptr) {
				return ERROR_RESULT(pipeline, "Cannot create a graphics Pipeline with no Framebuffer bound!");
			}
			createInfo.renderPass = context->bindings.framebuffer->vkRenderPass;
			createInfo.subpass = 0;
			createInfo.basePipelineHandle = VK_NULL_HANDLE;
			createInfo.basePipelineIndex = -1;
			
			if (VkResult result = vkCreateGraphicsPipelines(pipeline->device->vkDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline->vkPipeline); result != VK_SUCCESS) {
				return ERROR_RESULT(pipeline, "Failed to create graphics pipeline: ", VkResultString(result));
			}
			SetDebugMarker(pipeline->device, Stringify(pipeline->tag, " graphics pipeline"), VK_OBJECT_TYPE_PIPELINE, (u64)pipeline->vkPipeline);
		} else {
			return ERROR_RESULT(pipeline, "Compute pipelines are not implemented yet");
		}
	}
	return VoidResult_t();
}

#endif

#ifndef Context

Result<VoidResult_t, String> ContextInit(Context *context) {
	INIT_HEAD(context);
	VkCommandPoolCreateInfo poolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	poolCreateInfo.queueFamilyIndex = context->device->queueFamilyIndex;
	if (VkResult result = vkCreateCommandPool(context->device->vkDevice, &poolCreateInfo, nullptr, &context->vkCommandPool); result != VK_SUCCESS) {
		return ERROR_RESULT(context, "Failed to create command pool: ", VkResultString(result));
	}
	SetDebugMarker(context->device, Stringify(context->tag, " command pool"), VK_OBJECT_TYPE_COMMAND_POOL, (u64)context->vkCommandPool);
	context->frames.Resize(context->numFrames);
	for (i32 i = 0; i < context->numFrames; i++) {
		ContextFrame &frame = context->frames[i];
		frame.vkCommandBuffer = VK_NULL_HANDLE;
		frame.fence.device = context->device;
		frame.fence.tag = Stringify("Context Fence ", i);
		// We'll use signaled to mean not executing
		if (auto result = FenceInit(&frame.fence, true); result.isError) {
			return ERROR_RESULT(context, result.error);
		}
	}
	context->initted = true;
	return VoidResult_t();
}

void ContextDeinit(Context *context) {
	DEINIT_HEAD(context);
	vkDestroyCommandPool(context->device->vkDevice, context->vkCommandPool, nullptr);
	for (i32 i = 0; i < context->numFrames; i++) {
		ContextFrame &frame = context->frames[i];
		FenceDeinit(&frame.fence);
		for (Semaphore &semaphore : frame.semaphores) {
			SemaphoreDeinit(&semaphore);
		}
	}
	context->initted = false;
}


static Result<VoidResult_t, String> ContextEnsureSemaphoreCount(Context *context, i32 count, i32 frameIndex) {
	ContextFrame &frame = context->frames[frameIndex];
	if (frame.semaphores.size < count) {
		i32 prevSize = frame.semaphores.size;
		frame.semaphores.Resize(count);
		for (i32 i = prevSize; i < count; i++) {
			frame.semaphores[i].device = context->device;
			frame.semaphores[i].tag = Stringify(context->tag, " Frame ", frameIndex, " Semaphore ", i);
			if (auto result = SemaphoreInit(&frame.semaphores[i]); result.isError) {
				return ERROR_RESULT(context, "Couldn't ensure we had enough semaphores: ", result.error);
			}
		}
	}
	return VoidResult_t();
}

Semaphore* ContextGetCurrentSemaphore(Context *context, i32 index) {
	ContextFrame &frame = context->frames[context->currentFrame];
	ContextEnsureSemaphoreCount(context, index+1, context->currentFrame).AzUnwrap();
	return &frame.semaphores[index];
}

Semaphore* ContextGetPreviousSemaphore(Context *context, i32 index) {
	i32 lastFrame = context->currentFrame - 1;
	if (lastFrame < 0) lastFrame = context->numFrames-1;
	ContextFrame &frame = context->frames[lastFrame];
	ContextEnsureSemaphoreCount(context, index+1, lastFrame).AzUnwrap();
	return &frame.semaphores[index];
}

Result<VkDescriptorSetLayout, String> DeviceGetDescriptorSetLayout(Device *device, DescriptorSetLayout &layout) {
	VkDescriptorSetLayout &dst = device->vkDescriptorSetLayouts.ValueOf(layout, VK_NULL_HANDLE);
	if (dst == VK_NULL_HANDLE) {
		// Make the layout
		layout.createInfo.bindingCount = layout.bindings.size;
		layout.createInfo.pBindings = layout.bindings.data;
		if (VkResult result = vkCreateDescriptorSetLayout(device->vkDevice, &layout.createInfo, nullptr, &dst); result != VK_SUCCESS) {
			return ERROR_RESULT(device, "Failed to create descriptor set layout: ", VkResultString(result));
		}
	}
	return dst;
}

Result<DescriptorSet*, String> DeviceGetDescriptorSet(Device *device, VkDescriptorSetLayout vkDescriptorSetLayout, const DescriptorBindings &bindings, bool &dstDoWrite) {
	DescriptorSet* &dst = device->descriptorSetsMap.ValueOf(bindings, nullptr);
	if (dst == nullptr) {
		dstDoWrite = true;
		dst = device->descriptorSets.Append(new DescriptorSet()).RawPtr();
		// Make the descriptor pool
		u32 numUniformBuffers = 0;
		u32 numStorageBuffers = 0;
		u32 numImages = 0;
		for (const DescriptorBinding& binding : bindings.bindings) {
			switch (binding.kind) {
				case DescriptorBinding::UNIFORM_BUFFER:
					numUniformBuffers += binding.objects.size;
					for (void* obj : binding.objects) {
						dst->descriptorTimestamps.Append(&((Buffer*)obj)->timestamp);
					}
					break;
				case DescriptorBinding::STORAGE_BUFFER:
					numStorageBuffers += binding.objects.size;
					for (void* obj : binding.objects) {
						dst->descriptorTimestamps.Append(&((Buffer*)obj)->timestamp);
					}
					break;
				case DescriptorBinding::IMAGE_SAMPLER:
					numImages += binding.objects.size;
					for (void* obj : binding.objects) {
						dst->descriptorTimestamps.Append(&((Image*)obj)->timestamp);
					}
					break;
				default:
					AzAssert(false, "Unreachable");
					break;
			}
		}
		VkDescriptorPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
		createInfo.maxSets = 1;
		StaticArray<VkDescriptorPoolSize, 3> poolSizes;
		if (numUniformBuffers > 0) {
			VkDescriptorPoolSize poolSize;
			poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSize.descriptorCount = numUniformBuffers;
			poolSizes.Append(poolSize);
		}
		if (numStorageBuffers > 0) {
			VkDescriptorPoolSize poolSize;
			poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			poolSize.descriptorCount = numStorageBuffers;
			poolSizes.Append(poolSize);
		}
		if (numImages > 0) {
			VkDescriptorPoolSize poolSize;
			poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSize.descriptorCount = numImages;
			poolSizes.Append(poolSize);
		}
		createInfo.poolSizeCount = poolSizes.size;
		createInfo.pPoolSizes = poolSizes.data;
		if (VkResult result = vkCreateDescriptorPool(device->vkDevice, &createInfo, nullptr, &dst->vkDescriptorPool); result != VK_SUCCESS) {
			return ERROR_RESULT(device, "Failed to create descriptor pool: ", VkResultString(result));
		}
		// Make the descriptor set
		VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		allocInfo.descriptorPool = dst->vkDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &vkDescriptorSetLayout;
		if (VkResult result = vkAllocateDescriptorSets(device->vkDevice, &allocInfo, &dst->vkDescriptorSet); result != VK_SUCCESS) {
			return ERROR_RESULT(device, "Failed to allocate descriptor set: ", VkResultString(result));
		}
		dst->timestamp = GetTimestamp();
	}
	return dst;
}

Result<VoidResult_t, String> ContextDescriptorsCompose(Context *context) {
	ContextFrame &frame = context->frames[context->currentFrame];
	u32 numUniformBuffers = 0;
	u32 numStorageBuffers = 0;
	u32 numImages = 0;
	for (auto &node : context->bindings.descriptors) {
		Binding &binding = node.value;
		switch (binding.kind) {
			case Binding::UNIFORM_BUFFER:
				numUniformBuffers++;
				break;
			case Binding::STORAGE_BUFFER:
				numStorageBuffers++;
				break;
			case Binding::IMAGE_SAMPLER:
				numImages += binding.imageSampler.images.size;
				break;
			default: break;
		}
	}
	// These are for finding or creating them
	Array<DescriptorSetLayout> descriptorSetLayouts;
	Array<DescriptorBindings> descriptorBindings;
	
	Array<Array<VkWriteDescriptorSet>> vkWriteDescriptorSets;
	Array<VkDescriptorBufferInfo> descriptorBufferInfos;
	descriptorBufferInfos.Reserve(numUniformBuffers + numStorageBuffers);
	Array<VkDescriptorImageInfo> descriptorImageInfos;
	descriptorImageInfos.Reserve(numImages);
	for (auto &node : context->bindings.descriptors) {
		Binding &binding = node.value;
		i32 set = binding.anyDescriptor.binding.set;
		// NOTE: These are necessarily sorted by set first, then binding. This code will break if that is no longer the case.
		if (set+1 > descriptorSetLayouts.size) {
			descriptorSetLayouts.Resize(set+1);
			descriptorBindings.Resize(set+1);
			vkWriteDescriptorSets.Resize(set+1);
		}
		VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		VkDescriptorSetLayoutBinding bindingInfo{};
		bindingInfo.binding = binding.anyDescriptor.binding.binding;
		bindingInfo.descriptorCount = 1;
		switch (binding.kind) {
		case Binding::UNIFORM_BUFFER:
			// TODO: Support all kinds of arrays
			bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorBindings.Back().bindings.Append(DescriptorBinding({binding.uniformBuffer.object}));
			break;
		case Binding::STORAGE_BUFFER:
			// TODO: Support all kinds of arrays
			bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorBindings.Back().bindings.Append(DescriptorBinding({binding.storageBuffer.object}));
			break;
		case Binding::IMAGE_SAMPLER:
			bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindingInfo.descriptorCount = binding.imageSampler.images.size;
			descriptorBindings.Back().bindings.Append(DescriptorBinding(binding.imageSampler.images, binding.imageSampler.sampler));
			break;
		default:
			return ERROR_RESULT(context, "Invalid descriptor binding (kind is ", (u32)binding.kind, ")");
		}
		switch (binding.kind) {
			case Binding::UNIFORM_BUFFER:
			case Binding::STORAGE_BUFFER: {
				bindingInfo.stageFlags = (VkShaderStageFlags)binding.anyBuffer.object->shaderStages;
				VkDescriptorBufferInfo bufferInfo;
				bufferInfo.buffer = binding.anyBuffer.object->vkBuffer;
				bufferInfo.offset = 0;
				bufferInfo.range = binding.anyBuffer.object->size;
				write.pBufferInfo = &descriptorBufferInfos.Append(bufferInfo);
			} break;
			case Binding::IMAGE_SAMPLER: {
				bindingInfo.stageFlags = 0;
				VkDescriptorImageInfo imageInfo;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.sampler = binding.imageSampler.sampler->vkSampler;
				write.pImageInfo = descriptorImageInfos.data + descriptorImageInfos.size;
				for (Image *image : binding.imageSampler.images) {
					imageInfo.imageView = image->vkImageView;
					bindingInfo.stageFlags |= (VkShaderStageFlags)image->shaderStages;
					descriptorImageInfos.Append(imageInfo);
				}
			} break;
			default: break;
		}
		write.dstBinding = bindingInfo.binding;
		write.descriptorCount = bindingInfo.descriptorCount;
		write.descriptorType = bindingInfo.descriptorType;
		write.dstArrayElement = 0;
		vkWriteDescriptorSets[binding.anyDescriptor.binding.set].Append(write);
		descriptorSetLayouts.Back().bindings.Append(bindingInfo);
	}
	frame.descriptorSetsBound.ClearSoft();
	for (i32 i = 0; i < descriptorSetLayouts.size; i++) {
		BoundDescriptorSet boundDescriptorSet;
		if (auto result = DeviceGetDescriptorSetLayout(context->device, descriptorSetLayouts[i]); result.isError) {
			return result.error;
		} else {
			boundDescriptorSet.layout = result.value;
		}
		DescriptorSet *set;
		bool doWrite = false;
		if (auto result = DeviceGetDescriptorSet(context->device, boundDescriptorSet.layout, descriptorBindings[i], doWrite); result.isError) {
			return result.error;
		} else {
			set = result.value;
			boundDescriptorSet.set = set->vkDescriptorSet;
		}
		if (!doWrite) {
			for (u64 *timestamp : set->descriptorTimestamps) {
				if (*timestamp >= set->timestamp) {
					doWrite = true;
					break;
				}
			}
		}
		if (doWrite) {
			for (VkWriteDescriptorSet &write : vkWriteDescriptorSets[i]) {
				write.dstSet = boundDescriptorSet.set;
			}
			ContextFrame &lastFrame = context->frames[(context->currentFrame+context->numFrames-1)%context->numFrames];
			FenceWaitForSignal(&lastFrame.fence).AzUnwrap();
			// NOTE: Descriptor changes should only happen between frames and never within the same command buffer, so this should be okay.
			vkUpdateDescriptorSets(context->device->vkDevice, vkWriteDescriptorSets[i].size, vkWriteDescriptorSets[i].data, 0, nullptr);
		}
		frame.descriptorSetsBound.Append(boundDescriptorSet);
	}
	return VoidResult_t();
}

void ContextResetBindings(Context *context) {
	context->bindings.framebuffer = nullptr;
	context->bindings.pipeline = nullptr;
	context->bindings.vertexBuffer = nullptr;
	context->bindings.indexBuffer = nullptr;
	context->bindings.descriptors.Clear();
	context->bindCommands.ClearSoft();
}

Result<VoidResult_t, String> ContextBeginRecording(Context *context) {
	context->currentFrame = (context->currentFrame + 1) % context->numFrames;
	ContextFrame &frame = context->frames[context->currentFrame];
	AzAssert(context->initted, "Trying to record to a Context that's not initted");
	if ((u32)context->state >= (u32)Context::State::RECORDING_PRIMARY) {
		return ERROR_RESULT(context, "Cannot begin recording on a command buffer that's already recording");
	}
	ContextResetBindings(context);
	// TODO: This prevents us from building the next command buffers while the previous ones are running. Switch to double-buffered Contexts
	if (auto result = FenceWaitForSignal(&frame.fence); result.isError) {
		return ERROR_RESULT(context, result.error);
	}
	if (auto result = FenceResetSignaled(&frame.fence); result.isError) {
		return ERROR_RESULT(context, result.error);
	}

	if (context->state == Context::State::DONE_RECORDING) {
		vkFreeCommandBuffers(context->device->vkDevice, context->vkCommandPool, 1, &frame.vkCommandBuffer);
	}

	VkCommandBufferAllocateInfo bufferAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	bufferAllocInfo.commandPool = context->vkCommandPool;
	bufferAllocInfo.commandBufferCount = 1;
	bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	if (VkResult result = vkAllocateCommandBuffers(context->device->vkDevice, &bufferAllocInfo, &frame.vkCommandBuffer); result != VK_SUCCESS) {
		return ERROR_RESULT(context, "Failed to allocate primary command buffer: ", VkResultString(result));
	}
	VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	if (VkResult result = vkBeginCommandBuffer(frame.vkCommandBuffer, &beginInfo); result != VK_SUCCESS) {
		return ERROR_RESULT(context, "Failed to begin primary command buffer: ", VkResultString(result));
	}
	context->state = Context::State::RECORDING_PRIMARY;
	return VoidResult_t();
}

Result<VoidResult_t, String> ContextBeginRecordingSecondary(Context *context, Framebuffer *framebuffer, i32 subpass) {
	AzAssert(context->initted, "Trying to record to a Context that's not initted");
	if ((u32)context->state >= (u32)Context::State::RECORDING_PRIMARY) {
		return ERROR_RESULT(context, "Cannot begin recording on a command buffer that's already recording");
	}
	context->currentFrame = (context->currentFrame + 1) % context->numFrames;
	ContextFrame &frame = context->frames[context->currentFrame];
	ContextResetBindings(context);

	if (context->state == Context::State::DONE_RECORDING) {
		vkFreeCommandBuffers(context->device->vkDevice, context->vkCommandPool, 1, &frame.vkCommandBuffer);
	}

	VkCommandBufferAllocateInfo bufferAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	bufferAllocInfo.commandPool = context->vkCommandPool;
	bufferAllocInfo.commandBufferCount = 1;
	bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	if (VkResult result = vkAllocateCommandBuffers(context->device->vkDevice, &bufferAllocInfo, &frame.vkCommandBuffer); result != VK_SUCCESS) {
		return ERROR_RESULT(context, "Failed to allocate secondary command buffer: ", VkResultString(result));
	}
	VkCommandBufferInheritanceInfo inheritanceInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
	if (framebuffer != nullptr) {
		AzAssert(framebuffer->initted, "Trying to use a Framebuffer that isn't initted for recording a secondary command buffer");
		inheritanceInfo.renderPass = framebuffer->vkRenderPass;
		inheritanceInfo.subpass = subpass;
		inheritanceInfo.framebuffer = FramebufferGetCurrentVkFramebuffer(framebuffer);
	}
	VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	beginInfo.flags = framebuffer != nullptr ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0;
	beginInfo.pInheritanceInfo = &inheritanceInfo;
	if (VkResult result = vkBeginCommandBuffer(frame.vkCommandBuffer, &beginInfo); result != VK_SUCCESS) {
		return ERROR_RESULT(context, "Failed to begin secondary command buffer: ", VkResultString(result));
	}
	context->state = Context::State::RECORDING_SECONDARY;
	return VoidResult_t();
}

Result<VoidResult_t, String> ContextEndRecording(Context *context) {
	ContextFrame &frame = context->frames[context->currentFrame];
	AzAssert(context->initted, "Context not initted");
	if (!ContextIsRecording(context)) {
		return ERROR_RESULT(context, "Trying to End Recording but we haven't started recording.");
	}
	if (context->bindings.framebuffer) {
		CmdFinishFramebuffer(context);
	}
	if (VkResult result = vkEndCommandBuffer(frame.vkCommandBuffer); result != VK_SUCCESS) {
		return ERROR_RESULT(context, "Failed to End Recording: ", VkResultString(result));
	}
	context->state = Context::State::DONE_RECORDING;
	return VoidResult_t();
}

Result<VoidResult_t, String> SubmitCommands(Context *context, i32 numSemaphores, ArrayWithBucket<Semaphore*, 4> waitSemaphores) {
	ContextFrame &frame = context->frames[context->currentFrame];
	if (context->state != Context::State::DONE_RECORDING) {
		return ERROR_RESULT(context, "Trying to SubmitCommands without anything recorded.");
	}
	ArrayWithBucket<VkSemaphore, 4> waitVkSemaphores(waitSemaphores.size);
	ArrayWithBucket<VkPipelineStageFlags, 4> waitStages(waitSemaphores.size);
	for (i32 i = 0; i < waitSemaphores.size; i++) {
		waitVkSemaphores[i] = waitSemaphores[i]->vkSemaphore;
		// TODO: This is a safe assumption, but we could probably be more specific.
		waitStages[i] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
	if (context->bindings.framebuffer) {
		Window *window = FramebufferGetWindowAttachment(context->bindings.framebuffer);
		if (window) {
			waitVkSemaphores.Append(window->acquireSemaphores[window->currentSync].vkSemaphore);
			waitStages.Append(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		}
	}
	ArrayWithBucket<VkSemaphore, 4> signalVkSemaphores(numSemaphores);
	if (numSemaphores) {
		if (auto result = ContextEnsureSemaphoreCount(context, numSemaphores, context->currentFrame); result.isError) {
			return ERROR_RESULT(context, "Couldn't ensure we had ", numSemaphores, " semaphores");
		}
		for (i32 i = 0; i < numSemaphores; i++) {
			signalVkSemaphores[i] = frame.semaphores[i].vkSemaphore;
		}
	}
	VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frame.vkCommandBuffer;
	submitInfo.signalSemaphoreCount = signalVkSemaphores.size;
	submitInfo.pSignalSemaphores = signalVkSemaphores.data;
	submitInfo.waitSemaphoreCount = waitVkSemaphores.size;
	submitInfo.pWaitSemaphores = waitVkSemaphores.data;
	submitInfo.pWaitDstStageMask = waitStages.data;
	if (VkResult result = vkQueueSubmit(context->device->vkQueue, 1, &submitInfo, frame.fence.vkFence); result != VK_SUCCESS) {
		return ERROR_RESULT(context, "Failed to submit to queue: ", VkResultString(result));
	}
	return VoidResult_t();
}

Result<bool, String> ContextIsExecuting(Context *context) {
	ContextFrame &frame = context->frames[context->currentFrame];
	AzAssert(context->initted, "Context is not initted");
	VkResult result = FenceGetStatus(&frame.fence);
	switch (result) {
		case VK_SUCCESS:
			return false;
		case VK_NOT_READY:
			return true;
		case VK_ERROR_DEVICE_LOST:
			return Stringify("Device \"", context->device->tag, "\" error: Device Lost");
		default:
			return ERROR_RESULT(context, "IsExecuting returned ", VkResultString(result));
	}
}

Result<bool, String> ContextWaitUntilFinished(Context *context, Nanoseconds timeout) {
	ContextFrame &frame = context->frames[context->currentFrame];
	AzAssert(context->initted, "Context is not initted");
	AzAssert(timeout.count() >= 0, "Cannot have negative timeout");
	bool wasTimeout;
	if (auto result = FenceWaitForSignal(&frame.fence, (u64)timeout.count(), &wasTimeout); result.isError) {
		return ERROR_RESULT(context, result.error);
	}
	return wasTimeout;
}

#endif

#ifndef Commands

Result<VoidResult_t, String> CmdExecuteSecondary(Context *primary, Context *secondary) {
	return String("Unimplemented");
}

Result<VoidResult_t, String> CmdCopyDataToBuffer(Context *context, Buffer *dst, void *src, i64 dstOffset, i64 size) {
	ContextFrame &frame = context->frames[context->currentFrame];
	AzAssert(size+dstOffset <= (i64)dst->memoryRequirements.size, "size is bigger than our buffer");
	AzAssert(ContextIsRecording(context), "Trying to record into a context that's not recording");
	if (size == 0) {
		// We do the whole size
		size = dst->size - dstOffset;
	}
	if (!dst->hostVisible) {
		if (auto result = BufferHostInit(dst); result.isError) {
			return result.error;
		}
	} else {
		// Ensure the last frame completed the copy before we rewrite the staging buffer.
		i32 prevFrame = context->currentFrame - 1;
		if (prevFrame < 0) prevFrame = context->numFrames - 1;
		ContextFrame &framePrevious = context->frames[prevFrame];
		FenceWaitForSignal(&framePrevious.fence).AzUnwrap();
	}
	Allocation alloc = dst->allocHostVisible;
	VkDeviceMemory vkMemory = alloc.memory->pages[alloc.page].vkMemory;
	void *dstMapped;
	if (VkResult result = vkMapMemory(dst->device->vkDevice, vkMemory, alloc.offset + dstOffset, size, 0, &dstMapped); result != VK_SUCCESS) {
		return Stringify("Buffer \"", dst->tag, "\" error: Failed to map memory: ", VkResultString(result));
	}
	memcpy(dstMapped, src, size);
	vkUnmapMemory(dst->device->vkDevice, vkMemory);
	VkBufferCopy vkCopy;
	vkCopy.dstOffset = dstOffset;
	vkCopy.srcOffset = dstOffset;
	vkCopy.size = size;
	vkCmdCopyBuffer(frame.vkCommandBuffer, dst->vkBufferHostVisible, dst->vkBuffer, 1, &vkCopy);
	return VoidResult_t();
}

struct AccessAndStage {
	VkAccessFlags accessFlags;
	VkPipelineStageFlags stageFlags;
};

static AccessAndStage AccessAndStageFromImageLayout(VkImageLayout layout) {
	AccessAndStage result;
	result.accessFlags = VK_ACCESS_HOST_WRITE_BIT;
	result.stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	switch (layout) {
	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		result.accessFlags = VK_ACCESS_HOST_WRITE_BIT;
		result.stageFlags = VK_PIPELINE_STAGE_HOST_BIT;
		break;
	case VK_IMAGE_LAYOUT_UNDEFINED:
		result.accessFlags = 0;
		result.stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		result.accessFlags = VK_ACCESS_TRANSFER_WRITE_BIT;
		result.stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		result.accessFlags = VK_ACCESS_TRANSFER_READ_BIT;
		result.stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		result.accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		result.stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
	case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
	case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
		// NOTE: Not sure exactly how to handle the last two cases???
		result.accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		result.stageFlags = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		result.accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		result.stageFlags = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		result.accessFlags = VK_ACCESS_SHADER_READ_BIT;
		result.stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	default:
		io::cerr.PrintLn("AccessAndStageFromImageLayout layout not supported (plsfix)");
		exit(1);
	}
	return result;
}

static void CmdImageTransitionLayout(Context *context, Image *image, VkImageLayout from, VkImageLayout to, VkImageSubresourceRange subresourceRange) {
	ContextFrame &frame = context->frames[context->currentFrame];
	AccessAndStage srcAccessAndStage = AccessAndStageFromImageLayout(from);
	AccessAndStage dstAccessAndStage = AccessAndStageFromImageLayout(to);
	VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	barrier.srcAccessMask = srcAccessAndStage.accessFlags;
	barrier.dstAccessMask = dstAccessAndStage.accessFlags;
	barrier.oldLayout = from;
	barrier.newLayout = to;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image->vkImage;
	barrier.subresourceRange = subresourceRange;
	vkCmdPipelineBarrier(frame.vkCommandBuffer, srcAccessAndStage.stageFlags, dstAccessAndStage.stageFlags, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

static void CmdImageTransitionLayout(Context *context, Image *image, VkImageLayout from, VkImageLayout to, u32 baseMipLevel=0, u32 mipLevelCount=1) {
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = image->vkImageAspect;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;
	subresourceRange.baseMipLevel = baseMipLevel;
	subresourceRange.levelCount = mipLevelCount;

	CmdImageTransitionLayout(context, image, from, to, subresourceRange);
}

VkImageLayout GetVkImageLayout(Image *image, ImageLayout layout) {
	switch (layout) {
		case ImageLayout::UNDEFINED:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case ImageLayout::TRANSFER_DST:
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ImageLayout::TRANSFER_SRC:
			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case ImageLayout::ATTACHMENT:
			if (FormatIsDepth(image->vkFormat)) {
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			} else {
				return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
		case ImageLayout::SHADER_READ:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	AzAssert(false, "Unreachable");
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

void CmdImageTransitionLayout(Context *context, Image *image, ImageLayout from, ImageLayout to, i32 baseMipLevel, i32 mipLevelCount) {
	if (mipLevelCount == -1) {
		mipLevelCount = (i32)image->mipLevels;
	}
	CmdImageTransitionLayout(context, image, GetVkImageLayout(image, from), GetVkImageLayout(image, to), baseMipLevel, mipLevelCount);
}

static void CmdImageGenerateMipmaps(Context *context, Image *image, VkImageLayout startingLayout, VkImageLayout finalLayout, VkFilter filter=VK_FILTER_LINEAR) {
	ContextFrame &frame = context->frames[context->currentFrame];
	AzAssert(image->mipLevels > 1, "Calling CmdImageGenerateMipmaps on an image without mipmaps");
	if (startingLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		CmdImageTransitionLayout(context, image, startingLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	}
	for (u32 mip = 1; mip < image->mipLevels; mip++) {
		VkImageBlit imageBlit = {};
		imageBlit.srcSubresource.aspectMask = image->vkImageAspect;
		imageBlit.srcSubresource.layerCount = 1;
		imageBlit.srcSubresource.mipLevel = mip-1;
		imageBlit.srcOffsets[1].x = (i32)max(image->width >> (mip-1), 1);
		imageBlit.srcOffsets[1].y = (i32)max(image->height >> (mip-1), 1);
		imageBlit.srcOffsets[1].z = 1;

		imageBlit.dstSubresource.aspectMask = image->vkImageAspect;
		imageBlit.dstSubresource.layerCount = 1;
		imageBlit.dstSubresource.mipLevel = mip;
		imageBlit.dstOffsets[1].x = (i32)max(image->width >> mip, 1);
		imageBlit.dstOffsets[1].y = (i32)max(image->height >> mip, 1);
		imageBlit.dstOffsets[1].z = 1;

		CmdImageTransitionLayout(context, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip);
		vkCmdBlitImage(frame.vkCommandBuffer, image->vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, filter);
		CmdImageTransitionLayout(context, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mip);
	}

	CmdImageTransitionLayout(context, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, finalLayout, 0, image->mipLevels);
}

void CmdImageGenerateMipmaps(Context *context, Image *image, ImageLayout from, ImageLayout to) {
	CmdImageGenerateMipmaps(context, image, GetVkImageLayout(image, from), GetVkImageLayout(image, to));
}

Result<VoidResult_t, String> CmdCopyDataToImage(Context *context, Image *dst, void *src) {
	ContextFrame &frame = context->frames[context->currentFrame];
	AzAssert(ContextIsRecording(context), "Trying to record into a context that's not recording");
	if (!dst->hostVisible) {
		if (auto result = ImageHostInit(dst); result.isError) {
			return result.error;
		}
	} else {
		// Ensure the last frame completed the copy before we rewrite the staging buffer.
		i32 prevFrame = context->currentFrame - 1;
		if (prevFrame < 0) prevFrame = context->numFrames - 1;
		ContextFrame &framePrevious = context->frames[prevFrame];
		FenceWaitForSignal(&framePrevious.fence).AzUnwrap();
	}
	CmdImageTransitionLayout(context, dst, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	Allocation alloc = dst->allocHostVisible;
	VkDeviceMemory vkMemory = alloc.memory->pages[alloc.page].vkMemory;
	void *dstMapped;
	if (VkResult result = vkMapMemory(dst->device->vkDevice, vkMemory, alloc.offset, dst->memoryRequirementsHost.size, 0, &dstMapped); result != VK_SUCCESS) {
		return Stringify("Image \"", dst->tag, "\" error: Failed to map memory: ", VkResultString(result));
	}
	memcpy(dstMapped, src, dst->width * dst->height * dst->bytesPerPixel);
	vkUnmapMemory(dst->device->vkDevice, vkMemory);
	VkBufferImageCopy vkCopy = {};
	vkCopy.imageSubresource.aspectMask = dst->vkImageAspect;
	vkCopy.imageSubresource.mipLevel = 0;
	vkCopy.imageSubresource.baseArrayLayer = 0;
	vkCopy.imageSubresource.layerCount = 1;
	vkCopy.imageOffset = {0, 0, 0};
	vkCopy.imageExtent = {(u32)dst->width, (u32)dst->height, 1};
	vkCmdCopyBufferToImage(frame.vkCommandBuffer, dst->vkBufferHostVisible, dst->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vkCopy);
	VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (dst->shaderStages) {
		finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	} else if (dst->attachment) {
		finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	}
	if (dst->mipmapped && dst->mipLevels > 1) {
		CmdImageGenerateMipmaps(context, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout);
	} else {
		CmdImageTransitionLayout(context, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout);
	}
	return VoidResult_t();
}

void CmdBindFramebuffer(Context *context, Framebuffer *framebuffer) {
	Binding bind;
	bind.kind = Binding::FRAMEBUFFER;
	bind.framebuffer.object = framebuffer;
	context->bindCommands.Append(bind);
}

void CmdBindPipeline(Context *context, Pipeline *pipeline) {
	Binding bind;
	bind.kind = Binding::PIPELINE;
	bind.pipeline.object = pipeline;
	context->bindCommands.Append(bind);
}

void CmdBindVertexBuffer(Context *context, Buffer *buffer) {
	AzAssert(buffer->kind == Buffer::VERTEX_BUFFER, "Binding a buffer as a vertex buffer when it's not one");
	Binding bind;
	bind.kind = Binding::VERTEX_BUFFER;
	bind.vertexBuffer.object = buffer;
	context->bindCommands.Append(bind);
}

void CmdBindIndexBuffer(Context *context, Buffer *buffer) {
	AzAssert(buffer->kind == Buffer::INDEX_BUFFER, "Binding a buffer as an index buffer when it's not one");
	Binding bind;
	bind.kind = Binding::INDEX_BUFFER;
	bind.indexBuffer.object = buffer;
	context->bindCommands.Append(bind);
}

void CmdBindUniformBuffer(Context *context, Buffer *buffer, i32 set, i32 binding) {
	AzAssert(buffer->kind == Buffer::UNIFORM_BUFFER, "Binding a buffer as a uniform buffer when it's not one");
	Binding bind;
	bind.kind = Binding::UNIFORM_BUFFER;
	bind.uniformBuffer.object = buffer;
	bind.uniformBuffer.binding.set = set;
	bind.uniformBuffer.binding.binding = binding;
	context->bindCommands.Append(bind);
}

void CmdBindStorageBuffer(Context *context, Buffer *buffer, i32 set, i32 binding) {
	AzAssert(buffer->kind == Buffer::STORAGE_BUFFER, "Binding a buffer as a storage buffer when it's not one");
	Binding bind;
	bind.kind = Binding::STORAGE_BUFFER;
	bind.storageBuffer.object = buffer;
	bind.storageBuffer.binding.set = set;
	bind.storageBuffer.binding.binding = binding;
	context->bindCommands.Append(bind);
}

void CmdBindImageSampler(Context *context, Image *image, Sampler *sampler, i32 set, i32 binding) {
	Binding bind;
	bind.kind = Binding::IMAGE_SAMPLER;
	bind.imageSampler.images = {image};
	bind.imageSampler.sampler = sampler;
	bind.imageSampler.binding.set = set;
	bind.imageSampler.binding.binding = binding;
	context->bindCommands.Append(bind);
}

void CmdBindImageArraySampler(Context *context, const Array<Image*> &images, Sampler *sampler, i32 set, i32 binding) {
	Binding bind;
	bind.kind = Binding::IMAGE_SAMPLER;
	bind.imageSampler.images = images;
	bind.imageSampler.sampler = sampler;
	bind.imageSampler.binding.set = set;
	bind.imageSampler.binding.binding = binding;
	context->bindCommands.Append(bind);
}

Result<VoidResult_t, String> CmdCommitBindings(Context *context) {
	ContextFrame &frame = context->frames[context->currentFrame];
	Framebuffer *framebuffer = nullptr;
	Pipeline *pipeline = nullptr;
	Buffer *vertexBuffer = nullptr;
	Buffer *indexBuffer = nullptr;
	bool descriptorsChanged = false;
	for (Binding &bind : context->bindCommands) {
		switch (bind.kind) {
		case Binding::FRAMEBUFFER:
			framebuffer = bind.framebuffer.object;
			break;
		case Binding::PIPELINE:
			pipeline = bind.pipeline.object;
			break;
		case Binding::VERTEX_BUFFER:
			vertexBuffer = bind.vertexBuffer.object;
			break;
		case Binding::INDEX_BUFFER:
			indexBuffer = bind.indexBuffer.object;
			break;
		case Binding::UNIFORM_BUFFER:
			context->bindings.descriptors.Emplace(bind.uniformBuffer.binding, bind);
			descriptorsChanged = true;
			break;
		case Binding::STORAGE_BUFFER:
			context->bindings.descriptors.Emplace(bind.storageBuffer.binding, bind);
			descriptorsChanged = true;
			break;
		case Binding::IMAGE_SAMPLER:
			context->bindings.descriptors.Emplace(bind.imageSampler.binding, bind);
			descriptorsChanged = true;
			break;
		}
	}
	if (nullptr != framebuffer && context->bindings.framebuffer != framebuffer) {
		if (context->bindings.framebuffer) {
			vkCmdEndRenderPass(frame.vkCommandBuffer);
		}
		context->bindings.framebuffer = framebuffer;
		VkRenderPassBeginInfo beginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		beginInfo.renderPass = context->bindings.framebuffer->vkRenderPass;
		beginInfo.framebuffer = FramebufferGetCurrentVkFramebuffer(framebuffer);
		beginInfo.renderArea.offset = {0, 0};
		beginInfo.renderArea.extent.width = framebuffer->width;
		beginInfo.renderArea.extent.height = framebuffer->height;
		vkCmdBeginRenderPass(frame.vkCommandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}
	if (descriptorsChanged) {
		if (auto result = ContextDescriptorsCompose(context); result.isError) {
			return ERROR_RESULT(context, result.error);
		}
	}
	if (vertexBuffer) {
		context->bindings.vertexBuffer = vertexBuffer;
		VkDeviceSize zero = 0;
		// TODO: Support multiple vertex buffer bindings
		vkCmdBindVertexBuffers(frame.vkCommandBuffer, 0, 1, &vertexBuffer->vkBuffer, &zero);
	}
	if (indexBuffer) {
		context->bindings.indexBuffer = indexBuffer;
		vkCmdBindIndexBuffer(frame.vkCommandBuffer, indexBuffer->vkBuffer, 0, indexBuffer->indexType);
	}
	if (nullptr != pipeline && context->bindings.pipeline != pipeline) {
		context->bindings.pipeline = pipeline;
		if (auto result = PipelineCompose(pipeline, context); result.isError) {
			return ERROR_RESULT(context, "Failed to bind Pipeline: ", result.error);
		}
		vkCmdBindPipeline(frame.vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkPipeline);
	}
	if (context->bindings.pipeline != nullptr && frame.descriptorSetsBound.size != 0) {
		Array<VkDescriptorSet> vkDescriptorSets(frame.descriptorSetsBound.size);
		for (i32 i = 0; i < vkDescriptorSets.size; i++) {
			vkDescriptorSets[i] = frame.descriptorSetsBound[i].set;
		}
		vkCmdBindDescriptorSets(frame.vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->bindings.pipeline->vkPipelineLayout, 0, vkDescriptorSets.size, vkDescriptorSets.data, 0, nullptr);
	}
	CmdSetViewportAndScissor(context, (f32)context->bindings.framebuffer->width, (f32)context->bindings.framebuffer->height);
	context->bindCommands.ClearSoft();
	return VoidResult_t();
}

void CmdFinishFramebuffer(Context *context, bool doGenMipmaps) {
	Framebuffer *framebuffer = context->bindings.framebuffer;
	AzAssert(framebuffer != nullptr, "Expected a framebuffer to be bound and committed, but there wasn't one!");
	ContextFrame &frame = context->frames[context->currentFrame];
	vkCmdEndRenderPass(frame.vkCommandBuffer);
	ContextResetBindings(context);
}

void CmdPushConstants(Context *context, const void *src, u32 offset, u32 size) {
	AzAssert(context->bindings.pipeline != nullptr, "Cannot Push Constants without a Pipeline bound and committed.");
	Pipeline *pipeline = context->bindings.pipeline;
	ContextFrame &frame = context->frames[context->currentFrame];
	VkShaderStageFlags stageFlags = 0;
	// TODO: Is this dumb? Should we just pass in the stage?
	for (VkPushConstantRange &range : pipeline->pushConstantRanges) {
		if (range.offset <= offset+size && range.offset+range.size > offset) {
			stageFlags |= range.stageFlags;
		}
	}
	vkCmdPushConstants(frame.vkCommandBuffer, context->bindings.pipeline->vkPipelineLayout, stageFlags, offset, size, src);
}

void CmdSetViewport(Context *context, f32 width, f32 height, f32 minDepth, f32 maxDepth, f32 x, f32 y) {
	ContextFrame &frame = context->frames[context->currentFrame];
	VkViewport viewport;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	viewport.x = x;
	viewport.y = y;
	vkCmdSetViewport(frame.vkCommandBuffer, 0, 1, &viewport);
}

void CmdSetScissor(Context *context, u32 width, u32 height, i32 x, i32 y) {
	ContextFrame &frame = context->frames[context->currentFrame];
	VkRect2D scissor;
	scissor.extent.width = width;
	scissor.extent.height = height;
	scissor.offset.x = x;
	scissor.offset.y = y;
	vkCmdSetScissor(frame.vkCommandBuffer, 0, 1, &scissor);
}

void CmdClearColorAttachment(Context *context, vec4 color, i32 attachment) {
	ContextFrame &frame = context->frames[context->currentFrame];
	AzAssert(context->bindings.framebuffer != nullptr, "Cannot CmdClearColorAttachment without a Framebuffer bound");
	VkClearValue clearValue;
	clearValue.color.float32[0] = color.r;
	clearValue.color.float32[1] = color.g;
	clearValue.color.float32[2] = color.b;
	clearValue.color.float32[3] = color.a;
	VkClearRect clearRect;
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	clearRect.rect.extent.width = (u32)context->bindings.framebuffer->width;
	clearRect.rect.extent.height = (u32)context->bindings.framebuffer->height;
	clearRect.rect.offset.x = 0;
	clearRect.rect.offset.y = 0;
	VkClearAttachment clearAttachment;
	clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clearAttachment.clearValue = clearValue;
	clearAttachment.colorAttachment = (u32)attachment;
	vkCmdClearAttachments(frame.vkCommandBuffer, 1, &clearAttachment, 1, &clearRect);
}

void CmdClearDepthAttachment(Context *context, f32 depth) {
	ContextFrame &frame = context->frames[context->currentFrame];
	AzAssert(context->bindings.framebuffer != nullptr, "Cannot CmdClearDepthAttachment without a Framebuffer bound");
	AzAssert(FramebufferHasDepthBuffer(context->bindings.framebuffer), "Cannot CmdClearDepthAttachment when Framebuffer doesn't have a depth attachment");
	VkClearValue clearValue;
	clearValue.depthStencil.depth = depth;
	clearValue.depthStencil.stencil = 0;
	VkClearRect clearRect;
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	clearRect.rect.extent.width = (u32)context->bindings.framebuffer->width;
	clearRect.rect.extent.height = (u32)context->bindings.framebuffer->height;
	clearRect.rect.offset.x = 0;
	clearRect.rect.offset.y = 0;
	VkClearAttachment clearAttachment;
	clearAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	clearAttachment.clearValue = clearValue;
	vkCmdClearAttachments(frame.vkCommandBuffer, 1, &clearAttachment, 1, &clearRect);
}

void CmdDraw(Context *context, i32 count, i32 vertexOffset, i32 instanceCount, i32 instanceOffset) {
	ContextFrame &frame = context->frames[context->currentFrame];
	vkCmdDraw(frame.vkCommandBuffer, count, instanceCount, vertexOffset, instanceOffset);
}

void CmdDrawIndexed(Context *context, i32 count, i32 indexOffset, i32 vertexOffset, i32 instanceCount, i32 instanceOffset) {
	ContextFrame &frame = context->frames[context->currentFrame];
	AzAssert(context->bindings.indexBuffer != nullptr, "Cannot use CmdDrawIndexed without an index buffer bound");
	vkCmdDrawIndexed(frame.vkCommandBuffer, count, instanceCount, indexOffset, vertexOffset, instanceOffset);
}

#endif

} // namespace AzCore::GPU

// For array operator==
constexpr bool operator!=(VkDescriptorSetLayoutBinding lhs, VkDescriptorSetLayoutBinding rhs) {
	return lhs.binding != rhs.binding || lhs.descriptorType != rhs.descriptorType || lhs.descriptorCount != rhs.descriptorCount;
}
