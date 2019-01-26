/*
    File: vk.cpp
    Author: Philip Haynes
*/
#include "vk.hpp"
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

    Instance::Instance() {
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
            if (!DestroyAll()) {
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
            if (!DestroyAll())
                return false;
            if (!CreateAll())
                return false;
        }
        return true;
    }

    bool Instance::CreateAll() {
        cout << "--------Initializing Vulkan Tree--------" << std::endl;
        if (initted) {
            error = "Tree is already initialized!";
            return false;
        }
        if (enableLayers) {
            bool needToAdd = true;
            /* Since we might call CreateAll multiple times we
               should avoid adding this extension multiple times. */
            for (u32 i = extensionsRequired.size()-1; i >= 0; i--) {
                if (strcmp(extensionsRequired[i], VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
                    needToAdd = false;
                    break;
                }
            }
            if (needToAdd) {
                extensionsRequired.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            }
        }
        // Check required extensions
        Array<const char*> extensionsUnavailable(extensionsRequired);
        for (u32 i = 0; i < extensionsAvailable.size(); i++) {
            for (u32 j = 0; j < extensionsRequired.size(); j++) {
                if (strcmp(extensionsRequired[j], extensionsAvailable[i].extensionName) == 0) {
                    extensionsUnavailable.erase(extensionsUnavailable.begin() + i);
                    i--;
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
        VkInstanceCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = extensionsRequired.size();
        createInfo.ppEnabledExtensionNames = extensionsRequired.data();

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

            result = fpCreateDebugReportCallbackEXT(instance, &debugInfo, nullptr, &callback);
        }
        // Tell everything else to initialize here
        // If it fails, clean up the instance.
        initted = true;
        return true;
    }

    bool Instance::DestroyAll() {
        cout << "---------Destroying Vulkan Tree---------" << std::endl;
        if (!initted) {
            error = "Tree isn't initialized!";
            return false;
        }
        // Clean up everything else here
        if (enableLayers) {
            fpDestroyDebugReportCallbackEXT(instance, callback, nullptr);
        }
        vkDestroyInstance(instance, nullptr);
        initted = false;
        return true;
    }

}
