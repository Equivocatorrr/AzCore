/*
    File: vk.cpp
    Author: Philip Haynes
*/
#include "vk.hpp"
#ifdef IO_FOR_VULKAN
    #include "io.hpp"
#endif
#include "log_stream.hpp"
#include <cstring>

namespace vk {

    String error = "No Error";

    io::logStream cout("vk.log");

    String ErrorString(VkResult errorCode) {
        // Thanks to Sascha Willems for this snippet! :D
		switch (errorCode) {
#define STR(r) case VK_ ##r: return #r
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
			STR(ERROR_SURFACE_LOST_KHR);
			STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
			STR(SUBOPTIMAL_KHR);
			STR(ERROR_OUT_OF_DATE_KHR);
			STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
			STR(ERROR_VALIDATION_FAILED_EXT);
			STR(ERROR_INVALID_SHADER_NV);
#undef STR
		default:
			return "UNKNOWN_ERROR";
		}
	}

    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objType, u64 obj, size_t location,
        i32 code, const char* layerPrefix, const char* msg, void* userData) {

        cout << "layer(" << layerPrefix << "):\n" << msg << "\n" << std::endl;
        return VK_FALSE;
    }

    bool PhysicalDevice::Initialize(VkInstance instance) {
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        vkGetPhysicalDeviceFeatures(physicalDevice, &features);

        u32 extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        extensionsAvailable.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensionsAvailable.data());

        u32 queueFamiliesCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);
        queueFamiliesAvailable.resize(queueFamiliesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, queueFamiliesAvailable.data());

        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        score = 0;

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        score += properties.limits.maxImageDimension2D;
        return true;
    }

    bool PhysicalDevice::PrintInfo(VkSurfaceKHR surface, bool checkSurface) {
        // Basic info
        cout << "Name: " << properties.deviceName
            << "\nVulkan: "
            << VK_VERSION_MAJOR(properties.apiVersion) << "."
            << VK_VERSION_MINOR(properties.apiVersion) << "."
            << VK_VERSION_PATCH(properties.apiVersion) << std::endl;
        // Memory
        u64 deviceLocalMemory = 0;
        for (u32 i = 0; i < memoryProperties.memoryHeapCount; i++) {
            if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                deviceLocalMemory += memoryProperties.memoryHeaps[i].size;
        }
        cout << "Memory: " << deviceLocalMemory/1024/1024 << "MB" << std::endl;
        // Queue families
        cout << "Queue Families:";
        for (u32 i = 0; i < queueFamiliesAvailable.size(); i++) {
            const VkQueueFamilyProperties &props = queueFamiliesAvailable[i];
            cout << "\n\tFamily[" << i << "] Queue count: " << props.queueCount
                << "\tSupports: "
                << ((props.queueFlags & VK_QUEUE_COMPUTE_BIT) ? "COMPUTE " : "")
                << ((props.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "GRAPHICS " : "")
                << ((props.queueFlags & VK_QUEUE_TRANSFER_BIT) ? "TRANSFER " : "");
            if (checkSurface) {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                if (presentSupport)
                    cout << "PRESENT";
            }
        }
        cout << std::endl;
        return true;
    }

    Instance::Instance() {
        type = INSTANCE;
        u32 extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        extensionsAvailable.resize(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionsAvailable.data());
        u32 layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        layersAvailable.resize(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layersAvailable.data());
    }

    Instance::~Instance() {
        if (initted) {
            if (!Deinitialize()) {
                cout << "Failed to clean up vk::Instance: " << error << std::endl;
            }
        }
    }

    bool Instance::AppInfo(const char *name, u32 versionMajor, u32 versionMinor, u32 versionPatch) {
        appInfo.pApplicationName = name;
        appInfo.applicationVersion = VK_MAKE_VERSION(versionMajor, versionMinor, versionPatch);
        if (initted) {
            // Should we bother? It only really makes sense to call this at the beginning
            // and it won't change anything about the renderer itself...
            // Oh well, let's fire a warning.
            cout << "Warning: vk::Instance::AppInfo should be used before initializing." << std::endl;
            reconfigured = true;
        }
        return true;
    }

    bool Instance::SetWindowForSurface(io::Window *window) {
        useSurface = ((surfaceWindow = window) != nullptr);
        return true;
    }

    bool Instance::AddExtensions(Array<const char*> extensions) {
        for (u32 i = 0; i < extensions.size(); i++) {
            extensionsRequired.push_back(extensions[i]);
        }
        if (initted) {
            reconfigured = true;
        }
        return true;
    }

    bool Instance::AddLayers(Array<const char*> layers) {
        if (layers.size() > 0)
            enableLayers = true;
        for (u32 i = 0; i < layers.size(); i++) {
            layersRequired.push_back(layers[i]);
        }
        if (initted) {
            reconfigured = true;
        }
        return true;
    }

    bool Instance::Reconfigure() {
        if (!reconfigured) {
            error = "Nothing to reconfigure";
            return false;
        }
        if (initted) {
            if (!Deinitialize())
                return false;
            if (!Initialize())
                return false;
        }
        return true;
    }

    bool Instance::Initialize() {
        cout << "--------Initializing Vulkan Tree--------" << std::endl;
        if (initted) {
            error = "Tree is already initialized!";
            return false;
        }
        // Put together all needed extensions.
        Array<const char*> extensionsAll(extensionsRequired);
        if (enableLayers) {
            extensionsAll.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }
        if (useSurface) {
            extensionsAll.push_back("VK_KHR_surface");
#ifdef __unix
            extensionsAll.push_back("VK_KHR_xcb_surface");
#elif defined(_WIN32)
            extensionsAll.push_back("VK_KHR_win32_surface");
#endif
        }
        // Check required extensions
        Array<const char*> extensionsUnavailable(extensionsAll);
        for (i32 i = 0; i < (i32)extensionsUnavailable.size(); i++) {
            for (i32 j = 0; j < (i32)extensionsAvailable.size(); j++) {
                if (strcmp(extensionsUnavailable[i], extensionsAvailable[j].extensionName) == 0) {
                    extensionsUnavailable.erase(extensionsUnavailable.begin() + i);
                    i--;
                    break;
                }
            }
        }
        if (extensionsUnavailable.size() > 0) {
            error = "Instance extensions unavailable:";
            for (const char *extension : extensionsUnavailable) {
                error += "\n\t";
                error += extension;
            }
            return false;
        }
        // Check required layers
        Array<const char*> layersUnavailable(layersRequired);
        for (u32 i = 0; i < layersAvailable.size(); i++) {
            for (u32 j = 0; j < layersRequired.size(); j++) {
                if (strcmp(layersRequired[j], layersAvailable[i].layerName) == 0) {
                    layersUnavailable.erase(layersUnavailable.begin() + i);
                    i--;
                }
            }
        }
        if (layersUnavailable.size() > 0) {
            error = "Instance layers unavailable:";
            for (const char *layer : layersUnavailable) {
                error += "\n\t";
                error += layer;
            }
            return false;
        }
        // Create the instance
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = extensionsAll.size();
        createInfo.ppEnabledExtensionNames = extensionsAll.data();

        if (enableLayers) {
            createInfo.enabledLayerCount = layersRequired.size();
            createInfo.ppEnabledLayerNames = layersRequired.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            error = "vkCreateInstance failed with error: ";
            error += ErrorString(result);
            return false;
        }
        if (enableLayers) {
            // Use our debug report extension
            fpCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)
                    vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
            if (fpCreateDebugReportCallbackEXT == nullptr) {
                error = "vkGetInstanceProcAddr failed to get vkCreateDebugReportCallbackEXT";
                vkDestroyInstance(instance, nullptr);
                return false;
            }
            fpDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)
                    vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
            if (fpDestroyDebugReportCallbackEXT == nullptr) {
                error = "vkGetInstanceProcAddr failed to get vkDestroyDebugReportCallbackEXT";
                vkDestroyInstance(instance, nullptr);
                return false;
            }
            VkDebugReportCallbackCreateInfoEXT debugInfo;
            debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            debugInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
            debugInfo.pfnCallback = debugCallback;

            result = fpCreateDebugReportCallbackEXT(instance, &debugInfo, nullptr, &debugReportCallback);
        }
        // Create a surface if we want one
