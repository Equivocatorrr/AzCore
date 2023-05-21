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

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

namespace AzCore::GPU {

#ifndef Utils

constexpr i64 ShaderValueTypeStride[] = {
	/* U32 */   4,
	/* I32 */   4,
	/* IVEC2 */ 8,
	/* IVEC3 */ 16,
	/* IVEC4 */ 16,
	/* F32 */   4,
	/* VEC2 */  8,
	/* VEC3 */  16,
	/* VEC4 */  16,
};
constexpr VkFormat ShaderValueFormats[] = {
	/* U32 */   VK_FORMAT_R32_UINT,
	/* I32 */   VK_FORMAT_R32_SINT,
	/* IVEC2 */ VK_FORMAT_R32G32_SINT,
	/* IVEC3 */ VK_FORMAT_R32G32B32_SINT,
	/* IVEC4 */ VK_FORMAT_R32G32B32A32_SINT,
	/* F32 */   VK_FORMAT_R32_SFLOAT,
	/* VEC2 */  VK_FORMAT_R32G32_SFLOAT,
	/* VEC3 */  VK_FORMAT_R32G32B32_SFLOAT,
	/* VEC4 */  VK_FORMAT_R32G32B32A32_SFLOAT,
};

extern Str imageComponentTypeStrings[6] = {
	"SRGB",
	"UNORM",
	"SNORM",
	"UINT",
	"SINT",
	"SFLOAT",
};

String ErrorString(VkResult errorCode) {
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
	};
};

#endif

#ifndef Declarations_and_global_variables

template <typename T>
using List = Array<UniquePtr<T>>;

struct Window {
	bool vsync = false;
	
	io::Window *window;
	
	Str tag;
	VkSurfaceKHR surface;
	Window() = default;
	Window(io::Window *_window, Str _tag) : window(_window), tag(_tag) {}
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

struct Allocation {
	Memory *memory;
	i32 page;
	u32 offset;
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
	Str tag;
	
	Memory() = default;
	Memory(Device *_device, u32 _memoryTypeIndex, Str _tag=Str()) : device(_device), memoryTypeIndex(_memoryTypeIndex), tag(_tag) {}
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

	Str tag;
	bool initted = false;

	Device() = default;
	Device(Str _tag) : tag(_tag) {}
};

struct Context {
	VkCommandPool vkCommandPool;
	VkCommandBuffer vkCommandBuffer;
	VkFence vkFence;
	
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
	Str tag;
	bool initted = false;

	Context() = default;
	Context(Device *_device, Str _tag) : device(_device), tag(_tag) {}
};

inline bool ContextIsRecording(Context *context) {
	return (u32)context->state >= (u32)Context::State::RECORDING_PRIMARY;
}

struct Pipeline {
	f32 lineWidth = 1.0f;
	struct Shader {
		Str filename;
		ShaderStage stage;
	};
	Array<Shader> shaders;
	Array<Buffer*> buffers;
	Array<Image*> images;
	ArrayWithBucket<ShaderValueType, 8> vertexInputs;
	BlendMode blendMode = BlendMode::OPAQUE;

	enum Kind {
		GRAPHICS,
		COMPUTE,
	} kind;

	VkPipeline vkPipeline;

	Device *device;
	Str tag;
	bool initted = false;

	Pipeline() = default;
	Pipeline(Device *_device, Kind _kind, Str _tag) : device(_device), kind(_kind), tag(_tag) {}
};

struct Buffer {
	enum Kind {
		UNDEFINED=0,
		VERTEX_BUFFER,
		INDEX_BUFFER,
		STORAGE_BUFFER,
		UNIFORM_BUFFER,
	} kind=UNDEFINED;

	i64 size = -1;

	VkBuffer vkBuffer;
	VkBuffer vkBufferHostVisible;
	VkMemoryRequirements memoryRequirements;
	Allocation alloc;
	Allocation allocHostVisible;

	Device *device;
	Str tag;
	bool initted = false;
	// Whether our host-visible buffer is active
	bool hostVisible = false;

	Buffer() = default;
	Buffer(Kind _kind, Device *_device, Str _tag) : kind(_kind), device(_device), tag(_tag) {}
};

struct Image {
	// Usage flags
	u32 sampledStages = 0;
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
	i32 channels=-1, bitsPerChannel=-1;

	i32 anisotropy = 1;
	u32 mipLevels = 1;

	VkImage vkImage;
	VkImageView vkImageView;
	VkBuffer vkBufferHostVisible;
	VkFormat vkFormat;
	VkImageAspectFlags vkImageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
	VkMemoryRequirements memoryRequirements;
	VkMemoryRequirements bufferMemoryRequirements;
	Allocation alloc;
	Allocation allocHostVisible;

