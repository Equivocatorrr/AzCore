/*
    File: vk.hpp
    Author: Philip Haynes
    Utilities to manage a Vulkan renderer that can handle many common use-cases.
    Using the classes in here, one should be able to set up an entire
    system that manages its own resources in a user-defined tree structure.

    The ideal final product is one where recovering from most errors is done automatically,
    or in a way that makes it easy for the developer to make a logical decision and
    handle it gracefully.

    NOTE: This library is not intended to replace an understanding of the Vulkan API,
          but rather to reduce the total amount of code necessary to write in order
          to make good use of it. It does this by making several inferences based on
          the context of the configurations it's given.
          Also provided are numerous sanity checks to make development smoother, even
          if you don't have a solid grasp of the Vulkan API.
    TODO:
        - Ongoing: Add detection of features based on use.
        - Add pipeline cache support.
        - Add support for compute pipelines.
        - Make control over memory more dynamic.
        - Add some sort of texture caching/streaming setup in RAM.
        - Add example implementations for more feature-complete pipelines
              (including tesselation, geometry shaders, etc.)
        - Add support for screenshots.
*/
#ifndef AZCORE_VK_HPP
#define AZCORE_VK_HPP

#ifdef NDEBUG
    // This define will get rid of most sanity checks,
    // especially those related to Vulkan Tree Structure.
    // It's advised to keep the sanity checks when designing your program.
    #define AZCORE_VK_SANITY_CHECKS_MINIMAL
    // This disables console logging
    #define AZCORE_VK_LOGGING_NO_CONSOLE
#endif

// Having this defined makes keeping track of host memory precisely impossible
#define VK_NO_ALLOCATION_CALLBACKS

#include "common.hpp"
#include <vulkan/vulkan.h>

namespace AzCore {

namespace io {
    class Window;
}

namespace vk {

    String ErrorString(VkResult);

    extern String error;

    struct Window;
    struct PhysicalDevice;
    struct Image;
    struct Buffer;
    struct Memory;
    struct Sampler;
    struct DescriptorLayout;
    struct DescriptorSet;
    struct Descriptors;
    struct Attachment;
    struct Subpass;
    struct RenderPass;
    struct Framebuffer;
    struct Semaphore;
    struct Shader;
    struct ShaderRef;
    struct Pipeline;
    struct Queue;
    struct CommandBuffer;
    struct CommandPool;
    struct Swapchain;
    struct SemaphoreWait;
    struct QueueSubmission;
    struct Device;
    struct Instance;

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

    /*  struct: Image
        Author: Philip Haynes
        How we handle device-local images       */
    struct Image {
        struct {
            Device *device = nullptr;
            VkImage image;
            bool imageExists = false;
            VkImageView imageView;
            bool imageViewExists = false;
            String debugMarker[2]={{}}; // One for image, the other for imageView
            Memory *memory = nullptr;
            i32 offsetIndex;
        } data;

        // Configuration
        VkFormat format;
        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        VkImageUsageFlags usage;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        u32 width, height, mipLevels = 1;

        void Init(Device *device, String debugMarker = String());
        bool CreateImage(bool hostVisible=false);
        bool CreateImageView();
        // Copy to host-visible memory
        void CopyData(void *src, u32 bytesPerPixel); // We assume we're copying from something big enough
        void TransitionLayout(VkCommandBuffer commandBuffer, VkImageLayout from, VkImageLayout to, u32 baseMipLevel=0, u32 mipLevelCount=1);
        void TransitionLayout(VkCommandBuffer commandBuffer, VkImageLayout from, VkImageLayout to, VkImageSubresourceRange subResourceRange);
        void Copy(VkCommandBuffer commandBuffer, Ptr<Buffer> src);
        void GenerateMipMaps(VkCommandBuffer commandBuffer, VkImageLayout startingLayout, VkImageLayout finalLayout);
        void Clean();
        // void BindMemory(Memory memory, u32 index);
    };

