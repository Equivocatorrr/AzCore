/*
    File: vk.hpp
    Author: Philip Haynes
    Utilities to manage a Vulkan renderer in an RAII style.
    Using the classes in here, one should be able to set up an entire
    system that manages its own resources in a user-defined tree structure.

    NOTE: This library is not intended to replace an understanding of the Vulkan API,
          but rather to reduce the total amount of code necessary to write in order
          to make good use of it. It does this by making several assumptions based on
          the context of the configurations it's given.
*/
#ifndef VK_HPP
#define VK_HPP

#include "common.hpp"
#include <vulkan/vulkan.h>

namespace io {
    class Window;
}

namespace vk {

    extern String error;

    /*  enum: Type
        Author: Philip Haynes
        How Nodes know what other Nodes are.    */
    enum Type {
        UNKNOWN=0,
        INSTANCE,
        LOGICAL_DEVICE,
    };

    /*  struct: Node
        Author: Philip Haynes
        Any object that has a two-way relationship with another.
        Simply used as a basis to keep references of multiple types of nodes.  */
    struct Node {
        Type type = UNKNOWN;
    };

    /*  struct: Window
        Author: Philip Haynes
        Everything we need to know about a window to use it for drawing     */
    struct Window {
        io::Window *surfaceWindow = nullptr;
        VkSurfaceKHR surface;
    };

    /*  struct: PhysicalDevice
        Author: Philip Haynes
        Some kind of GPU which we use to create our logical device  */
    struct PhysicalDevice {
        i32 score; // How the device rates for desirability (to choose a logical default)
        VkPhysicalDevice physicalDevice;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        Array<VkExtensionProperties> extensionsAvailable{};
        Array<VkQueueFamilyProperties> queueFamiliesAvailable{};
        VkPhysicalDeviceMemoryProperties memoryProperties;
        bool Init(VkInstance instance);
        void PrintInfo(Array<Window> windows, bool checkSurface=false);
    };

    struct Device;

    /*  struct: Image
        Author: Philip Haynes
        How we handle device-local images       */
    struct Image {
        VkDevice device;
        VkImage image;
        bool imageExists = false;
        VkImageView imageView;
        bool imageViewExists = false;

        // Configuration
        VkFormat format;
        VkImageAspectFlags aspectFlags;
        VkImageUsageFlags usage;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        u32 width, height, channels, mipLevels = 1;

        ~Image();
        void Init(VkDevice dev);
        void Clean();
        bool CreateImage(bool hostVisible=false);
        bool CreateImageView();
        // void BindMemory(Memory memory, u32 index);
    };

    /*  struct: Buffer
        Author: Philip Haynes
        What we use for device-local generic data, and to stage transfers       */
    struct Buffer {
        VkDevice device;
        VkBuffer buffer;
        bool exists = false;

        // Configuration
        VkBufferUsageFlags usage;
        VkDeviceSize size;

        ~Buffer();
        void Init(VkDevice dev);
        bool Create();
        // Bind our buffer to a memory at memory.offsets[index]
        // void BindMemory(Memory memory, u32 index);
        // void Copy(VkCommandBuffer commandBuffer, Buffer src);
        void Clean();
    };

    struct Sampler {
        bool exists = false;
        VkDevice device;
        VkSampler sampler;

        // Configuration
        VkFilter magFilter = VK_FILTER_LINEAR;
        VkFilter minFilter = VK_FILTER_LINEAR;
        VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        u32 anisotropy = 1; // 1 is disabled, 4 is low, 8 is medium, and 16 is high
        VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        bool unnormalizedCoordinates = false;
        VkCompareOp compareOp = VK_COMPARE_OP_NEVER;
        VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        f32 mipLodBias = 0.0;
        f32 minLod = 0.0;
        // Change maxLod to an integer multiple of the number of mip levels you generate
        f32 maxLod = 0.0;

        ~Sampler();
        void Init(VkDevice dev);
        bool Create();
        void Clean();
    };

    struct BufferDescriptor {
        Array<Buffer> *buffers;
    };

    struct ImageDescriptor {
        Array<Image> *images;
        ArrayPtr<Sampler> sampler;
    };

    struct DescriptorBinding {
        u32 binding; // Which descriptor we're describing
        u32 count; // How many indices in this descriptor array
    };

    /*  struct: DescriptorLayout
        Author: Philip Haynes
        Describes a single layout that may be used by multiple descriptor sets      */
    struct DescriptorLayout {
        bool exists = false;
        VkDevice device;
        VkDescriptorSetLayout layout;
        // Configuration
        VkDescriptorType type;
        VkShaderStageFlags stage;
        Array<DescriptorBinding> bindings;

        ~DescriptorLayout();
        void Init(VkDevice dev);
        bool Create();
        void Clean();
    };

    struct DescriptorSet {
        bool exists = false;
        VkDescriptorSet set;
        ArrayPtr<DescriptorLayout> layout;