	Device *device;
	Str tag;
	bool initted = false;
	// Whether our host-visible buffer is active
	bool hostVisible = false;

	Image() = default;
	Image(Device *_device, Str _tag) : device(_device), tag(_tag) {}
};

struct Framebuffer {
	Window *window = nullptr;
	Image *image = nullptr;

	VkFramebuffer vkFramebuffer;
	VkRenderPass vkRenderPass;

	Device *device;
	Str tag;
	bool initted = false;

	Framebuffer() = default;
	Framebuffer(Device *_device, Str _tag) : device(_device), tag(_tag) {}
};

Instance instance;
List<Device> devices;
List<Window> windows;

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
		if (windows.size) {
			extensions.Append("VK_KHR_surface");
	#ifdef __unix
			if (windows[0]->data->useWayland) {
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
		return Stringify("vkCreateInstance failed with ", ErrorString(vkResult));
	}
	instance.initted = true;

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

#ifndef Window

Result<Window*, String> AddWindow(io::Window *window, Str tag) {
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
	AzAssert(framebuffer->image == nullptr && framebuffer->window == nullptr, "Cannot add a Window to a Framebuffer that already has a binding");
	framebuffer->window = window;
}

void SetVSync(Window *window, bool enable) {
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
		VkResult result = vkCreateWaylandSurfaceKHR(instance.vkInstance, &createInfo, nullptr, &window->surface);
		if (result != VK_SUCCESS) {
			return Stringify("Failed to create Vulkan Wayland surface: ", ErrorString(result));
		}
	} else {
		VkXcbSurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
		createInfo.connection = window->window->data->x11.connection;
		createInfo.window = window->window->data->x11.window;
		VkResult result = vkCreateXcbSurfaceKHR(instance.vkInstance, &createInfo, nullptr, &window->surface);
		if (result != VK_SUCCESS) {
			return Stringify("Failed to create Vulkan XCB surface: ", ErrorString(result));
		}
	}
#elif defined(_WIN32)
	VkWin32SurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	createInfo.hinstance = window->window->data->instance;
	createInfo.hwnd = window->window->data->window;
	VkResult result = vkCreateWin32SurfaceKHR(instance.vkInstance, &createInfo, nullptr, &window->surface);
	if (result != VK_SUCCESS) {
		return Stringify("Failed to create Win32 Surface: ", ErrorString(result));
	}
#endif
	return VoidResult_t();
}

void WindowSurfaceDeinit(Window *window) {
	vkDestroySurfaceKHR(instance.vkInstance, window->surface, nullptr);
}

Result<VoidResult_t, String> WindowUpdate(Window *window) {
	return String("Unimplemented");
}

Result<VoidResult_t, String> WindowPresent(Window *window) {
	return String("Unimplemented");
}

#endif

#ifndef Creating_new_objects

Device* NewDevice(Str tag) {
	return devices.Append(new Device(tag)).RawPtr();
}

Context* NewContext(Device *device, Str tag) {
	return device->contexts.Append(new Context(device, tag)).RawPtr();
}

Pipeline* NewGraphicsPipeline(Device *device, Str tag) {
	return device->pipelines.Append(new Pipeline(device, Pipeline::GRAPHICS, tag)).RawPtr();
}

Pipeline* NewComputePipeline(Device *device, Str tag) {
	return device->pipelines.Append(new Pipeline(device, Pipeline::COMPUTE, tag)).RawPtr();
}

Buffer* NewVertexBuffer(Device *device, Str tag) {
	return device->buffers.Append(new Buffer(Buffer::VERTEX_BUFFER, device, tag)).RawPtr();
}

Buffer* NewIndexBuffer(Device *device, Str tag) {
	return device->buffers.Append(new Buffer(Buffer::INDEX_BUFFER, device, tag)).RawPtr();
}

Buffer* NewStorageBuffer(Device *device, Str tag) {
	return device->buffers.Append(new Buffer(Buffer::STORAGE_BUFFER, device, tag)).RawPtr();
}

Buffer* NewUniformBuffer(Device *device, Str tag) {
	return device->buffers.Append(new Buffer(Buffer::UNIFORM_BUFFER, device, tag)).RawPtr();
}

Image* NewImage(Device *device, Str tag) {
	return device->images.Append(new Image(device, tag)).RawPtr();
}

Framebuffer* NewFramebuffer(Device *device, Str tag) {
	return device->framebuffers.Append(new Framebuffer(device, tag)).RawPtr();
}

#endif

#ifndef Physical_Device

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
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			break;
	}
	score += device->properties.properties.limits.maxImageDimension2D;
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
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice->vkPhysicalDevice, i, windows[j]->surface, &presentSupport);
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
		result = &device->memory.Emplace(memoryType, Memory(device, memoryType));
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
	return "Failed to find a suitable memory type!";
}