    /*  struct: Buffer
        Author: Philip Haynes
        What we use for device-local generic data, and to stage transfers       */
    struct Buffer {
        struct {
            Device *device = nullptr;
            VkBuffer buffer;
            String debugMarker{};
            bool exists = false;
            Memory *memory = nullptr;
            i32 offsetIndex;
        } data;

        // Configuration
        VkBufferUsageFlags usage;
        VkDeviceSize size;

        void Init(Device *dev, String debugMarker = String());
        bool Create();
        // Bind our buffer to a memory at memory.offsets[index]
        // void BindMemory(Memory memory, u32 index);
        // Copy to host-visible memory
        void CopyData(void *src, VkDeviceSize copySize=0, VkDeviceSize dstOffset=0);
        // Record a copy command to commandBuffer
        void Copy(VkCommandBuffer commandBuffer, Ptr<Buffer> src,
                VkDeviceSize copySize=0, VkDeviceSize dstOffset=0, VkDeviceSize srcOffset=0);
        void QueueOwnershipRelease(VkCommandBuffer commandBuffer, Ptr<Queue> srcQueue, Ptr<Queue> dstQueue,
                                   VkAccessFlags srcAccessMask, VkPipelineStageFlags srcStageMask);
        void QueueOwnershipAcquire(VkCommandBuffer commandBuffer, Ptr<Queue> srcQueue, Ptr<Queue> dstQueue,
                                   VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask);
        void Clean();
    };

    /*  struct: Memory
        Author: Philip Haynes
        We use this to preinitialize memory allocations such that we can
        use a singular allocation block to bind to multiple chunks of data.
        TODO: Add a dynamic allocator.       */
    struct Memory {
        struct {
            PhysicalDevice *physicalDevice;
            Device *device = nullptr;
            VkDeviceMemory memory;
            String debugMarker{};
            bool initted = false;
            bool allocated = false;
            bool mapped = false;
            Array<VkDeviceSize> offsets{}; // size of each chunk is x[i+1] - x[i]
            u32 memoryTypeBits = 0;
            // What we really want
            VkMemoryPropertyFlags memoryProperties;
            // What we'll settle for if the above isn't available
            VkMemoryPropertyFlags memoryPropertiesDeferred;

            Array<Image> images{};
            Array<Buffer> buffers{};
        } data;

        // Configuration
        bool deviceLocal = true; // If false, it's host visible

        Ptr<Image> AddImage(Image image=Image());
        Ptr<Buffer> AddBuffer(Buffer buffer=Buffer());
        Range<Image> AddImages(u32 count, Image image=Image());
        Range<Buffer> AddBuffers(u32 count, Buffer buffer=Buffer());

        // Behind the scenes
        bool Init(Device *device, String debugMarker = String());
        bool Deinit();
        // Adds a chunk of memory for an image
        // Returns the index to the corresponding offset or -1 for failure
        i32 GetImageChunk(Image image, bool noChange=false);
        i32 GetBufferChunk(Buffer buffer, bool noChange=false);
        VkDeviceSize ChunkSize(i32 index);
        i32 FindMemoryType();
        // Allocates everything
        bool Allocate();
        // Allocates a chunk of data for later use
        // bool PreAllocate(VkDeviceSize size, Buffer type);
        // Copies any data to host-visible memory using offsets[index]
        void CopyData(void *src, VkDeviceSize size, i32 index);
        void CopyData2D(void *src, Ptr<Image> image, i32 index, u32 bytesPerPixel);
        void* MapMemory(VkDeviceSize size, i32 index); // Returns a pointer to our host-visible data
        void UnmapMemory();
    };

    struct Sampler {
        struct {
            bool exists = false;
            Device *device = nullptr;
            VkSampler sampler;
            String debugMarker{};
        } data;

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
        f32 mipLodBias = 0.0f;
        f32 minLod = 0.0f;
        // Change maxLod to an integer multiple of the number of mip levels you generate
        f32 maxLod = 0.0f;

        ~Sampler();
        void Init(Device *device, String debugMarker = String());
        bool Create();
        void Clean();
    };

    struct BufferDescriptor {
        Range<Buffer> buffers;
    };

