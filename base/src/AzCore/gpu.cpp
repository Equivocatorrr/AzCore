/*
	File: gpu.cpp
	Author: Philip Haynes
*/

#include "gpu.hpp"
#include "common.hpp"
#include "io.hpp"
#include "QuickSort.hpp"

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

// Per-location stride
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
			Image *object;
			DescriptorIndex binding;
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
	VkPhysicalDeviceFeatures2 features;
	VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures;
	Array<VkExtensionProperties> extensionsAvailable{};
	Array<VkQueueFamilyProperties2> queueFamiliesAvailable{};
	VkPhysicalDeviceMemoryProperties2 memoryProperties;

	VkPhysicalDevice vkPhysicalDevice;

	PhysicalDevice() = default;
	PhysicalDevice(VkPhysicalDevice _vkPhysicalDevice) : vkPhysicalDevice(_vkPhysicalDevice) {
		properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
		vkGetPhysicalDeviceProperties2(vkPhysicalDevice, &properties);

		io::cout.PrintLnDebug("Reading Physical Device Info for \"", properties.properties.deviceName, "\"");

		features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
		features.pNext = &scalarBlockLayoutFeatures;
		scalarBlockLayoutFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES};
		vkGetPhysicalDeviceFeatures2(vkPhysicalDevice, &features);

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

struct Device {
	List<Context> contexts;
	List<Pipeline> pipelines;
	List<Buffer> buffers;
	List<Image> images;
	List<Framebuffer> framebuffers;
	// Map from memoryType to Memory
	HashMap<u32, Memory> memory;

	Ptr<PhysicalDevice> physicalDevice;
	VkDevice vkDevice;
	VkQueue vkQueue;
	i32 queueFamilyIndex;

	String tag;
	bool initted = false;

	Device() = default;
	Device(String _tag) : tag(_tag) {}
};

// For de-duplication purposes
struct DescriptorSetLayout {
	VkDescriptorSetLayoutCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	Array<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;
	
	// Some housekeeping, since createInfo must reference bindings, moving/copying etc would break createInfo.
	DescriptorSetLayout() = default;
	DescriptorSetLayout(const DescriptorSetLayout&) = delete;
	DescriptorSetLayout(DescriptorSetLayout &&other) : bindings(std::move(other.bindings)), vkDescriptorSetLayout(other.vkDescriptorSetLayout) {
		createInfo.bindingCount = bindings.size;
		createInfo.pBindings = bindings.data;
		other.vkDescriptorSetLayout = VK_NULL_HANDLE;
	}
	DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
	DescriptorSetLayout& operator=(DescriptorSetLayout &&other) {
		bindings = std::move(other.bindings);
		createInfo.bindingCount = bindings.size;
		createInfo.pBindings = bindings.data;
		vkDescriptorSetLayout = other.vkDescriptorSetLayout;
		other.vkDescriptorSetLayout = VK_NULL_HANDLE;
		return *this;
	}
};

struct Context {
	VkCommandPool vkCommandPool;
	VkCommandBuffer vkCommandBuffer;
	Fence fence;
	Semaphore semaphore;
	VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;
	Array<DescriptorSetLayout> descriptorSetLayouts;
	Array<VkDescriptorSet> vkDescriptorSets;

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

	Device *device;
	String tag;
	bool initted = false;

	Context() = default;
	Context(Device *_device, String _tag) : device(_device), tag(_tag) {}
};

inline bool ContextIsRecording(Context *context) {
	return (u32)context->state >= (u32)Context::State::RECORDING_PRIMARY;
}

struct Pipeline {
	struct Shader {
		String filename;
		ShaderStage stage;
		VkShaderModule vkShaderModule;
		bool initted = false;
	};
	Array<Shader> shaders;
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

	enum Kind {
		GRAPHICS,
		COMPUTE,
	} kind=GRAPHICS;

	// Keep track of current layout properties so we don't have to recreate everything all the time
	VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo{};
	VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
	VkPipeline vkPipeline = VK_NULL_HANDLE;

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

	i64 size = -1;

	// Used only for index buffers
	VkIndexType indexType = VK_INDEX_TYPE_UINT16;
	
	VkBuffer vkBuffer;
	VkBuffer vkBufferHostVisible;
	VkMemoryRequirements memoryRequirements;
	Allocation alloc;
	Allocation allocHostVisible;

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
	bool mipmapped = false;

	enum State {
		PREINITIALIZED=0,
		READY_FOR_SAMPLING,
		READY_FOR_ATTACHMENT,
		READY_FOR_TRANSFER_SRC,
		READY_FOR_TRANSFER_DST,
	} state = PREINITIALIZED;

	i32 width=-1, height=-1;
	i32 bytesPerPixel=-1;

	i32 anisotropy = 1;
	u32 mipLevels = 1;

	VkImage vkImage;
	VkImageView vkImageView;
	// TODO: Global deduplication of samplers
	VkSampler vkSampler = VK_NULL_HANDLE;
	VkBuffer vkBufferHostVisible;
	VkFormat vkFormat;
	VkImageAspectFlags vkImageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
	VkMemoryRequirements memoryRequirements;
	VkMemoryRequirements memoryRequirementsHost;
	Allocation alloc;
	Allocation allocHostVisible;

	Device *device;
	String tag;
	bool initted = false;
	// Whether our host-visible buffer is active
	bool hostVisible = false;

	Image() = default;
	Image(Device *_device, String _tag) : device(_device), tag(_tag) {}
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
};

struct Framebuffer {
	Array<Attachment> attachments;

	// If we have a WINDOW attachment, this will match the number of swapchain images, else it will just be size 1
	Array<VkFramebuffer> vkFramebuffers;
	VkRenderPass vkRenderPass;
	
	// width and height will be set automagically, just used for easy access
	i32 width, height;

