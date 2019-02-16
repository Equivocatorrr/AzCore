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
        VkFormat format;
        VkImageAspectFlags aspectFlags;
        VkImageUsageFlags usage;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        u32 width, height, channels, mipLevels = 1;
        VkImage image;
        bool imageExists = false;
        VkImageView imageView;
        bool imageViewExists = false;

        void Init(VkDevice dev);
        void Clean();
        bool CreateImage(bool hostVisible=false);
        bool CreateImageView();
        // void BindMemory(VulkanMemory memory, u32 index);
    };

    /*  struct: Attachment
        Author: Philip Haynes
        Some implicit attachment management that allows
        automated MSAA and depth buffers to be created and used     */
    struct Attachment {
        Array<VkAttachmentDescription> descriptions{};

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

        void Config(VkSampleCountFlagBits sampleCount, bool resolveColor); // Generates basic descriptions
    };

    /*  struct: Subpass
        Author: Philip Haynes
        Basic configuration of a subpass, which is then completed by creation of the RenderPass */
    struct Subpass {
        Array<Attachment> attachmentDescriptions{}; // Each can contain up to 4 attachments
        // Subpass configuration
        Array<VkAttachmentReference> attachmentReferencesColor{};
        Array<VkAttachmentReference> attachmentReferencesResolve{};
        VkAttachmentReference attachmentReferenceDepthStencil{};
        // Dependency configuration
        VkAccessFlagBits initialAccess{};
        VkAccessFlagBits finalAccess = VkAccessFlagBits(VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        VkPipelineStageFlagBits initialAccessStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkPipelineStageFlagBits finalAccessStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        // NOTE: Do we need this? Can renderpasses be used outside of graphics? Probably not.
        VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        // Attachment configuration
        // Change this to enable MSAA
        VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
        // Whether we should resolve our multi-sampled images
        bool resolveColor = true;

        ArrayPtr<Attachment> AddAttachment();

        void Config();
    };

    /*  struct: RenderPass
        Author: Philip Haynes
        Automatically configures renderpass based on Subpasses      */
    struct RenderPass {
        bool initted = false;
        bool created = false;
        Device *device = nullptr;
        VkRenderPass renderPass{};
        Array<VkAttachmentDescription> attachments;
        Array<VkSubpassDescription> subpassDescriptions;
        Array<VkSubpassDependency> subpassDependencies;

        // Configuration
        List<Subpass> subpasses{};

        Subpass* AddSubpass();

        bool Init(Device *dev);
        bool Create();
        void Clean();
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
        List<Queue> queues{};
        List<Swapchain> swapchains{};
        Array<const char*> extensionsRequired{};
        VkPhysicalDeviceFeatures deviceFeaturesRequired{};
        VkPhysicalDeviceFeatures deviceFeaturesOptional{};
        Device();
        ~Device();

        Queue* AddQueue();
        Swapchain* AddSwapchain();

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