    struct ImageDescriptor {
        Range<Image> images;
        Ptr<Sampler> sampler;
    };

    struct DescriptorBinding {
        i32 binding; // Which descriptor we're describing
        i32 count; // How many indices in this descriptor array
    };

    /*  struct: DescriptorLayout
        Author: Philip Haynes
        Describes a single layout that may be used by multiple descriptor sets      */
    struct DescriptorLayout {
        struct {
            bool exists = false;
            Device *device = nullptr;
            VkDescriptorSetLayout layout;
            String debugMarker{};
        } data;
        // Configuration
        VkDescriptorType type;
        VkShaderStageFlags stage;
        Array<DescriptorBinding> bindings{};

        ~DescriptorLayout();
        void Init(Device *device, String debugMarker = String());
        bool Create();
        void Clean();
    };

    struct DescriptorSet {
        struct {
            bool exists = false;
            VkDescriptorSet set;
            String debugMarker{};
            Ptr<DescriptorLayout> layout;

            Array<DescriptorBinding> bindings{};
            Array<BufferDescriptor> bufferDescriptors{};
            Array<ImageDescriptor> imageDescriptors{};
        } data;

        bool AddDescriptor(Range<Buffer> buffers, i32 binding);
        bool AddDescriptor(Range<Image> images, Ptr<Sampler> sampler, i32 binding);
        bool AddDescriptor(Ptr<Buffer> buffer, i32 binding);
        bool AddDescriptor(Ptr<Image> image, Ptr<Sampler> sampler, i32 binding);
    };

    /*  struct: Descriptors
        Author: Philip Haynes
        Defines a descriptor pool and all descriptor sets from that pool    */
    struct Descriptors {
        struct {
            Device *device = nullptr;
            bool exists = false;
            VkDescriptorPool pool;
            String debugMarker{};

            Array<DescriptorLayout> layouts{};
            Array<DescriptorSet> sets{};
        } data;

        ~Descriptors();
        void Init(Device *device, String debugMarker = String());
        Ptr<DescriptorLayout> AddLayout();
        Ptr<DescriptorSet> AddSet(Ptr<DescriptorLayout> layout);

        bool Create();
        bool Update();
        void Clean();
    };

    /*  struct: Attachment
        Author: Philip Haynes
        Some implicit attachment management that allows
        automated MSAA and depth buffers to be created and used     */
    struct Attachment {
        struct {
            i32 firstIndex = 0; // Which index in our RenderPass VkAttachmentDescripion Array corresponds to our 0
            Array<VkAttachmentDescription> descriptions{};
        } data;

        // If swapchain isn't nullptr, our color buffer is what's presented and we use its format
        Ptr<Swapchain> swapchain = nullptr;
        // Enabling different kinds of outputs
        bool bufferColor = false;
        bool bufferDepthStencil = false;
        // The image layouts we expect the images to be in.
        // You only need to change these if the contents of the images should be preserved between renderPasses.
        // If clear is true or load is false, you can leave these be.
        VkImageLayout initialLayoutColor = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout initialLayoutDepthStencil = VK_IMAGE_LAYOUT_UNDEFINED;
        // Whether our buffers will be cleared before use
        bool clearColor = false;
        bool clearDepth = false;
        bool clearStencil = false;
        VkClearColorValue clearColorValue = {0.0f, 0.0f, 0.0f, 1.0f};
        VkClearDepthStencilValue clearDepthStencilValue = {1.0f, 0};
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
        VkFormat formatDepthStencil = VK_FORMAT_D32_SFLOAT;
        // Change this to enable MSAA
        VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
        // Whether we should resolve our multi-sampled images
        bool resolveColor = false;

        Attachment();
        Attachment(Ptr<Swapchain> swch);
        bool Config(); // Generates basic descriptions
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
        i32 index; // Which attachment we're using
        // Out of an Attachment that can have multiple attachments, this defines which one
        AttachmentType type = ATTACHMENT_ALL;
        VkAccessFlags accessFlags; // Describes how this attachment is accessed in the Subpass
    };