#ifdef IO_FOR_VULKAN
        if (useSurface) {
            if (!surfaceWindow->CreateVkSurface(this, &surface)) {
                error = "Failed to CreateVkSurface!";
                vkDestroyInstance(instance, nullptr);
                return false;
            }
        }
#endif
        // Get our list of physical devices
        u32 physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
        if (physicalDeviceCount == 0) {
            error = "Failed to find GPUs with Vulkan support";
            vkDestroyInstance(instance, nullptr);
            return false;
        }
        Array<VkPhysicalDevice> devices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, devices.data());
        // Sort them by score while adding them to our permanent list
        for (u32 i = 0; i < physicalDeviceCount; i++) {
            PhysicalDevice temp;
            temp.physicalDevice = devices[i];
            if (!temp.Initialize(instance)) {
                vkDestroyInstance(instance, nullptr);
                return false;
            }
            u32 spot;
            for (spot = 0; spot < physicalDevices.size(); spot++) {
                if (temp.score > physicalDevices[spot].score) {
                    break;
                }
            }
            physicalDevices.insert(physicalDevices.begin() + spot, temp);
        }
        cout << "Physical Devices:";
        for (u32 i = 0; i < physicalDeviceCount; i++) {
            cout << "\n\tDevice #" << i << "\n";
            if (!physicalDevices[i].PrintInfo(surface, useSurface)) {
                vkDestroyInstance(instance, nullptr);
                return false;
            }
        }

        // Tell everything else to initialize here
        // If it fails, clean up the instance.
        initted = true;
        return true;
    }

    bool Instance::Deinitialize() {
        cout << "---------Destroying Vulkan Tree---------" << std::endl;
        if (!initted) {
            error = "Tree isn't initialized!";
            return false;
        }
        // Clean up everything else here
#ifdef IO_FOR_VULKAN
        if (useSurface) {
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }
#endif
        if (enableLayers) {
            fpDestroyDebugReportCallbackEXT(instance, debugReportCallback, nullptr);
        }
        vkDestroyInstance(instance, nullptr);
        initted = false;
        return true;
    }

}
