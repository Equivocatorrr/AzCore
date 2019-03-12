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
#include <fstream>

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
        i32 width = 80-(i32)str.size();
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

    String FormatSize(u64 size) {
        String str = "";
        bool space = false;
        if (size > 1024*1024*1024) {
            str = std::to_string(size/(1024*1024*1024)) + " GiB";
            size %= (1024*1024*1024);
            space = true;
        }
        if (size > 1024*1024) {
            if (space) {
                str += ", ";
            }
            str = std::to_string(size/(1024*1024)) + " MiB";
            size %= (1024*1024);
            space = true;
        }
        if (size > 1024) {
            if (space) {
                str += ", ";
            }
            str += std::to_string(size/1024) + " KiB";
            size %= 1024;
            space = true;
        }
        if (size > 0) {
            if (space) {
                str += ", ";
            }
            str += std::to_string(size) + " B";
        }
        return str;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objType, u64 obj, size_t location,
        i32 code, const char* layerPrefix, const char* msg, void* userData) {

        cout << "layer(" << layerPrefix << "):\n";
        if (userData != nullptr) {
            ((Instance*)userData)->PrintObjectLocation(objType, obj);
        }
        cout << msg << "\n" << std::endl;
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
        cout << "Memory: " << FormatSize(deviceLocalMemory) << std::endl;
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
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
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
        createInfo.size = size;
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

    ArrayPtr<Image> Memory::AddImage(Image image) {
        images.push_back(image);
        return ArrayPtr<Image>(&images, images.size()-1);
    }

    ArrayPtr<Buffer> Memory::AddBuffer(Buffer buffer) {
        buffers.push_back(buffer);
        return ArrayPtr<Buffer>(&buffers, buffers.size()-1);
    }

    ArrayRange<Image> Memory::AddImages(u32 count, Image image) {
        images.resize(images.size()+count, image);
        return ArrayRange<Image>(&images, images.size()-count, count);
    }

    ArrayRange<Buffer> Memory::AddBuffers(u32 count, Buffer buffer) {
        buffers.resize(buffers.size()+count, buffer);
        return ArrayRange<Buffer>(&buffers, buffers.size()-count, count);
    }

    bool Memory::Init(PhysicalDevice *phy, VkDevice dev) {
        PrintDashed("Initializing Memory");
        if (initted) {
            error = "Memory has already been initialized!";
            return false;
        }
        if (allocated) {
            error = "Memory has already been allocated!";
            return false;
        }
        if ((physicalDevice = phy) == nullptr) {
            error = "physicalDevice is nullptr!";
            return false;
        }
        device = dev;
        // First we figure out how big we are by going through the images and buffers
        offsets.resize(1);
        offsets[0] = 0;
        memoryTypeBits = 0;
        u32 index = 0;
        VkResult result;
        if (deviceLocal) {
            memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        } else {
            memoryProperties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        }

        cout << "Memory will create " << images.size() << " images and " << buffers.size() << " buffers." << std::endl;

        for (Image& image : images) {
            image.Init(device);
            if (!image.CreateImage(!deviceLocal)) {
                goto failure;
            }
            if (GetImageChunk(image, memoryTypeBits!=0) == -1) {
                goto failure;
            }
        }
        for (Buffer& buffer : buffers) {
            buffer.Init(device);
            if (!buffer.Create()) {
                goto failure;
            }
            if (GetBufferChunk(buffer, memoryTypeBits!=0) == -1) {
                // There's a solid chance that memory types are incompatible between
                // images and buffers, so this will probably fail if you have images too.
                goto failure;
            }
        }
        // Then allocate as much space as we need
        if (!Allocate()) {
            goto failure;
        }
        // Now bind our images and buffers to the memory
        for (Image& image : images) {
            result = vkBindImageMemory(device, image.image, memory, offsets[index++]);
            if (result != VK_SUCCESS) {
                error = "Failed to bind image to memory: " + ErrorString(result);
                goto failure;
            }
            if (!image.CreateImageView()) {
                goto failure;
            }
        }
        for (Buffer& buffer : buffers) {
            result = vkBindBufferMemory(device, buffer.buffer, memory, offsets[index++]);
            if (result != VK_SUCCESS) {
                error = "Failed to bind buffer to memory: " + ErrorString(result);
                goto failure;
            }
        }
        initted = true;
        return true;
failure:
        for (Image& image : images) {
            image.Clean();
        }
        for (Buffer& buffer : buffers) {
            buffer.Clean();
        }
        if (allocated) {
            vkFreeMemory(device, memory, nullptr);
            allocated = false;
        }
        return false;
    }

    bool Memory::Deinit() {
        PrintDashed("Destroying Memory");
        if (!initted) {
            error = "Memory isn't initialized!";
            return false;
        }
        for (Image& image : images) {
            image.Clean();
        }
        for (Buffer& buffer : buffers) {
            buffer.Clean();
        }
        initted = false;
        if (allocated)
            vkFreeMemory(device, memory, nullptr);
        allocated = false;
        return true;
    }

    i32 Memory::GetImageChunk(Image image, bool noChange) {
        i32 index = offsets.size()-1;
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, image.image, &memReqs);
        if (noChange && memoryTypeBits != memReqs.memoryTypeBits) {
            error = "An image is incompatible with the memory previously alotted!";
            return -1;
        }
        memoryTypeBits = memReqs.memoryTypeBits;

        u32 alignedOffset;
        if (memReqs.size % memReqs.alignment == 0) {
            alignedOffset = memReqs.size;
        } else {
            alignedOffset = (memReqs.size/memReqs.alignment+1)*memReqs.alignment;
        }

        offsets.push_back(offsets.back() + alignedOffset);
        return index;
    }

    i32 Memory::GetBufferChunk(Buffer buffer, bool noChange) {
        i32 index = offsets.size()-1;
        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, buffer.buffer, &memReqs);
        if (noChange && memoryTypeBits != memReqs.memoryTypeBits) {
            error = "A buffer is incompatible with the memory previously alotted!";
            return -1;
        }
        memoryTypeBits = memReqs.memoryTypeBits;

        u32 alignedOffset;
        if (memReqs.size % memReqs.alignment == 0) {
            alignedOffset = memReqs.size;
        } else {
            alignedOffset = (memReqs.size/memReqs.alignment+1)*memReqs.alignment;
        }

        offsets.push_back(offsets.back() + alignedOffset);
        return index;
    }

    VkDeviceSize Memory::ChunkSize(u32 index) {
        return offsets[index+1] - offsets[index];
    }

    i32 Memory::FindMemoryType() {
        VkPhysicalDeviceMemoryProperties memProps = physicalDevice->memoryProperties;
        for (u32 i = 0; i < memProps.memoryTypeCount; i++) {
            if ((memoryTypeBits & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties) {
                return i;
            }
        }
        for (u32 i = 0; i < memProps.memoryTypeCount; i++) {
            if ((memoryTypeBits & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & memoryPropertiesDeferred) == memoryPropertiesDeferred) {
                return i;
            }
        }

        error = "Failed to find a suitable memory type!";
        return -1;
    }

    bool Memory::Allocate() {
        if (allocated) {
            error = "Memory already allocated!";
            return false;
        }
        cout << "Allocating Memory with size: " << FormatSize(offsets.back()) << std::endl;
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = offsets.back();
        i32 mti = FindMemoryType();
        if (mti == -1) {
            return false;
        }
        allocInfo.memoryTypeIndex = (u32)mti;

        VkResult result = vkAllocateMemory(device, &allocInfo, nullptr, &memory);
        if (result != VK_SUCCESS) {
            error = "Failed to allocate memory: " + ErrorString(result);
            return false;
        }
        allocated = true;
        return true;
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

    bool DescriptorSet::AddDescriptor(ArrayRange<Buffer> buffers, u32 binding) {
        // TODO: Support other types of descriptors
        if (layout->type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            error = "AddDescriptor failed because layout type is not for uniform buffers!";
            return false;
        }
        for (u32 i = 0; i < layout->bindings.size(); i++) {
            if (layout->bindings[i].binding == binding) {
                if (layout->bindings[i].count != buffers.size) {
                    error = "AddDescriptor failed because input size is wrong("
                          + std::to_string(buffers.size) + ") for binding "
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

    bool DescriptorSet::AddDescriptor(ArrayRange<Image> images, ArrayPtr<Sampler> sampler, u32 binding) {
        // TODO: Support other types of descriptors
        if (layout->type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            error = "AddDescriptor failed because layout type is not for combined image samplers!";
            return false;
        }
        for (u32 i = 0; i < layout->bindings.size(); i++) {
            if (layout->bindings[i].binding == binding) {
                if (layout->bindings[i].count != images.size) {
                    error = "AddDescriptor failed because input size is wrong("
                          + std::to_string(images.size) + ") for binding "
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

    bool DescriptorSet::AddDescriptor(ArrayPtr<Buffer> buffer, u32 binding) {
        return AddDescriptor(ArrayRange<Buffer>(buffer.array, buffer.index, 1), binding);
    }

    bool DescriptorSet::AddDescriptor(ArrayPtr<Image> image, ArrayPtr<Sampler> sampler, u32 binding) {
        return AddDescriptor(ArrayRange<Image>(image.array, image.index, 1), sampler, binding);
    }

    Descriptors::~Descriptors() {
        Clean();
    }

    void Descriptors::Init(VkDevice dev) {
        device = dev;
    }

    ArrayPtr<DescriptorLayout> Descriptors::AddLayout() {
        layouts.push_back(DescriptorLayout());
        return ArrayPtr<DescriptorLayout>(&layouts, layouts.size()-1);
    }

    ArrayPtr<DescriptorSet> Descriptors::AddSet(ArrayPtr<DescriptorLayout> layout) {
        DescriptorSet set{};
        set.layout = layout;
        sets.push_back(set);
        return ArrayPtr<DescriptorSet>(&sets, sets.size()-1);
    }

    bool Descriptors::Create() {
        PrintDashed("Creating Descriptors");
        if (exists) {
            error = "Descriptors already exist!";
            return false;
        }
        Array<VkDescriptorPoolSize> poolSizes(layouts.size());
        for (u32 i = 0; i < layouts.size(); i++) {
            layouts[i].Init(device);
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
        cout << "Allocating " << sets.size() << " Descriptor Sets." << std::endl;
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
                totalBufferInfos += sets[i].bufferDescriptors[j].buffers.size;
            }
            for (u32 j = 0; j < sets[i].imageDescriptors.size(); j++) {
                totalImageInfos += sets[i].imageDescriptors[j].images.size;
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
                        Buffer &buffer = sets[i].bufferDescriptors[setBufferDescriptor].buffers[x];
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
                        imageInfo.imageView = imageDescriptor.images[x].imageView;
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
            PrintDashed("Destroying Descriptors");
            vkDestroyDescriptorPool(device, pool, nullptr);
            for (u32 i = 0; i < sets.size(); i++) {
                sets[i].exists = false;
            }
        }
        for (u32 i = 0; i < layouts.size(); i++) {
            layouts[i].Clean();
        }
        exists = false;
    }

    Attachment::Attachment() {}

    Attachment::Attachment(Swapchain *swch) {
        swapchain = swch;
        if (swapchain != nullptr) {
            bufferColor = true;
            keepColor = true;
        }
    }

    bool Attachment::Config() {
        if (swapchain != nullptr) {
            formatColor = swapchain->surfaceFormat.format;
        }
        descriptions.resize(0);
        if (bufferColor) {
            if (initialLayoutColor == VK_IMAGE_LAYOUT_UNDEFINED && loadColor) {
                error = "For the contents of this attachment to be loaded, you must specify an initialLayout for Color.";
                return false;
            }
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

                description.initialLayout = initialLayoutColor;
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
                if (swapchain != nullptr) {
                    description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                } else {
                    description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
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

                description.initialLayout = initialLayoutColor;
                if (swapchain != nullptr) {
                    description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                } else {
                    description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
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

            if (initialLayoutDepthStencil == VK_IMAGE_LAYOUT_UNDEFINED
            && (loadDepth || loadStencil)) {
                error = "For the contents of this attachment to be loaded, you must specify an initialLayout for DepthStencil.";
                return false;
            }

            description.initialLayout = initialLayoutDepthStencil;
            description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            descriptions.push_back(description);
        }
        return true;
    }

    void Subpass::UseAttachment(ArrayPtr<Attachment> attachment, AttachmentType type, VkAccessFlags accessFlags) {
        AttachmentUsage usage = {attachment.index, type, accessFlags};
        attachments.push_back(usage);
    }

    ArrayPtr<Subpass> RenderPass::AddSubpass() {
        subpasses.push_back(Subpass());
        return ArrayPtr<Subpass>(&subpasses, subpasses.size()-1);
    }

    ArrayPtr<Attachment> RenderPass::AddAttachment(Swapchain *swapchain) {
        attachments.push_back(Attachment(swapchain));
        return ArrayPtr<Attachment>(&attachments, attachments.size()-1);
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
            if (!attachments[i].Config()) {
                error = "With attachment[" + std::to_string(i) + "]: " + error;
                return false;
            }
        }
        // Then we concatenate our attachmentDescriptions from the Attachments
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
                    if (usage.type == ATTACHMENT_ALL || usage.type == ATTACHMENT_DEPTH_STENCIL) {
                        depthStencilTaken = true;
                    }
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
                    ref.attachment = depthStencilIndex;
                    ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    subpass.referenceDepthStencil = ref;
                }
                if (usage.accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) {
                    VkAttachmentReference ref{};
                    ref.attachment = index;
                    if (usage.type == ATTACHMENT_COLOR || usage.type == ATTACHMENT_RESOLVE) {
                        ref.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    } else if (usage.type == ATTACHMENT_DEPTH_STENCIL) {
                        ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                    }
                    subpass.referencesInput.push_back(ref);
                }
            }
            cout << "Subpass[" << i << "] is using the following attachments:\n";
            if (subpass.referencesColor.size() != 0) {
                cout << "\tColor: ";
                for (auto& ref : subpass.referencesColor) {
                    cout << ref.attachment << " ";
                }
            }
            if (subpass.referencesResolve.size() != 0) {
                cout << "\n\tResolve: ";
                for (auto& ref : subpass.referencesResolve) {
                    cout << ref.attachment << " ";
                }
            }
            if (subpass.referencesInput.size() != 0) {
                cout << "\n\tInput: ";
                for (auto& ref : subpass.referencesInput) {
                    cout << ref.attachment << " ";
                }
            }
            if (subpass.referencesPreserve.size() != 0) {
                cout << "\n\tPreserve: ";
                for (auto& ref : subpass.referencesPreserve) {
                    cout << ref << " ";
                }
            }
            if (depthStencilTaken) {
                cout << "\n\tDepth: " << subpass.referenceDepthStencil.attachment;
            }
            cout << std::endl;
            VkSubpassDescription description{};
            description.pipelineBindPoint = subpass.pipelineBindPoint;
            description.colorAttachmentCount = subpass.referencesColor.size();
            description.pColorAttachments = subpass.referencesColor.data();
            description.inputAttachmentCount = subpass.referencesInput.size();
            description.pInputAttachments = subpass.referencesInput.data();
            description.preserveAttachmentCount = subpass.referencesPreserve.size();
            description.pPreserveAttachments = subpass.referencesPreserve.data();
            if (subpass.referencesResolve.size() != 0) {
                description.pResolveAttachments = subpass.referencesResolve.data();
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

    Framebuffer::~Framebuffer() {
        if (initted) {
            if (!Deinit()) {
                cout << "Failed to clean up vk::Framebuffer: " << error << std::endl;
            }
        }
    }

    bool Framebuffer::Init(Device *dev) {
        PrintDashed("Initializing Framebuffer");
        if (initted) {
            error = "Framebuffer is already initialized!";
            return false;
        }
        if ((device = dev) == nullptr) {
            error = "Device is nullptr!";
            return false;
        }
        if (!renderPass->initted) {
            error = "RenderPass is not initialized!";
            return false;
        }
        bool depth = false, color = false;
        bool sameSwapchain = true;
        for (auto& attachment : renderPass->attachments) {
            if (attachment.swapchain != nullptr && swapchain != nullptr) {
                if (attachment.swapchain != swapchain) {
                    sameSwapchain = false;
                    if (attachment.swapchain->surfaceFormat.format != swapchain->surfaceFormat.format) {
                        error = "Framebuffer swapchain differing from RenderPass swapchain: Surface formats do not match!";
                        return false;
                    }
                }
            }
            if (attachment.swapchain != nullptr && swapchain == nullptr) {
                error = "RenderPass is connected to a swapchain, but this Framebuffer is not!";
                return false;
            }
            if (attachment.bufferDepthStencil) {
                depth = true;
            }
            if (attachment.bufferColor) {
                if (attachment.swapchain == nullptr
                || (attachment.resolveColor && attachment.sampleCount != VK_SAMPLE_COUNT_1_BIT)) {
                    color = true;
                }
            }
        }
        if (!sameSwapchain) {
            cout << "Warning: Framebuffer is using a different swapchain than RenderPass! This may be valid for swapchains that have the same surface formats, but that is not guaranteed to be the case.";
            return false;
        }
        if (swapchain != nullptr) {
            numFramebuffers = swapchain->imageCount;
            width = swapchain->extent.width;
            height = swapchain->extent.height;
        }
        cout << "Width: " << width << "  Height: " << height << std::endl;
        if (ownMemory) {
            // Create Memory objects according to the RenderPass attachments
            if (depth && depthMemory == nullptr) {
                cout << "Adding depth Memory to device." << std::endl;
                depthMemory = device->AddMemory();
            }
            if (color && colorMemory == nullptr) {
                cout << "Adding color Memory to device." << std::endl;
                colorMemory = device->AddMemory();
            }
        } else {
            // Verify that we have the memory we need
            bool failed = false;
            if (depth && depthMemory == nullptr) {
                error = "Framebuffer set to use outside memory, but none is given for depth ";
                failed = true;
            }
            if (color && colorMemory == nullptr) {
                if (!failed) {
                    error = "Framebuffer set to use outside memory, but none is given f";
                }
                error = error + "or color ";
                failed = true;
            }
            if (failed) {
                error.back() = '!';
                return false;
            }
        }
        if (ownImages) {
            if (width == 0 || height == 0) {
                error = "Framebuffer can't create images with size (" + std::to_string(width) + ", " + std::to_string(height) + ")!";
                return false;
            }
            if (attachmentImages.size() == 0) {
                Array<ArrayPtr<Image>> ourImages{};
                // Create Images according to the RenderPass attachments
                u32 i = 0;
                for (auto& attachment : renderPass->attachmentDescriptions) {
                    Image image;
                    image.width = width;
                    image.height = height;
                    image.format = attachment.format;
                    image.samples = attachment.samples;
                    if (attachment.finalLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                        cout << "Adding color image " << i << " to colorMemory." << std::endl;
                        image.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
                        image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                        ourImages.push_back(colorMemory->AddImage(image));
                    } else if (attachment.finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                        cout << "Adding depth image " << i << " to depthMemory." << std::endl;
                        image.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
                        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                        ourImages.push_back(depthMemory->AddImage(image));
                    } else if (attachment.finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                        cout << "Adding null image " << i << " for replacement with swapchain images." << std::endl;
                        ourImages.push_back(ArrayPtr<Image>());
                    }
                    i++;
                }
                // Now we need to make sure we know which images are also being used as input attachments
                for (auto& subpass : renderPass->subpasses) {
                    for (auto& input : subpass.referencesInput) {
                        ourImages[input.attachment]->usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                    }
                }
                // Now add ourImages to each index of attachmentImages
                attachmentImages.resize(numFramebuffers, ourImages);
                // Now replace any null ArrayPtr's with the actual swapchain images
                for (u32 i = 0; i < numFramebuffers; i++) {
                    bool once = false;
                    for (auto& ptr : attachmentImages[i]) {
                        if (ptr.array == nullptr) {
                            if (once) {
                                error = "You can't have multiple swapchain images in a single framebuffer!";
                                return false;
                            }
                            once = true;
                            ptr.SetPtr(&swapchain->images, i);
                        }
                    }
                }
            } else {
                // Probably resizing
                for (auto& attachment : attachmentImages[0]) {
                    attachment->width = width;
                    attachment->height = height;
                }
            }
        } else {
            // Verify the images are set up correctly
            if (attachmentImages.size() != numFramebuffers) {
                error = "attachmentImages must have the size numFramebuffers.";
                return false;
            }
            for (u32 i = 0; i < numFramebuffers; i++) {
                if (attachmentImages[i].size() == 0) {
                    error = "Framebuffer has no attachment images!";
                    return false;
                }
                if (attachmentImages[i].size() != renderPass->attachmentDescriptions.size()) {
                    error = "RenderPass expects " + std::to_string(renderPass->attachmentDescriptions.size())
                    + " attachment images but this framebuffer only has " + std::to_string(attachmentImages.size()) + ".";
                    return false;
                }
                if (i >= 1) {
                    if (attachmentImages[i-1].size() != attachmentImages[i].size()) {
                        error = "attachmentImages[" + std::to_string(i-1) + "].size() is "
                              + std::to_string(attachmentImages[i-1].size())
                              + " while attachmentImages[" + std::to_string(i) + "].size() is "
                              + std::to_string(attachmentImages[i].size())
                              + ". These must be equal across every framebuffer!";
                        return false;
                    }
                }
                for (u32 j = 0; j < attachmentImages[i].size(); j++) {
                    if (attachmentImages[i][j]->width != width || attachmentImages[i][j]->height != height) {
                        error = "All attached images must be the same width and height!";
                        return false;
                    }
                }
            }
        }
        initted = true;
        return true;
    }

    bool Framebuffer::Create() {
        PrintDashed("Creating Framebuffer");
        if (created) {
            error = "Framebuffer already exists!";
            return false;
        }
        cout << "Making " << numFramebuffers << " total framebuffers." << std::endl;
        framebuffers.resize(numFramebuffers);
        for (u32 fb = 0; fb < numFramebuffers; fb++) {
            Array<VkImageView> attachments(attachmentImages[0].size());
            for (u32 i = 0; i < attachmentImages[fb].size(); i++) {
                if (!attachmentImages[fb][i]->imageViewExists) {
                    error = "Framebuffer attachment " + std::to_string(i) + "'s image view has not been created!";
                    return false;
                }
                attachments[i] = attachmentImages[fb][i]->imageView;
            }
            VkFramebufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.renderPass = renderPass->renderPass;
            createInfo.width = width;
            createInfo.height = height;
            createInfo.layers = 1;
            createInfo.attachmentCount = attachments.size();
            createInfo.pAttachments = attachments.data();

            VkResult result = vkCreateFramebuffer(device->device, &createInfo, nullptr, &framebuffers[fb]);

            if (result != VK_SUCCESS) {
                for (fb--; fb >= 0; fb--) {
                    vkDestroyFramebuffer(device->device, framebuffers[fb], nullptr);
                }
                error = "Failed to create framebuffers[" + std::to_string(fb) + "]: " + ErrorString(result);
                return false;
            }
        }
        created = true;
        return true;
    }

    bool Framebuffer::Deinit() {
        if (!initted) {
            error = "Framebuffer has not been initialized!";
            return false;
        }
        if (created) {
            for (u32 i = 0; i < framebuffers.size(); i++) {
                vkDestroyFramebuffer(device->device, framebuffers[i], nullptr);
            }
            created = false;
        }
        initted = false;
        return true;
    }

    bool Shader::Init(VkDevice dev) {
        if (initted) {
            error = "Shader is already initialized!";
            return false;
        }
        device = dev;
        // Load in the code
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            error = "Failed to load shader file: \"" + filename + "\"";
            return false;
        }

        size_t fileSize = (size_t) file.tellg();
        if (fileSize % 4 != 0) {
            code.resize(fileSize/4+1, 0);
        } else {
            code.resize(fileSize/4);
        }

        file.seekg(0);
        file.read((char*)code.data(), fileSize);
        file.close();

        // Now create the shader module
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = fileSize;
        createInfo.pCode = code.data();

        VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &module);

        if (result != VK_SUCCESS) {
            error = "Failed to create shader module: " + ErrorString(result);
            return false;
        }
        initted = true;
        return true;
    }

    void Shader::Clean() {
        if (initted) {
            code.resize(0);
            vkDestroyShaderModule(device, module, nullptr);
            initted = false;
        }
    }

    ShaderRef::ShaderRef(String fn) : shader() , stage() , functionName(fn) {}

    ShaderRef::ShaderRef(ArrayPtr<Shader> ptr, VkShaderStageFlagBits s, String fn) : shader(ptr) , stage(s) , functionName(fn) {}

    Pipeline::Pipeline() {
        // Time for a WHOLE LOTTA ASSUMPTIONS
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0;
		rasterizer.depthBiasClamp = 0.0;
		rasterizer.depthBiasSlopeFactor = 0.0;

		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0;
		depthStencil.maxDepthBounds = 1.0;
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {};
		depthStencil.back = {};

		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.blendConstants[0] = 0.0;
		colorBlending.blendConstants[1] = 0.0;
		colorBlending.blendConstants[2] = 0.0;
		colorBlending.blendConstants[3] = 0.0;
    }

    Pipeline::~Pipeline() {
        if (initted) {
            if (!Deinit()) {
                cout << "Failed to clean up pipeline: " << error << std::endl;
            }
        }
    }

    bool Pipeline::Init(Device *dev) {
        PrintDashed("Initializing Pipeline");
        if (initted) {
            error = "Pipeline is already initialized!";
            return false;
        }
        if ((device = dev) == nullptr) {
            error = "Device is nullptr!";
            return false;
        }
        if (renderPass == nullptr) {
            error = "Pipeline needs a renderPass!";
            return false;
        }
        if (!renderPass->initted) {
            error = "RenderPass isn't initialized!";
            return false;
        }
        if (subpass >= renderPass->subpasses.size()) {
            error = "subpass[" + std::to_string(subpass) + "] is out of bounds for the RenderPass which only has " + std::to_string(renderPass->subpasses.size()) + " subpasses!";
            return false;
        }
        if (renderPass->subpasses[subpass].referencesColor.size() != colorBlendAttachments.size()) {
            error = "You must have one colorBlendAttachment per color attachment in the associated Subpass!\nThe subpass has " + std::to_string(renderPass->subpasses[subpass].referencesColor.size()) + " color attachments while the pipeline has " + std::to_string(colorBlendAttachments.size()) + " colorBlendAttachments.";
            return false;
        }
        // Inherit multisampling information from renderPass
        for (auto& attachment : renderPass->subpasses[subpass].attachments) {
            if (attachment.accessFlags &
                (VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) {
                multisampling.rasterizationSamples = renderPass->attachments[attachment.index].sampleCount;
                break;
            }
        }
        // First we grab our shaders
        Array<VkPipelineShaderStageCreateInfo> shaderStages(shaders.size());
        for (u32 i = 0; i < shaders.size(); i++) {
            switch (shaders[i].stage) {
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                cout << "Tesselation Control ";
                break;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                cout << "Tesselation Evaluation ";
                break;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                cout << "Geometry ";
                break;
            case VK_SHADER_STAGE_VERTEX_BIT:
                cout << "Vertex ";
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                cout << "Fragment ";
                break;
            default:
                break;
            }
            cout << "Shader: \"" << shaders[i].shader->filename
                 << "\" accessing function \"" << shaders[i].functionName << "\"" << std::endl;
            shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[i].stage = shaders[i].stage;
            shaderStages[i].module = shaders[i].shader->module;
            shaderStages[i].pName = shaders[i].functionName.c_str();
        }
        // Then we set up our vertex buffers
        vertexInputInfo.vertexBindingDescriptionCount = inputBindingDescriptions.size();
        vertexInputInfo.pVertexBindingDescriptions = inputBindingDescriptions.data();
        vertexInputInfo.vertexAttributeDescriptionCount = inputAttributeDescriptions.size();
        vertexInputInfo.pVertexAttributeDescriptions = inputAttributeDescriptions.data();
        // Viewport
        VkViewport viewport = {};
        viewport.x = 0.0;
        viewport.y = 0.0;
        viewport.width = (float) 640;
        viewport.height = (float) 480;
        viewport.minDepth = 0.0;
        viewport.maxDepth = 1.0;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = {640, 480};

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;
        // Color blending
        colorBlending.attachmentCount = colorBlendAttachments.size();
		colorBlending.pAttachments = colorBlendAttachments.data();
        // Dynamic states
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();
        // Pipeline layout
        Array<VkDescriptorSetLayout> descriptorSetLayouts(descriptorLayouts.size());
        for (u32 i = 0; i < descriptorLayouts.size(); i++) {
            if (!descriptorLayouts[i]->exists) {
                error = "descriptorLayouts[" + std::to_string(i) + "] doesn't exist!";
                return false;
            }
            descriptorSetLayouts[i] = descriptorLayouts[i]->layout;
        }
        // TODO: Separate out pipeline layouts
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

        VkResult result;
        result = vkCreatePipelineLayout(device->device, &pipelineLayoutInfo, nullptr, &layout);
        if (result != VK_SUCCESS) {
            error = "Failed to create pipeline layout: " + ErrorString(result);
            return false;
        }
        // Pipeline time!
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = layout;
		pipelineInfo.renderPass = renderPass->renderPass;
		pipelineInfo.subpass = subpass;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

        result = vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
        if (result != VK_SUCCESS) {
            error = "Failed to create pipeline: " + ErrorString(result);
            vkDestroyPipelineLayout(device->device, layout, nullptr);
            return false;
        }
        initted = true;
        return true;
    }

    bool Pipeline::Deinit() {
        PrintDashed("Destroying Pipeline");
        if (!initted) {
            error = "Pipeline is not initted!";
            return false;
        }
        vkDestroyPipeline(device->device, pipeline, nullptr);
        vkDestroyPipelineLayout(device->device, layout, nullptr);
        initted = false;
        return true;
    }

    CommandPool::CommandPool(Queue* q) : queue(q) {}

    CommandPool::~CommandPool() {
        Clean();
    }

    ArrayPtr<CommandBuffer> CommandPool::AddCommandBuffer() {
        commandBuffers.push_back(CommandBuffer());
        return ArrayPtr<CommandBuffer>(&commandBuffers, commandBuffers.size()-1);
    }

    CommandBuffer* CommandPool::CreateDynamicBuffer(bool secondary) {
        if (!initted) {
            error = "Command Pool is not initialized!";
            return nullptr;
        }
        dynamicBuffers.push_back(CommandBuffer());
        CommandBuffer *buffer = &dynamicBuffers[dynamicBuffers.size()-1];
        buffer->secondary = secondary;
        buffer->pool = this;
        buffer->device = device;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        allocInfo.level = secondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkResult result = vkAllocateCommandBuffers(device->device, &allocInfo, &buffer->commandBuffer);
        if (result != VK_SUCCESS) {
            error = "Failed to allocate dynamic command buffer: " + ErrorString(result);
            return nullptr;
        }
        return buffer;
    }

    void CommandPool::DestroyDynamicBuffer(CommandBuffer *buffer) {
        if (!initted) {
            return;
        }
        vkFreeCommandBuffers(device->device, commandPool, 1, &buffer->commandBuffer);
        u32 i = 0;
        for (CommandBuffer& b : dynamicBuffers) {
            if (&b == buffer) {
                break;
            }
            i++;
        }
        if (i >= dynamicBuffers.size()) {
            return;
        }
        dynamicBuffers.erase(i);
    }

    bool CommandPool::Init(Device *dev) {
        PrintDashed("Initializing Command Pool");
        if (initted) {
            error = "CommandPool is already initialized!";
            return false;
        }
        if ((device = dev) == nullptr) {
            error = "Device is nullptr!";
            return false;
        }
        if (queue == nullptr) {
            error = "You must specify a queue to create a CommandPool!";
            return false;
        }
        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.queueFamilyIndex = queue->queueFamilyIndex;
        if (transient) {
            createInfo.flags &= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        }
        if (resettable) {
            createInfo.flags &= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        }
        if (protectedMemory) {
            createInfo.flags &= VK_COMMAND_POOL_CREATE_PROTECTED_BIT;
        }

        VkResult result = vkCreateCommandPool(device->device, &createInfo, nullptr, &commandPool);

        if (result != VK_SUCCESS) {
            error = "Failed to create CommandPool: " + ErrorString(result);
            return false;
        }

        // Now the command buffers
        u32 p = 0, s = 0;
        Array<VkCommandBuffer> primaryBuffers;
        Array<VkCommandBuffer> secondaryBuffers;
        for (CommandBuffer& buffer : commandBuffers) {
            if (buffer.secondary) {
                s++;
            } else {
                p++;
            }
        }
        cout << "Allocating " << p << " primary command buffers and " << s << " secondary command buffers." << std::endl;
        primaryBuffers.resize(p);
        secondaryBuffers.resize(s);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        if (p != 0) {
            allocInfo.commandBufferCount = p;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

            VkResult result = vkAllocateCommandBuffers(device->device, &allocInfo, primaryBuffers.data());
            if (result != VK_SUCCESS) {
                error = "Failed to allocate primary command buffers: " + ErrorString(result);
                vkDestroyCommandPool(device->device, commandPool, nullptr);
                return false;
            }
        }
        if (s != 0) {
            allocInfo.commandBufferCount = s;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

            VkResult result = vkAllocateCommandBuffers(device->device, &allocInfo, secondaryBuffers.data());
            if (result != VK_SUCCESS) {
                error = "Failed to allocate secondary command buffers: " + ErrorString(result);
                vkDestroyCommandPool(device->device, commandPool, nullptr);
                return false;
            }
        }
        p = 0; s = 0;
        for (CommandBuffer& buffer : commandBuffers) {
            if (buffer.secondary) {
                buffer.commandBuffer = secondaryBuffers[s];
                s++;
            } else {
                buffer.commandBuffer = primaryBuffers[p];
                p++;
            }
            buffer.device = device;
            buffer.pool = this;
        }
        initted = true;
        return true;
    }

    void CommandPool::Clean() {
        if (initted) {
            PrintDashed("Destroying Command Pool");
            vkDestroyCommandPool(device->device, commandPool, nullptr);
            initted = false;
        }
    }

    Swapchain::Swapchain() {}

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

    ArrayPtr<Sampler> Device::AddSampler() {
        samplers.push_back(Sampler());
        return ArrayPtr<Sampler>(&samplers, samplers.size()-1);
    }

    Memory* Device::AddMemory() {
        memories.push_back(Memory());
        return &memories[memories.size()-1];
    }

    Descriptors* Device::AddDescriptors() {
        descriptors.push_back(Descriptors());
        return &descriptors[descriptors.size()-1];
    }

    ArrayPtr<Shader> Device::AddShader() {
        shaders.push_back(Shader());
        return ArrayPtr<Shader>(&shaders, shaders.size()-1);
    }

    ArrayRange<Shader> Device::AddShaders(u32 count) {
        shaders.resize(shaders.size()+count);
        return ArrayRange<Shader>(&shaders, shaders.size()-count, count);
    }

    Pipeline* Device::AddPipeline() {
        pipelines.push_back(Pipeline());
        return &pipelines[pipelines.size()-1];
    }

    CommandPool* Device::AddCommandPool(Queue *queue) {
        commandPools.push_back(CommandPool(queue));
        return &commandPools[commandPools.size()-1];
    }

    Framebuffer* Device::AddFramebuffer() {
        framebuffers.push_back(Framebuffer());
        return &framebuffers[framebuffers.size()-1];
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
                    break;
                }
            }
            if (anisotropy) {
                cout << "Enabling samplerAnisotropy optional device feature" << std::endl;
                deviceFeaturesOptional.samplerAnisotropy = VK_TRUE;
            }
            bool independentBlending = false;
            for (auto& pipeline : pipelines) {
                for (u32 i = 1; i < pipeline.colorBlendAttachments.size(); i++) {
                    VkPipelineColorBlendAttachmentState& s1 = pipeline.colorBlendAttachments[i-1];
                    VkPipelineColorBlendAttachmentState& s2 = pipeline.colorBlendAttachments[i];
                    if (s1.blendEnable != s2.blendEnable
                    ||  s1.alphaBlendOp != s2.alphaBlendOp
                    ||  s1.colorBlendOp != s2.colorBlendOp
                    ||  s1.colorWriteMask != s2.colorWriteMask
                    ||  s1.dstAlphaBlendFactor != s2.dstAlphaBlendFactor
                    ||  s1.dstColorBlendFactor != s2.dstColorBlendFactor
                    ||  s1.srcAlphaBlendFactor != s2.srcAlphaBlendFactor
                    ||  s1.srcColorBlendFactor != s2.srcColorBlendFactor) {
                        independentBlending = true;
                        break;
                    }
                }
            }
            if (independentBlending) {
                cout << "Enabling independentBlend device feature" << std::endl;
                deviceFeaturesRequired.independentBlend = VK_TRUE;
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
        // Framebuffer init phase, may allocate Memory objects with Images
        for (auto& framebuffer : framebuffers) {
            if (!framebuffer.Init(this)) {
                goto failed;
            }
        }
        // Memory
        for (auto& memory : memories) {
            if (!memory.Init(&physicalDevice, device)) {
                goto failed;
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
        // Shaders
        for (auto& shader : shaders) {
            if (!shader.Init(device)) {
                goto failed;
            }
        }
        // Pipelines
        for (auto& pipeline : pipelines) {
            if (!pipeline.Init(this)) {
                goto failed;
            }
        }
        // CommandPools
        for (auto& commandPool : commandPools) {
            if (!commandPool.Init(this)) {
                goto failed;
            }
        }
        // Framebuffer create phase
        for (auto& framebuffer : framebuffers) {
            if (!framebuffer.Create()) {
                goto failed;
            }
        }

        // Once the pipelines are set with all the shaders,
        // we can clean them since they're no longer needed
        // NOTE: Unless we want to recreate the pipelines. Might reconsider this.
        for (auto& shader : shaders) {
            shader.Clean();
        }

        // Init everything else here
        initted = true;
        return true;
failed:
        for (auto& swapchain : swapchains) {
            if (swapchain.initted) {
                swapchain.Deinit();
            }
        }
        for (auto& renderPass : renderPasses) {
            if (renderPass.initted) {
                renderPass.Deinit();
            }
        }
        for (auto& memory : memories) {
            if (memory.initted) {
                memory.Deinit();
            }
        }
        for (auto& sampler : samplers) {
            sampler.Clean();
        }
        for (auto& descriptor : descriptors) {
            descriptor.Clean();
        }
        for (auto& shader : shaders) {
            shader.Clean();
        }
        for (auto& pipeline : pipelines) {
            if (pipeline.initted) {
                pipeline.Deinit();
            }
        }
        for (auto& commandPool : commandPools) {
            commandPool.Clean();
        }
        for (auto& framebuffer : framebuffers) {
            if (framebuffer.initted) {
                framebuffer.Deinit();
            }
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
            if (swapchain.initted) {
                swapchain.Deinit();
            }
        }
        for (auto& renderPass : renderPasses) {
            if (renderPass.initted) {
                renderPass.Deinit();
            }
        }
        for (auto& memory : memories) {
            if (memory.initted) {
                memory.Deinit();
            }
        }
        for (auto& sampler : samplers) {
            sampler.Clean();
        }
        for (auto& descriptor : descriptors) {
            descriptor.Clean();
        }
        // We don't need to clean the shaders since they should have already been cleaned
        for (auto& pipeline : pipelines) {
            if (pipeline.initted) {
                pipeline.Deinit();
            }
        }
        for (auto& commandPool : commandPools) {
            commandPool.Clean();
        }
        for (auto& framebuffer : framebuffers) {
            if (framebuffer.initted) {
                framebuffer.Deinit();
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

    ArrayPtr<Window> Instance::AddWindowForSurface(io::Window *window) {
        Window w;
        w.surfaceWindow = window;
        windows.push_back(w);
        return ArrayPtr<Window>(&windows, windows.size()-1);
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
            debugInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            debugInfo.pfnCallback = debugCallback;
            debugInfo.pUserData = this;

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
        cout << "\n\n";
        PrintDashed("Vulkan Tree Initialized");
        cout << "\n\n";
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

    void Instance::PrintObjectLocation(VkDebugReportObjectTypeEXT objType, u64 obj) {
        cout << "Object is ";
        u32 deviceIndex = 0;
        if (obj == VK_NULL_HANDLE) {
            cout << "null handle!" << std::endl;
            return;
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT) {
            cout << "device at ";
            for (Device& device : devices) {
                if (device.device == (VkDevice)obj) {
                    cout << "devices[" << deviceIndex << "]" << std::endl;
                    return;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) {
            cout << "physical device at ";
            for (PhysicalDevice& device : physicalDevices) {
                if (device.physicalDevice == (VkPhysicalDevice)obj) {
                    cout << "physicalDevices[" << deviceIndex << "]" << std::endl;
                    return;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT) {
            cout << "window at ";
            for (Window& window : windows) {
                if (window.surface == (VkSurfaceKHR)obj) {
                    cout << "windows[" << deviceIndex << "]" << std::endl;
                    return;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT) {
            cout << "queue at ";
            for (Device& device : devices) {
                u32 queueIndex = 0;
                for (Queue& queue : device.queues) {
                    if (queue.queue == (VkQueue)obj) {
                        cout << "devices[" << deviceIndex << "].queue[" << queueIndex << "]" << std::endl;
                        return;
                    }
                    queueIndex++;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT) {
            cout << "swapchain at ";
            for (Device& device : devices) {
                u32 swapchainIndex = 0;
                for (Swapchain& swapchain : device.swapchains) {
                    if (swapchain.swapchain == (VkSwapchainKHR)obj) {
                        cout << "devices[" << deviceIndex << "].swapchain[" << swapchainIndex << "]" << std::endl;
                        return;
                    }
                    swapchainIndex++;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT) {
            cout << "render pass at ";
            for (Device& device : devices) {
                u32 renderpassIndex = 0;
                for (RenderPass& renderPass : device.renderPasses) {
                    if (renderPass.renderPass == (VkRenderPass)obj) {
                        cout << "devices[" << deviceIndex << "].renderPass[" << renderpassIndex << "]" << std::endl;
                        return;
                    }
                    renderpassIndex++;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT) {
            cout << "device memory at ";
            for (Device& device : devices) {
                u32 memoryIndex = 0;
                for (Memory& memory : device.memories) {
                    if (memory.memory == (VkDeviceMemory)obj) {
                        cout << "devices[" << deviceIndex << "].memories[" << memoryIndex << "]" << std::endl;
                        return;
                    }
                    memoryIndex++;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT) {
            cout << "sampler at ";
            for (Device& device : devices) {
                u32 samplerIndex = 0;
                for (Sampler& sampler : device.samplers) {
                    if (sampler.sampler == (VkSampler)obj) {
                        cout << "devices[" << deviceIndex << "].samplers[" << samplerIndex << "]" << std::endl;
                        return;
                    }
                    samplerIndex++;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT) {
            cout << "shader module at ";
            for (Device& device : devices) {
                u32 shaderIndex = 0;
                for (Shader& shader : device.shaders) {
                    if (shader.module == (VkShaderModule)obj) {
                        cout << "devices[" << deviceIndex << "].shaders[" << shaderIndex << "]" << std::endl;
                        return;
                    }
                    shaderIndex++;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT) {
            cout << "descriptor pool at ";
            for (Device& device : devices) {
                u32 descriptorPoolIndex = 0;
                for (Descriptors& descriptor : device.descriptors) {
                    if (descriptor.pool == (VkDescriptorPool)obj) {
                        cout << "devices[" << deviceIndex << "].descriptors[" << descriptorPoolIndex << "]" << std::endl;
                        return;
                    }
                    descriptorPoolIndex++;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT) {
            cout << "descriptor set at ";
            for (Device& device : devices) {
                u32 descriptorPoolIndex = 0;
                for (Descriptors& descriptor : device.descriptors) {
                    u32 descriptorSetIndex = 0;
                    for (DescriptorSet& set : descriptor.sets) {
                        if (set.set == (VkDescriptorSet)obj) {
                            cout << "devices[" << deviceIndex << "].descriptors[" << descriptorPoolIndex << "].sets[" << descriptorSetIndex << "]" << std::endl;
                            return;
                        }
                        descriptorSetIndex++;
                    }
                    descriptorPoolIndex++;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT) {
            cout << "descriptor set layout at ";
            for (Device& device : devices) {
                u32 descriptorPoolIndex = 0;
                for (Descriptors& descriptor : device.descriptors) {
                    u32 descriptorSetLayoutIndex = 0;
                    for (DescriptorLayout& layout : descriptor.layouts) {
                        if (layout.layout == (VkDescriptorSetLayout)obj) {
                            cout << "devices[" << deviceIndex << "].descriptors[" << descriptorPoolIndex << "].layouts[" << descriptorSetLayoutIndex << "]" << std::endl;
                            return;
                        }
                        descriptorSetLayoutIndex++;
                    }
                    descriptorPoolIndex++;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT) {
            cout << "image at ";
            for (Device& device : devices) {
                u32 memoryIndex = 0;
                for (Memory& memory : device.memories) {
                    u32 imageIndex = 0;
                    for (Image& image : memory.images) {
                        if (image.image == (VkImage)obj) {
                            cout << "devices[" << deviceIndex << "].memories[" << memoryIndex << "].images[" << imageIndex << "]" << std::endl;
                            return;
                        }
                        imageIndex++;
                    }
                    memoryIndex++;
                }
                u32 swapchainIndex = 0;
                for (Swapchain& swapchain : device.swapchains) {
                    u32 imageIndex = 0;
                    for (Image& image : swapchain.images) {
                        if (image.image == (VkImage)obj) {
                            cout << "devices[" << deviceIndex << "].swapchains[" << swapchainIndex << "].images[" << imageIndex << "]" << std::endl;
                            return;
                        }
                        imageIndex++;
                    }
                    memoryIndex++;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT) {
            cout << "buffer at ";
            for (Device& device : devices) {
                u32 memoryIndex = 0;
                for (Memory& memory : device.memories) {
                    u32 bufferIndex = 0;
                    for (Buffer& buffer : memory.buffers) {
                        if (buffer.buffer == (VkBuffer)obj) {
                            cout << "devices[" << deviceIndex << "].memories[" << memoryIndex << "].buffers[" << bufferIndex << "]" << std::endl;
                            return;
                        }
                        bufferIndex++;
                    }
                    memoryIndex++;
                }
                deviceIndex++;
            }
        } else if (objType == VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT) {
            cout << "image view at ";
            for (Device& device : devices) {
                u32 memoryIndex = 0;
                for (Memory& memory : device.memories) {
                    u32 imageIndex = 0;
                    for (Image& image : memory.images) {
                        if (image.imageView == (VkImageView)obj) {
                            cout << "devices[" << deviceIndex << "].memories[" << memoryIndex << "].images[" << imageIndex << "]" << std::endl;
                            return;
                        }
                        imageIndex++;
                    }
                    memoryIndex++;
                }
                u32 swapchainIndex = 0;
                for (Swapchain& swapchain : device.swapchains) {
                    u32 imageIndex = 0;
                    for (Image& image : swapchain.images) {
                        if (image.imageView == (VkImageView)obj) {
                            cout << "devices[" << deviceIndex << "].swapchains[" << swapchainIndex << "].images[" << imageIndex << "]" << std::endl;
                            return;
                        }
                        imageIndex++;
                    }
                    memoryIndex++;
                }
                deviceIndex++;
            }
        }
        cout << "not found???" << std::endl;
    }

}