    /*  struct: Subpass
        Author: Philip Haynes
        Basic configuration of a subpass, which is then completed by creation of the RenderPass */
    struct Subpass {
        struct {
            // The indices of attachments added to the RenderPass
            Array<AttachmentUsage> attachments{};
            // Subpass configuration
            Array<VkAttachmentReference> referencesColor{};
            Array<VkAttachmentReference> referencesResolve{};
            Array<VkAttachmentReference> referencesInput{};
            Array<u32> referencesPreserve{};
            VkAttachmentReference referenceDepthStencil{};
        } data;
        // NOTE: Do we need this? Can renderpasses be used outside of graphics?
        VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // Use one or more images from attachment, depends on AttachmentType
        // For single-subpass renderpasses, ATTACHMENT_ALL should suffice
        // For input attachments, you have to specify which image you're using
        //      Color inputs should be ATTACHMENT_COLOR
        //      Depth/Stencil inputs should be ATTACHMENT_DEPTH_STENCIL
        //      Resolved multisampled inputs should be ATTACHMENT_RESOLVE
        // accessFlags should reflect how the image is accessed in the subpass
        //      For single-subpass renderpasses, the following should suffice:
        //          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        //      For depth-only subpasses try:
        //          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
        //      For input attachments, try:
        //          VK_ACCESS_INPUT_ATTACHMENT_READ_BIT
        void UseAttachment(Ptr<Attachment> attachment, AttachmentType type, VkAccessFlags accessFlags);
    };

    /*  struct: RenderPass
        Author: Philip Haynes
        Automatically configures RenderPass based on Subpasses      */
    struct RenderPass {
        struct {
            bool initted = false;
            Device *device = nullptr;
            VkRenderPass renderPass{};
            String debugMarker{};
            Array<VkAttachmentDescription> attachmentDescriptions;
            Array<VkSubpassDescription> subpassDescriptions;
            Array<VkSubpassDependency> subpassDependencies;

            Array<Subpass> subpasses{};
            // Each can contain up to 3 actual attachments
            Array<Attachment> attachments{};
            // TODO: Multiview renderPasses
        } data;


        // Dependency configuration
        // Use these to transition attachment image layouts
        // Initial goes from external to subpass 0
        bool initialTransition = true; // Whether we enable this transition
        // The access mask we expect the first subpass attachments to be in
        // If we don't do anything with the attachments between frames, this should be the same as finalAccess.
        VkAccessFlags initialAccess = VK_ACCESS_MEMORY_READ_BIT;
        // The stage at which we first start using our attachments
        VkPipelineStageFlagBits initialAccessStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        // Final goes from the last subpass to external
        bool finalTransition = true;
        // The access mask for transitioning our last subpass attachments for use outside of the RenderPass
        VkAccessFlags finalAccess = VK_ACCESS_MEMORY_READ_BIT; // Default, used for swapchain presenting
        // The stage at which our attachments are expected to be "done"
        VkPipelineStageFlagBits finalAccessStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        Ptr<Subpass> AddSubpass();
        Ptr<Attachment> AddAttachment(Ptr<Swapchain> swapchain = nullptr);

        void Begin(VkCommandBuffer commandBuffer, Ptr<Framebuffer> framebuffer, bool subpassContentsInline=true);

        ~RenderPass();
        bool Init(Device *dev, String debugMarker = String());
        bool Deinit();
    };

    /*  struct: Framebuffer
        Author: Philip Haynes
        All the actual images we're drawing to in a single render pass (including all subpasses).   */
    struct Framebuffer {
        struct {
            bool initted = false, created = false;
            Device *device = nullptr;
            String debugMarker{};
            Array<VkFramebuffer> framebuffers{};
            Array<String> debugMarkers{};
            u32 currentFramebuffer = 0; // This can be set manually, or inherited from a Swapchain image acquisition
        } data;