        Array<DescriptorBinding> bindings{};
        Array<BufferDescriptor> bufferDescriptors{};
        Array<ImageDescriptor> imageDescriptors{};

        bool AddDescriptor(Array<Buffer> *buffers, u32 binding);
        bool AddDescriptor(Array<Image> *images, ArrayPtr<Sampler> sampler, u32 binding);
    };

    /*  struct: Descriptors
        Author: Philip Haynes
        Defines a descriptor pool and all descriptor sets from that pool    */
    struct Descriptors {
        VkDevice device;
        bool exists = false;
        VkDescriptorPool pool;

        // Configuration
        Array<DescriptorLayout> layouts{};
        Array<DescriptorSet> sets{};

        ~Descriptors();
        void Init(VkDevice dev);
        ArrayPtr<DescriptorLayout> AddLayout();
        ArrayPtr<DescriptorSet> AddSet(ArrayPtr<DescriptorLayout> layout);

        bool Create();
        bool Update();
        void Clean();
    };

    struct Swapchain;

    /*  struct: Attachment
        Author: Philip Haynes
        Some implicit attachment management that allows
        automated MSAA and depth buffers to be created and used     */
    struct Attachment {
        u32 firstIndex = 0; // Which index in our RenderPass VkAttachmentDescripion Array corresponds to our 0
        Array<VkAttachmentDescription> descriptions{};

        // If swapchain isn't nullptr, our color buffer is what's presented and we use its format
        Swapchain *swapchain = nullptr;
        // Enabling different kinds of outputs
        bool bufferColor = false;
        bool bufferDepthStencil = false;
        // Whether our buffers will be cleared before use
        bool clearColor = false;
        bool clearDepth = false;
        bool clearStencil = false;
        // Whether we should load previous values from the buffers
        // Overwrites clearing if true
        bool loadColor = false;
        bool loadDepth = false;
        bool loadStencil = false;
        // Whether we hold on to our buffers
        bool keepColor = false;
        bool keepDepth = false;
        bool keepStencil = false;
        // Image formats
        VkFormat formatColor = VK_FORMAT_B8G8R8A8_UNORM;
        VkFormat formatDepthStencil = VK_FORMAT_D24_UNORM_S8_UINT;
        // Change this to enable MSAA
        VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
        // Whether we should resolve our multi-sampled images
        bool resolveColor = false;

        Attachment();
        Attachment(Swapchain *swch);
        void Config(); // Generates basic descriptions
    };

    enum AttachmentType {
        ATTACHMENT_COLOR,
        ATTACHMENT_DEPTH_STENCIL,
        ATTACHMENT_RESOLVE,
        ATTACHMENT_ALL // Use this for rendering to an image, and the others for reading from one
    };

    /*  struct: AttachmentUsage
        Author: Philip Haynes
        Defines how a Subpass uses a given attachment in our RenderPass     */
    struct AttachmentUsage {
        u32 index; // Which attachment we're using
        // Out of an Attachment that can have multiple attachments, this defines which one
        AttachmentType type = ATTACHMENT_ALL;
        VkAccessFlags accessFlags; // Describes how this attachment is accessed in the Subpass
    };

    /*  struct: Subpass
        Author: Philip Haynes
        Basic configuration of a subpass, which is then completed by creation of the RenderPass */
    struct Subpass {
        // The indices of attachments added to the RenderPass
        Array<AttachmentUsage> attachments{};
        // Subpass configuration
        Array<VkAttachmentReference> referencesColor{};
        Array<VkAttachmentReference> referencesResolve{};
        Array<VkAttachmentReference> referencesInput{};
        Array<u32> referencesPreserve{};
        VkAttachmentReference referenceDepthStencil{};
        // NOTE: Do we need this? Can renderpasses be used outside of graphics?
        VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        void UseAttachment(ArrayPtr<Attachment> attachment, AttachmentType type, VkAccessFlags accessFlags);
    };

    /*  struct: RenderPass
        Author: Philip Haynes
        Automatically configures RenderPass based on Subpasses      */
    struct RenderPass {
        bool initted = false;
        Device *device = nullptr;
        VkRenderPass renderPass{};
        Array<VkAttachmentDescription> attachmentDescriptions;
        Array<VkSubpassDescription> subpassDescriptions;
        Array<VkSubpassDependency> subpassDependencies;

        // Configuration
        Array<Subpass> subpasses{};
        // Each can contain up to 3 actual attachments
        Array<Attachment> attachments{};

        // Dependency configuration
        // Use these to transition attachment image layouts
        // Initial goes from external to subpass 0
        bool initialTransition = true; // Whether we enable this transition
        // The access mask we expect the first subpass attachments to be in
        VkAccessFlags initialAccess = VK_ACCESS_MEMORY_READ_BIT;
        // The stage at which we first start using our attachments
        VkPipelineStageFlagBits initialAccessStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        // Final goes from the last subpass to external
        bool finalTransition = true;
        // The access mask for transitioning our last subpass attachments for use outside of the RenderPass
        VkAccessFlags finalAccess = VK_ACCESS_MEMORY_READ_BIT; // Default, used for swapchain presenting
        // The stage at which our attachments are expected to be "done"
        VkPipelineStageFlagBits finalAccessStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        ArrayPtr<Subpass> AddSubpass();
        ArrayPtr<Attachment> AddAttachment(Swapchain *swapchain = nullptr);