Result<VoidResult_t, String> MemoryAddPage(Memory *memory, u32 minSize) {
	AzAssert(memory->device->initted, "Device not initted!");
	minSize = max(minSize, memory->pageSizeMin);
	Memory::Page &newPage = memory->pages.Append(Memory::Page());
	VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.memoryTypeIndex = memory->memoryTypeIndex;
	allocInfo.allocationSize = minSize;
	if (VkResult result = vkAllocateMemory(memory->device->vkDevice, &allocInfo, nullptr, &newPage.vkMemory); result != VK_SUCCESS) {
		return Stringify("Memory \"", memory->tag, "\" error: Failed to allocate a new page: ", ErrorString(result));
	}
	newPage.segments.Append(Memory::Page::Segment{0, minSize, false});
	return VoidResult_t();
}

u32 alignedSize(u32 offset, u32 size, u32 alignment) {
	return size - (align(offset, alignment) - offset);
}

i32 PageFindSegment(Memory::Page *page, u32 size, u32 alignment) {
	for (i32 i = 0; i < page->segments.size; i++) {
		if (page->segments[i].used) continue;
		if (alignedSize(page->segments[i].begin, page->segments[i].size, alignment) <= size) {
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
	if (page.segments[segmentIndex].size < size) {
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

void MemoryFree(Memory *memory, Allocation allocation) {
	Memory::Page &page = memory->pages[allocation.page];
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
		return Stringify("Memory \"", memory->tag, "\" error: Failed to bind Buffer to Memory: ", ErrorString(result));
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
		return Stringify("Memory \"", memory->tag, "\" error: Failed to bind Image to Memory: ", ErrorString(result));
	}
	return alloc;
}

#endif

#ifndef Device

Result<VoidResult_t, String> DeviceInit(Device *device) {
	AzAssert(device->initted == false, "Trying to Init a Device that's already initted");
	io::cout.PrintLnTrace("Initializing Device \"", device->tag, "\"");

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
			if (fb->window) {
				// If even one framebuffer outputs to a Window, we use a Swapchain
				extensions.Append(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
				needsPresent = true;
				break;
			}
		}
		auto physicalDevice = FindBestPhysicalDeviceWithExtensions(extensions);
		if (physicalDevice.isError) return physicalDevice.error;
		device->physicalDevice = physicalDevice.value;
	}
	VkPhysicalDeviceFeatures2 featuresAvailable = device->physicalDevice->features;
	VkPhysicalDeviceFeatures2 featuresEnabled = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	{ // Select needed features based on what we use
		bool anisotropyAvailable = featuresAvailable.features.samplerAnisotropy;
		if (!anisotropyAvailable) {
			for (auto &image : device->images) {
				if (image->anisotropy != 1) {
					io::cout.PrintLn("Warning: samplerAnisotropy unavailable, so image \"", image->tag, "\" anisotropy is being reset to 1");
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
					io::cout.PrintLn("Warning: wide lines unavailable, so pipeline \"", pipeline->tag, "\" lineWidth is being reset to 1.0f");
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
				if (framebuffer->window) {
					vkGetPhysicalDeviceSurfaceSupportKHR(device->physicalDevice->vkPhysicalDevice, i, framebuffer->window->surface, &supportsPresent);
					if (!supportsPresent) break;
				}
			}
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
		return Stringify("There were no queues available that had everything we needed");
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
		return Stringify("Failed to create Device: ", ErrorString(result));
	}

	device->initted = true;

	vkGetDeviceQueue(device->vkDevice, device->queueFamilyIndex, 0, &device->vkQueue);

	for (auto &context : device->contexts) {
		if (auto result = ContextInit(context.RawPtr()); result.isError) {
			return result.error;
		}
	}
	for (auto &buffer : device->buffers) {
		if (auto result = BufferInit(buffer.RawPtr()); result.isError) {
			return result.error;
		}
	}
	for (auto &image : device->images) {
		if (auto result = ImageInit(image.RawPtr()); result.isError) {
			return result.error;
		}
	}
	// TODO: Init everything else

	return VoidResult_t();
}

void DeviceDeinit(Device *device) {
	AzAssert(device->initted, "Trying to Deinit a Device that isn't initted");
	io::cout.PrintLnTrace("Deinitializing Device \"", device->tag, "\"");
	for (auto &context : device->contexts) {
		ContextDeinit(context.RawPtr());
	}
	for (auto &buffer : device->buffers) {
		BufferDeinit(buffer.RawPtr());
	}
	for (auto &image : device->images) {
		ImageDeinit(image.RawPtr());
	}
	vkDestroyDevice(device->vkDevice, nullptr);
}

#endif

#ifndef Resources

Result<VoidResult_t, String> BufferInit(Buffer *buffer) {
	AzAssert(buffer->initted == false, "trying to init a buffer that's already initted");
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
			return Stringify("Buffer \"", buffer->tag, "\" error: Cannot initialize buffer with undefined Kind");
	}
	if (VkResult result = vkCreateBuffer(buffer->device->vkDevice, &createInfo, nullptr, &buffer->vkBuffer); result != VK_SUCCESS) {
		return Stringify("Buffer \"", buffer->tag, "\" error: Failed to create buffer: ", ErrorString(result));
	}
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
	AzAssert(buffer->initted == true, "Trying to deinit a buffer that's not initted");
	vkDestroyBuffer(buffer->device->vkDevice, buffer->vkBuffer, nullptr);
	if (buffer->hostVisible) {
		vkDestroyBuffer(buffer->device->vkDevice, buffer->vkBufferHostVisible, nullptr);
		buffer->hostVisible = false;
	}
	buffer->initted = false;
}

Result<VoidResult_t, String> BufferHostInit(Buffer *buffer) {
	AzAssert(buffer->initted == true, "Trying to init staging buffer for buffer that's not initted");
	AzAssert(buffer->hostVisible == false, "Trying to init staging buffer that's already initted");
	VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	createInfo.size = buffer->size;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (VkResult result = vkCreateBuffer(buffer->device->vkDevice, &createInfo, nullptr, &buffer->vkBufferHostVisible); result != VK_SUCCESS) {
		return Stringify("Buffer \"", buffer->tag, "\" error: Failed to create staging buffer: ", ErrorString(result));
	}
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
	vkDestroyBuffer(buffer->device->vkDevice, buffer->vkBufferHostVisible, nullptr);
	buffer->hostVisible = false;
}

Result<VoidResult_t, String> BufferSetSize(Buffer *buffer, i64 sizeBytes) {
	bool initted = buffer->initted;
	buffer->size = sizeBytes;
	if (initted) {
		// Reinit
	}
	return VoidResult_t();
}

Result<VoidResult_t, String> ImageInit(Image *image) {
	AzAssert(image->initted == false, "trying to init an image that's already initted");
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
	if (image->sampledStages) {
		createInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	if (image->attachment) {
		createInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	
	if (VkResult result = vkCreateImage(image->device->vkDevice, &createInfo, nullptr, &image->vkImage); result != VK_SUCCESS) {
		return Stringify("Image \"", image->tag, "\" error: Failed to create image: ", ErrorString(result));
	}
	VkImageViewCreateInfo viewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	viewCreateInfo.image = image->vkImage;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = image->vkFormat;
	viewCreateInfo.subresourceRange.aspectMask = image->vkImageAspect;
	viewCreateInfo.subresourceRange.levelCount = image->mipLevels;
	viewCreateInfo.subresourceRange.layerCount = 1;
	
	if (VkResult result = vkCreateImageView(image->device->vkDevice, &viewCreateInfo, nullptr, &image->vkImageView); result != VK_SUCCESS) {
		return Stringify("Image \"", image->tag, "\" error: Failed to create image view: ", ErrorString(result));
	}
	vkGetImageMemoryRequirements(image->device->vkDevice, image->vkImage, &image->memoryRequirements);
	if (auto result = AllocateImage(image->device, image->vkImage, image->memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); result.isError) {
		return result.error;
	} else {
		image->alloc = result.value;
	}
	image->initted = true;
	return VoidResult_t();
}

void ImageDeinit(Image *image) {
	AzAssert(image->initted == true, "Trying to deinit an image that's not initted");
	vkDestroyImageView(image->device->vkDevice, image->vkImageView, nullptr);
	vkDestroyImage(image->device->vkDevice, image->vkImage, nullptr);
	if (image->hostVisible) {
		vkDestroyBuffer(image->device->vkDevice, image->vkBufferHostVisible, nullptr);
		image->hostVisible = false;
	}
	image->initted = false;
}

Result<VoidResult_t, String> ImageHostInit(Image *image) {
	AzAssert(image->initted == true, "Trying to init image staging buffer that's not initted");
	AzAssert(image->hostVisible == false, "Trying to init image staging buffer that's already initted");
	VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	createInfo.size = image->width * image->height * image->channels * intDivCeil(image->bitsPerChannel, 8);
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (VkResult result = vkCreateBuffer(image->device->vkDevice, &createInfo, nullptr, &image->vkBufferHostVisible); result != VK_SUCCESS) {
		return Stringify("Buffer \"", image->tag, "\" error: Failed to create image staging buffer: ", ErrorString(result));
	}
	vkGetBufferMemoryRequirements(image->device->vkDevice, image->vkBufferHostVisible, &image->bufferMemoryRequirements);
	if (auto result = AllocateBuffer(image->device, image->vkBufferHostVisible, image->bufferMemoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT); result.isError) {
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
	vkDestroyBuffer(image->device->vkDevice, image->vkBufferHostVisible, nullptr);
	image->hostVisible = false;
}

Result<VoidResult_t, String> ImageSetFormat(Image *image, i32 channels, i32 bitsPerChannel, ImageComponentType componentType) {
	image->channels = channels;
	image->bitsPerChannel = bitsPerChannel;
	switch (bitsPerChannel) {
		case 8:
			switch (componentType) {
				case ImageComponentType::SRGB: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R8_SRGB; break;
						case 2: image->vkFormat = VK_FORMAT_R8G8_SRGB; break;
						case 3: image->vkFormat = VK_FORMAT_R8G8B8_SRGB; break;
						case 4: image->vkFormat = VK_FORMAT_R8G8B8A8_SRGB; break;
					}
				} break;
				case ImageComponentType::UNORM: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R8_UNORM; break;
						case 2: image->vkFormat = VK_FORMAT_R8G8_UNORM; break;
						case 3: image->vkFormat = VK_FORMAT_R8G8B8_UNORM; break;
						case 4: image->vkFormat = VK_FORMAT_R8G8B8A8_UNORM; break;
					}
				} break;
				case ImageComponentType::SNORM: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R8_SNORM; break;
						case 2: image->vkFormat = VK_FORMAT_R8G8_SNORM; break;
						case 3: image->vkFormat = VK_FORMAT_R8G8B8_SNORM; break;
						case 4: image->vkFormat = VK_FORMAT_R8G8B8A8_SNORM; break;
					}
				} break;
				case ImageComponentType::UINT: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R8_UINT; break;
						case 2: image->vkFormat = VK_FORMAT_R8G8_UINT; break;
						case 3: image->vkFormat = VK_FORMAT_R8G8B8_UINT; break;
						case 4: image->vkFormat = VK_FORMAT_R8G8B8A8_UINT; break;
					}
				} break;
				case ImageComponentType::SINT: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R8_SINT; break;
						case 2: image->vkFormat = VK_FORMAT_R8G8_SINT; break;
						case 3: image->vkFormat = VK_FORMAT_R8G8B8_SINT; break;
						case 4: image->vkFormat = VK_FORMAT_R8G8B8A8_SINT; break;
					}
				} break;
				default: goto bad_format;
			} break;
		case 16:
			switch (componentType) {
				case ImageComponentType::UNORM: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R16_UNORM; break;
						case 2: image->vkFormat = VK_FORMAT_R16G16_UNORM; break;
						case 3: image->vkFormat = VK_FORMAT_R16G16B16_UNORM; break;
						case 4: image->vkFormat = VK_FORMAT_R16G16B16A16_UNORM; break;
					}
				} break;
				case ImageComponentType::SNORM: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R16_SNORM; break;
						case 2: image->vkFormat = VK_FORMAT_R16G16_SNORM; break;
						case 3: image->vkFormat = VK_FORMAT_R16G16B16_SNORM; break;
						case 4: image->vkFormat = VK_FORMAT_R16G16B16A16_SNORM; break;
					}
				} break;
				case ImageComponentType::UINT: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R16_UINT; break;
						case 2: image->vkFormat = VK_FORMAT_R16G16_UINT; break;
						case 3: image->vkFormat = VK_FORMAT_R16G16B16_UINT; break;
						case 4: image->vkFormat = VK_FORMAT_R16G16B16A16_UINT; break;
					}
				} break;
				case ImageComponentType::SINT: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R16_SINT; break;
						case 2: image->vkFormat = VK_FORMAT_R16G16_SINT; break;
						case 3: image->vkFormat = VK_FORMAT_R16G16B16_SINT; break;
						case 4: image->vkFormat = VK_FORMAT_R16G16B16A16_SINT; break;
					}
				} break;
				case ImageComponentType::SFLOAT: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R16_SFLOAT; break;
						case 2: image->vkFormat = VK_FORMAT_R16G16_SFLOAT; break;
						case 3: image->vkFormat = VK_FORMAT_R16G16B16_SFLOAT; break;
						case 4: image->vkFormat = VK_FORMAT_R16G16B16A16_SFLOAT; break;
					}
				} break;
				default: goto bad_format;
			} break;
		case 32:
			switch (componentType) {
				case ImageComponentType::UINT: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R32_UINT; break;
						case 2: image->vkFormat = VK_FORMAT_R32G32_UINT; break;
						case 3: image->vkFormat = VK_FORMAT_R32G32B32_UINT; break;
						case 4: image->vkFormat = VK_FORMAT_R32G32B32A32_UINT; break;
					}
				} break;
				case ImageComponentType::SINT: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R32_SINT; break;
						case 2: image->vkFormat = VK_FORMAT_R32G32_SINT; break;
						case 3: image->vkFormat = VK_FORMAT_R32G32B32_SINT; break;
						case 4: image->vkFormat = VK_FORMAT_R32G32B32A32_SINT; break;
					}
				} break;
				case ImageComponentType::SFLOAT: {
					switch (channels) {
						case 1: image->vkFormat = VK_FORMAT_R32_SFLOAT; break;
						case 2: image->vkFormat = VK_FORMAT_R32G32_SFLOAT; break;
						case 3: image->vkFormat = VK_FORMAT_R32G32B32_SFLOAT; break;
						case 4: image->vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT; break;
					}
				} break;
				default: goto bad_format;
			} break;
		default: goto bad_format;
	}
	return VoidResult_t();