        // Configuration
        Ptr<RenderPass> renderPass = nullptr;
        // If renderPass hooks up to a swapchain, so should the framebuffer.
        // For swapchains with multiple buffers, we will have multiple framebuffers, one for each swapchain image.
        // That way, we don't have to have duplicate images for back-end rendering,
        // and only the final image should have multiple images allocated.
        Ptr<Swapchain> swapchain = nullptr;
        // If swapchain is not nullptr, this will be set to however many swapchain images there are.
        i32 numFramebuffers = 1;
        Array<Array<Ptr<Image>>> attachmentImages{};
        // If renderPass is connected to a swapchain, these values will be set automatically
        u32 width=0, height=0;
        // This means that the Framebuffer will create its own distinct memory.
        // Disable this if you plan to allocate multiple framebuffers from a single memory pool.
        bool ownMemory = true;
        // If ownMemory is false, you need to provide valid Memory pointers
        Memory* depthMemory = nullptr;
        Memory* colorMemory = nullptr;
        // This means that the framebuffer will automatically allocate images from the above memory.
        // Disable this if you want more precise control over the images.
        bool ownImages = true;
        // If ownImages is false, you need to provide valid attachmentImages.

        ~Framebuffer();
        bool Init(Device *dev, String debugMarker = String());
        bool Create();
        void Destroy();
        bool Deinit();
    };

    /*  struct: Semaphore
        Author: Philip Haynes
        Only really holds the semaphore and the debugMarker string.     */
    struct Semaphore {
        VkSemaphore semaphore = VK_NULL_HANDLE;
        String debugMarker{};
    };

    /*  struct: Shader
        Author: Philip Haynes
        A single shader module, which can load in Spirv code for a single shader.   */
    struct Shader {
        struct {
            bool initted = false;
            Device *device = nullptr;
            Array<u32> code;
            VkShaderModule module;
            String debugMarker{};
        } data;

        // Configuration
        String filename{};

        bool Init(Device *device, String debugMarker = String());
        void Clean();
    };

    /*  struct: ShaderRef
        Author: Philip Haynes
        A reference to a single function in a shader module for a single shader stage.  */
    struct ShaderRef {
        Ptr<Shader> shader;
        VkShaderStageFlagBits stage;
        String functionName{}; // Most shaders will probably use just main, but watch out

        ShaderRef(String fn="main");
        ShaderRef(Ptr<Shader> ptr, VkShaderStageFlagBits s, String fn="main");
    };

    /*  struct: Pipeline
        Author: Philip Haynes
        Everything you need for a complete graphics pipeline
        Most things have usable defaults to help with brevity        */
    struct Pipeline {
        struct {
            Ptr<Device> device = nullptr;
            bool initted = false;
            VkPipelineLayout layout;
            VkPipeline pipeline;
            String debugMarker{};
            VkPipelineMultisampleStateCreateInfo multisampling{}; // Infer most from RenderPass
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{}; // Infer from vertex buffer
        } data;

        // Configuration
        Ptr<RenderPass> renderPass = nullptr;
        Array<ShaderRef> shaders{};
        i32 subpass = 0; // Of our RenderPass, which subpass are we used in?
        bool multisampleShading = false;
        // TODO: Break this up into simpler more bite-sized pieces
        Array<VkVertexInputBindingDescription> inputBindingDescriptions{};
        Array<VkVertexInputAttributeDescription> inputAttributeDescriptions{};
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        Array<VkPipelineColorBlendAttachmentState> colorBlendAttachments{};
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        Array<VkDynamicState> dynamicStates{};
        Array<Ptr<DescriptorLayout>> descriptorLayouts{};
        Array<VkPushConstantRange> pushConstantRanges{};

        void Bind(VkCommandBuffer commandBuffer) const;

        Pipeline(); // We configure some defaults
        ~Pipeline();
        bool Init(Device *dev, String debugMarker = String());
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
        VkQueue queue = VK_NULL_HANDLE;
        String debugMarker{};
        i32 queueFamilyIndex = -1;
        QueueType queueType = UNDEFINED;
        f32 queuePriority = 1.0f;
    };

