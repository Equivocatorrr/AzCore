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

    void PrintDashed(String str) {
        i32 width = 120-(i32)str.size();
        if (width > 0) {
            for (u32 i = (width+1)/2; i > 0; i--) {
                cout << "-";
            }
            cout << str;
            for (u32 i = width/2; i > 0; i--) {
                cout << "-";
            }
        } else {
            cout << str;
        }
        cout << std::endl;
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

    Image::~Image() {
        Clean();
    }

    void Image::Init(VkDevice dev) {
        device = dev;
    }

    void Image::Clean() {
		if (imageViewExists) {
			vkDestroyImageView(device, imageView, nullptr);
			imageViewExists = false;
		}
		if (imageExists) {
			vkDestroyImage(device, image, nullptr);
			imageExists = false;
		}
	}

    bool Image::CreateImage(bool hostVisible) {
        if (imageExists) {
            error = "Attempting to create image that already exists!";
            return false;
        }
		VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1; // TODO: Animations?
		imageInfo.format = format;
		imageInfo.tiling = hostVisible ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = samples;
		imageInfo.flags = 0;

        VkResult result = vkCreateImage(device, &imageInfo, nullptr, &image);
		if (result != VK_SUCCESS) {
			error = "Failed to create image: " + ErrorString(result);
			return false;
		}
		imageExists = true;
        return true;
	}

    bool Image::CreateImageView() {
        if (imageViewExists) {
            error = "Attempting to create an image view that already exists!";
            return false;
        }
		VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;
		createInfo.subresourceRange.aspectMask = aspectFlags;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = mipLevels;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device, &createInfo, nullptr, &imageView);
		if (result != VK_SUCCESS) {
            error = "Failed to create image view: " + ErrorString(result);
			return false;
		}
		imageViewExists = true;
        return true;
	}

    Buffer::~Buffer() {
        Clean();
    }

    void Buffer::Init(VkDevice dev) {
        device = dev;
    }

    bool Buffer::Create() {
        if (exists) {
            error = "Buffer already exists!";
            return false;
        }
        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.usage = usage;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(device, &createInfo, nullptr, &buffer);

        if (result != VK_SUCCESS) {
            error = "Failed to create buffer: " + ErrorString(result);
            return false;
        }
        exists = true;
        return true;
    }

    void Buffer::Clean() {
        if (exists) {
            vkDestroyBuffer(device, buffer, nullptr);
            exists = false;
        }
    }

    Sampler::~Sampler() {
        Clean();
    }

    void Sampler::Init(VkDevice dev) {
        device = dev;
    }

    bool Sampler::Create() {
        if (exists) {
            error = "Sampler already exists!";
            return false;
        }
        VkSamplerCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.magFilter = magFilter;
		createInfo.minFilter = minFilter;
		createInfo.addressModeU = addressModeU;
		createInfo.addressModeV = addressModeV;
		createInfo.addressModeW = addressModeW;
        createInfo.maxAnisotropy = anisotropy;
		if (anisotropy != 1) {
			createInfo.anisotropyEnable = VK_TRUE;
		} else {
			createInfo.anisotropyEnable = VK_FALSE;
		}
		createInfo.borderColor = borderColor;
		createInfo.unnormalizedCoordinates = (VkBool32)unnormalizedCoordinates;
		createInfo.compareOp = compareOp;
        if (compareOp == VK_COMPARE_OP_NEVER) {
            createInfo.compareEnable = VK_FALSE;
        } else {
            createInfo.compareEnable = VK_TRUE;
        }
		createInfo.mipmapMode = mipmapMode;
		createInfo.mipLodBias = mipLodBias;
		createInfo.minLod = minLod;
		createInfo.maxLod = maxLod;

        VkResult result = vkCreateSampler(device, &createInfo, nullptr, &sampler);

        if (result != VK_SUCCESS) {
            error = "Failed to create sampler: " + ErrorString(result);
            return false;
        }
        exists = true;
        return true;
    }

    void Sampler::Clean() {
        if (exists) {
            vkDestroySampler(device, sampler, nullptr);
            exists = false;
        }
    }

    DescriptorLayout::~DescriptorLayout() {
        Clean();
    }

    void DescriptorLayout::Init(VkDevice dev) {
        device = dev;
    }

    bool DescriptorLayout::Create() {
        if (exists) {
            error = "DescriptorLayout already created!";
            return false;
        }
        Array<VkDescriptorSetLayoutBinding> bindingInfo(bindings.size());
        for (u32 i = 0; i < bindingInfo.size(); i++) {
            bindingInfo[i].binding = bindings[i].binding;
            bindingInfo[i].descriptorCount = bindings[i].count;
            bindingInfo[i].descriptorType = type;
            bindingInfo[i].stageFlags = stage;
            bindingInfo[i].pImmutableSamplers = nullptr; // TODO: Use these
        }
        VkDescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = bindingInfo.size();
        createInfo.pBindings = bindingInfo.data();

        VkResult result = vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout);

        if (result != VK_SUCCESS) {
            error = "Failed to create Descriptor Set Layout: " + ErrorString(result);
            return false;
        }
        exists = true;
        return true;
    }

    void DescriptorLayout::Clean() {
        if (exists) {
            vkDestroyDescriptorSetLayout(device, layout, nullptr);
            exists = false;
        }
    }

    bool DescriptorSet::AddDescriptor(Array<Buffer> *buffers, u32 binding) {
        // TODO: Support other types of descriptors
        if (layout->type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            error = "AddDescriptor failed because layout type is not for uniform buffers!";
            return false;
        }
        for (u32 i = 0; i < layout->bindings.size(); i++) {
            if (layout->bindings[i].binding == binding) {
                if (layout->bindings[i].count != buffers->size()) {
                    error = "AddDescriptor failed because buffers Array is wrong size("
                          + std::to_string(buffers->size()) + ") for binding "
                          + std::to_string(binding) + " which expects "
                          + std::to_string(layout->bindings[i].count) + " buffers.";
                    return false;
                }
                bindings.push_back(layout->bindings[i]);
                break;
            }
        }
        bufferDescriptors.push_back({buffers});
        return true;
    }

    bool DescriptorSet::AddDescriptor(Array<Image> *images, ArrayPtr<Sampler> sampler, u32 binding) {
        // TODO: Support other types of descriptors
        if (layout->type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            error = "AddDescriptor failed because layout type is not for combined image samplers!";
            return false;
        }
        for (u32 i = 0; i < layout->bindings.size(); i++) {
            if (layout->bindings[i].binding == binding) {
                if (layout->bindings[i].count != images->size()) {
                    error = "AddDescriptor failed because images Array is wrong size("
                          + std::to_string(images->size()) + ") for binding "
                          + std::to_string(binding) + " which expects "
                          + std::to_string(layout->bindings[i].count) + " images.";
                    return false;
                }
                bindings.push_back(layout->bindings[i]);
                break;
            }
        }
        imageDescriptors.push_back({images, sampler});
        return true;
    }

    Descriptors::~Descriptors() {
        Clean();
    }

    void Descriptors::Init(VkDevice dev) {
        device = dev;
    }

    ArrayPtr<DescriptorLayout> Descriptors::AddLayout() {
        layouts.push_back(DescriptorLayout());
        return ArrayPtr<DescriptorLayout>(layouts, layouts.size()-1);
    }

    ArrayPtr<DescriptorSet> Descriptors::AddSet(ArrayPtr<DescriptorLayout> layout) {
        DescriptorSet set{};
        set.layout = layout;
        sets.push_back(set);
        return ArrayPtr<DescriptorSet>(sets, sets.size()-1);
    }

    bool Descriptors::Create() {
        PrintDashed("Creating Descriptors");
        if (exists) {
            error = "Descriptors already exist!";
            return false;
        }
        Array<VkDescriptorPoolSize> poolSizes(layouts.size());
        for (u32 i = 0; i < layouts.size(); i++) {
            if (!layouts[i].Create()) {
                error = "Failed to created descriptor set layout[" + std::to_string(i) + "]: " + error;
                Clean();
                return false;
            }
            poolSizes[i].type = layouts[i].type;
            poolSizes[i].descriptorCount = 0;
            for (u32 j = 0; j < layouts[i].bindings.size(); j++) {
                poolSizes[i].descriptorCount += layouts[i].bindings[j].count;
            }
        }

        VkDescriptorPoolCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.poolSizeCount = poolSizes.size();
        createInfo.pPoolSizes = poolSizes.data();
        createInfo.maxSets = sets.size();

        VkResult result = vkCreateDescriptorPool(device, &createInfo, nullptr, &pool);

        if (result != VK_SUCCESS) {
            error = "Failed to create Descriptor Pool: " + ErrorString(result);
            return false;
        }
        exists = true;
        cout << "Allocating Descriptor Sets..." << std::endl;
        Array<VkDescriptorSetLayout> setLayouts(sets.size());
        for (u32 i = 0; i < sets.size(); i++) {
            setLayouts[i] = sets[i].layout->layout;
        }
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = setLayouts.size();
        allocInfo.pSetLayouts = setLayouts.data();

        Array<VkDescriptorSet> setsTemp(sets.size());

        result = vkAllocateDescriptorSets(device, &allocInfo, setsTemp.data());
        if (result != VK_SUCCESS) {
            error = "Failed to allocate Descriptor Sets: " + ErrorString(result);
            Clean();
            return false;
        }
        for (u32 i = 0; i < sets.size(); i++) {
            sets[i].set = setsTemp[i];
            sets[i].exists = true;
        }

        return true;
    }

    bool Descriptors::Update() {
        // Find out how many of each we have ahead of time so we can size our arrays appropriately
        u32 totalBufferInfos = 0;
        u32 totalImageInfos = 0;

        for (u32 i = 0; i < sets.size(); i++) {
            for (u32 j = 0; j < sets[i].bufferDescriptors.size(); j++) {
                totalBufferInfos += sets[i].bufferDescriptors[j].buffers->size();
            }
            for (u32 j = 0; j < sets[i].imageDescriptors.size(); j++) {
                totalImageInfos += sets[i].imageDescriptors[j].images->size();
            }
        }

        Array<VkWriteDescriptorSet> writes{};

        Array<VkDescriptorBufferInfo> bufferInfos(totalBufferInfos);
        Array<VkDescriptorImageInfo> imageInfos(totalImageInfos);

        u32 bInfoOffset = 0;
        u32 iInfoOffset = 0;

        for (u32 i = 0; i < sets.size(); i++) {
            u32 setBufferDescriptor = 0;
            u32 setImageDescriptor = 0;
            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = sets[i].set;
            write.dstArrayElement = 0;
            write.descriptorType = sets[i].layout->type;
            for (u32 j = 0; j < sets[i].bindings.size(); j++) {
                write.dstBinding = sets[i].bindings[j].binding;
                write.descriptorCount = sets[i].bindings[j].count;

                switch(write.descriptorType) {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    for (u32 x = 0; x < sets[i].bindings[j].count; x++) {
                        VkDescriptorBufferInfo bufferInfo = {};
                        Buffer &buffer = (*sets[i].bufferDescriptors[setBufferDescriptor].buffers)[x];
                        bufferInfo.buffer = buffer.buffer;
                        bufferInfo.offset = 0;
                        bufferInfo.range = buffer.size;
                        bufferInfos[bInfoOffset++] = bufferInfo;
                    }
                    setBufferDescriptor++;
                    write.pBufferInfo = &bufferInfos[bInfoOffset - sets[i].bindings[j].count];
                    break;
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    for (u32 x = 0; x < sets[i].bindings[j].count; x++) {
                        VkDescriptorImageInfo imageInfo = {};
                        ImageDescriptor &imageDescriptor = sets[i].imageDescriptors[setImageDescriptor];
                        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        imageInfo.imageView = (*imageDescriptor.images)[x].imageView;
                        imageInfo.sampler = imageDescriptor.sampler->sampler;
                        imageInfos[iInfoOffset++] = imageInfo;
                    }
                    setImageDescriptor++;
                    write.pImageInfo = &imageInfos[iInfoOffset - sets[i].bindings[j].count];
                    break;
                default:
                    error = "Unsupported descriptor type for updating descriptors!";
                    return false;
                }
                writes.push_back(write);
            }
        }
        // Well wasn't that just a lovely mess of code?
        vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
        return true;
    }

    void Descriptors::Clean() {
        if (exists) {
            vkDestroyDescriptorPool(device, pool, nullptr);
            for (u32 i = 0; i < sets.size(); i++) {
                sets[i].exists = false;
            }
        }
        for (u32 i = 0; i < layouts.size(); i++) {
            layouts[i].Clean();
        }
    }

    Attachment::Attachment() {}

    Attachment::Attachment(Swapchain *swch) {
        swapchain = swch;
        if (swapchain != nullptr) {
            bufferColor = true;
            keepColor = true;
        }
    }

    void Attachment::Config() {
        if (swapchain != nullptr) {
            formatColor = swapchain->surfaceFormat.format;
        }
        descriptions.resize(0);
        if (bufferColor) {
            if (sampleCount != VK_SAMPLE_COUNT_1_BIT && resolveColor) {
                // SSAA enabled, first attachment should be multisampled color buffer
                VkAttachmentDescription description{};
                description.format = formatColor;
                description.samples = sampleCount;

                if (clearColor) {
                    description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                } else {
                    description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                }
                if (loadColor) {
                    description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                }

                description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

                description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

                if (loadColor) {
                    description.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                } else {
                    description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                }
                description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                descriptions.push_back(description);
                // Next attachment should be the color buffer we resolve to
                // Keep same format.
                description.samples = VK_SAMPLE_COUNT_1_BIT;

                description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                if (keepColor) {
                    description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                } else {
                    description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                }

                // We don't care about the stencil since we're a color buffer.
                description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                // We also keep the same final layout as the first attachment.
                descriptions.push_back(description);
            } else {
                // Resolving disabled or unnecessary
                VkAttachmentDescription description{};
                description.format = formatColor;
                description.samples = sampleCount;

                if (clearColor) {
                    description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                } else {
                    description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                }
                if (loadColor) {
                    description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                }
                if (keepColor) {
                    description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                } else {
                    description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                }

                description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

                description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                descriptions.push_back(description);
            }
        }
        if (bufferDepthStencil) {
            // Even with multisampling, we only have one attachment for depth/stencil
            VkAttachmentDescription description{};
            description.format = formatDepthStencil;
            description.samples = sampleCount;

            if (clearDepth) {
                description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            } else {
                description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            }
            if (loadDepth) {
                description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            }

            if (keepDepth) {
                description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            } else {
                description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            }

            if (clearStencil) {
                description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            } else {
                description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            }
            if (loadStencil) {
                description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            }

            if (keepStencil) {
                description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            } else {
                description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            }

            description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            descriptions.push_back(description);
        }
    }

    void Subpass::UseAttachment(ArrayPtr<Attachment> attachment, AttachmentType type, VkAccessFlags accessFlags) {
        AttachmentUsage usage = {attachment.index, type, accessFlags};
        attachments.push_back(usage);
    }

    ArrayPtr<Subpass> RenderPass::AddSubpass() {
        subpasses.push_back(Subpass());
        return ArrayPtr<Subpass>(subpasses, subpasses.size()-1);
    }

    ArrayPtr<Attachment> RenderPass::AddAttachment(Swapchain *swapchain) {
        attachments.push_back(Attachment(swapchain));
        return ArrayPtr<Attachment>(attachments, attachments.size()-1);
    }

    RenderPass::~RenderPass() {
        if (initted) {
            if (!Deinit()) {
                cout << "Failed to clean up vk::RenderPass: " << error << std::endl;
            }
        }
    }

    bool RenderPass::Init(Device *dev) {
        PrintDashed("Initializing RenderPass");
        if (initted) {
            error = "Renderpass is already initialized!";
            return false;
        }
        if ((device = dev) == nullptr) {
            error = "Device is nullptr!";
            return false;
        }
        if (subpasses.size() == 0) {
            error = "You must have at least 1 subpass in your renderpass!";
            return false;
        }
        // First we need to configure our subpass attachments
        for (u32 i = 0; i < attachments.size(); i++) {
            attachments[i].Config();
        }
        // First we concatenate our attachmentDescriptions from the Attachments
        u32 nextAttachmentIndex = 0; // Since each Attachment can map to multiple attachments
        for (u32 i = 0; i < attachments.size(); i++) {
            attachments[i].firstIndex = nextAttachmentIndex;
            attachmentDescriptions.resize(nextAttachmentIndex + attachments[i].descriptions.size());
            for (u32 x = 0; x < attachments[i].descriptions.size(); x++) {
                attachmentDescriptions[nextAttachmentIndex++] = attachments[i].descriptions[x];
            }
        }
        for (u32 i = 0; i < subpasses.size(); i++) {
            bool depthStencilTaken = false; // Verify that we only have one depth/stencil attachment
            Subpass& subpass = subpasses[i];
            subpass.referencesColor.resize(0);
            subpass.referencesResolve.resize(0);
            subpass.referencesInput.resize(0);
            subpass.referencesPreserve.resize(0);

            for (u32 j = 0; j < subpass.attachments.size(); j++) {
                String errorPrefix = "Subpass[" + std::to_string(i) + "] AttachmentUsage[" + std::to_string(j) + "] ";
                AttachmentUsage& usage = subpass.attachments[j];
                if (usage.index >= attachments.size()) {
                    error = errorPrefix + "index is out of bounds: " + std::to_string(usage.index);
                    return false;
                }
                Attachment& attachment = attachments[usage.index];
                nextAttachmentIndex = attachment.firstIndex;
                i32 colorIndex = -1;
                i32 resolveIndex = -1;
                i32 depthStencilIndex = -1;
                if (attachment.bufferColor) {
                    colorIndex = nextAttachmentIndex++;
                    if (attachment.resolveColor && attachment.sampleCount != VK_SAMPLE_COUNT_1_BIT) {
                        resolveIndex = nextAttachmentIndex++;
                    }
                }
                if (attachment.bufferDepthStencil) {
                    if (depthStencilTaken) {
                        error = errorPrefix + "defines a second depth/stencil attachment. "
                                "You can't have more than one depth/stencil attachment in a single subpass!";
                        return false;
                    }
                    depthStencilTaken = true;
                    depthStencilIndex = nextAttachmentIndex++;
                }
                u32 index = 0;
                if (usage.type == ATTACHMENT_COLOR) {
                    if (colorIndex != -1) {
                        index = colorIndex;
                    } else {
                        error = errorPrefix + "expects there to be a color buffer when there is not!";
                        return false;
                    }
                } else if (usage.type == ATTACHMENT_DEPTH_STENCIL) {
                    if (depthStencilIndex != -1) {
                        index = depthStencilIndex;
                    } else {
                        error = errorPrefix + "expects there to be a depth/stencil buffer when there is not!";
                        return false;
                    }
                } else if (usage.type == ATTACHMENT_RESOLVE) {
                    if (resolveIndex != -1) {
                        index = resolveIndex;
                    } else {
                        error = errorPrefix + "expects there to be a resolved color buffer when there is not!";
                        return false;
                    }
                } else if (usage.type != ATTACHMENT_ALL) {
                    error = errorPrefix + "usage.type is an invalid value: " + std::to_string(usage.type);
                    return false;
                }
                if (usage.accessFlags & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT) {
                    if (colorIndex == -1) {
                        error = errorPrefix + "requests a color buffer for writing, but none is available.";
                        return false;
                    }
                    VkAttachmentReference ref{};
                    ref.attachment = colorIndex;
                    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    subpass.referencesColor.push_back(ref);
                    if (attachment.resolveColor && attachment.sampleCount != VK_SAMPLE_COUNT_1_BIT) {
                        ref.attachment = resolveIndex;
                        subpass.referencesResolve.push_back(ref);
                    }
                }
                if (usage.accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT) && attachment.bufferDepthStencil) {
                    if (depthStencilIndex == -1) {
                        error = errorPrefix + "requests a depth/stencil buffer for writing, but none is available.";
                        return false;
                    }
                    VkAttachmentReference ref{};
                    ref.attachment = index;
                    ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    subpass.referenceDepthStencil = ref;
                }
                if (usage.accessFlags & VK_ACCESS_SHADER_READ_BIT) {
                    VkAttachmentReference ref{};
                    ref.attachment = index;
                    ref.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    subpass.referencesInput.push_back(ref);
                }
            }
            VkSubpassDescription description{};
            description.pipelineBindPoint = subpass.pipelineBindPoint;
            description.colorAttachmentCount = subpass.referencesColor.size();
            description.pColorAttachments = subpass.referencesColor.data();
            description.inputAttachmentCount = subpass.referencesInput.size();
            description.pInputAttachments = subpass.referencesInput.data();
            description.preserveAttachmentCount = subpass.referencesPreserve.size();
            description.pPreserveAttachments = subpass.referencesPreserve.data();
            if (subpass.referencesResolve.size() != 0) {
                description.pColorAttachments = subpass.referencesResolve.data();
            }
            if (depthStencilTaken) {
                description.pDepthStencilAttachment = &subpass.referenceDepthStencil;
            }
            subpassDescriptions.push_back(description);
        }
        // Now we configure our dependencies
        if (initialTransition) {
            VkSubpassDependency dep;
            dep.srcSubpass = VK_SUBPASS_EXTERNAL; // Transition from before this RenderPass
            dep.dstSubpass = 0; // Our first subpass
            dep.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Make sure the image is done being used
            dep.dstStageMask = initialAccessStage; // The stage at which we need the image in a certain layout
            dep.srcAccessMask = initialAccess;
            bool depth = false;
            bool color = false;
            bool resolve = false;
            for (AttachmentUsage& usage : subpasses[0].attachments) {
                switch (usage.type) {
                    case ATTACHMENT_COLOR: {
                        color = true;
                        break;
                    }
                    case ATTACHMENT_DEPTH_STENCIL: {
                        depth = true;
                        break;
                    }
                    case ATTACHMENT_RESOLVE: {
                        resolve = true;
                        break;
                    }
                    case ATTACHMENT_ALL: {
                        if (attachments[usage.index].resolveColor && attachments[usage.index].sampleCount != VK_SAMPLE_COUNT_1_BIT) {
                            resolve = true;
                        } else {
                            color = true;
                        }
                        break;
                    }
                }
            }
            if (depth && !color && !resolve) {
                dep.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            } else if (color || resolve) {
                dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            }
            dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            subpassDependencies.push_back(dep);
        }
        for (u32 i = 1; i < subpasses.size()-1; i++) {
            // VkSubpassDependency dep;
            // dep.srcSubpass = i-1;
            // dep.dstSubpass = i;
            // TODO: Finish inter-subpass dependencies
        }
        if (finalTransition) {
            VkSubpassDependency dep;
            dep.srcSubpass = subpasses.size()-1; // Our last subpass
            dep.dstSubpass = VK_SUBPASS_EXTERNAL; // Transition for use outside our RenderPass
            dep.srcStageMask = finalAccessStage; // Make sure the image is done being used
            dep.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            bool depth = false;
            bool color = false;
            bool resolve = false;
            for (AttachmentUsage& usage : subpasses[0].attachments) {
                switch (usage.type) {
                    case ATTACHMENT_COLOR: {
                        color = true;
                        break;
                    }
                    case ATTACHMENT_DEPTH_STENCIL: {
                        depth = true;
                        break;
                    }
                    case ATTACHMENT_RESOLVE: {
                        resolve = true;
                        break;
                    }
                    case ATTACHMENT_ALL: {
                        if (attachments[usage.index].resolveColor && attachments[usage.index].sampleCount != VK_SAMPLE_COUNT_1_BIT) {
                            resolve = true;
                        } else {
                            color = true;
                        }
                        break;
                    }
                }
            }
            if (depth && !color && !resolve) {
                dep.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            } else if (color || resolve) {
                dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            }
            dep.dstAccessMask = finalAccess;
            dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            subpassDependencies.push_back(dep);
        }
        // Finally put it all together
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachmentDescriptions.size();
        renderPassInfo.pAttachments = attachmentDescriptions.data();
        renderPassInfo.subpassCount = subpassDescriptions.size();
        renderPassInfo.pSubpasses = subpassDescriptions.data();
        renderPassInfo.subpassCount = subpassDescriptions.size();
        renderPassInfo.dependencyCount = subpassDependencies.size();
        renderPassInfo.pDependencies = subpassDependencies.data();

        VkResult result = vkCreateRenderPass(device->device, &renderPassInfo, nullptr, &renderPass);

        if (result != VK_SUCCESS) {
            error = "Failed to create RenderPass: " + ErrorString(result);
            return false;
        }
        initted = true;
        return true;
    }

    bool RenderPass::Deinit() {
        PrintDashed("Destroying RenderPass");
        if (!initted) {
            error = "RenderPass hasn't been initialized yet!";
            return false;
        }

        vkDestroyRenderPass(device->device, renderPass, nullptr);
        initted = false;
        return true;
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
        PrintDashed("Initializing Swapchain");
        if (initted) {
            error = "Swapchain is already initialized!";
            return false;
        }
        if ((device = dev) == nullptr) {
            error = "Device is nullptr!";
            return false;
        }
        if (!window.Valid()) {
            error = "Cannot create a swapchain without a window surface!";
            return false;
        }
        surface = window->surface;
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
            extent = {(u32)window->surfaceWindow->width, (u32)window->surfaceWindow->height};

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
                if (device->queues[i].queueFamilyIndex == (i32)queueFamilies[j]) {
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

        cout << "Acquiring images and creating image views..." << std::endl;

        // Get our images
        Array<VkImage> imagesTemp;
        vkGetSwapchainImagesKHR(device->device, swapchain, &imageCount, nullptr);
        imagesTemp.resize(imageCount);
        vkGetSwapchainImagesKHR(device->device, swapchain, &imageCount, imagesTemp.data());

        images.resize(imageCount);
        for (u32 i = 0; i < imageCount; i++) {
            images[i].Clean();
            images[i].Init(device->device);
            images[i].image = imagesTemp[i];
            images[i].format = surfaceFormat.format;
            images[i].width = extent.width;
            images[i].height = extent.height;
            images[i].aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
            images[i].usage = usage;
            if (!images[i].CreateImageView()) {
                return false;
            }
        }

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
        PrintDashed("Destroying Swapchain");
        if (!initted) {
            error = "Swapchain isn't initialized!";
            return false;
        }
        for (u32 i = 0; i < imageCount; i++) {
            images[i].Clean();
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

    Queue* Device::AddQueue() {
        queues.push_back(Queue());
        return &queues[queues.size()-1];
    }

    Swapchain* Device::AddSwapchain() {
        swapchains.push_back(Swapchain());
        return &swapchains[swapchains.size()-1];
    }

    RenderPass* Device::AddRenderPass() {
        renderPasses.push_back(RenderPass());
        return &renderPasses[renderPasses.size()-1];
    }

    Array<Image>* Device::AddImages(u32 count) {
        Array<Image> array(count);
        images.push_back(array);
        return &images[images.size()-1];
    }

    Array<Buffer>* Device::AddBuffers(u32 count) {
        Array<Buffer> array(count);
        buffers.push_back(array);
        return &buffers[buffers.size()-1];
    }

    ArrayPtr<Sampler> Device::AddSampler() {
        samplers.push_back(Sampler());
        return ArrayPtr<Sampler>(samplers, samplers.size()-1);
    }

    Descriptors* Device::AddDescriptors() {
        descriptors.push_back(Descriptors());
        return &descriptors[descriptors.size()-1];
    }

    bool Device::Init(Instance *inst) {
        PrintDashed("Initializing Logical Device");
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
        {
            // What features to we want?
            bool anisotropy = false;
            for (auto& sampler : samplers) {
                if (sampler.anisotropy != 1) {
                    anisotropy = true;
                }
            }
            if (anisotropy) {
                deviceFeaturesOptional.samplerAnisotropy = VK_TRUE;
            }
            // Which ones are available?
            for (u32 i = 0; i < sizeof(VkPhysicalDeviceFeatures)/4; i++) {
                // I'm not sure why these aren't bit-masked values,
                // but I'm treating it like that anyway.
                *(((u32*)&deviceFeatures + i)) = *(((u32*)&deviceFeaturesRequired + i))
                || (*(((u32*)&physicalDevice.features + i)) && *(((u32*)&deviceFeaturesOptional + i)));
            }
            // Which ones don't we have that we wanted?
            if (anisotropy && deviceFeatures.samplerAnisotropy == VK_FALSE) {
                cout << "Sampler Anisotropy desired, but unavailable...disabling." << std::endl;
                for (auto& sampler : samplers) {
                    sampler.anisotropy = 1;
                }
            }
        }


        // Set up queues
        // First figure out which queue families each queue should use
        const bool preferSameQueueFamilies = true;
        const bool preferMonolithicQueues = true;

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
                        error = "queues[" + std::to_string(i) + "] has a QueueType of UNDEFINED!";
                        return false;
                    }
                }
                if (preferSameQueueFamilies && queues[i].queueFamilyIndex != -1)
                    break;
            }
            if (queues[i].queueFamilyIndex == -1) {
                error = "queues[" + std::to_string(i) + "] couldn't find a queue family :(";
                return false;
            }
            if (!preferMonolithicQueues) {
                queuesPerFamily[queues[i].queueFamilyIndex]--;
            }
        }

        Array<VkDeviceQueueCreateInfo> queueCreateInfos{};
        Array<Set<f32>> queuePriorities(queueFamilies);
        for (u32 i = 0; i < queueFamilies; i++) {
            for (auto& queue : queues) {
                if (queue.queueFamilyIndex == (i32)i) {
                    queuePriorities[i].insert(queue.queuePriority);
                }
            }
        }
        Array<Array<f32>> queuePrioritiesArray(queueFamilies);
        for (u32 i = 0; i < queueFamilies; i++) {
            for (const f32& p : queuePriorities[i]) {
                queuePrioritiesArray[i].push_back(p);
            }
        }
        for (u32 i = 0; i < queueFamilies; i++) {
            cout << "Allocating " << queuePrioritiesArray[i].size() << " queues from family " << i << std::endl;
            if (queuePrioritiesArray[i].size() != 0) {
                VkDeviceQueueCreateInfo queueCreateInfo = {};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = i;
                queueCreateInfo.queueCount = queuePrioritiesArray[i].size();
                queueCreateInfo.pQueuePriorities = queuePrioritiesArray[i].data();
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
            for (auto& queue : queues) {
                if (queue.queueFamilyIndex == (i32)i) {
                    u32 queueIndex = 0;
                    for (u32 p = 0; p < queuePrioritiesArray[i].size(); p++) {
                        if (queue.queuePriority == queuePrioritiesArray[i][p]) {
                            queueIndex = p;
                            break;
                        }
                    }
                    vkGetDeviceQueue(device, i, queueIndex, &queue.queue);
                }
            }
        }
        // Swapchains
        for (auto& swapchain : swapchains) {
            if (!swapchain.Init(this)) {
                goto failed;
            }
        }
        // RenderPasses
        for (auto& renderPass : renderPasses) {
            if (!renderPass.Init(this)) {
                goto failed;
            }
        }
        // Images
        for (auto& imageArray : images) {
            for (u32 i = 0; i < imageArray.size(); i++) {
                imageArray[i].device = device;
                if (!imageArray[i].CreateImage()) {
                    goto failed;
                }
                if (!imageArray[i].CreateImageView()) {
                    goto failed;
                }
            }
        }
        // Buffers
        for (auto& bufferArray : buffers) {
            for (u32 i = 0; i < bufferArray.size(); i++) {
                bufferArray[i].device = device;
                if (!bufferArray[i].Create()) {
                    goto failed;
                }
            }
        }
        // Samplers
        for (auto& sampler : samplers) {
            sampler.device = device;
            if (!sampler.Create()) {
                goto failed;
            }
        }
        // Descriptors
        for (auto& descriptor : descriptors) {
            descriptor.Init(device);
            if (!descriptor.Create()) {
                goto failed;
            }
            if (!descriptor.Update()) {
                goto failed;
            }
        }

        // Init everything else here
        initted = true;
        return true;
failed:
        for (auto& swapchain : swapchains) {
            if (swapchain.initted)
                swapchain.Deinit();
        }
        for (auto& renderPass : renderPasses) {
            if (renderPass.initted)
                renderPass.Deinit();
        }
        for (auto& imageArray : images) {
            for (u32 i = 0; i < imageArray.size(); i++) {
                imageArray[i].Clean();
            }
        }
        for (auto& bufferArray : buffers) {
            for (u32 i = 0; i < bufferArray.size(); i++) {
                bufferArray[i].Clean();
            }
        }
        for (auto& sampler : samplers) {
            sampler.Clean();
        }
        for (auto& descriptor : descriptors) {
            descriptor.Clean();
        }
        vkDestroyDevice(device, nullptr);
        return false;
    }

    bool Device::Deinit() {
        PrintDashed("Destroying Logical Device");
        if (!initted) {
            error = "Device isn't initialized!";
            return false;
        }
        for (auto& swapchain : swapchains) {
            if (swapchain.initted)
                swapchain.Deinit();
        }
        for (auto& renderPass : renderPasses) {
            if (renderPass.initted)
                renderPass.Deinit();
        }
        for (auto& imageArray : images) {
            for (u32 i = 0; i < imageArray.size(); i++) {
                imageArray[i].Clean();
            }
        }
        for (auto& bufferArray : buffers) {
            for (u32 i = 0; i < bufferArray.size(); i++) {
                bufferArray[i].Clean();
            }
        }
        for (auto& sampler : samplers) {
            sampler.Clean();
        }
        for (auto& descriptor : descriptors) {
            descriptor.Clean();
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

    ArrayPtr<Window> Instance::AddWindowForSurface(io::Window *window) {
        Window w;
        w.surfaceWindow = window;
        windows.push_back(w);
        return ArrayPtr<Window>(windows, windows.size()-1);
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

    Device* Instance::AddDevice() {
        devices.push_back(Device());
        return &devices[devices.size()-1];
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
        PrintDashed("Initializing Vulkan Tree");
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
                if (equals(extensionsUnavailable[i], extensionsAvailable[j].extensionName)) {
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
        for (u32 i = 0; i < layersUnavailable.size(); i++) {
            for (u32 j = 0; j < layersAvailable.size(); j++) {
                if (equals(layersUnavailable[i], layersAvailable[j].layerName)) {
                    layersUnavailable.erase(layersUnavailable.begin() + i);
                    i--;
                    break;
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
            error = "vkCreateInstance failed with error: " + ErrorString(result);
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
            if (result != VK_SUCCESS) {
                error = "fpCreateDebugReportCallbackEXT failed with error: " + ErrorString(result);
                vkDestroyInstance(instance, nullptr);
                return false;
            }
        }
        // Create a surface if we want one
#ifdef IO_FOR_VULKAN
        for (Window& w : windows) {
            if (!w.surfaceWindow->CreateVkSurface(this, &w.surface)) {
                error = "Failed to CreateVkSurface: " + error;
                goto failed;
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
                    goto failed;
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
        for (u32 i = 0; i < devices.size(); i++) {
            if (!devices[i].Init(this)) {
                goto failed;
            }
        }

        // Tell everything else to initialize here
        // If it fails, clean up the instance.
        initted = true;
        return true;
failed:
        for (u32 i = 0; i < devices.size(); i++) {
            if (devices[i].initted)
            devices[i].Deinit();
        }
        if (enableLayers) {
            fpDestroyDebugReportCallbackEXT(instance, debugReportCallback, nullptr);
        }
        vkDestroyInstance(instance, nullptr);
        return false;
    }

    bool Instance::Deinit() {
        PrintDashed("Destroying Vulkan Tree");
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