bad_format:
	return Stringify("Cannot match ", bitsPerChannel, " bpc, ", channels, " channels, and component type ", componentType);
}

Result<VoidResult_t, String> ImageSetSize(Image *image, i32 width, i32 height) {
	image->width = width;
	image->height = height;
	if (image->mipmapped) {
		image->mipLevels = (u32)ceil(log2((f64)max(image->width, image->height)));
	}
	return VoidResult_t();
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
void ImageSetUsageSampled(Image *image, u32 shaderStages) {
	image->sampledStages = shaderStages;
}

#endif

#ifndef Pipeline

void PipelineAddShader(Pipeline *pipeline, Str filename, ShaderStage stage) {
	pipeline->shaders.Append(Pipeline::Shader{filename, stage});
}

void PipelineAddBuffer(Pipeline *pipeline, Buffer *buffer) {
	pipeline->buffers.Append(buffer);
}

void PipelineAddImage(Pipeline *pipeline, Image *image) {
	pipeline->images.Append(image);
}

void PipelineAddVertexInputs(Pipeline *pipeline, ArrayWithBucket<ShaderValueType, 8> inputs) {
	pipeline->vertexInputs.Append(inputs);
}

void PipelineSetBlendMode(Pipeline *pipeline, BlendMode blendMode) {
	pipeline->blendMode = blendMode;
}

#endif

#ifndef Context

Result<VoidResult_t, String> ContextInit(Context *context) {
	AzAssert(!context->initted, "Trying to init a context that's already initted");
	VkCommandPoolCreateInfo poolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	poolCreateInfo.queueFamilyIndex = context->device->queueFamilyIndex;
	if (VkResult result = vkCreateCommandPool(context->device->vkDevice, &poolCreateInfo, nullptr, &context->vkCommandPool); result != VK_SUCCESS) {
		return Stringify("Context \"", context->tag, "\": Failed to create command pool: ", ErrorString(result));
	}
	VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	// We'll use signaled to mean not executing
	if (VkResult result = vkCreateFence(context->device->vkDevice, &fenceInfo, nullptr, &context->vkFence); result != VK_SUCCESS) {
		vkDestroyCommandPool(context->device->vkDevice, context->vkCommandPool, nullptr);
		return Stringify("Context \"", context->tag, "\": Failed to create fence: ", ErrorString(result));
	}
	context->initted = true;
	return VoidResult_t();
}

void ContextDeinit(Context *context) {
	AzAssert(context->initted, "Trying to deinit a context that's not initted");
	vkDestroyCommandPool(context->device->vkDevice, context->vkCommandPool, nullptr);
	vkDestroyFence(context->device->vkDevice, context->vkFence, nullptr);
	context->initted = false;
}

void ContextResetBindings(Context *context) {
	context->bindings.framebuffer = nullptr;
	context->bindings.pipeline = nullptr;
	context->bindings.vertexBuffer = nullptr;
	context->bindings.indexBuffer = nullptr;
	context->bindings.descriptors.Clear();
	context->bindCommands.ClearSoft();
	context->state = Context::State::RECORDING_PRIMARY;
}

Result<VoidResult_t, String> ContextBeginRecording(Context *context) {
	AzAssert(context->initted, "Trying to record to a Context that's not initted");
	if ((u32)context->state >= (u32)Context::State::RECORDING_PRIMARY) {
		return Stringify("Context \"", context->tag, "\" error: Cannot begin recording on a command buffer that's already recording");
	}
	ContextResetBindings(context);
	
	if (context->state == Context::State::DONE_RECORDING) {
		vkFreeCommandBuffers(context->device->vkDevice, context->vkCommandPool, 1, &context->vkCommandBuffer);
	}
	
	VkCommandBufferAllocateInfo bufferAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	bufferAllocInfo.commandPool = context->vkCommandPool;
	bufferAllocInfo.commandBufferCount = 1;
	bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	if (VkResult result = vkAllocateCommandBuffers(context->device->vkDevice, &bufferAllocInfo, &context->vkCommandBuffer); result != VK_SUCCESS) {
		return Stringify("Context \"", context->tag, "\" error: Failed to allocate primary command buffer: ", ErrorString(result));
	}
	VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	if (VkResult result = vkBeginCommandBuffer(context->vkCommandBuffer, &beginInfo); result != VK_SUCCESS) {
		return Stringify("Context \"", context->tag, "\" error: Failed to begin primary command buffer: ", ErrorString(result));
	}
	context->state = Context::State::RECORDING_PRIMARY;
	return VoidResult_t();
}

Result<VoidResult_t, String> ContextBeginRecordingSecondary(Context *context, Framebuffer *framebuffer, i32 subpass) {
	AzAssert(context->initted, "Trying to record to a Context that's not initted");
	if ((u32)context->state >= (u32)Context::State::RECORDING_PRIMARY) {
		return Stringify("Context \"", context->tag, "\" error: Cannot begin recording on a command buffer that's already recording");
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
		return Stringify("Context \"", context->tag, "\" error: Failed to allocate secondary command buffer: ", ErrorString(result));
	}
	VkCommandBufferInheritanceInfo inheritanceInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
	if (framebuffer != nullptr) {
		AzAssert(framebuffer->initted, "Trying to use a Framebuffer that isn't initted for recording a secondary command buffer");
		inheritanceInfo.renderPass = framebuffer->vkRenderPass;
		inheritanceInfo.subpass = subpass;
		inheritanceInfo.framebuffer = framebuffer->vkFramebuffer;
	}
	VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	beginInfo.flags = framebuffer != nullptr ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0;
	beginInfo.pInheritanceInfo = &inheritanceInfo;
	if (VkResult result = vkBeginCommandBuffer(context->vkCommandBuffer, &beginInfo); result != VK_SUCCESS) {
		return Stringify("Context \"", context->tag, "\" error: Failed to begin secondary command buffer: ", ErrorString(result));
	}
	context->state = Context::State::RECORDING_SECONDARY;
	return VoidResult_t();
}

Result<VoidResult_t, String> ContextEndRecording(Context *context) {
	AzAssert(context->initted, "Context not initted");
	if (!ContextIsRecording(context)) {
		return Stringify("Context \"", context->tag, "\" error: Trying to End Recording but we haven't started recording.");
	}
	if (VkResult result = vkEndCommandBuffer(context->vkCommandBuffer); result != VK_SUCCESS) {
		return Stringify("Context \"", context->tag, "\" error: Failed to End Recording: ", ErrorString(result));
	}
	context->state = Context::State::DONE_RECORDING;
	return VoidResult_t();
}

Result<VoidResult_t, String> SubmitCommands(Context *context) {
	if (context->state != Context::State::DONE_RECORDING) {
		return Stringify("Context \"", context->tag, "\" error: Trying to SubmitCommands without anything recorded.");
	}
	VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &context->vkCommandBuffer;
	if (VkResult result = vkQueueSubmit(context->device->vkQueue, 1, &submitInfo, context->vkFence); result != VK_SUCCESS) {
		return Stringify("Context \"", context->tag, "\" error: Failed to submit to queue: ", ErrorString(result));
	}
	return VoidResult_t();
}

Result<bool, String> ContextIsExecuting(Context *context) {
	AzAssert(context->initted, "Context is not initted");
	VkResult result = vkGetFenceStatus(context->device->vkDevice, context->vkFence);
	switch (result) {
		case VK_SUCCESS:
			return false;
		case VK_NOT_READY:
			return true;
		case VK_ERROR_DEVICE_LOST:
			return Stringify("Device \"", context->device->tag, "\" error: Device Lost");
		default:
			return Stringify("Context \"", context->tag, "\" error: IsExecuting returned ", ErrorString(result));
	}
}

Result<bool, String> ContextWaitUntilFinished(Context *context, Nanoseconds timeout) {
	AzAssert(context->initted, "Context is not initted");
	AzAssert(timeout.count() >= 0, "Cannot have negative timeout");
	VkResult result = vkWaitForFences(context->device->vkDevice, 1, &context->vkFence, VK_TRUE, (u64)timeout.count());
	switch (result) {
		case VK_SUCCESS:
			return false;
		case VK_TIMEOUT:
			return true;
		case VK_ERROR_DEVICE_LOST:
			return Stringify("Device \"", context->device->tag, "\" error: Device Lost");
		default:
			return Stringify("Context \"", context->tag, "\" error: WaitUntilFinished returned ", ErrorString(result));
	}
}

#endif

#ifndef Commands

Result<VoidResult_t, String> CmdExecuteSecondary(Context *primary, Context *secondary) {
	return String("Unimplemented");
}

Result<VoidResult_t, String> CmdCopyDataToBuffer(Context *context, Buffer *dst, void *src, i64 dstOffset, i64 size) {
	AzAssert(size+dstOffset <= (i64)dst->memoryRequirements.size, "size is bigger than our buffer");
	AzAssert(ContextIsRecording(context), "Trying to record into a context that's not recording");
	if (!dst->hostVisible) {
		if (auto result = BufferHostInit(dst); result.isError) {
			return result.error;
		}
	}
	Allocation alloc = dst->allocHostVisible;
	VkDeviceMemory vkMemory = alloc.memory->pages[alloc.page].vkMemory;
	void *dstMapped;
	if (VkResult result = vkMapMemory(dst->device->vkDevice, vkMemory, align(alloc.offset, dst->memoryRequirements.alignment)+dstOffset, size, 0, &dstMapped); result != VK_SUCCESS) {
		return Stringify("Buffer \"", dst->tag, "\" error: Failed to map memory: ", ErrorString(result));
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

AccessAndStage AccessAndStageFromImageLayout(VkImageLayout layout) {
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

void CmdImageTransitionLayout(Context *context, Image *image, VkImageLayout from, VkImageLayout to, VkImageSubresourceRange subresourceRange) {
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

void CmdImageTransitionLayout(Context *context, Image *image, VkImageLayout from, VkImageLayout to, u32 baseMipLevel=0, u32 mipLevelCount=1) {
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = image->vkImageAspect;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;
	subresourceRange.baseMipLevel = baseMipLevel;
	subresourceRange.levelCount = mipLevelCount;

	CmdImageTransitionLayout(context, image, from, to, subresourceRange);
}

void CmdImageGenerateMipmaps(Context *context, Image *image, VkImageLayout startingLayout, VkImageLayout finalLayout) {
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
		vkCmdBlitImage(context->vkCommandBuffer, image->vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);
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
	Allocation alloc = dst->allocHostVisible;
	VkDeviceMemory vkMemory = alloc.memory->pages[alloc.page].vkMemory;
	void *dstMapped;
	if (VkResult result = vkMapMemory(dst->device->vkDevice, vkMemory, align(alloc.offset, dst->memoryRequirements.alignment), dst->memoryRequirements.size, 0, &dstMapped); result != VK_SUCCESS) {
		return Stringify("Image \"", dst->tag, "\" error: Failed to map memory: ", ErrorString(result));
	}
	memcpy(dstMapped, src, dst->memoryRequirements.size);
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
	if (dst->sampledStages) {
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
	AzAssert(false, "Unimplemented");
}

void CmdBindPipeline(Context *context, Pipeline *pipeline) {
	AzAssert(false, "Unimplemented");
}

void CmdBindVertexBuffer(Context *context, Buffer *buffer) {
	AzAssert(false, "Unimplemented");
}

void CmdBindIndexBuffer(Context *context, Buffer *buffer) {
	AzAssert(false, "Unimplemented");
}

void CmdBindUniformBuffer(Context *context, Buffer *buffer, i32 set, i32 binding) {
	AzAssert(false, "Unimplemented");
}

void CmdBindStorageBuffer(Context *context, Buffer *buffer, i32 set, i32 binding) {
	AzAssert(false, "Unimplemented");
}

void CmdBindImageSampler(Context *context, Image *image, i32 set, i32 binding) {
	AzAssert(false, "Unimplemented");
}

Result<VoidResult_t, String> CmdCommitBindings(Context *context) {
	return String("Unimplemented");
}

void CmdDrawIndexed(Context *context, i32 count, i32 indexOffset, i32 vertexOffset, i32 instanceCount, i32 instanceOffset) {
	AzAssert(false, "Unimplemented");
}

#endif

} // namespace AzCore::GPU
