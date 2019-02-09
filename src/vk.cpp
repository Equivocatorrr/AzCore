/*
    File: vk.cpp
    Author: Philip Haynes
*/
#include "vk.hpp"
// #ifdef IO_FOR_VULKAN
    #include "io.hpp"
// #endif
#include "log_stream.hpp"
#include <cstring>

namespace vk {

    String error = "No Error";

    io::logStream cout("vk.log");

    const char *QueueTypeString[5] = {
        "UNDEFINED",
        "COMPUTE",
        "GRAPHICS",
        "TRANSFER",
        "PRESENT"
    };

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

    bool PhysicalDevice::Init(VkInstance instance) {
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

    void PhysicalDevice::PrintInfo(Array<Window> windows, bool checkSurface) {
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
                String presentString = "PRESENT on windows {";
                VkBool32 presentSupport = false;
                bool first = true;
                for (u32 j = 0; j < windows.size(); j++) {
                    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windows[j].surface, &presentSupport);
                    if (presentSupport) {
                        if (!first)
                            presentString += ", ";
                        presentString += std::to_string(j);
                        first = false;
                        break;
                    }
                }
                presentString += "}";
                if (!first)
                    cout << presentString;
            }
        }
        cout << std::endl;
    }

    Swapchain::Swapchain() {

    }

    Swapchain::~Swapchain() {
        if (initted) {
            if (!Deinit()) {
                cout << "Failed to clean up vk::Swapchain: " << error << std::endl;
            }
        }
    }

    bool Swapchain::Init(Device *dev) {
        cout << "----Initializing Swapchain----" << std::endl;
        if (initted) {
            error = "Swapchain is already initialized!";
            return false;
        }
        if ((device = dev) == nullptr) {
            error = "Device is nullptr!";
            return false;
        }
        if (windowIndex < 0) {
            error = "Cannot create a swapchain without a window surface!";
            return false;
        }
        if (windowIndex > (i32)device->instance->windows.size()) {
            error = "Window index is out of bounds!";
            return false;
        }
        surface = device->instance->windows[windowIndex].surface;
        // Get information about what we can or can't do
        VkPhysicalDevice physicalDevice = device->physicalDevice.physicalDevice;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
        u32 count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
        if (count != 0) {
            surfaceFormats.resize(count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, surfaceFormats.data());
        }
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
        if (count != 0) {
            presentModes.resize(count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, presentModes.data());
        }
        // We'll probably re-create the swapchain a bunch of times without a full Deinit() Init() cycle
        if (!Create()) {
            return false;
        }
        initted = true;
        return true;
    }

    bool Swapchain::Create() {
        // Choose our surface format
        {
            bool found = false;
            if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
                surfaceFormat = formatPreferred;
                found = true;
            } else {
                for (const auto& format : surfaceFormats) {
                    if (format.format == formatPreferred.format
                    && format.colorSpace == formatPreferred.colorSpace) {
                        surfaceFormat = formatPreferred;
                        found = true;
                        break;
                    }
                }
            }
            if (!found && surfaceFormats.size() > 0) {
                cout << "We couldn't use our preferred window surface format!" << std::endl;
                surfaceFormat = surfaceFormats[0];
                found = true;
            }
            if (!found) {
                error = "We don't have any surface formats to choose from!!! >:(";
                return false;
            }
        }
        // Choose our present mode
        {
            bool found = false;
            if (vsync) {
                for (const auto& mode : presentModes) {
                    if (mode == VK_PRESENT_MODE_FIFO_KHR) {
                        presentMode = mode;
                        found = true;
                        break;
                    }
                }
            } else {
                for (const auto& mode : presentModes) {
                    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                        presentMode = mode;
                        found = true;
                        break; // Ideal choice, don't keep looking
                    } else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                        presentMode = mode;
                        found = true;
                        // Acceptable choice, but keep looking
                    }
                }
            }
            if (!found && presentModes.size() > 0) {
                cout << "Our preferred present modes aren't available, but we can still do something" << std::endl;
                presentMode = presentModes[0];
                found = true;
            }
    		if (!found) {
        		error = "No adequate present modes available! ¯\\_(ツ)_/¯";
                return false;
    		}
            cout << "Present Mode: ";
            switch(presentMode) {
    			case VK_PRESENT_MODE_FIFO_KHR:
    				cout << "VK_PRESENT_MODE_FIFO_KHR" << std::endl;
    				break;
    			case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
    				cout << "VK_PRESENT_MODE_FIFO_RELAXED_KHR" << std::endl;
    				break;
    			case VK_PRESENT_MODE_MAILBOX_KHR:
    				cout << "VK_PRESENT_MODE_MAILBOX_KHR" << std::endl;
    				break;
    			case VK_PRESENT_MODE_IMMEDIATE_KHR:
    				cout << "VK_PRESENT_MODE_IMMEDIATE_KHR" << std::endl;
    				break;
    			default:
    				cout << "wtf the fuck" << std::endl;
    				break;
    		}
        }
        // Now we gotta find our extent
        if (surfaceCapabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
            extent = surfaceCapabilities.currentExtent;
        } else {
            const io::Window *window = device->instance->windows[windowIndex].surfaceWindow;
            extent = {(u32)window->width, (u32)window->height};

            extent.width = max(surfaceCapabilities.minImageExtent.width,
                           min(surfaceCapabilities.maxImageExtent.width, extent.width));
            extent.height = max(surfaceCapabilities.minImageExtent.height,
                            min(surfaceCapabilities.maxImageExtent.height, extent.height));
        }
        imageCount = max(surfaceCapabilities.minImageCount, min(surfaceCapabilities.maxImageCount, imageCountPreferred));
        cout << "Swapchain will use " << imageCount << " images" << std::endl;
        // Put it all together
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = usage;
        // TODO: Inherit imageUsage from ???

        // Queue family sharing...ugh
        Array<u32> queueFamilies{};
        for (u32 i = 0; i < device->queues.size(); i++) {
            bool found = false;
            for (u32 j = 0; j < queueFamilies.size(); j++) {
                if (device->queues[i].queueFamilyIndex == (i32)queueFamilies[i]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                queueFamilies.push_back(device->queues[i].queueFamilyIndex);
            }
        }
        if (queueFamilies.size() > 1) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = queueFamilies.size();
            createInfo.pQueueFamilyIndices = queueFamilies.data();
            cout << "Swapchain image sharing mode is concurrent" << std::endl;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
            cout << "Swapchain image sharing mode is exclusive" << std::endl;
        }
        createInfo.preTransform = surfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        if (created) {
            VkSwapchainKHR oldSwapchain = swapchain;
            createInfo.oldSwapchain = oldSwapchain;
        }

		VkSwapchainKHR newSwapchain;
        VkResult result = vkCreateSwapchainKHR(device->device, &createInfo, nullptr, &newSwapchain);
        if (result != VK_SUCCESS) {
            error = "Failed to create swap chain: ";
            error += ErrorString(result);
            return false;
        }
        if (created) {
            vkDestroySwapchainKHR(device->device, swapchain, nullptr);
        }
        swapchain = newSwapchain;

        // Get our images
        vkGetSwapchainImagesKHR(device->device, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device->device, swapchain, &imageCount, swapchainImages.data());

        created = true;
        return true;
    }

    bool Swapchain::Reconfigure() {
        if (initted) {
            if (!Create())
                return false;
        }
        return true;
    }

    bool Swapchain::Deinit() {
        if (!initted) {
            error = "Swapchain isn't initialized!";
            return false;
        }
        vkDestroySwapchainKHR(device->device, swapchain, nullptr);
        initted = false;
        created = false;
        return true;
    }

    Device::Device() {
        // type = LOGICAL_DEVICE;
    }

    Device::~Device() {
        if (initted) {
            if (!Deinit()) {
                cout << "Failed to clean up vk::Device: " << error << std::endl;
            }
        }
    }

    u32 Device::AddQueue(Queue queue) {
        queues.push_back(queue);
        return queues.size() - 1;
    }

    Queue* Device::GetQueue(u32 index) {
        if (index >= queues.size()) {
            error = "Device::GetQueue index out of bounds";
            return nullptr;
        }
        return &queues[index];
    }

    u32 Device::AddSwapchain(Swapchain swapchain) {
        swapchains.push_back(swapchain);
        return swapchains.size() - 1;
    }

    Swapchain* Device::GetSwapchain(u32 index) {
        if (index >= swapchains.size()) {
            error = "Device::GetSwapchain index out of bounds";
            return nullptr;
        }
        return &swapchains[index];
    }

    bool Device::Init(Instance *inst) {
        cout << "------Initializing Logical Device-------" << std::endl;
        if (initted) {
            error = "Device is already initialized!";
            return false;
        }
        if ((instance = inst) == nullptr) {
            error = "Instance is nullptr!";
            return false;
        }

        // Select physical device first based on needs.
        // TODO: Right now we just choose the first in the pre-sorted list. We should instead select
        //       them based on whether they have our desired features.
        physicalDevice = instance->physicalDevices[0];

        // Put together all our needed extensions
        Array<const char*> extensionsAll(extensionsRequired);

        // TODO: Find out what extensions we need based on context

        // Verify that our device extensions are available
        Array<const char*> extensionsUnavailable(extensionsAll);
        for (i32 i = 0; i < (i32)extensionsUnavailable.size(); i++) {
            for (i32 j = 0; j < (i32)physicalDevice.extensionsAvailable.size(); j++) {
                if (strcmp(extensionsUnavailable[i], physicalDevice.extensionsAvailable[j].extensionName) == 0) {
                    extensionsUnavailable.erase(extensionsUnavailable.begin() + i);
                    i--;
                    break;
                }
            }
        }
        if (extensionsUnavailable.size() > 0) {
            error = "Device extensions unavailable:";
            for (const char *extension : extensionsUnavailable) {
                error += "\n\t";
                error += extension;
            }
            return false;
        }

        VkPhysicalDeviceFeatures deviceFeatures;
        // I'm not sure why these aren't bit-masked values,
        // but I'm treating it like that anyway.
        for (u32 i = 0; i < sizeof(VkPhysicalDeviceFeatures)/4; i++) {
            *(((u32*)&deviceFeatures + i)) = *(((u32*)&deviceFeaturesRequired + i))
            || (*(((u32*)&physicalDevice.features + i)) && *(((u32*)&deviceFeaturesOptional + i)));
        }

        // Set up queues
        // First figure out which queue families each queue should use
        const bool preferSameQueueFamilies = true;

        // Make sure we have enough queues in every family
        u32 queueFamilies = physicalDevice.queueFamiliesAvailable.size();
        Array<u32> queuesPerFamily(queueFamilies);
        for (u32 i = 0; i < queueFamilies; i++) {
            queuesPerFamily[i] = physicalDevice.queueFamiliesAvailable[i].queueCount;
        }

        for (u32 i = 0; i < queues.size(); i++) {
            for (u32 j = 0; j < queueFamilies; j++) {
                if (queuesPerFamily[j] == 0)
                    continue; // This family has been exhausted of queues, try the next.
                VkQueueFamilyProperties& props = physicalDevice.queueFamiliesAvailable[j];
                if (props.queueCount == 0)
                    continue;
                VkBool32 presentSupport = VK_FALSE;
                for (const Window& w : instance->windows) {
                    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice.physicalDevice, j, w.surface, &presentSupport);
                    if (presentSupport)
                        break;
                }
                switch(queues[i].queueType) {
                    case COMPUTE: {
                        if (props.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                            queues[i].queueFamilyIndex = j;
                        }
                        break;
                    }
                    case GRAPHICS: {
                        if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                            queues[i].queueFamilyIndex = j;
                        }
                        break;
                    }
                    case TRANSFER: {
                        if (props.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                            queues[i].queueFamilyIndex = j;
                        }
                        break;
                    }
                    case PRESENT: {
                        if (presentSupport) {
                            queues[i].queueFamilyIndex = j;
                        }
                        break;
                    }
                    default: {
                        error = "queues[";
                        error += std::to_string(i);
                        error += "] has a QueueType of UNDEFINED!";
                        return false;
                    }
                }
                if (preferSameQueueFamilies && queues[i].queueFamilyIndex != -1)
                    break;
            }
            if (queues[i].queueFamilyIndex == -1) {
                error = "queues[";
                error += std::to_string(i);
                error += "] couldn't find a queue family :(";
                return false;
            }
            queuesPerFamily[queues[i].queueFamilyIndex]--;
        }

        Array<VkDeviceQueueCreateInfo> queueCreateInfos{};
        Array<Array<f32>> queuePriorities(queueFamilies);
        for (u32 i = 0; i < queueFamilies; i++) {
            for (u32 j = 0; j < queues.size(); j++) {
                if (queues[j].queueFamilyIndex == (i32)i) {
                    queuePriorities[i].push_back(queues[j].queuePriority);
                }
            }
        }
        for (u32 i = 0; i < queueFamilies; i++) {
            cout << "Allocating " << queuePriorities[i].size() << " queues from family " << i << std::endl;
            if (queuePriorities[i].size() != 0) {
                VkDeviceQueueCreateInfo queueCreateInfo = {};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = i;
                queueCreateInfo.queueCount = queuePriorities[i].size();
                queueCreateInfo.pQueuePriorities = queuePriorities[i].data();
                queueCreateInfos.push_back(queueCreateInfo);
            }
        }


        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = queueCreateInfos.size();

        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = extensionsAll.size();
        createInfo.ppEnabledExtensionNames = extensionsAll.data();

        if (instance->enableLayers) {
            createInfo.enabledLayerCount = instance->layersRequired.size();
            createInfo.ppEnabledLayerNames = instance->layersRequired.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        VkResult result = vkCreateDevice(physicalDevice.physicalDevice, &createInfo, nullptr, &device);
        if (result != VK_SUCCESS) {
            error = "Failed to create logical device: ";
            error += ErrorString(result);
            return false;
        }
        // Get our queues
        for (u32 i = 0; i < queueFamilies; i++) {
            u32 queueIndex = 0;
            for (u32 j = 0; j < queues.size(); j++) {
                if (queues[j].queueFamilyIndex == (i32)i) {
                    vkGetDeviceQueue(device, i, queueIndex++, &queues[j].queue);
                }
            }
        }
        // Swapchains
        for (u32 i = 0; i < swapchains.size(); i++) {
            if (!swapchains[i].Init(this)) {
                vkDestroyDevice(device, nullptr);
                return false;
            }
        }
        // Init everything else here
        initted = true;
        return true;
    }

    bool Device::Deinit() {
        cout << "--------Destroying Logical Device-------" << std::endl;
        if (!initted) {
            error = "Device isn't initialized!";
            return false;
        }
        for (u32 i = 0; i < swapchains.size(); i++) {
            if (!swapchains[i].Deinit()) {
                return false;
            }
        }
        // Destroy everything allocated from the device here
        vkDestroyDevice(device, nullptr);
        initted = false;
        return true;
    }

    Instance::Instance() {
        // type = INSTANCE;
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
            if (!Deinit()) {
                cout << "Failed to clean up vk::Instance: " << error << std::endl;
            }
        }
    }

    void Instance::AppInfo(const char *name, u32 versionMajor, u32 versionMinor, u32 versionPatch) {
        appInfo.pApplicationName = name;
        appInfo.applicationVersion = VK_MAKE_VERSION(versionMajor, versionMinor, versionPatch);
        if (initted) {
            // Should we bother? It only really makes sense to call this at the beginning
            // and it won't change anything about the renderer itself...
            // Oh well, let's fire a warning.
            cout << "Warning: vk::Instance::AppInfo should be used before initializing." << std::endl;
        }
    }

    u32 Instance::AddWindowForSurface(io::Window *window) {
        Window w;
        w.surfaceWindow = window;
        windows.push_back(w);
        return windows.size()-1;
    }

    void Instance::AddExtensions(Array<const char*> extensions) {
        for (u32 i = 0; i < extensions.size(); i++) {
            extensionsRequired.push_back(extensions[i]);
        }
    }

    void Instance::AddLayers(Array<const char*> layers) {
        if (layers.size() > 0)
            enableLayers = true;
        for (u32 i = 0; i < layers.size(); i++) {
            layersRequired.push_back(layers[i]);
        }
    }

    u32 Instance::AddDevice(Device device) {
        devices.push_back(device);
        return devices.size()-1;
    }

    Device* Instance::GetDevice(u32 index) {
        if (index >= devices.size()) {
            error = "Instance::GetDevice index is out of bounds";
            return nullptr;
        }
        return &devices[index];
    }

    bool Instance::Reconfigure() {
        if (initted) {
            if (!Deinit())
                return false;
            if (!Init())
                return false;
        }
        return true;
    }

    bool Instance::Initted() const {
        return initted;
    }

    bool Instance::Init() {
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
        if (windows.size() > 0) {
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
        for (Window& w : windows) {
            if (!w.surfaceWindow->CreateVkSurface(this, &w.surface)) {
                error = "Failed to CreateVkSurface!";
                vkDestroyInstance(instance, nullptr);
                return false;
            }
        }
#endif
        {
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
                if (!temp.Init(instance)) {
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
                physicalDevices[i].PrintInfo(windows, windows.size() > 0);
            }
        }
        // Initialize our logical devices according to their rules
        bool failed = false;
        for (u32 i = 0; i < devices.size(); i++) {
            if (!devices[i].Init(this)) {
                failed = true;
                break;
            }
        }
        if (failed) {
            // Deinit will fail for uninitialized devices, so copy the last error
            String err = error;
            for (u32 i = 0; i < devices.size(); i++) {
                devices[i].Deinit();
            }
            vkDestroyInstance(instance, nullptr);
            error = err;
            return false;
        }

        // Tell everything else to initialize here
        // If it fails, clean up the instance.
        initted = true;
        return true;
    }

    bool Instance::Deinit() {
        cout << "---------Destroying Vulkan Tree---------" << std::endl;
        if (!initted) {
            error = "Tree isn't initialized!";
            return false;
        }
        for (u32 i = 0; i < devices.size(); i++) {
            devices[i].Deinit();
        }
        // Clean up everything else here
#ifdef IO_FOR_VULKAN
        for (const Window& w : windows) {
            vkDestroySurfaceKHR(instance, w.surface, nullptr);
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