        ~RenderPass();
        bool Init(Device *dev);
        bool Deinit();
    };

    extern const char *QueueTypeString[5];

    enum QueueType {
        UNDEFINED=0,
        COMPUTE=1,
        GRAPHICS=2,
        TRANSFER=3,
        PRESENT=4
    };

    /*  struct: Queue
        Author: Philip Haynes
        What we use to submit work to the GPU       */
    struct Queue {
        VkQueue queue;
        i32 queueFamilyIndex = -1;
        QueueType queueType = UNDEFINED;
        f32 queuePriority = 1.0;
    };

    /*  struct: Swapchain
        Author: Philip Haynes
        Manages how we interact with our window surface         */
    struct Swapchain {
        bool initted = false;
        bool created = false;
        Device *device = nullptr;
        VkSwapchainKHR swapchain{};
        VkSurfaceKHR surface{};
        Array<Image> images{};
        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR presentMode;
        VkExtent2D extent;
        u32 imageCount = 2;

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        Array<VkSurfaceFormatKHR> surfaceFormats{};
        Array<VkPresentModeKHR> presentModes{};

        // Configuration
        VkSurfaceFormatKHR formatPreferred = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}; // You probably won't need to change this
        bool vsync = true; // To determine the ideal present mode
        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        u32 imageCountPreferred = 2;
        ArrayPtr<Window> window{};

        Swapchain();
        ~Swapchain();
        bool Init(Device *dev);
        bool Create();
        bool Reconfigure();
        bool Deinit();
    };

    struct Instance;

    /*  struct: Device
        Author: Philip Haynes
        Our interface to actually use our physical GPUs to do work  */
    struct Device {
        bool initted = false;
        Instance *instance = nullptr;
        PhysicalDevice physicalDevice;
        VkDevice device;

        // Resources and structures
        List<Queue> queues{};
        List<Swapchain> swapchains{};
        List<RenderPass> renderPasses{};
        List<Array<Image>> images{};
        List<Array<Buffer>> buffers{};
        Array<Sampler> samplers{};
        List<Descriptors> descriptors{};

        // Manual configuration (mostly unnecessary)
        Array<const char*> extensionsRequired{};
        VkPhysicalDeviceFeatures deviceFeaturesRequired{};
        VkPhysicalDeviceFeatures deviceFeaturesOptional{};
        Device();
        ~Device();

        Queue* AddQueue();
        Swapchain* AddSwapchain();
        RenderPass* AddRenderPass();
        Array<Image>* AddImages(u32 count);
        Array<Buffer>* AddBuffers(u32 count);
        ArrayPtr<Sampler> AddSampler();
        Descriptors* AddDescriptors();

        bool Init(Instance *inst);
        bool Reconfigure();
        bool Deinit();
    };

    /*  struct: Instance
        Author: Philip Haynes
        More or less the context for the whole renderer.
        Manages the state of everything else in this toolkit.
        Used as a top-level control of all of the tasks created for it to execute.  */
    struct Instance {
        PFN_vkCreateDebugReportCallbackEXT
            fpCreateDebugReportCallbackEXT;
        PFN_vkDestroyDebugReportCallbackEXT
            fpDestroyDebugReportCallbackEXT;
        bool initted = false;
        bool enableLayers = false;
        VkInstance instance;
        Array<Window> windows{};
        VkDebugReportCallbackEXT debugReportCallback;
        VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "AzCore Test",
            .applicationVersion = 1,
            .pEngineName = "AzCore",
            .engineVersion = VK_MAKE_VERSION(0, 1, 0),
            .apiVersion = VK_API_VERSION_1_1
        };
        Array<VkExtensionProperties> extensionsAvailable;
        Array<const char*> extensionsRequired{};
        Array<VkLayerProperties> layersAvailable;
        Array<const char*> layersRequired{};

        Array<PhysicalDevice> physicalDevices{};
        // We hold and Init() the devices according to their parameters.
        List<Device> devices{};

        Instance();
        ~Instance();

        // Configuring functions
        void AppInfo(const char *name, u32 versionMajor, u32 versionMinor, u32 versionPatch);
        ArrayPtr<Window> AddWindowForSurface(io::Window *window);
        void AddExtensions(Array<const char*> extensions);
        void AddLayers(Array<const char*> layers);

        Device* AddDevice();

        // If the instance is active, you must call this for the changes to be effective.
        bool Reconfigure();

        bool Initted() const;
        bool Init(); // Constructs the entire tree
        bool Deinit(); // Cleans everything up
    };

}

#endif