    /*  struct: CommandBuffer
        Author: Philip Haynes
        What we use to control our command buffers
        that get allocated from Command Pools           */
    struct CommandBuffer {
        struct {
            bool recording = false;
            CommandPool *pool = nullptr;
            Device *device = nullptr;
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
            String debugMarker{};
        } data;

        // Configuration
        // Use this if the command buffer will only be submitted once and then reused
        bool oneTimeSubmit = false;
        // Use this if the command buffer can be used multiple times without re-recording
        bool simultaneousUse = false;
        // Whether resetting the CommandBuffer should release resources back to the CommandPool
        bool releaseResourcesOnReset = false;

        // Secondary-only stuff
        bool secondary = false; // If this is false, all the below configuration values are ignored.
        // Use this if it's a secondary command buffer completely inside a render pass
        bool renderPassContinue = false;
        // If used inside a render pass, we need to know which one, and then which subpass
        Ptr<RenderPass> renderPass = nullptr;
        u32 subpass = 0;
        // Do we know which framebuffer we'll be using? Not strictly necessary.
        Ptr<Framebuffer> framebuffer = nullptr;
        // If the primary command buffer is running an occlusion query, this must be true
        bool occlusionQueryEnable = false;
        // We also need to know about the query
        VkQueryControlFlags queryControlFlags{};
        VkQueryPipelineStatisticFlags queryPipelineStatisticFlags{};

        // Usage functions
        VkCommandBuffer Begin(); // Starts recording. Returns VK_NULL_HANDLE on failure.
        bool End();
        bool Reset();
    };

    /*  struct: CommandPool
        Author: Philip Haynes
        What we use to allocate command buffers.
        For multi-threaded situations, we need one pool per thread to avoid using mutexes.     */
    struct CommandPool {
        struct {
            bool initted = false;
            Device *device = nullptr;
            VkCommandPool commandPool;
            String debugMarker{};
            Array<CommandBuffer> commandBuffers{};
            List<CommandBuffer> dynamicBuffers{};
        } data;

        // Configuration
        // Use when command buffers will be reset or freed shortly after executing
        bool transient = false;
        // Use when command buffers are meant to be reusable
        bool resettable = false;
        // I'm not sure what this is used for
        bool protectedMemory = false;
        // Which queue this pool will be used on
        Ptr<Queue> queue;

        Ptr<CommandBuffer> AddCommandBuffer();

        // Commands you can call after Vulkan Tree initialization
        Ptr<CommandBuffer> CreateDynamicBuffer(bool secondary=false);
        void DestroyDynamicBuffer(CommandBuffer* buffer);


        CommandPool(Ptr<Queue> q=nullptr);
        ~CommandPool();
        bool Init(Device *dev, String debugMarker = String());
        void Clean();
    };

    /*  struct: Swapchain
        Author: Philip Haynes
        Manages how we interact with our window surface         */
    struct Swapchain {
        struct {
            bool initted = false;
            bool created = false;
            Device *device = nullptr;
            VkSwapchainKHR swapchain{};
            String debugMarker{};
            VkSurfaceKHR surface{};
            Array<Image> images{};
            VkSurfaceFormatKHR surfaceFormat;
            VkPresentModeKHR presentMode;
            VkExtent2D extent;
            u32 imageCount = 2;
            u32 currentImage = 0;
            bool buffer = true; // Which semaphore are we going to signal?
            // We need semaphores to synchronize image acquisition
            Ptr<Semaphore> semaphores[2] = {{}};
            // Keep pointers to all the Framebuffers that use our images so we can make sure they're using the right image.
            Array<Ptr<Framebuffer>> framebuffers{};

            VkSurfaceCapabilitiesKHR surfaceCapabilities;
            Array<VkSurfaceFormatKHR> surfaceFormats{};
            Array<VkPresentModeKHR> presentModes{};
        } data;

        // Configuration
        VkSurfaceFormatKHR formatPreferred = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}; // You probably won't need to change this
        bool vsync = true; // To determine the ideal present mode
        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        u32 imageCountPreferred = 2;
        Ptr<Window> window{};
        // How long we will wait for an image before timing out in nanoseconds
        u64 timeout = UINT64_MAX;

