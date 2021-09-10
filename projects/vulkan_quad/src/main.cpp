/*
	File: main.cpp
	Author: Philip Haynes
	Description: High-level definition of the structure of our program.
*/

#include "AzCore/io.hpp"
#include "AzCore/vk.hpp"

using namespace AzCore;

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

io::Log cout("test.log");

i32 main(i32 argumentCount, char** argumentValues) {

	f32 scale = 1.0f;

	bool enableLayers = false, enableCoreValidation = false;

	cout.PrintLn("\nTest program received ", argumentCount, " arguments:");
	for (i32 i = 0; i < argumentCount; i++) {
		cout.PrintLn(i, ": ", argumentValues[i]);
		if (equals(argumentValues[i], "--enable-layers")) {
			enableLayers = true;
		} else if (equals(argumentValues[i], "--core-validation")) {
			enableCoreValidation = true;
		}
	}

	struct Image {
		u8* pixels;
		i32 width;
		i32 height;
		i32 channels;
		Image(const char *filename) : pixels(stbi_load(filename, &width, &height, &channels, 4)) {
			channels = 4;
		}
		~Image() {
			stbi_image_free(pixels);
		}
	};
	Image image("data/icon.png");
	if (image.pixels == nullptr) {
		cout.PrintLn("Failed to load image!");
		return 1;
	}

	vk::Instance vkInstance;
	vkInstance.AppInfo("AzCore Test Program", 1, 0, 0);

	if (enableLayers) {
		cout.PrintLn("Validation layers enabled.");
		Array<const char*> layers = {
			"VK_LAYER_GOOGLE_threading",
			"VK_LAYER_LUNARG_parameter_validation",
			"VK_LAYER_LUNARG_object_tracker",
			"VK_LAYER_GOOGLE_unique_objects"
		};
		if (enableCoreValidation) {
			layers.Append("VK_LAYER_LUNARG_core_validation");
		}
		vkInstance.AddLayers(layers);
	}

	Ptr<vk::Device> vkDevice = vkInstance.AddDevice();

	Ptr<vk::Queue> queueGraphics = vkDevice->AddQueue();
	Ptr<vk::Queue> queuePresent = vkDevice->AddQueue();
	Ptr<vk::Queue> queueTransfer = vkDevice->AddQueue();
	Ptr<vk::Queue> queueCompute = vkDevice->AddQueue();
	queueGraphics->queueType = vk::GRAPHICS;
	queuePresent->queueType = vk::PRESENT;
	queueTransfer->queueType = vk::TRANSFER;
	queueCompute->queueType = vk::COMPUTE;

	io::Window window;
	io::Input input;
	window.input = &input;
	window.width = 480;
	window.height = 480;
	if (!window.Open()) {
		cout.PrintLn("Failed to open Window: ", io::error);
		return 1;
	}

	scale = (f32)window.GetDPI() / 96.0f;
	window.Resize(u32((f32)window.width * scale), u32((u32)window.height * scale));

	Ptr<vk::Swapchain> vkSwapchain = vkDevice->AddSwapchain();
	vkSwapchain->window = vkInstance.AddWindowForSurface(&window);
	vkSwapchain->vsync = false;

	Ptr<vk::RenderPass> vkRenderPass = vkDevice->AddRenderPass();

	Ptr<vk::Attachment> attachment = vkRenderPass->AddAttachment(vkSwapchain);
	attachment->clearColor = true;
	attachment->clearColorValue = {0.0f, 0.05f, 0.1f, 1.0f};
	// attachment->sampleCount = VK_SAMPLE_COUNT_4_BIT;
	// attachment->resolveColor = true;

	Ptr<vk::Subpass> subpass = vkRenderPass->AddSubpass();
	subpass->UseAttachment(attachment, vk::ATTACHMENT_COLOR,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	Ptr<vk::Memory> vkBufferStagingMemory = vkDevice->AddMemory();
	vkBufferStagingMemory->deviceLocal = false;
	Ptr<vk::Memory> vkBufferMemory = vkDevice->AddMemory();

	Ptr<vk::Memory> vkImageMemory = vkDevice->AddMemory();

	struct Vertex {
		vec2 position;
		vec2 texCoord;
	};

	Array<Vertex> vertices = {
		{vec2(-0.5f, -0.5f), vec2(0.0f, 0.0f)},
		{vec2(-0.5f,  0.5f), vec2(0.0f, 1.0f)},
		{vec2( 0.5f,  0.5f), vec2(1.0f, 1.0f)},
		{vec2( 0.5f, -0.5f), vec2(1.0f, 0.0f)}
	};
	Array<u32> indices = {0, 1, 2, 2, 3, 0};

	Range<vk::Buffer> vkStagingBuffers = vkBufferStagingMemory->AddBuffers(3);
	vkStagingBuffers[0].size = vertices.size * sizeof(Vertex);
	vkStagingBuffers[1].size = indices.size * sizeof(u32);
	vkStagingBuffers[2].size = image.width * image.height * image.channels;
	vkStagingBuffers[0].usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkStagingBuffers[1].usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkStagingBuffers[2].usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	Ptr<vk::Buffer> vkVertexBuffer = vkBufferMemory->AddBuffer();
	Ptr<vk::Buffer> vkIndexBuffer = vkBufferMemory->AddBuffer();
	vkVertexBuffer->size = vkStagingBuffers[0].size;
	vkIndexBuffer->size = vkStagingBuffers[1].size;
	vkVertexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vkIndexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	Ptr<vk::Image> vkTextureImage = vkImageMemory->AddImage();
	vkTextureImage->format = VK_FORMAT_R8G8B8A8_UNORM;
	vkTextureImage->width = image.width;
	vkTextureImage->height = image.height;
	vkTextureImage->mipLevels = (u32)floor(log2(max(image.width, image.height))) + 1;
	vkTextureImage->usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	Ptr<vk::Sampler> vkSampler = vkDevice->AddSampler();
	vkSampler->maxLod = vkTextureImage->mipLevels;
	vkSampler->anisotropy = 16;
	vkSampler->mipLodBias = -0.5f; // Keep things crisp

	Ptr<vk::Descriptors> vkDescriptors = vkDevice->AddDescriptors();
	Ptr<vk::DescriptorLayout> vkDescriptorLayoutTexture = vkDescriptors->AddLayout();
	vkDescriptorLayoutTexture->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	vkDescriptorLayoutTexture->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	vkDescriptorLayoutTexture->bindings.Resize(1);
	vkDescriptorLayoutTexture->bindings[0].binding = 0;
	vkDescriptorLayoutTexture->bindings[0].count = 1;
	Ptr<vk::DescriptorSet> vkDescriptorSetTexture = vkDescriptors->AddSet(vkDescriptorLayoutTexture);
	if (!vkDescriptorSetTexture->AddDescriptor(vkTextureImage, vkSampler, 0)) {
		cout.PrintLn("Failed to add Texture Descriptor: ", vk::error);
		return 1;
	}

	Range<vk::Shader> vkShaders = vkDevice->AddShaders(2);
	vkShaders[0].filename = "data/shaders/test.vert.spv";
	vkShaders[1].filename = "data/shaders/test.frag.spv";

	vk::ShaderRef vkShaderRefs[2] = {
		vk::ShaderRef(vkShaders.GetPtr(0), VK_SHADER_STAGE_VERTEX_BIT),
		vk::ShaderRef(vkShaders.GetPtr(1), VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	Ptr<vk::Pipeline> vkPipeline = vkDevice->AddPipeline();
	vkPipeline->renderPass = vkRenderPass;
	vkPipeline->subpass = 0;
	vkPipeline->shaders.Append(vkShaderRefs[0]);
	vkPipeline->shaders.Append(vkShaderRefs[1]);

	vkPipeline->descriptorLayouts.Append(vkDescriptorLayoutTexture);

	vkPipeline->dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkVertexInputAttributeDescription vertexInputAttributeDescription = {};
	vertexInputAttributeDescription.binding = 0;
	vertexInputAttributeDescription.location = 0;
	vertexInputAttributeDescription.offset = offsetof(Vertex, position);
	vertexInputAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
	vkPipeline->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
	vertexInputAttributeDescription.location = 1;
	vertexInputAttributeDescription.offset = offsetof(Vertex, texCoord);
	vertexInputAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
	vkPipeline->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
	VkVertexInputBindingDescription vertexInputBindingDescription = {};
	vertexInputBindingDescription.binding = 0;
	vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexInputBindingDescription.stride = sizeof(Vertex);
	vkPipeline->inputBindingDescriptions.Append(vertexInputBindingDescription);

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
										| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	vkPipeline->colorBlendAttachments.Append(colorBlendAttachment);

	Ptr<vk::CommandPool> vkCommandPool = vkDevice->AddCommandPool(queueGraphics);
	vkCommandPool->transient = true;
	vkCommandPool->resettable = true;
	Ptr<vk::CommandBuffer> vkCommandBuffer = vkCommandPool->AddCommandBuffer();
	vkCommandBuffer->oneTimeSubmit = true;

	Ptr<vk::Framebuffer> vkFramebuffer = vkDevice->AddFramebuffer();
	vkFramebuffer->renderPass = vkRenderPass;
	vkFramebuffer->swapchain = vkSwapchain;

	Ptr<vk::Semaphore> semaphoreRenderFinished = vkDevice->AddSemaphore();

	Ptr<vk::QueueSubmission> vkQueueSubmission = vkDevice->AddQueueSubmission();
	vkQueueSubmission->commandBuffers = {vkCommandBuffer};
	vkQueueSubmission->signalSemaphores = {semaphoreRenderFinished};
	vkQueueSubmission->waitSemaphores = {vk::SemaphoreWait(vkSwapchain, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)};
	// We don't necessarily have to do this, but might as well since we manually config every frame.
	vkQueueSubmission->noAutoConfig = true;

	Ptr<vk::QueueSubmission> vkTransferQueueSubmission = vkDevice->AddQueueSubmission();
	vkTransferQueueSubmission->commandBuffers = {vkCommandBuffer};
	vkTransferQueueSubmission->signalSemaphores = {};
	vkTransferQueueSubmission->waitSemaphores = {};

	if (!vkInstance.Init()) { // Do this once you've set up the structure of your program.
		cout.PrintLn("Failed to initialize Vulkan: ", vk::error);
		return 1;
	}

	vkStagingBuffers[0].CopyData(vertices.data);
	vkStagingBuffers[1].CopyData(indices.data);
	vkStagingBuffers[2].CopyData(image.pixels);

	VkCommandBuffer cmdBufCopy = vkCommandBuffer->Begin();
	vkVertexBuffer->Copy(cmdBufCopy, vkStagingBuffers.GetPtr(0));
	vkIndexBuffer->Copy(cmdBufCopy, vkStagingBuffers.GetPtr(1));

	vkTextureImage->TransitionLayout(cmdBufCopy, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vkTextureImage->Copy(cmdBufCopy, vkStagingBuffers.GetPtr(2));
	vkTextureImage->GenerateMipMaps(cmdBufCopy, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	if (!vkCommandBuffer->End()) {
		cout.PrintLn("Failed to copy from staging buffers: ", vk::error);
		return 1;
	}
	vkDevice->SubmitCommandBuffers(queueGraphics, {vkTransferQueueSubmission});
	vk::QueueWaitIdle(queueGraphics);

	if (!vkDescriptors->Update()) {
		cout.PrintLn("Failed to update descriptors: ", vk::error);
		return 1;
	}

	if(!window.Show()) {
		cout.PrintLn("Failed to show Window: ", io::error);
		return 1;
	}
	RandomNumberGenerator rng;
	bool resize = false;
	do {
		for (i32 i = 0; i < 256; i++) {
			if (input.inputs[i].Pressed()) {
				cout.PrintLn("Pressed   HID 0x", ToString(i, 16), "\t", window.InputName(i));
			}
			if (input.inputs[i].Released()) {
				cout.PrintLn("Released  HID 0x", ToString(i, 16), "\t", window.InputName(i));
			}
		}
		input.Tick(1.0f/60.0f);

		if (window.resized || resize) {
			if (!vkSwapchain->Resize()) {
				cout.PrintLn("Failed to resize vkSwapchain: ", vk::error);
				return 1;
			}
			resize = false;
		}

		VkResult acquisitionResult = vkSwapchain->AcquireNextImage();

		if (acquisitionResult == VK_ERROR_OUT_OF_DATE_KHR || acquisitionResult == VK_NOT_READY) {
			cout.PrintLn("Skipping a frame because acquisition returned: ", vk::ErrorString(acquisitionResult));
			resize = true;
			continue; // Don't render this frame.
		} else if (acquisitionResult == VK_TIMEOUT) {
			cout.PrintLn("Skipping a frame because acquisition returned: ", vk::ErrorString(acquisitionResult));
			continue;
		} else if (acquisitionResult != VK_SUCCESS) {
			cout.PrintLn(vk::error);
			return 1;
		}

		// Begin recording commands

		VkCommandBuffer cmdBuf = vkCommandBuffer->Begin();
		if (cmdBuf == VK_NULL_HANDLE) {
			cout.PrintLn("Failed to Begin recording vkCommandBuffer: ", vk::error);
			return 1;
		}

		vkRenderPass->Begin(cmdBuf, vkFramebuffer);

		vkPipeline->Bind(cmdBuf);

		vk::CmdSetViewportAndScissor(cmdBuf, window.width, window.height);

		vk::CmdBindVertexBuffer(cmdBuf, 0, vkVertexBuffer);
		vk::CmdBindIndexBuffer(cmdBuf, vkIndexBuffer, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->data.layout, 0, 1, &vkDescriptors->data.sets[0].data.set, 0, nullptr);

		vkCmdDrawIndexed(cmdBuf, 6, 1, 0, 0, 0);

		vkCmdEndRenderPass(cmdBuf);

		vkCommandBuffer->End();

		// We have to re-config any QueueSubmissions connected to a Swapchain every time we acquire a Swapchain image.
		if (!vkQueueSubmission->Config()) {
			cout.PrintLn("Failed to re-Config vkQueueSubmission: ", vk::error);
			return 1;
		}

		// Submit to queue
		if (!vkDevice->SubmitCommandBuffers(queueGraphics, {vkQueueSubmission})) {
			cout.PrintLn("Failed to SubmitCommandBuffers: ", vk::error);
			return 1;
		}

		if (!vkSwapchain->Present(queuePresent, {semaphoreRenderFinished->semaphore})) {
			cout.PrintLn(vk::error);
			return 1;
		}

		vk::DeviceWaitIdle(vkDevice);

	} while (window.Update());
	// This should be all you need to call to clean everything up
	// But you also could just let the vk::Instance go out of scope and it will
	// clean itself up.
	if (!vkInstance.Deinit()) {
		cout.PrintLn("Failed to cleanup Vulkan Tree: ", vk::error);
	}
	if (!window.Close()) {
		cout.PrintLn("Failed to close Window: ", io::error);
		return 1;
	}
	cout.PrintLn("Last io::error was \"", io::error, "\"");
	cout.PrintLn("Last vk::error was \"", vk::error, "\"");

	return 0;
}