	Device *device;
	String tag;
	bool initted = false;

	Framebuffer() = default;
	Framebuffer(Device *_device, String _tag) : device(_device), tag(_tag) {}
};

Instance instance;
List<Device> devices;
List<Window> windows;


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
	appInfo.apiVersion = VK_API_VERSION_1_2;

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

void FramebufferAddWindow(Framebuffer *framebuffer, Window *window) {
	AzAssert(framebuffer->attachments.size == 0, "Cannot add a Window to a Framebuffer that already has an attachment.");
	framebuffer->attachments.Append(Attachment{Attachment::WINDOW, window});
	window->framebuffer = framebuffer;
}

void SetVSync(Window *window, bool enable) {
	if (window->initted) {
		window->shouldReconfigure = enable != window->vsync;
	}
	window->vsync = enable;
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
	// NOTE: Defaulting to double-buffering for most present modes helps keep latency low, but may result in underutilization of the hardware. Is it possible to automatically choose a number that works for all situations? Maybe make it a setting like most games do.
	u32 imageCountPreferred = 2;
	{ // Choose present mode
		bool found = false;
		if (window->vsync) {
			// The Vulkan Spec requires this present mode to exist
			window->presentMode = VK_PRESENT_MODE_FIFO_KHR;
			found = true;
		} else {
			for (const VkPresentModeKHR& mode : window->presentModesAvailable) {
				if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
					window->presentMode = mode;
					found = true;
					imageCountPreferred = 3;
					break; // Ideal choice, don't keep looking
				} else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
					window->presentMode = mode;
					found = true;
					// Acceptable choice, but keep looking
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
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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
			FenceDeinit(&window->acquireFences[i]);
			SemaphoreDeinit(&window->acquireSemaphores[i]);
		}
		window->acquireFences.Resize(window->numImages);
		window->acquireSemaphores.Resize(window->numImages);
	} else if (window->acquireFences.size < window->numImages) {
		window->acquireFences.Resize(window->numImages, Fence(window->device, Stringify(window->tag, " Fence")));
		window->acquireSemaphores.Resize(window->numImages, Semaphore(window->device, Stringify(window->tag, " Semaphore")));
		for (i32 i = 0; i < window->numImages; i++) {
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
		FenceWaitForSignal(&fence).Unwrap();
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

Result<VoidResult_t, String> WindowPresent(Window *window, ArrayWithBucket<Context*, 4> waitContexts) {
	ArrayWithBucket<VkSemaphore, 4> waitSemaphores(waitContexts.size);
	for (i32 i = 0; i < waitContexts.size; i++) {
		waitSemaphores[i] = waitContexts[i]->semaphore.vkSemaphore;
	}
	VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	presentInfo.waitSemaphoreCount = waitSemaphores.size;
	presentInfo.pWaitSemaphores = waitSemaphores.data;
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

Memory* DeviceGetMemory(Device *device, u32 memoryType) {
	Memory *result;
	if (auto *node = device->memory.Find(memoryType); node == nullptr) {
		result = &device->memory.Emplace(memoryType, Memory(device, memoryType, Stringify("Memory (type ", memoryType, ")")));
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
	return size - (align(offset, alignment) - offset);
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

Allocation PageAllocInSegment(Memory *memory, i32 pageIndex, i32 segmentIndex, u32 size) {
	Allocation result;
	Memory::Page &page = memory->pages[pageIndex];
	AzAssert(size < page.segments[segmentIndex].size, "segment is too small for alloc");
	AzAssert(page.segments[segmentIndex].used == false, "Trying to allocate in a segment that's already in use!");
	using Segment = Memory::Page::Segment;
	if (page.segments[segmentIndex].size > size) {
		Segment &newSegment = page.segments.Insert(segmentIndex+1, Segment());
		Segment &lastSegment = page.segments[segmentIndex];
		newSegment.begin = lastSegment.begin + size;
		newSegment.size = lastSegment.size - size;
		newSegment.used = false;
		lastSegment.size = size;
		lastSegment.used = true;
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
	return PageAllocInSegment(memory, page, segment, size);
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
	Memory *memory = DeviceGetMemory(device, memoryType);
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

Result<Allocation, String> AllocateImage(Device *device, VkImage image, VkMemoryRequirements memoryRequirements, VkMemoryPropertyFlags memoryPropertyFlags) {
	u32 memoryType;
	if (auto result = FindMemoryType(memoryRequirements.memoryTypeBits, memoryPropertyFlags, device->physicalDevice->memoryProperties.memoryProperties); result.isError) {
		return result.error;
	} else {
		memoryType = result.value;
	}
	Memory *memory = DeviceGetMemory(device, memoryType);
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
			for (Attachment &attachment : fb->attachments) {
				if (attachment.kind == Attachment::WINDOW) {
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
	VkPhysicalDeviceFeatures2 featuresAvailable = device->physicalDevice->features;
	VkPhysicalDeviceFeatures2 featuresEnabled = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	{ // Select needed features based on what we use
		bool anisotropyAvailable = featuresAvailable.features.samplerAnisotropy;
		if (!anisotropyAvailable) {
			for (auto &image : device->images) {
				if (image->anisotropy != 1) {
					WARNING(image, "samplerAnisotropy unavailable, so anisotropy is being reset to 1");
					image->anisotropy = 1;
				}
			}
		} else {
			for (auto &image : device->images) {
				if (image->anisotropy != 1) {
					featuresEnabled.features.samplerAnisotropy = VK_TRUE;
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
					break;
				}
			}
		}
	}
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
				for (Attachment &attachment : framebuffer->attachments) {
					if (attachment.kind == Attachment::WINDOW) {
						vkGetPhysicalDeviceSurfaceSupportKHR(device->physicalDevice->vkPhysicalDevice, i, attachment.window->vkSurface, &supportsPresent);
						if (!supportsPresent) goto breakout2;
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
	for (auto &pipeline : device->pipelines) {
		PipelineDeinit(pipeline.RawPtr());
	}
	for (auto node : device->memory) {
		Memory &memory = node.value;
		MemoryClear(&memory);
	}
	vkDestroyDevice(device->vkDevice, nullptr);
}

#endif

#ifndef Resources

Result<VoidResult_t, String> BufferInit(Buffer *buffer) {
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
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (image->mipmapped) {
		createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	if (image->shaderStages) {
		createInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		// TODO: Make controls for all of these
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		// TODO: Support trilinear filtering
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerCreateInfo.mipLodBias = -0.5f;
		samplerCreateInfo.anisotropyEnable = image->anisotropy != 1 ? VK_TRUE : VK_FALSE;
		samplerCreateInfo.maxAnisotropy = image->anisotropy;
		// TODO: Support shadow map compares
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_LESS;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		if (VkResult result = vkCreateSampler(image->device->vkDevice, &samplerCreateInfo, nullptr, &image->vkSampler); result != VK_SUCCESS) {
			return ERROR_RESULT(image, "Failed to create sampler: ", VkResultString(result));
		}
	}
	if (image->attachment) {
		createInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

	if (VkResult result = vkCreateImage(image->device->vkDevice, &createInfo, nullptr, &image->vkImage); result != VK_SUCCESS) {
		return ERROR_RESULT(image, "Failed to create image: ", VkResultString(result));
	}
	SetDebugMarker(image->device, image->tag, VK_OBJECT_TYPE_IMAGE, (u64)image->vkImage);
	vkGetImageMemoryRequirements(image->device->vkDevice, image->vkImage, &image->memoryRequirements);
	if (auto result = AllocateImage(image->device, image->vkImage, image->memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); result.isError) {
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
	SetDebugMarker(image->device, Stringify(image->tag, " image view"), VK_OBJECT_TYPE_IMAGE_VIEW, (u64)image->vkImageView);
	image->initted = true;
	return VoidResult_t();
}

void ImageDeinit(Image *image) {
	DEINIT_HEAD(image);
	vkDestroyImageView(image->device->vkDevice, image->vkImageView, nullptr);
	vkDestroyImage(image->device->vkDevice, image->vkImage, nullptr);
	if (image->vkSampler != VK_NULL_HANDLE) {
		vkDestroySampler(image->device->vkDevice, image->vkSampler, nullptr);
		image->vkSampler = VK_NULL_HANDLE;
	}
	MemoryFree(image->alloc);
	if (image->hostVisible) {
		vkDestroyBuffer(image->device->vkDevice, image->vkBufferHostVisible, nullptr);
		MemoryFree(image->allocHostVisible);
		image->hostVisible = false;
	}
	image->initted = false;
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

void ImageSetMipmapping(Image *image, bool enableMipmapping, i32 anisotropy) {
	image->mipmapped = enableMipmapping;
	image->anisotropy = enableMipmapping ? anisotropy : 1;
	if (image->mipmapped) {
		image->mipLevels = (u32)ceil(log2((f64)max(image->width, image->height)));
	} else {
		image->mipLevels = 1;
	}
}

void ImageSetShaderUsage(Image *image, u32 shaderStages) {
	image->shaderStages = shaderStages;
}

#endif

#ifndef Framebuffer

Result<VoidResult_t, String> FramebufferInit(Framebuffer *framebuffer) {
	INIT_HEAD(framebuffer);
	if (framebuffer->attachments.size == 0) {
		return ERROR_RESULT(framebuffer, "We have no attachments!");
	}
	{ // RenderPass
		bool hasDepth = false;
		Array<VkAttachmentDescription> attachments(framebuffer->attachments.size);
		Array<VkAttachmentReference> attachmentRefsColor;
		VkAttachmentReference attachmentRefDepth;
		Array<u32> preserveAttachments;
		for (i32 i = 0; i < framebuffer->attachments.size; i++) {
			Attachment &attachment = framebuffer->attachments[i];
			switch (attachment.kind) {
			case Attachment::WINDOW:
				if (!attachment.window->initted) return ERROR_RESULT(framebuffer, "Cannot init Framebuffer when attachment ", i, " (Window) is not initialized");
				break;
			case Attachment::IMAGE:
				if (!attachment.image->initted) return ERROR_RESULT(framebuffer, "Cannot init Framebuffer when attachment ", i, " (Image) is not initialized");
				break;
			case Attachment::DEPTH_BUFFER:
				if (!attachment.depthBuffer->initted) return ERROR_RESULT(framebuffer, "Cannot init Framebuffer when attachment ", i, " (depth buffer Image) is not initialized");
				break;
			}
			VkAttachmentReference ref;
			ref.attachment = i;
			VkAttachmentDescription &desc = attachments[i];
			desc = {};
			if (attachment.kind == Attachment::WINDOW) {
				desc.format = attachment.window->surfaceFormat.format;
			} else {
				desc.format = attachment.image->vkFormat;
			}
			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			if (attachment.store) {
				desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			} else {
				desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			}
			switch (attachment.kind) {
			case Attachment::WINDOW:
				desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachmentRefsColor.Append(ref);
				break;
			case Attachment::IMAGE:
				desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachmentRefsColor.Append(ref);
				break;
			case Attachment::DEPTH_BUFFER:
				if (hasDepth) {
					return ERROR_RESULT(framebuffer, "Cannot have more than one depth attachment");
				}
				hasDepth = true;
				desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				attachmentRefDepth = ref;
				break;
			}
			if (attachment.load) {
				desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				return ERROR_RESULT(framebuffer, "Framebuffer image preserving is not implemented yet");
				// TODO: desc.initialLayout =
				if (attachment.store) {
					preserveAttachments.Append(i);
				}
			} else {
				desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			}
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = attachmentRefsColor.size;
		subpass.pColorAttachments = attachmentRefsColor.data;
		subpass.pResolveAttachments = nullptr;
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
	return FramebufferCreate(framebuffer);
}

void FramebufferDeinit(Framebuffer *framebuffer) {
	DEINIT_HEAD(framebuffer);
	vkDestroyRenderPass(framebuffer->device->vkDevice, framebuffer->vkRenderPass, nullptr);
	for (VkFramebuffer fb : framebuffer->vkFramebuffers) {
		vkDestroyFramebuffer(framebuffer->device->vkDevice, fb, nullptr);
	}
}

Result<VoidResult_t, String> FramebufferCreate(Framebuffer *framebuffer) {
	AzAssert(framebuffer->initted, "Framebuffer is not initialized");
	i32 numFramebuffers = 1;
	for (i32 i = 0; i < framebuffer->attachments.size; i++) {
		Attachment &attachment = framebuffer->attachments[i];
		i32 ourWidth, ourHeight;
		switch (attachment.kind) {
		case Attachment::WINDOW:
			numFramebuffers = attachment.window->numImages;
			ourWidth = attachment.window->extent.width;
			ourHeight = attachment.window->extent.height;
			break;
		case Attachment::IMAGE:
			ourWidth = attachment.image->width;
			ourHeight = attachment.image->height;
			break;
		case Attachment::DEPTH_BUFFER:
			ourWidth = attachment.depthBuffer->width;
			ourHeight = attachment.depthBuffer->height;
			break;
		}
		if (i == 0) {
			framebuffer->width = ourWidth;
			framebuffer->height = ourHeight;
		} else if (framebuffer->width != ourWidth || framebuffer->height != ourHeight) {
			return ERROR_RESULT(framebuffer, "Attachment ", i, " dimensions mismatch. Expected ", framebuffer->width, "x", framebuffer->height, ", but got ", ourWidth, "x", ourHeight);
		}
	}
	for (VkFramebuffer fb : framebuffer->vkFramebuffers) {
		vkDestroyFramebuffer(framebuffer->device->vkDevice, fb, nullptr);
	}
	framebuffer->vkFramebuffers.Resize(numFramebuffers);
	for (i32 i = 0; i < numFramebuffers; i++) {
		VkFramebuffer &vkFramebuffer = framebuffer->vkFramebuffers[i];
		Array<VkImageView> imageViews(framebuffer->attachments.size);
		for (i32 j = 0; j < imageViews.size; j++) {
			VkImageView &imageView = imageViews[j];
			Attachment &attachment = framebuffer->attachments[j];
			switch (attachment.kind) {
			case Attachment::WINDOW:
				imageView = attachment.window->swapchainImages[i].vkImageView;
				break;
			case Attachment::IMAGE:
				imageView = attachment.image->vkImageView;
				break;
			case Attachment::DEPTH_BUFFER:
				imageView = attachment.depthBuffer->vkImageView;
				break;
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
		for (i32 i = 0; i < framebuffer->attachments.size; i++) {
			if (framebuffer->attachments[i].kind == Attachment::WINDOW) {
				currentFramebuffer = framebuffer->attachments[i].window->currentImage;
				break;
			}
		}
		AzAssert(currentFramebuffer != -1, "Unreachable");
		return framebuffer->vkFramebuffers[currentFramebuffer];
	}
	return 0; // Unnecessary, but shushes the warning
}

bool FramebufferHasDepthBuffer(Framebuffer *framebuffer) {
	for (Attachment &attachment : framebuffer->attachments) {
		if (attachment.kind == Attachment::DEPTH_BUFFER) return true;
	}
	return false;
}

Window* FramebufferGetWindowAttachment(Framebuffer *framebuffer) {
	for (Attachment &attachment : framebuffer->attachments) {
		if (attachment.kind == Attachment::WINDOW) return attachment.window;
	}
	return nullptr;
}

#endif

#ifndef Pipeline

void PipelineAddShader(Pipeline *pipeline, Str filename, ShaderStage stage) {
	pipeline->shaders.Append(Pipeline::Shader{filename, stage, VK_NULL_HANDLE});
}

void PipelineAddVertexInputs(Pipeline *pipeline, ArrayWithBucket<ShaderValueType, 8> inputs) {
	pipeline->vertexInputs.Append(inputs);
}

void PipelineSetBlendMode(Pipeline *pipeline, BlendMode blendMode, i32 attachment) {
	pipeline->blendModes[attachment] = blendMode;
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

Result<VoidResult_t, String> PipelineInit(Pipeline *pipeline) {
	INIT_HEAD(pipeline);
	for (i32 i = 0; i < pipeline->shaders.size; i++) {
		Pipeline::Shader *shader = &pipeline->shaders[i];
		CHECK_INIT(shader);
		Array<char> code = FileContents(shader->filename);
		if (code.size == 0) {
			return ERROR_RESULT(pipeline, "Failed to open shader source \"", shader->filename, "\"");
		}
		VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		createInfo.codeSize = code.size;
		createInfo.pCode = (u32*)code.data;
		if (VkResult result = vkCreateShaderModule(pipeline->device->vkDevice, &createInfo, nullptr, &shader->vkShaderModule); result != VK_SUCCESS) {
			return ERROR_RESULT(pipeline, "Failed to create shader module for \"", shader->filename, "\": ", VkResultString(result));
		}
		shader->initted = true;
	}
	// We create the actual pipeline objects in PipelineCompose
	pipeline->initted = true;
	return VoidResult_t();
}

void PipelineDeinit(Pipeline *pipeline) {
	DEINIT_HEAD(pipeline);
	for (i32 i = 0; i < pipeline->shaders.size; i++) {
		Pipeline::Shader *shader = &pipeline->shaders[i];
		CHECK_DEINIT(shader);
		vkDestroyShaderModule(pipeline->device->vkDevice, shader->vkShaderModule, nullptr);
		shader->initted = false;
	}
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
	VkPipelineLayoutCreateInfo layoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	Array<VkDescriptorSetLayout> vkDescriptorSetLayouts(context->descriptorSetLayouts.size);
	for (i32 i = 0; i < vkDescriptorSetLayouts.size; i++) {
		vkDescriptorSetLayouts[i] = context->descriptorSetLayouts[i].vkDescriptorSetLayout;
	}
	layoutCreateInfo.setLayoutCount = vkDescriptorSetLayouts.size;
	layoutCreateInfo.pSetLayouts = vkDescriptorSetLayouts.data;
	// TODO: Support push constants
	layoutCreateInfo.pushConstantRangeCount = 0;
	
	bool create = false;
	
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
			Pipeline::Shader &shader = pipeline->shaders[i];
			VkPipelineShaderStageCreateInfo &createInfo = shaderStages[i];
			createInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
			switch (shader.stage) {
			case ShaderStage::COMPUTE:
				io::cout.PrintLnDebug("Compute shader \"", shader.filename, "\"");
				createInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
				break;
			case ShaderStage::VERTEX:
				io::cout.PrintLnDebug("Vertex shader \"", shader.filename, "\"");
				createInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case ShaderStage::FRAGMENT:
				io::cout.PrintLnDebug("Fragment shader \"", shader.filename, "\"");
				createInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			default: return ERROR_RESULT(pipeline, "Unimplemented");
			}
			io::cout.IndentLess();
			createInfo.module = shader.vkShaderModule;
			// The Vulkan API pretends we can use something other than "main", but we really can't :(
			createInfo.pName = "main";
		}
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
						i64 myStride;
						if (inputType == ShaderValueType::DVEC3 && j == 1) {
							// Handle our special case, as DVEC3 is the only input type that takes multiple locations with different strides/formats
							myStride = ShaderValueTypeStride[(u16)inputType]/2;
							attributeDescription.format = VK_FORMAT_R64_SFLOAT;
						} else {
							myStride = ShaderValueTypeStride[(u16)inputType];
							attributeDescription.format = ShaderValueFormats[(u16)inputType];
						}
						attributeDescription.offset = align(offset, myStride);
						offset += myStride;
						vertexInputAttributeDescriptions.Append(attributeDescription);
					}
				}
				if (pipeline->vertexInputs.size == 0) {
					vertexInputBindingDescription.stride = 0;
				} else {
					vertexInputBindingDescription.stride = align(offset, ShaderValueTypeStride[(u16)pipeline->vertexInputs[0]]);
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
			
			// TODO: Support multisampling (need to be able to resolve images, probably in the framebuffer)
			VkPipelineMultisampleStateCreateInfo multisampleState{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
			multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampleState.sampleShadingEnable = VK_FALSE;
			// Controls what fraction of samples get shaded with the above turned on. No effect otherwise.
			multisampleState.minSampleShading = 1.0f;
			multisampleState.pSampleMask = nullptr;
			multisampleState.alphaToCoverageEnable = VK_FALSE;
			multisampleState.alphaToOneEnable = VK_FALSE;
			
			VkPipelineDepthStencilStateCreateInfo depthStencilState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
			// depthStencilState.flags; // VkPipelineDepthStencilStateCreateFlags
			bool framebufferHasDepthBuffer = FramebufferHasDepthBuffer(context->bindings.framebuffer);
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
				for (i32 i = 0; i < context->bindings.framebuffer->attachments.size; i++) {
					Attachment &attachment = context->bindings.framebuffer->attachments[i];
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
	context->fence.device = context->device;
	context->fence.tag = "Context Fence";
	// We'll use signaled to mean not executing
	if (auto result = FenceInit(&context->fence, true); result.isError) {
		return ERROR_RESULT(context, result.error);
	}
	context->semaphore.device = context->device;
	context->semaphore.tag = "Context Semaphore";
	if (auto result = SemaphoreInit(&context->semaphore); result.isError) {
		return ERROR_RESULT(context, result.error);
	}
	context->initted = true;
	return VoidResult_t();
}

void ContextDeinit(Context *context) {
	DEINIT_HEAD(context);
	vkDestroyCommandPool(context->device->vkDevice, context->vkCommandPool, nullptr);
	FenceDeinit(&context->fence);
	SemaphoreDeinit(&context->semaphore);
	if (context->vkDescriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(context->device->vkDevice, context->vkDescriptorPool, nullptr);
		context->vkDescriptorPool = VK_NULL_HANDLE;
	}
	for (DescriptorSetLayout &dsl : context->descriptorSetLayouts) {
		if (dsl.vkDescriptorSetLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(context->device->vkDevice, dsl.vkDescriptorSetLayout, nullptr);
		}
	}
	context->descriptorSetLayouts.ClearSoft();
	context->initted = false;
}

bool DescriptorSetLayoutMatches(DescriptorSetLayout &a, DescriptorSetLayout &b) {
	if (a.bindings.size != b.bindings.size) return false;
	for (i32 i = 0; i < a.bindings.size; i++) {
		AzAssert(a.bindings[i].pImmutableSamplers == nullptr && b.bindings[i].pImmutableSamplers == nullptr, "We don't currently support pImmutableSamplers in DescriptorSetLayoutMatches");
		if (a.bindings[i].binding != b.bindings[i].binding) return false;
		if (a.bindings[i].descriptorCount != b.bindings[i].descriptorCount) return false;
		if (a.bindings[i].descriptorType != b.bindings[i].descriptorType) return false;
		if (a.bindings[i].stageFlags != b.bindings[i].stageFlags) return false;
	}
	return true;
}

Result<VoidResult_t, String> ContextDescriptorsCompose(Context *context) {
	if (context->vkDescriptorPool == VK_NULL_HANDLE) {
		// TODO: Allow recreation of the pool to allow it to grow
		VkDescriptorPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
		createInfo.maxSets = 4;
		VkDescriptorPoolSize poolSizes[3];
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 10;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[1].descriptorCount = 10;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = 10;
		createInfo.poolSizeCount = 3;
		createInfo.pPoolSizes = poolSizes;
		createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		if (VkResult result = vkCreateDescriptorPool(context->device->vkDevice, &createInfo, nullptr, &context->vkDescriptorPool); result != VK_SUCCESS) {
			return ERROR_RESULT(context, "Failed to create descriptor pool: ", VkResultString(result));
		}
	}
	Array<DescriptorSetLayout> descriptorSetLayouts;
	Array<Array<VkWriteDescriptorSet>> vkWriteDescriptorSets;
	// NOTE: In order to update array descriptors, we'll need to switch to contiguous arrays of Buffer and Image Infos like how it's done in vk.cpp (even though vk::Descriptors::Update() is incredibly confusing, it's the optimal way to do it)
	List<VkDescriptorBufferInfo> descriptorBufferInfos;
	List<VkDescriptorImageInfo> descriptorImageInfos;
	for (auto &node : context->bindings.descriptors) {
		Binding &binding = node.value;
		// NOTE: These are necessarily sorted by set first, then binding. This code will break if that is no longer the case.
		if (binding.anyDescriptor.binding.set+1 > descriptorSetLayouts.size) {
			descriptorSetLayouts.Resize(binding.anyDescriptor.binding.set+1);
			vkWriteDescriptorSets.Resize(binding.anyDescriptor.binding.set+1);
		}
		VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		VkDescriptorSetLayoutBinding bindingInfo{};
		bindingInfo.binding = binding.anyDescriptor.binding.binding;
		// TODO: Support arrays
		bindingInfo.descriptorCount = 1;
		switch (binding.kind) {
		case Binding::UNIFORM_BUFFER:
			bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			break;
		case Binding::STORAGE_BUFFER:
			bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			break;
		case Binding::IMAGE_SAMPLER:
			bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			break;
		default:
			return ERROR_RESULT(context, "Invalid descriptor binding (kind is ", (u32)binding.kind, ")");
		}
		switch (binding.kind) {
		case Binding::UNIFORM_BUFFER:
			case Binding::STORAGE_BUFFER: {
				bindingInfo.stageFlags = (VkShaderStageFlags)binding.anyBuffer.object->shaderStages;
				UniquePtr<VkDescriptorBufferInfo> bufferInfo;
				bufferInfo->buffer = binding.anyBuffer.object->vkBuffer;
				bufferInfo->offset = 0;
				bufferInfo->range = binding.anyBuffer.object->size;
				write.pBufferInfo = bufferInfo.RawPtr();
				descriptorBufferInfos.Append(std::move(bufferInfo));
			} break;
			case Binding::IMAGE_SAMPLER: {
				bindingInfo.stageFlags = (VkShaderStageFlags)binding.imageSampler.object->shaderStages;
				UniquePtr<VkDescriptorImageInfo> imageInfo;
				imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo->imageView = binding.imageSampler.object->vkImageView;
				imageInfo->sampler = binding.imageSampler.object->vkSampler;
				write.pImageInfo = imageInfo.RawPtr();
				descriptorImageInfos.Append(std::move(imageInfo));
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
	for (i32 i = 0; i < descriptorSetLayouts.size; i++) {
		DescriptorSetLayout &src = descriptorSetLayouts[i];
		src.createInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		src.createInfo.bindingCount = src.bindings.size;
		src.createInfo.pBindings = src.bindings.data;
		if (context->descriptorSetLayouts.size <= i) {
			context->descriptorSetLayouts.Resize(i+1);
			context->vkDescriptorSets.Resize(i+1, VK_NULL_HANDLE);
		}
		DescriptorSetLayout &dst = context->descriptorSetLayouts[i];
		if (!DescriptorSetLayoutMatches(src, dst)) {
			io::cout.PrintLnDebug("Recreating descriptor set ", i);
			// We need to recreate the layout as well as the descriptor set
			if (dst.vkDescriptorSetLayout != VK_NULL_HANDLE) {
				vkDestroyDescriptorSetLayout(context->device->vkDevice, dst.vkDescriptorSetLayout, nullptr);
			}
			dst = std::move(src);
			if (VkResult result = vkCreateDescriptorSetLayout(context->device->vkDevice, &dst.createInfo, nullptr, &dst.vkDescriptorSetLayout); result != VK_SUCCESS) {
				return ERROR_RESULT(context, "Failed to create descriptor set layout ", i, ": ", VkResultString(result));
			}
			if (context->vkDescriptorSets[i] != VK_NULL_HANDLE) {
				vkFreeDescriptorSets(context->device->vkDevice, context->vkDescriptorPool, 1, &context->vkDescriptorSets[i]);
			}
			VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
			allocInfo.descriptorPool = context->vkDescriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &dst.vkDescriptorSetLayout;
			if (VkResult result = vkAllocateDescriptorSets(context->device->vkDevice, &allocInfo, &context->vkDescriptorSets[i]); result != VK_SUCCESS) {
				return ERROR_RESULT(context, "Failed to allocate descriptor set ", i, ": ", VkResultString(result));
			}
			for (VkWriteDescriptorSet &write : vkWriteDescriptorSets[i]) {
				write.dstSet = context->vkDescriptorSets[i];
			}
			vkUpdateDescriptorSets(context->device->vkDevice, vkWriteDescriptorSets[i].size, vkWriteDescriptorSets[i].data, 0, nullptr);
		}
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
	AzAssert(context->initted, "Trying to record to a Context that's not initted");
	if ((u32)context->state >= (u32)Context::State::RECORDING_PRIMARY) {
		return ERROR_RESULT(context, "Cannot begin recording on a command buffer that's already recording");
	}
	ContextResetBindings(context);
	// TODO: This prevents us from building the next command buffers while the previous ones are running. Switch to double-buffered Contexts
	if (auto result = FenceWaitForSignal(&context->fence); result.isError) {
		return ERROR_RESULT(context, result.error);
	}
	if (auto result = FenceResetSignaled(&context->fence); result.isError) {
		return ERROR_RESULT(context, result.error);
	}

	if (context->state == Context::State::DONE_RECORDING) {
		vkFreeCommandBuffers(context->device->vkDevice, context->vkCommandPool, 1, &context->vkCommandBuffer);
	}

	VkCommandBufferAllocateInfo bufferAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	bufferAllocInfo.commandPool = context->vkCommandPool;
	bufferAllocInfo.commandBufferCount = 1;
	bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	if (VkResult result = vkAllocateCommandBuffers(context->device->vkDevice, &bufferAllocInfo, &context->vkCommandBuffer); result != VK_SUCCESS) {
		return ERROR_RESULT(context, "Failed to allocate primary command buffer: ", VkResultString(result));
	}
	VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	if (VkResult result = vkBeginCommandBuffer(context->vkCommandBuffer, &beginInfo); result != VK_SUCCESS) {
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
	ContextResetBindings(context);

	if (context->state == Context::State::DONE_RECORDING) {
		vkFreeCommandBuffers(context->device->vkDevice, context->vkCommandPool, 1, &context->vkCommandBuffer);
	}

	VkCommandBufferAllocateInfo bufferAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	bufferAllocInfo.commandPool = context->vkCommandPool;
	bufferAllocInfo.commandBufferCount = 1;
	bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	if (VkResult result = vkAllocateCommandBuffers(context->device->vkDevice, &bufferAllocInfo, &context->vkCommandBuffer); result != VK_SUCCESS) {
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
	if (VkResult result = vkBeginCommandBuffer(context->vkCommandBuffer, &beginInfo); result != VK_SUCCESS) {
		return ERROR_RESULT(context, "Failed to begin secondary command buffer: ", VkResultString(result));
	}
	context->state = Context::State::RECORDING_SECONDARY;
	return VoidResult_t();
}

Result<VoidResult_t, String> ContextEndRecording(Context *context) {
	AzAssert(context->initted, "Context not initted");
	if (!ContextIsRecording(context)) {
		return ERROR_RESULT(context, "Trying to End Recording but we haven't started recording.");
	}
	if (context->bindings.framebuffer) {
		vkCmdEndRenderPass(context->vkCommandBuffer);
	}
	if (VkResult result = vkEndCommandBuffer(context->vkCommandBuffer); result != VK_SUCCESS) {
		return ERROR_RESULT(context, "Failed to End Recording: ", VkResultString(result));
	}
	context->state = Context::State::DONE_RECORDING;
	return VoidResult_t();
}

Result<VoidResult_t, String> SubmitCommands(Context *context, bool useSemaphore, ArrayWithBucket<Context*, 4> waitContexts) {
	if (context->state != Context::State::DONE_RECORDING) {
		return ERROR_RESULT(context, "Trying to SubmitCommands without anything recorded.");
	}
	ArrayWithBucket<VkSemaphore, 4> waitSemaphores(waitContexts.size);
	ArrayWithBucket<VkPipelineStageFlags, 4> waitStages(waitContexts.size);
	for (i32 i = 0; i < waitSemaphores.size; i++) {
		waitSemaphores[i] = waitContexts[i]->semaphore.vkSemaphore;
		// TODO: This is a safe assumption, but we could probably be more specific.
		waitStages[i] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
	if (context->bindings.framebuffer) {
		Window *window = FramebufferGetWindowAttachment(context->bindings.framebuffer);
		if (window) {
			waitSemaphores.Append(window->acquireSemaphores[window->currentSync].vkSemaphore);
			waitStages.Append(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		}
	}
	VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &context->vkCommandBuffer;
	if (useSemaphore) {
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &context->semaphore.vkSemaphore;
	}
	submitInfo.waitSemaphoreCount = waitSemaphores.size;
	submitInfo.pWaitSemaphores = waitSemaphores.data;
	submitInfo.pWaitDstStageMask = waitStages.data;
	if (VkResult result = vkQueueSubmit(context->device->vkQueue, 1, &submitInfo, context->fence.vkFence); result != VK_SUCCESS) {
		return ERROR_RESULT(context, "Failed to submit to queue: ", VkResultString(result));
	}
	return VoidResult_t();
}

Result<bool, String> ContextIsExecuting(Context *context) {
	AzAssert(context->initted, "Context is not initted");
	VkResult result = FenceGetStatus(&context->fence);
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
	AzAssert(context->initted, "Context is not initted");
	AzAssert(timeout.count() >= 0, "Cannot have negative timeout");
	bool wasTimeout;
	if (auto result = FenceWaitForSignal(&context->fence, (u64)timeout.count(), &wasTimeout); result.isError) {
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
	AzAssert(size+dstOffset <= (i64)dst->memoryRequirements.size, "size is bigger than our buffer");
	AzAssert(ContextIsRecording(context), "Trying to record into a context that's not recording");
	if (size == 0) {
		// We do the whole size
		size = dst->memoryRequirements.size;
	}
	if (!dst->hostVisible) {
		if (auto result = BufferHostInit(dst); result.isError) {
			return result.error;
		}
	}
	Allocation alloc = dst->allocHostVisible;
	VkDeviceMemory vkMemory = alloc.memory->pages[alloc.page].vkMemory;
	void *dstMapped;
	if (VkResult result = vkMapMemory(dst->device->vkDevice, vkMemory, align(alloc.offset, dst->memoryRequirements.alignment)+dstOffset, size, 0, &dstMapped); result != VK_SUCCESS) {
		return Stringify("Buffer \"", dst->tag, "\" error: Failed to map memory: ", VkResultString(result));
	}
	memcpy(dstMapped, src, size);
	vkUnmapMemory(dst->device->vkDevice, vkMemory);
	VkBufferCopy vkCopy;
	vkCopy.dstOffset = dstOffset;
	vkCopy.srcOffset = dstOffset;
	vkCopy.size = size;
	vkCmdCopyBuffer(context->vkCommandBuffer, dst->vkBufferHostVisible, dst->vkBuffer, 1, &vkCopy);
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
	vkCmdPipelineBarrier(context->vkCommandBuffer, srcAccessAndStage.stageFlags, dstAccessAndStage.stageFlags, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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

static void CmdImageGenerateMipmaps(Context *context, Image *image, VkImageLayout startingLayout, VkImageLayout finalLayout, VkFilter filter=VK_FILTER_LINEAR) {
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
		vkCmdBlitImage(context->vkCommandBuffer, image->vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, filter);
		CmdImageTransitionLayout(context, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mip);
	}

	CmdImageTransitionLayout(context, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, finalLayout, 0, image->mipLevels);
}

Result<VoidResult_t, String> CmdCopyDataToImage(Context *context, Image *dst, void *src) {
	AzAssert(ContextIsRecording(context), "Trying to record into a context that's not recording");
	if (!dst->hostVisible) {
		if (auto result = ImageHostInit(dst); result.isError) {
			return result.error;
		}
	}
	CmdImageTransitionLayout(context, dst, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	Allocation alloc = dst->allocHostVisible;
	VkDeviceMemory vkMemory = alloc.memory->pages[alloc.page].vkMemory;
	void *dstMapped;
	if (VkResult result = vkMapMemory(dst->device->vkDevice, vkMemory, align(alloc.offset, dst->memoryRequirementsHost.alignment), dst->memoryRequirementsHost.size, 0, &dstMapped); result != VK_SUCCESS) {
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
	vkCmdCopyBufferToImage(context->vkCommandBuffer, dst->vkBufferHostVisible, dst->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vkCopy);
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

void CmdBindImageSampler(Context *context, Image *image, i32 set, i32 binding) {
	Binding bind;
	bind.kind = Binding::IMAGE_SAMPLER;
	bind.imageSampler.object = image;
	bind.imageSampler.binding.set = set;
	bind.imageSampler.binding.binding = binding;
	context->bindCommands.Append(bind);
}

Result<VoidResult_t, String> CmdCommitBindings(Context *context) {
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
			vkCmdEndRenderPass(context->vkCommandBuffer);
		}
		context->bindings.framebuffer = framebuffer;
		VkRenderPassBeginInfo beginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		beginInfo.renderPass = context->bindings.framebuffer->vkRenderPass;
		beginInfo.framebuffer = FramebufferGetCurrentVkFramebuffer(framebuffer);
		beginInfo.renderArea.offset = {0, 0};
		beginInfo.renderArea.extent.width = framebuffer->width;
		beginInfo.renderArea.extent.height = framebuffer->height;
		vkCmdBeginRenderPass(context->vkCommandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
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
		vkCmdBindVertexBuffers(context->vkCommandBuffer, 0, 1, &vertexBuffer->vkBuffer, &zero);
	}
	if (indexBuffer) {
		context->bindings.indexBuffer = indexBuffer;
		vkCmdBindIndexBuffer(context->vkCommandBuffer, indexBuffer->vkBuffer, 0, indexBuffer->indexType);
	}
	if (nullptr != pipeline && context->bindings.pipeline != pipeline) {
		context->bindings.pipeline = pipeline;
		if (auto result = PipelineCompose(pipeline, context); result.isError) {
			return ERROR_RESULT(context, "Failed to bind Pipeline: ", result.error);
		}
		vkCmdBindPipeline(context->vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkPipeline);
	}
	if (context->bindings.pipeline != nullptr && context->vkDescriptorSets.size != 0) {
		vkCmdBindDescriptorSets(context->vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->bindings.pipeline->vkPipelineLayout, 0, context->vkDescriptorSets.size, context->vkDescriptorSets.data, 0, nullptr);
	}
	CmdSetViewportAndScissor(context, (f32)context->bindings.framebuffer->width, (f32)context->bindings.framebuffer->height);
	context->bindCommands.ClearSoft();
	return VoidResult_t();
}

void CmdSetViewport(Context *context, f32 width, f32 height, f32 minDepth, f32 maxDepth, f32 x, f32 y) {
	VkViewport viewport;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	viewport.x = x;
	viewport.y = y;
	vkCmdSetViewport(context->vkCommandBuffer, 0, 1, &viewport);
}

void CmdSetScissor(Context *context, u32 width, u32 height, i32 x, i32 y) {
	VkRect2D scissor;
	scissor.extent.width = width;
	scissor.extent.height = height;
	scissor.offset.x = x;
	scissor.offset.y = y;
	vkCmdSetScissor(context->vkCommandBuffer, 0, 1, &scissor);
}

void CmdClearColorAttachment(Context *context, vec4 color, i32 attachment) {
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
	vkCmdClearAttachments(context->vkCommandBuffer, 1, &clearAttachment, 1, &clearRect);
}

void CmdDrawIndexed(Context *context, i32 count, i32 indexOffset, i32 vertexOffset, i32 instanceCount, i32 instanceOffset) {
	AzAssert(context->bindings.indexBuffer != nullptr, "Cannot use CmdDrawIndexed without an index buffer bound");
	vkCmdDrawIndexed(context->vkCommandBuffer, count, instanceCount, indexOffset, vertexOffset, instanceOffset);
}

#endif

} // namespace AzCore::GPU