        VkResult AcquireNextImage();
        Ptr<Semaphore> SemaphoreImageAvailable();
        bool Present(Ptr<Queue> queue, Array<VkSemaphore> waitSemaphores);

        bool Resize();

        ~Swapchain();
        bool Init(Device *dev, String debugMarker = String());
        bool Create();
        bool Reconfigure();
        bool Deinit();
    };

    struct SemaphoreWait {
        // TODO: Maybe make Ptr polymorphic?
        Ptr<Semaphore> semaphore;
        Ptr<Swapchain> swapchain;
        VkPipelineStageFlags dstStageMask;
        SemaphoreWait()=default;
        SemaphoreWait(Ptr<Semaphore> inSemaphore, VkPipelineStageFlags inDstStageMask);
        SemaphoreWait(Ptr<Swapchain> inSwapchain, VkPipelineStageFlags inDstStageMask);
    };

    /*  struct: QueueSubmission
        Author: Philip Haynes
        Manages a single VkSubmitInfo, making sure it's up to date, and only updating when necessary.   */
    struct QueueSubmission {
        struct {
            VkSubmitInfo submitInfo={VK_STRUCTURE_TYPE_SUBMIT_INFO};
            Array<VkCommandBuffer> commandBuffers{};
            Array<VkSemaphore> waitSemaphores{};
            Array<VkPipelineStageFlags> waitDstStageMasks{};
            Array<VkSemaphore> signalSemaphores{};
        } data;

        // Configuration
        Array<Ptr<CommandBuffer>> commandBuffers{};
        Array<SemaphoreWait> waitSemaphores{};
        Array<Ptr<Semaphore>> signalSemaphores{};
        // Set this to true if you plan to Config manually (like if it waits on a Swapchain)
        bool noAutoConfig = false;

        bool Config();
    };

    /*  struct: Device
        Author: Philip Haynes
        Our interface to actually use our physical GPUs to do work  */
    struct Device {
        struct {
            bool initted = false;
            Instance *instance = nullptr;
            PhysicalDevice physicalDevice;
            VkDevice device;
            String debugMarker{};

            // Resources and structures
            List<Queue> queues{};
            List<Swapchain> swapchains{};
            List<RenderPass> renderPasses{};
            List<Memory> memories{};
            Array<Sampler> samplers{};
            List<Descriptors> descriptors{};
            Array<Shader> shaders{};
            List<Pipeline> pipelines{};
            List<CommandPool> commandPools{};
            List<Framebuffer> framebuffers{};
            Array<Semaphore> semaphores{};
            List<QueueSubmission> queueSubmissions{};

            Array<const char*> extensionsRequired{};
            VkPhysicalDeviceFeatures deviceFeaturesRequired{};
            VkPhysicalDeviceFeatures deviceFeaturesOptional{};
        } data;

        Device();
        ~Device();

        Ptr<Queue> AddQueue();
        Ptr<Swapchain> AddSwapchain();
        Ptr<RenderPass> AddRenderPass();
        Ptr<Sampler> AddSampler();
        Ptr<Memory> AddMemory();
        Ptr<Descriptors> AddDescriptors();
        Ptr<Shader> AddShader();
        Range<Shader> AddShaders(u32 count);
        Ptr<Pipeline> AddPipeline();
        Ptr<CommandPool> AddCommandPool(Ptr<Queue> queue);
        Ptr<Framebuffer> AddFramebuffer();
        Ptr<Semaphore> AddSemaphore();
        Ptr<QueueSubmission> AddQueueSubmission();

        // TODO: Add Fence support to this
        bool SubmitCommandBuffers(Ptr<Queue> queue, Array<Ptr<QueueSubmission>> submissions);

        bool Init(Instance *inst, String debugMarker=String());
        bool Reconfigure();
        bool Deinit();
    };

    /*  struct: Instance
        Author: Philip Haynes
        More or less the context for the whole renderer.
        Manages the state of everything created directly from the VkInstance.
        Used as a top-level control of the Vulkan Tree.  */
    struct Instance {
        struct data_t {
            PFN_vkSetDebugUtilsObjectNameEXT
                fpSetDebugUtilsObjectNameEXT;
            PFN_vkCreateDebugUtilsMessengerEXT
                fpCreateDebugUtilsMessengerEXT;
            PFN_vkDestroyDebugUtilsMessengerEXT
                fpDestroyDebugUtilsMessengerEXT;
            VkDebugUtilsMessengerEXT debugUtilsMessenger;
            bool initted = false;
            bool enableLayers = false;
            VkInstance instance;
            Array<Window> windows{};
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
#ifndef VK_NO_ALLOCATION_CALLBACKS
            // Information about allocated data
            struct Allocation {
                void *ptr;
                size_t size;
            };
            List<Allocation> allocations{};
            Mutex allocationMutex;
            size_t totalHeapMemory=0;
            VkAllocationCallbacks allocationCallbacks;
#endif
        } data;

        Instance();
        ~Instance();

        // Configuring functions
        void AppInfo(const char *name, u32 versionMajor, u32 versionMinor, u32 versionPatch);
        Ptr<Window> AddWindowForSurface(io::Window *window);
        void AddExtensions(Array<const char*> extensions);
        void AddLayers(Array<const char*> layers);

        // Change this if you want to distinguish between multiple instances (Why would you tho?)
        String debugMarker;

        Ptr<Device> AddDevice();

        // If the instance is active, you must call this for the changes to be effective.
        bool Reconfigure();

        bool Initted() const;
        bool Init(); // Constructs the entire tree
        bool Deinit(); // Cleans everything up
    };

    void CmdBindVertexBuffers(VkCommandBuffer commandBuffer, u32 firstBinding,
                              Array<Ptr<Buffer>> buffers, Array<VkDeviceSize> offsets={});

    inline void CmdBindVertexBuffer(VkCommandBuffer commandBuffer, u32 binding,
                                    Ptr<Buffer> buffer, VkDeviceSize offset=0) {
        vkCmdBindVertexBuffers(commandBuffer, binding, 1, &buffer->data.buffer, &offset);
    }

    inline void CmdBindIndexBuffer(VkCommandBuffer commandBuffer, Ptr<Buffer> buffer,
                                    VkIndexType indexType, VkDeviceSize offset=0) {
        vkCmdBindIndexBuffer(commandBuffer, buffer->data.buffer, offset, indexType);
    }

    inline void CmdSetViewport(VkCommandBuffer commandBuffer, f32 width, f32 height,
                               f32 minDepth=0.0f, f32 maxDepth=1.0f, f32 x=0.0f, f32 y=0.0f) {
        VkViewport viewport;
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = minDepth;
        viewport.maxDepth = maxDepth;
        viewport.x = x;
        viewport.y = y;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    }

    inline void CmdSetScissor(VkCommandBuffer commandBuffer, u32 width, u32 height, i32 x=0, i32 y=0) {
        VkRect2D scissor;
        scissor.extent.width = width;
        scissor.extent.height = height;
        scissor.offset.x = x;
        scissor.offset.y = y;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    inline void CmdSetViewportAndScissor(VkCommandBuffer commandBuffer, f32 width, f32 height,
                                         f32 minDepth=0.0f, f32 maxDepth=0.0f, f32 x=0.0f, f32 y=0.0f) {
        CmdSetViewport(commandBuffer, width, height, minDepth, maxDepth, x, y);
        CmdSetScissor(commandBuffer, static_cast<u32>(width), static_cast<u32>(height),
                      static_cast<i32>(x), static_cast<i32>(y));
    }

    inline void QueueWaitIdle(Ptr<Queue> queue) {
        vkQueueWaitIdle(queue->queue);
    }

    inline void DeviceWaitIdle(Ptr<Device> device) {
        vkDeviceWaitIdle(device->data.device);
    }

    void CmdExecuteCommands(VkCommandBuffer commandBuffer, Array<Ptr<CommandBuffer>> commandBuffers);

} // namespace vk

} // namespace AzCore

#endif // AZCORE_VK_HPP
