/*
	File: rendering.cpp
	Author: Philip Haynes
*/

#include "rendering.hpp"
#include "game_systems.hpp"
#include "settings.hpp"
#include "assets.hpp"
#include "gui_basics.hpp"
#include "profiling.hpp"
#include "entity_basics.hpp"

#include "AzCore/IO/Log.hpp"
#include "AzCore/io.hpp"
#include "AzCore/font.hpp"

namespace Az2D::Rendering {

using GameSystems::sys;

io::Log cout("rendering.log");

String error = "No error.";

void AddPointLight(vec3 pos, vec3 color, f32 distMin, f32 distMax, f32 attenuation) {
	AzAssert(distMin < distMax, "distMin must be < distMax, else shit breaks");
	Light light;
	light.position = pos;
	light.color = color;
	light.distMin = distMin;
	light.distMax = distMax;
	light.attenuation = attenuation;

	light.direction = vec3(0.0f, 0.0f, -1.0f);
	light.angleMin = pi;
	light.angleMax = tau;
	sys->rendering.lightsMutex.Lock();
	sys->rendering.lights.Append(light);
	sys->rendering.lightsMutex.Unlock();
}

void AddLight(vec3 pos, vec3 color, vec3 direction, f32 angleMin, f32 angleMax, f32 distMin, f32 distMax, f32 attenuation) {
	AzAssert(angleMin < angleMax, "angleMin must be < angleMax, else shit breaks");
	AzAssert(distMin < distMax, "distMin must be < distMax, else shit breaks");
	Light light;
	light.position = pos;
	light.color = color;
	light.direction = direction;
	light.angleMin = angleMin;
	light.angleMax = angleMax;
	light.distMin = distMin;
	light.distMax = distMax;
	light.attenuation = attenuation;
	sys->rendering.lightsMutex.Lock();
	sys->rendering.lights.Append(light);
	sys->rendering.lightsMutex.Unlock();
}

void PushConstants::vert_t::Push(VkCommandBuffer commandBuffer, const Manager *rendering) const {
	vkCmdPushConstants(commandBuffer, rendering->data.pipelines[PIPELINE_BASIC_2D]->data.layout,
			VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vert_t), this);
}

void PushConstants::frag_t::Push(VkCommandBuffer commandBuffer, const Manager *rendering) const {
	vkCmdPushConstants(commandBuffer, rendering->data.pipelines[PIPELINE_BASIC_2D]->data.layout,
			VK_SHADER_STAGE_FRAGMENT_BIT, offsetof(PushConstants, frag), sizeof(frag_t), this);
}

void PushConstants::font_circle_t::font_t::Push(VkCommandBuffer commandBuffer, const Manager *rendering) const {
	vkCmdPushConstants(commandBuffer, rendering->data.pipelines[PIPELINE_FONT_2D]->data.layout,
			VK_SHADER_STAGE_FRAGMENT_BIT, offsetof(PushConstants, frag), sizeof(frag_t) + sizeof(font_t), (char*)this - sizeof(frag_t));
}

void PushConstants::font_circle_t::circle_t::Push(VkCommandBuffer commandBuffer, const Manager *rendering) const {
	vkCmdPushConstants(commandBuffer, rendering->data.pipelines[PIPELINE_CIRCLE_2D]->data.layout,
			VK_SHADER_STAGE_FRAGMENT_BIT, offsetof(PushConstants, frag), sizeof(frag_t) + sizeof(circle_t), (char*)this - sizeof(frag_t));
}

void PushConstants::Push2D(VkCommandBuffer commandBuffer, const Manager *rendering) const {
	vert.Push(commandBuffer, rendering);
	frag.Push(commandBuffer, rendering);
}

void PushConstants::PushFont(VkCommandBuffer commandBuffer, const Manager *rendering) const {
	vert.Push(commandBuffer, rendering);
	font_circle.font.Push(commandBuffer, rendering);
}

void PushConstants::PushCircle(VkCommandBuffer commandBuffer, const Manager *rendering) const {
	vert.Push(commandBuffer, rendering);
	font_circle.circle.Push(commandBuffer, rendering);
}

bool Manager::Init() {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Rendering::Manager::Init)
	data.device = data.instance.AddDevice();
	data.device->data.vk12FeaturesRequired.scalarBlockLayout = VK_TRUE;
	data.device->data.vk12FeaturesRequired.uniformAndStorageBuffer8BitAccess = VK_TRUE;
	uniforms.ambientLight = vec3(0.001f);

	data.queueGraphics = data.device->AddQueue();
	data.queueGraphics->queueType = vk::QueueType::GRAPHICS;
	data.queueTransfer = data.device->AddQueue();
	data.queueTransfer->queueType = vk::QueueType::TRANSFER;
	data.queuePresent = data.device->AddQueue();
	data.queuePresent->queueType = vk::QueueType::PRESENT;

	data.swapchain = data.device->AddSwapchain();
	data.swapchain->vsync = Settings::ReadBool(Settings::sVSync);
	data.swapchain->window = data.instance.AddWindowForSurface(&sys->window);
	data.swapchain->formatPreferred.format = VK_FORMAT_B8G8R8A8_SRGB;
	data.swapchain->formatPreferred.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	// data.swapchain->useFences = true;
	// data.swapchain->imageCountPreferred = 3;

	data.framebuffer = data.device->AddFramebuffer();
	data.framebuffer->swapchain = data.swapchain;

	data.renderPass = data.device->AddRenderPass();
	auto attachment = data.renderPass->AddAttachment(data.swapchain);
	if (msaa) {
		attachment->sampleCount = VK_SAMPLE_COUNT_4_BIT;
		attachment->resolveColor = true;
	}
	auto subpass = data.renderPass->AddSubpass();
	subpass->UseAttachment(attachment, vk::AttachmentType::ATTACHMENT_ALL,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	data.framebuffer->renderPass = data.renderPass;
	// attachment->initialLayoutColor = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// attachment->loadColor = true;
	// attachment->clearColor = true;
	// attachment->clearColorValue = {1.0, 1.0, 1.0, 1.0};
	// attachment->clearColorValue = {0.0f, 0.1f, 0.2f, 1.0f}; // AzCore blue
	if (data.concurrency < 1) {
		data.concurrency = 1;
	}
	data.commandPools.Resize(data.concurrency);
	data.commandBuffersSecondary[0].Resize(data.concurrency);
	data.commandBuffersSecondary[1].Resize(data.concurrency);
	for (i32 i = 0; i < data.concurrency; i++) {
		data.commandPools[i] = data.device->AddCommandPool(data.queueGraphics);
		data.commandPools[i]->resettable = true;
		for (i32 j = 0; j < 2; j++) {
			data.commandBuffersSecondary[j][i] = data.commandPools[i]->AddCommandBuffer();
			data.commandBuffersSecondary[j][i]->oneTimeSubmit = true;
			data.commandBuffersSecondary[j][i]->secondary = true;
			data.commandBuffersSecondary[j][i]->renderPass = data.renderPass;
			data.commandBuffersSecondary[j][i]->renderPassContinue = true;
			data.commandBuffersSecondary[j][i]->simultaneousUse = true;
			data.commandBuffersSecondary[j][i]->framebuffer = data.framebuffer;
		}
	}

	data.semaphoreRenderComplete = data.device->AddSemaphore();

	for (i32 i = 0; i < 2; i++) {
		data.commandBufferPrimary[i] = data.commandPools[0]->AddCommandBuffer();
		data.queueSubmission[i] = data.device->AddQueueSubmission();
		data.queueSubmission[i]->commandBuffers = {data.commandBufferPrimary[i]};
		data.queueSubmission[i]->signalSemaphores = {data.semaphoreRenderComplete};
		data.queueSubmission[i]->waitSemaphores = {vk::SemaphoreWait(data.swapchain, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)};
		data.queueSubmission[i]->noAutoConfig = true;
	}
	data.commandBufferGraphicsTransfer = data.commandPools[0]->AddCommandBuffer();

	data.commandPoolTransfer = data.device->AddCommandPool(data.queueTransfer);
	data.commandPoolTransfer->resettable = true;
	data.commandBufferTransfer = data.commandPoolTransfer->AddCommandBuffer();

	data.queueSubmissionTransfer = data.device->AddQueueSubmission();
	data.queueSubmissionTransfer->commandBuffers = {data.commandBufferTransfer};

	data.queueSubmissionGraphicsTransfer = data.device->AddQueueSubmission();
	data.queueSubmissionGraphicsTransfer->commandBuffers = {data.commandBufferGraphicsTransfer};

	data.textureSampler = data.device->AddSampler();
	data.textureSampler->addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	data.textureSampler->addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	data.textureSampler->anisotropy = 4;
	data.textureSampler->mipLodBias = -1.0f; // Crisp!!!
	data.textureSampler->maxLod = 1000000000000.0f; // Just, like, BIG

	data.stagingMemory = data.device->AddMemory();
	data.stagingMemory->deviceLocal = false;
	data.bufferMemory = data.device->AddMemory();
	data.textureMemory = data.device->AddMemory();
	
	data.fontStagingMemory = data.device->AddMemory();
	data.fontStagingMemory->deviceLocal = false;
	data.fontBufferMemory = data.device->AddMemory();
	data.fontImageMemory = data.device->AddMemory();

	// Unit square
	Array<Vertex> vertices = {
		{vec2(0.0f, 0.0f), vec2(0.0f, 0.0f)},
		{vec2(0.0f, 1.0f), vec2(0.0f, 1.0f)},
		{vec2(1.0f, 1.0f), vec2(1.0f, 1.0f)},
		{vec2(1.0f, 0.0f), vec2(1.0f, 0.0f)}
	};
	Array<u32> indices = {0, 1, 2, 2, 3, 0};

	vk::Buffer baseBuffer = vk::Buffer();
	baseBuffer.size = 1;
	baseBuffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	Range<vk::Buffer> bufferStagingBuffers = data.stagingMemory->AddBuffers(3, baseBuffer);
	bufferStagingBuffers[0].size = vertices.size * sizeof(Vertex);
	bufferStagingBuffers[1].size = indices.size * sizeof(u32);
	data.uniformStagingBuffer = bufferStagingBuffers.GetPtr(2);
	data.uniformStagingBuffer->size = sizeof(UniformBuffer);

	data.uniformBuffer = data.bufferMemory->AddBuffer();
	data.vertexBuffer = data.bufferMemory->AddBuffer();
	data.indexBuffer = data.bufferMemory->AddBuffer();
	data.uniformBuffer->size = data.uniformStagingBuffer->size;
	data.uniformBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	data.vertexBuffer->size = bufferStagingBuffers[0].size;
	data.vertexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	data.indexBuffer->size = bufferStagingBuffers[1].size;
	data.indexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	auto texStagingBuffers = data.stagingMemory->AddBuffers(sys->assets.textures.size, baseBuffer);

	data.fontStagingVertexBuffer = data.fontStagingMemory->AddBuffer(baseBuffer);
	data.fontStagingImageBuffers = data.fontStagingMemory->AddBuffers(sys->assets.fonts.size, baseBuffer);

	data.fontVertexBuffer = data.fontBufferMemory->AddBuffer(baseBuffer);
	data.fontVertexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	vk::Image baseImage = vk::Image();
	baseImage.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	baseImage.format = VK_FORMAT_R8G8B8A8_SRGB;
	auto texImages = data.textureMemory->AddImages(sys->assets.textures.size, baseImage);
	for (i32 i = 0; i < sys->assets.textures.size; i++) {
		if (sys->assets.textures[i].linear) {
			data.textureMemory->data.images[i].format = VK_FORMAT_R8G8B8A8_UNORM;
		}
	}

	baseImage.format = VK_FORMAT_R8_UNORM;
	baseImage.width = 1;
	baseImage.height = 1;
	data.fontImages = data.fontImageMemory->AddImages(sys->assets.fonts.size, baseImage);

	for (i32 i = 0; i < texImages.size; i++) {
		const i32 channels = sys->assets.textures[i].channels;
		if (channels != 4) {
			error = Stringify("Invalid channel count (", channels, ") in textures[", i, "]");
			return false;
		}
		texImages[i].width = sys->assets.textures[i].width;
		texImages[i].height = sys->assets.textures[i].height;
		texImages[i].mipLevels = (u32)floor(log2((f32)max(texImages[i].width, texImages[i].height))) + 1;

		texStagingBuffers[i].size = channels * texImages[i].width * texImages[i].height;
	}

	data.descriptors = data.device->AddDescriptors();
	Ptr<vk::DescriptorLayout> descriptorLayout2D = data.descriptors->AddLayout();
	descriptorLayout2D->bindings.Resize(2);
	descriptorLayout2D->bindings[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorLayout2D->bindings[0].stage = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
	descriptorLayout2D->bindings[0].binding = 0;
	descriptorLayout2D->bindings[0].count = 1;
	descriptorLayout2D->bindings[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorLayout2D->bindings[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorLayout2D->bindings[1].binding = 1;
	descriptorLayout2D->bindings[1].count = sys->assets.textures.size;
	Ptr<vk::DescriptorLayout> descriptorLayoutFont = data.descriptors->AddLayout();
	descriptorLayoutFont->bindings.Resize(1);
	descriptorLayoutFont->bindings[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorLayoutFont->bindings[0].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorLayoutFont->bindings[0].binding = 0;
	descriptorLayoutFont->bindings[0].count = sys->assets.fonts.size;

	data.descriptorSet2D = data.descriptors->AddSet(descriptorLayout2D);
	if (!data.descriptorSet2D->AddDescriptor(data.uniformBuffer, 0)) {
		error = "Failed to add Uniform Buffer Descriptor: " + vk::error;
		return false;
	}
	if (!data.descriptorSet2D->AddDescriptor(texImages, data.textureSampler, 1)) {
		error = "Failed to add Texture Descriptor: " + vk::error;
		return false;
	}
	data.descriptorSetFont = data.descriptors->AddSet(descriptorLayoutFont);
	if (!data.descriptorSetFont->AddDescriptor(data.fontImages, data.textureSampler, 0)) {
		error = "Failed to add Font Descriptor: " + vk::error;
		return false;
	}

	Range<vk::Shader> shaders = data.device->AddShaders(8);
	shaders[0].filename = "data/Az2D/shaders/Basic2D.vert.spv";
	shaders[1].filename = "data/Az2D/shaders/Basic2D.frag.spv";
	shaders[2].filename = "data/Az2D/shaders/Font2D.frag.spv";
	shaders[3].filename = "data/Az2D/shaders/Circle2D.frag.spv";
	shaders[4].filename = "data/Az2D/shaders/Basic2DPixel.frag.spv";
	shaders[5].filename = "data/Az2D/shaders/Shaded2D.vert.spv";
	shaders[6].filename = "data/Az2D/shaders/Shaded2D.frag.spv";
	shaders[7].filename = "data/Az2D/shaders/Shaded2DPixel.frag.spv";

	vk::ShaderRef shaderRefVert = vk::ShaderRef(shaders.GetPtr(0), VK_SHADER_STAGE_VERTEX_BIT);
	vk::ShaderRef shaderRefBasic2D = vk::ShaderRef(shaders.GetPtr(1), VK_SHADER_STAGE_FRAGMENT_BIT);
	vk::ShaderRef shaderRefFont2D = vk::ShaderRef(shaders.GetPtr(2), VK_SHADER_STAGE_FRAGMENT_BIT);
	vk::ShaderRef shaderRefCircle2D = vk::ShaderRef(shaders.GetPtr(3), VK_SHADER_STAGE_FRAGMENT_BIT);
	vk::ShaderRef shaderRefBasic2DPixel = vk::ShaderRef(shaders.GetPtr(4), VK_SHADER_STAGE_FRAGMENT_BIT);
	vk::ShaderRef shaderRefShaded2DVert = vk::ShaderRef(shaders.GetPtr(5), VK_SHADER_STAGE_VERTEX_BIT);
	vk::ShaderRef shaderRefShaded2D = vk::ShaderRef(shaders.GetPtr(6), VK_SHADER_STAGE_FRAGMENT_BIT);
	vk::ShaderRef shaderRefShaded2DPixel = vk::ShaderRef(shaders.GetPtr(7), VK_SHADER_STAGE_FRAGMENT_BIT);

	data.pipelines.Resize(PIPELINE_COUNT);
	data.pipelineDescriptorSets.Resize(PIPELINE_COUNT);
	data.pipelineDescriptorSets[PIPELINE_BASIC_2D] = {data.descriptorSet2D};
	data.pipelineDescriptorSets[PIPELINE_BASIC_2D_PIXEL] = {data.descriptorSet2D};
	data.pipelineDescriptorSets[PIPELINE_FONT_2D] = {data.descriptorSetFont};
	data.pipelineDescriptorSets[PIPELINE_CIRCLE_2D] = {data.descriptorSet2D};
	data.pipelineDescriptorSets[PIPELINE_SHADED_2D] = {data.descriptorSet2D};
	data.pipelineDescriptorSets[PIPELINE_SHADED_2D_PIXEL] = {data.descriptorSet2D};
	
	data.pipelines[PIPELINE_BASIC_2D] = data.device->AddPipeline();
	data.pipelines[PIPELINE_BASIC_2D]->renderPass = data.renderPass;
	data.pipelines[PIPELINE_BASIC_2D]->subpass = 0;
	data.pipelines[PIPELINE_BASIC_2D]->shaders.Append(shaderRefVert);
	data.pipelines[PIPELINE_BASIC_2D]->shaders.Append(shaderRefBasic2D);
	data.pipelines[PIPELINE_BASIC_2D]->rasterizer.cullMode = VK_CULL_MODE_NONE;

	data.pipelines[PIPELINE_BASIC_2D]->descriptorLayouts.Append(descriptorLayout2D);

	data.pipelines[PIPELINE_BASIC_2D]->dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	data.pipelines[PIPELINE_BASIC_2D_PIXEL] = data.device->AddPipeline();
	data.pipelines[PIPELINE_BASIC_2D_PIXEL]->renderPass = data.renderPass;
	data.pipelines[PIPELINE_BASIC_2D_PIXEL]->subpass = 0;
	data.pipelines[PIPELINE_BASIC_2D_PIXEL]->shaders.Append(shaderRefVert);
	data.pipelines[PIPELINE_BASIC_2D_PIXEL]->shaders.Append(shaderRefBasic2DPixel);
	data.pipelines[PIPELINE_BASIC_2D_PIXEL]->rasterizer.cullMode = VK_CULL_MODE_NONE;

	data.pipelines[PIPELINE_BASIC_2D_PIXEL]->descriptorLayouts.Append(descriptorLayout2D);

	data.pipelines[PIPELINE_BASIC_2D_PIXEL]->dynamicStates = data.pipelines[PIPELINE_BASIC_2D]->dynamicStates;

	data.pipelines[PIPELINE_FONT_2D] = data.device->AddPipeline();
	data.pipelines[PIPELINE_FONT_2D]->renderPass = data.renderPass;
	data.pipelines[PIPELINE_FONT_2D]->subpass = 0;
	data.pipelines[PIPELINE_FONT_2D]->shaders.Append(shaderRefVert);
	data.pipelines[PIPELINE_FONT_2D]->shaders.Append(shaderRefFont2D);

	data.pipelines[PIPELINE_FONT_2D]->descriptorLayouts.Append(descriptorLayoutFont);

	data.pipelines[PIPELINE_FONT_2D]->dynamicStates = data.pipelines[PIPELINE_BASIC_2D]->dynamicStates;

	data.pipelines[PIPELINE_CIRCLE_2D] = data.device->AddPipeline();
	data.pipelines[PIPELINE_CIRCLE_2D]->renderPass = data.renderPass;
	data.pipelines[PIPELINE_CIRCLE_2D]->subpass = 0;
	data.pipelines[PIPELINE_CIRCLE_2D]->shaders.Append(shaderRefVert);
	data.pipelines[PIPELINE_CIRCLE_2D]->shaders.Append(shaderRefCircle2D);

	data.pipelines[PIPELINE_CIRCLE_2D]->descriptorLayouts.Append(descriptorLayout2D);

	data.pipelines[PIPELINE_CIRCLE_2D]->dynamicStates = data.pipelines[PIPELINE_BASIC_2D]->dynamicStates;

	data.pipelines[PIPELINE_SHADED_2D] = data.device->AddPipeline();
	data.pipelines[PIPELINE_SHADED_2D]->renderPass = data.renderPass;
	data.pipelines[PIPELINE_SHADED_2D]->subpass = 0;
	data.pipelines[PIPELINE_SHADED_2D]->shaders.Append(shaderRefShaded2DVert);
	data.pipelines[PIPELINE_SHADED_2D]->shaders.Append(shaderRefShaded2D);
	data.pipelines[PIPELINE_SHADED_2D]->rasterizer.cullMode = VK_CULL_MODE_NONE;

	data.pipelines[PIPELINE_SHADED_2D]->descriptorLayouts.Append(descriptorLayout2D);

	data.pipelines[PIPELINE_SHADED_2D]->dynamicStates = data.pipelines[PIPELINE_BASIC_2D]->dynamicStates;

	data.pipelines[PIPELINE_SHADED_2D_PIXEL] = data.device->AddPipeline();
	data.pipelines[PIPELINE_SHADED_2D_PIXEL]->renderPass = data.renderPass;
	data.pipelines[PIPELINE_SHADED_2D_PIXEL]->subpass = 0;
	data.pipelines[PIPELINE_SHADED_2D_PIXEL]->shaders.Append(shaderRefShaded2DVert);
	data.pipelines[PIPELINE_SHADED_2D_PIXEL]->shaders.Append(shaderRefShaded2DPixel);
	data.pipelines[PIPELINE_SHADED_2D_PIXEL]->rasterizer.cullMode = VK_CULL_MODE_NONE;

	data.pipelines[PIPELINE_SHADED_2D_PIXEL]->descriptorLayouts.Append(descriptorLayout2D);

	data.pipelines[PIPELINE_SHADED_2D_PIXEL]->dynamicStates = data.pipelines[PIPELINE_BASIC_2D]->dynamicStates;

	VkVertexInputAttributeDescription vertexInputAttributeDescription = {};
	vertexInputAttributeDescription.binding = 0;
	vertexInputAttributeDescription.location = 0;
	vertexInputAttributeDescription.offset = offsetof(Vertex, pos);
	vertexInputAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
	for (i32 i = 1; i < data.pipelines.size; i++) {
		data.pipelines[i]->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
	}
	vertexInputAttributeDescription.location = 1;
	vertexInputAttributeDescription.offset = offsetof(Vertex, tex);
	vertexInputAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
	for (i32 i = 1; i < data.pipelines.size; i++) {
		data.pipelines[i]->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
	}
	VkVertexInputBindingDescription vertexInputBindingDescription = {};
	vertexInputBindingDescription.binding = 0;
	vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexInputBindingDescription.stride = sizeof(Vertex);
	for (i32 i = 1; i < data.pipelines.size; i++) {
		data.pipelines[i]->inputBindingDescriptions.Append(vertexInputBindingDescription);
	}

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
										| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	for (i32 i = 1; i < data.pipelines.size; i++) {
		data.pipelines[i]->colorBlendAttachments.Append(colorBlendAttachment);
	}

	data.pipelines[PIPELINE_BASIC_2D]->pushConstantRanges = {
		{
			/* stage flags */ VK_SHADER_STAGE_VERTEX_BIT,
			/* offset */ 0,
			/* size */ sizeof(PushConstants::vert_t)
		},
		{
			/* stage flags */ VK_SHADER_STAGE_FRAGMENT_BIT,
			/* offset */ 48,
			/* size */ sizeof(Material) + 4
		}
	};
	data.pipelines[PIPELINE_BASIC_2D_PIXEL]->pushConstantRanges = data.pipelines[PIPELINE_BASIC_2D]->pushConstantRanges;
	data.pipelines[PIPELINE_FONT_2D]->pushConstantRanges = {
		{
			/* stage flags */ VK_SHADER_STAGE_VERTEX_BIT,
			/* offset */ 0,
			/* size */ sizeof(PushConstants::vert_t)
		},
		{
			/* stage flags */ VK_SHADER_STAGE_FRAGMENT_BIT,
			/* offset */ offsetof(PushConstants, frag),
			/* size */ sizeof(PushConstants::frag_t) + sizeof(PushConstants::font_circle_t::font_t)
		}
	};
	data.pipelines[PIPELINE_CIRCLE_2D]->pushConstantRanges = {
		{
			/* stage flags */ VK_SHADER_STAGE_VERTEX_BIT,
			/* offset */ 0,
			/* size */ sizeof(PushConstants::vert_t)
		},
		{
			/* stage flags */ VK_SHADER_STAGE_FRAGMENT_BIT,
			/* offset */ offsetof(PushConstants, frag),
			/* size */ sizeof(PushConstants::frag_t) + sizeof(PushConstants::font_circle_t::circle_t)
		}
	};
	data.pipelines[PIPELINE_SHADED_2D]->pushConstantRanges = {
		{
			/* stage flags */ VK_SHADER_STAGE_VERTEX_BIT,
			/* offset */ 0,
			/* size */ sizeof(PushConstants::vert_t)
		},
		{
			/* stage flags */ VK_SHADER_STAGE_FRAGMENT_BIT,
			/* offset */ offsetof(PushConstants, frag),
			/* size */ sizeof(PushConstants::frag_t)
		}
	};
	data.pipelines[PIPELINE_SHADED_2D_PIXEL]->pushConstantRanges = {
		{
			/* stage flags */ VK_SHADER_STAGE_VERTEX_BIT,
			/* offset */ 0,
			/* size */ sizeof(PushConstants::vert_t)
		},
		{
			/* stage flags */ VK_SHADER_STAGE_FRAGMENT_BIT,
			/* offset */ offsetof(PushConstants, frag),
			/* size */ sizeof(PushConstants::frag_t)
		}
	};

	if (!data.instance.Init()) {
		error = "Failed to init vk::instance: " + vk::error;
		return false;
	}
	
	uniforms.lights[0].position = vec3(0.0f);
	uniforms.lights[0].color = vec3(0.0f);
	uniforms.lights[0].attenuation = 0.0f;
	uniforms.lights[0].direction = vec3(0.0f, 0.0f, 1.0f);
	uniforms.lights[0].angleMin = 0.0f;
	uniforms.lights[0].angleMax = 0.0f;
	uniforms.lights[0].distMin = 0.0f;
	uniforms.lights[0].distMax = 0.0f;

	bufferStagingBuffers[0].CopyData(vertices.data);
	bufferStagingBuffers[1].CopyData(indices.data);
	for (i32 i = 0; i < texStagingBuffers.size; i++) {
		texStagingBuffers[i].CopyData(sys->assets.textures[i].pixels);
	}

	VkCommandBuffer cmdBufCopy = data.commandBufferGraphicsTransfer->Begin();
	data.vertexBuffer->Copy(cmdBufCopy, bufferStagingBuffers.GetPtr(0));
	data.indexBuffer->Copy(cmdBufCopy, bufferStagingBuffers.GetPtr(1));

	for (i32 i = 0; i < texStagingBuffers.size; i++) {
		texImages[i].TransitionLayout(cmdBufCopy, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		texImages[i].Copy(cmdBufCopy, texStagingBuffers.GetPtr(i));
		texImages[i].GenerateMipMaps(cmdBufCopy, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	if (!data.commandBufferGraphicsTransfer->End()) {
		error = "Failed to copy from staging buffers: " + vk::error;
		return false;
	}
	if (!data.device->SubmitCommandBuffers(data.queueGraphics, {data.queueSubmissionGraphicsTransfer})) {
		error = "Failed to submit transfer command buffers: " + vk::error;
		return false;
	}
	vk::QueueWaitIdle(data.queueGraphics);

	if (!UpdateFonts()) {
		error = "Failed to update fonts: " + error;
		return false;
	}
	UpdateBackground();

	return true;
}

bool Manager::Deinit() {
	if (!data.instance.Deinit()) {
		error = vk::error;
		return false;
	}
	return true;
}

using Entities::AABB;
AABB GetAABB(const Light &light) {
	AABB result;
	vec2 center = {light.position.x, light.position.y};
	
	result.minPos = center;
	result.maxPos = center;
	
	f32 dist = light.distMax;// * sqrt(1.0f - square(light.direction.z));
	Angle32 cardinalDirs[4] = {0.0f, halfpi, pi, halfpi * 3.0f};
	vec2 cardinalVecs[4] = {
		{dist, 0.0f},
		{0.0f, dist},
		{-dist, 0.0f},
		{0.0f, -dist},
	};
	if (light.direction.x != 0.0f || light.direction.y != 0.0f) {
		Angle32 dir = atan2(light.direction.y, light.direction.x);
		Angle32 dirMin = dir + -light.angleMax;
		Angle32 dirMax = dir + light.angleMax;
		result.Extend(center + vec2(cos(dirMin), sin(dirMin)) * dist);
		result.Extend(center + vec2(cos(dirMax), sin(dirMax)) * dist);
		for (i32 i = 0; i < 4; i++) {
			if (abs(cardinalDirs[i] - dir) < light.angleMax) {
				result.Extend(center + cardinalVecs[i]);
			}
		}
	} else {
		for (i32 i = 0; i < 4; i++) {
			result.Extend(center + cardinalVecs[i]);
		}
	}
	return result;
}

vec2i GetLightBin(vec2 point, vec2 screenSize) {
	vec2i result;
	result.x = (point.x) / screenSize.x * LIGHT_BIN_COUNT_X;
	result.y = (point.y) / screenSize.y * LIGHT_BIN_COUNT_Y;
	return result;
}

i32 LightBinIndex(vec2i bin) {
	return bin.y * LIGHT_BIN_COUNT_X + bin.x;
}

void Manager::UpdateLights() {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Rendering::Manager::UpdateLights)
	i32 lightCounts[LIGHT_BIN_COUNT] = {0};
	i32 totalLights = 1;
	// By default, they all point to the default light which has no light at all
	memset(uniforms.lightBins, 0, sizeof(uniforms.lightBins));
#if 1
	for (const Light &light : lights) {
		if (totalLights >= MAX_LIGHTS) break;
		AABB lightAABB = GetAABB(light);
		vec2i binMin = GetLightBin(lightAABB.minPos, screenSize);
		if (binMin.x >= LIGHT_BIN_COUNT_X || binMin.y >= LIGHT_BIN_COUNT_Y) continue;
		vec2i binMax = GetLightBin(lightAABB.maxPos, screenSize);
		if (binMax.x < 0 || binMax.y < 0) continue;
		binMin.x = max(binMin.x, 0);
		binMin.y = max(binMin.y, 0);
		binMax.x = min(binMax.x, LIGHT_BIN_COUNT_X-1);
		binMax.y = min(binMax.y, LIGHT_BIN_COUNT_Y-1);
		i32 lightIndex = totalLights;
		uniforms.lights[lightIndex] = light;
		bool atLeastOne = false;
		for (i32 y = binMin.y; y <= binMax.y; y++) {
			for (i32 x = binMin.x; x <= binMax.x; x++) {
				i32 i = LightBinIndex({x, y});
				LightBin &bin = uniforms.lightBins[i];
				if (lightCounts[i] >= MAX_LIGHTS_PER_BIN) continue;
				atLeastOne = true;
				bin.lightIndices[lightCounts[i]] = lightIndex;
				lightCounts[i]++;
			}
		}
		if (atLeastOne) {
			totalLights++;
		}
	}
#else
	for (i32 i = 0; i < LIGHT_BIN_COUNT; i++) {
		// for (i32 l = 0; l < MAX_LIGHTS_PER_BIN; l++)
			uniforms.lightBins[i].lightIndices[0] = i;
	}
#endif
}

bool Manager::UpdateFonts() {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Rendering::Manager::UpdateFonts)
	// Will be done on-the-fly
	if (data.fontStagingMemory->data.initted) {
		data.fontStagingMemory->Deinit();
	}
	if (data.fontBufferMemory->data.initted) {
		data.fontBufferMemory->Deinit();
	}
	if (data.fontImageMemory->data.initted) {
		data.fontImageMemory->Deinit();
	}

	// Vertex buffer
	Array<Vertex> fontVertices;
	fontIndexOffsets = {0};
	for (i32 i = 0; i < sys->assets.fonts.size; i++) {
		for (font::Glyph& glyph : sys->assets.fonts[i].fontBuilder.glyphs) {
			if (glyph.info.size.x == 0.0f || glyph.info.size.y == 0.0f) {
				continue;
			}
			const f32 boundSquare = sys->assets.fonts[i].fontBuilder.boundSquare;
			f32 posTop = -glyph.info.offset.y * boundSquare;
			f32 posLeft = -glyph.info.offset.x * boundSquare;
			f32 posBot = -glyph.info.size.y * boundSquare + posTop;
			f32 posRight = glyph.info.size.x * boundSquare + posLeft;
			f32 texLeft = glyph.info.pos.x;
			f32 texBot = glyph.info.pos.y;
			f32 texRight = (glyph.info.pos.x + glyph.info.size.x);
			f32 texTop = (glyph.info.pos.y + glyph.info.size.y);
			Vertex quad[4];
			quad[0].pos = vec2(posLeft, posTop);
			quad[0].tex = vec2(texLeft, texTop);
			quad[1].pos = vec2(posLeft, posBot);
			quad[1].tex = vec2(texLeft, texBot);
			quad[2].pos = vec2(posRight, posBot);
			quad[2].tex = vec2(texRight, texBot);
			quad[3].pos = vec2(posRight, posTop);
			quad[3].tex = vec2(texRight, texTop);
			fontVertices.Append(quad[3]);
			fontVertices.Append(quad[2]);
			fontVertices.Append(quad[1]);
			fontVertices.Append(quad[0]);
		}
		fontIndexOffsets.Append(
			fontIndexOffsets.Back() + sys->assets.fonts[i].fontBuilder.glyphs.size * 4
		);
	}

	data.fontStagingVertexBuffer->size = fontVertices.size * sizeof(Vertex);
	data.fontVertexBuffer->size = data.fontStagingVertexBuffer->size;

	for (i32 i = 0; i < data.fontImages.size; i++) {
		data.fontImages[i].width = sys->assets.fonts[i].fontBuilder.dimensions.x;
		data.fontImages[i].height = sys->assets.fonts[i].fontBuilder.dimensions.y;
		data.fontImages[i].mipLevels = (u32)floor(log2((f32)max(data.fontImages[i].width, data.fontImages[i].height))) + 1;

		data.fontStagingImageBuffers[i].size = data.fontImages[i].width * data.fontImages[i].height;
	}

	// Initialize everything
	if (!data.fontStagingMemory->Init(&(*data.device))) {
		return false;
	}
	if (!data.fontBufferMemory->Init(&(*data.device))) {
		return false;
	}
	if (!data.fontImageMemory->Init(&(*data.device))) {
		return false;
	}

	// Update the descriptors
	if (!data.descriptors->Update()) {
		return false;
	}

	data.fontStagingVertexBuffer->CopyData(fontVertices.data);
	for (i32 i = 0; i < data.fontStagingImageBuffers.size; i++) {
		data.fontStagingImageBuffers[i].CopyData(sys->assets.fonts[i].fontBuilder.pixels.data);
	}

	VkCommandBuffer cmdBufCopy = data.commandBufferGraphicsTransfer->Begin();

	data.fontVertexBuffer->Copy(cmdBufCopy, data.fontStagingVertexBuffer);

	for (i32 i = 0; i < data.fontStagingImageBuffers.size; i++) {
		data.fontImages[i].TransitionLayout(cmdBufCopy, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		data.fontImages[i].Copy(cmdBufCopy, data.fontStagingImageBuffers.GetPtr(i));
		data.fontImages[i].GenerateMipMaps(cmdBufCopy, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	if (!data.commandBufferGraphicsTransfer->End()) {
		error = "Failed to copy from font staging buffers: " + vk::error;
		return false;
	}
	if (!data.device->SubmitCommandBuffers(data.queueGraphics, {data.queueSubmissionGraphicsTransfer})) {
		error = "Failed to submit transfer command buffer for fonts: " + vk::error;
		return false;
	}
	vk::QueueWaitIdle(data.queueGraphics);

	return true;
}

bool Manager::UpdateUniforms() {
	UpdateLights();
	
	data.uniformStagingBuffer->CopyData(&uniforms);
	VkCommandBuffer cmdBuf = data.commandBufferTransfer->Begin();
	data.uniformBuffer->Copy(cmdBuf, data.uniformStagingBuffer);
	if (!data.commandBufferTransfer->End()) {
		error = "Failed to copy from uniform staging buffer: " + vk::error;
		return false;
	}
	if (!data.device->SubmitCommandBuffers(data.queueTransfer, {data.queueSubmissionTransfer})) {
		error = "Failed to submit transer command buffer for uniforms: " + vk::error;
		return false;
	}
	// TODO: Synchronize this with the graphics queue using a semaphore
	vk::QueueWaitIdle(data.queueTransfer);

	return true;
}

bool Manager::Draw() {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Rendering::Manager::Draw)
	if (vk::hadValidationError) {
		error = "Quitting due to vulkan validation error.";
		return false;
	}
	if (sys->window.resized || data.resized || data.zeroExtent) {
		AZ2D_PROFILING_EXCEPTION_START();
		vk::DeviceWaitIdle(data.device);
		AZ2D_PROFILING_EXCEPTION_END();
		data.swapchain->UpdateSurfaceCapabilities();
		VkExtent2D extent = data.swapchain->data.surfaceCapabilities.currentExtent;
		if (extent.width == 0 || extent.height == 0) {
			data.zeroExtent = true;
			return true;
		}
		data.zeroExtent = false;
		if (!data.swapchain->Resize()) {
			error = "Failed to resize swapchain: " + vk::error;
			return false;
		}
		data.resized = false;
	}
	if (Settings::ReadBool(Settings::sVSync) != data.swapchain->vsync) {
		AZ2D_PROFILING_EXCEPTION_START();
		vk::DeviceWaitIdle(data.device);
		AZ2D_PROFILING_EXCEPTION_END();
		data.swapchain->vsync = Settings::ReadBool(Settings::sVSync);
		if (!data.swapchain->Reconfigure()) {
			error = "Failed to set VSync: " + vk::error;
			return false;
		}
	}

	bool updateFontMemory = false;
	for (i32 i = 0; i < sys->assets.fonts.size; i++) {
		Assets::Font& font = sys->assets.fonts[i];
		if (font.fontBuilder.indicesToAdd.size != 0) {
			font.fontBuilder.Build();
			updateFontMemory = true;
		}
	}
	if (updateFontMemory) {
		AZ2D_PROFILING_EXCEPTION_START();
		vk::DeviceWaitIdle(data.device);
		AZ2D_PROFILING_EXCEPTION_END();
		if (!UpdateFonts()) {
			return false;
		}
	}

	static Az2D::Profiling::AString sAcquisition("Swapchain::AcquireNextImage");
	Az2D::Profiling::Timer timerAcquisition(sAcquisition);
	timerAcquisition.Start();
	AZ2D_PROFILING_EXCEPTION_START();
	VkResult acquisitionResult = data.swapchain->AcquireNextImage();
	timerAcquisition.End();
	AZ2D_PROFILING_EXCEPTION_END();

	if (acquisitionResult == VK_ERROR_OUT_OF_DATE_KHR || acquisitionResult == VK_NOT_READY) {
		cout.PrintLn("Skipping a frame because acquisition returned: ", vk::ErrorString(acquisitionResult));
		data.resized = true;
		return true; // Don't render this frame.
	} else if (acquisitionResult == VK_TIMEOUT) {
		cout.PrintLn("Skipping a frame because acquisition returned: ", vk::ErrorString(acquisitionResult));
		return true;
	} else if (acquisitionResult == VK_SUBOPTIMAL_KHR) {
		data.resized = true;
		// We'll try to render this frame anyway
	} else if (acquisitionResult != VK_SUCCESS) {
		error = "Failed to acquire swapchain image: " + vk::error;
		return false;
	}

	data.buffer = !data.buffer;

	screenSize = vec2((f32)sys->window.width, (f32)sys->window.height);
	aspectRatio = screenSize.y / screenSize.x;

	Array<DrawingContext> commandBuffersSecondary;
	commandBuffersSecondary.Reserve(data.commandBuffersSecondary[data.buffer].size);

	// TODO: Do these asynchronously
	for (Ptr<vk::CommandBuffer> &commandBuffer : data.commandBuffersSecondary[data.buffer]) {
		VkCommandBuffer cmdBuf = commandBuffer->Begin();
		vk::CmdSetViewportAndScissor(cmdBuf, sys->window.width, sys->window.height);
		vk::CmdBindIndexBuffer(cmdBuf, data.indexBuffer, VK_INDEX_TYPE_UINT32);
		commandBuffersSecondary.Append({cmdBuf, PIPELINE_NONE, {{vec2i(0), vec2i((i32)sys->window.width, (i32)sys->window.height)}}});
	}
	/*{ // Fade
		DrawQuadSS(commandBuffersSecondary[0], texBlank, vec4(backgroundRGB, 0.2f), vec2(-1.0), vec2(2.0), vec2(1.0));
	}*/
	{ // Clear
		vk::CmdClearColorAttachment(
			commandBuffersSecondary[0].commandBuffer,
			data.renderPass->data.subpasses[0].data.referencesColor[0].attachment,
			vec4(sRGBToLinear(backgroundRGB), 1.0f),
			sys->window.width,
			sys->window.height
		);
	}
	// Clear lights so we get new ones this frame
	lights.size = 0;

	for (auto& renderCallback : data.renderCallbacks) {
		renderCallback.callback(renderCallback.userdata, this, commandBuffersSecondary);
	}

	{ // Debug info
		if (Settings::ReadBool(Settings::sDebugInfo)) {
			f32 msAvg = sys->frametimes.Average();
			f32 msMax = sys->frametimes.Max();
			f32 msMin = sys->frametimes.Min();
			f32 msDiff = msMax - msMin;
			f32 fps = 1000.0f / msAvg;
			DrawQuad(commandBuffersSecondary.Back(), 0.0f, vec2(500.0f, 20.0f) * Gui::guiBasic->scale, 1.0f, 0.0f, 0.0f, PIPELINE_BASIC_2D, vec4(vec3(0.0f), 0.5f));
			WString strings[] = {
				ToWString(Stringify("fps: ", FormatFloat(fps, 10, 1))),
				ToWString(Stringify("avg: ", FormatFloat(msAvg, 10, 1), "ms")),
				ToWString(Stringify("max: ", FormatFloat(msMax, 10, 1), "ms")),
				ToWString(Stringify("min: ", FormatFloat(msMin, 10, 1), "ms")),
				ToWString(Stringify("diff: ", FormatFloat(msDiff, 10, 1), "ms")),
				ToWString(Stringify("timestep: ", FormatFloat(sys->timestep * 1000.0f, 10, 1), "ms")),
			};
			for (i32 i = 0; i < (i32)(sizeof(strings)/sizeof(WString)); i++) {
				vec2 pos = vec2(4.0f + f32(i*80), 4.0f) * Gui::guiBasic->scale;
				DrawText(commandBuffersSecondary.Back(), strings[i], 0, vec4(1.0f), pos, vec2(12.0f * Gui::guiBasic->scale), LEFT, TOP);
			}
		}
	}


	for (auto& commandBuffer : data.commandBuffersSecondary[data.buffer]) {
		commandBuffer->End();
	}

	static Az2D::Profiling::AString sWaitIdle("vk::DeviceWaitIdle()");
	Az2D::Profiling::Timer timerWaitIdle(sWaitIdle);
	timerWaitIdle.Start();
	AZ2D_PROFILING_EXCEPTION_START();
	vk::DeviceWaitIdle(data.device);
	AZ2D_PROFILING_EXCEPTION_END();
	timerWaitIdle.End();
	
	uniforms.screenSize = screenSize;
	if (!UpdateUniforms()) return false;

	VkCommandBuffer cmdBuf = data.commandBufferPrimary[data.buffer]->Begin();
	if (cmdBuf == VK_NULL_HANDLE) {
		error = "Failed to Begin recording primary command buffer: " + vk::error;
		return false;
	}

	data.renderPass->Begin(cmdBuf, data.framebuffer, false);

	vk::CmdExecuteCommands(cmdBuf, data.commandBuffersSecondary[data.buffer]);

	vkCmdEndRenderPass(cmdBuf);

	data.commandBufferPrimary[data.buffer]->End();

	if (!data.queueSubmission[data.buffer]->Config()) {
		error = "Failed to configure queue submisson: " + vk::error;
		return false;
	}

	// Submit to queue
	if (!data.device->SubmitCommandBuffers(data.queueGraphics, {data.queueSubmission[data.buffer]})) {
		error = "Failed to SubmitCommandBuffers: " + vk::error;
		return false;
	}
	return true;
}

bool Manager::Present() {
	if (data.zeroExtent) {
		Thread::Sleep(Milliseconds(clamp((i32)sys->frametimes.AverageWithoutOutliers(), 5, 50)));
		return true;
	}
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Rendering::Manager::Present)
	if (!data.swapchain->Present(data.queuePresent, {data.semaphoreRenderComplete->semaphore})) {
		error = "Failed to present: " + vk::error;
		return false;
	}
	return true;
}

void Manager::BindPipeline(DrawingContext &context, PipelineIndex pipeline) const {
	if (context.currentPipeline == pipeline) return;
	context.currentPipeline = pipeline;
	data.pipelines[pipeline]->Bind(context.commandBuffer);
	vk::CmdBindVertexBuffer(context.commandBuffer, 0, pipeline == PIPELINE_FONT_2D ? data.fontVertexBuffer : data.vertexBuffer);
	StaticArray<VkDescriptorSet, 4> sets;
	for (const Ptr<vk::DescriptorSet> &set : data.pipelineDescriptorSets[pipeline]) {
		sets.Append(set->data.set);
	}
	if (sets.size != 0) {
		vkCmdBindDescriptorSets(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, data.pipelines[pipeline]->data.layout, 0, sets.size, sets.data, 0, nullptr);
	}
}

void Manager::PushScissor(DrawingContext &context, vec2i min, vec2i max) {
	const ScissorState &prev = context.scissorStack.Back();
	ScissorState state;
	state.min.x = ::max(min.x, prev.min.x);
	state.min.y = ::max(min.y, prev.min.y);
	state.max.x = ::min(max.x, prev.max.x);
	state.max.y = ::min(max.y, prev.max.y);
	context.scissorStack.Append(state);
	vk::CmdSetScissor(context.commandBuffer, (u32)::max(state.max.x-state.min.x, 0), (u32)::max(state.max.y-state.min.y, 0), state.min.x, state.min.y);
}

void Manager::PopScissor(DrawingContext &context) {
	context.scissorStack.Erase(context.scissorStack.size-1);
	const ScissorState &state = context.scissorStack.Back();
	vk::CmdSetScissor(context.commandBuffer, (u32)(state.max.x-state.min.x), (u32)(state.max.y-state.min.y), state.min.x, state.min.y);
}

void Manager::UpdateBackground() {
	backgroundRGB = hsvToRgb(backgroundHSV);
}

f32 Manager::CharacterWidth(char32 character, const Assets::Font *fontDesired, const Assets::Font *fontFallback) const {
	const Assets::Font *actualFont = fontDesired;
	i32 glyphIndex = fontDesired->font.GetGlyphIndex(character);
	if (glyphIndex == 0) {
		i32 glyphIndexFallback = fontFallback->font.GetGlyphIndex(character);
		if (glyphIndexFallback != 0) {
			glyphIndex = glyphIndexFallback;
			actualFont = fontFallback;
		}
	}
	const i32 glyphId = actualFont->fontBuilder.indexToId[glyphIndex];
	return actualFont->fontBuilder.glyphs[glyphId].info.advance.x;
}

f32 Manager::LineWidth(const char32 *string, i32 fontIndex) const {
	const Assets::Font *fontDesired = &sys->assets.fonts[fontIndex];
	const Assets::Font *fontFallback = &sys->assets.fonts[0];
	f32 size = 0.0f;
	for (i32 i = 0; string[i] != '\n' && string[i] != 0; i++) {
		size += CharacterWidth(string[i], fontDesired, fontFallback);
	}
	return size;
}

vec2 Manager::StringSize(WString string, i32 fontIndex) const {
	const Assets::Font *fontDesired = &sys->assets.fonts[fontIndex];
	const Assets::Font *fontFallback = &sys->assets.fonts[0];
	// vec2 size = vec2(0.0, lineHeight * 2.0 - 1.0);
	vec2 size = vec2(0.0f, (1.0f + lineHeight) * 0.5f);
	f32 lineSize = 0.0f;
	for (i32 i = 0; i < string.size; i++) {
		const char32 character = string[i];
		if (character == '\n') {
			lineSize = 0.0f;
			size.y += lineHeight;
			continue;
		}
		lineSize += CharacterWidth(character, fontDesired, fontFallback);
		if (lineSize > size.x) {
			size.x = lineSize;
		}
	}
	return size;
}

f32 Manager::StringWidth(WString string, i32 fontIndex) const {
	return StringSize(string, fontIndex).x;
}

f32 StringHeight(WString string) {
	// f32 size = lineHeight * 2.0 - 1.0;
	f32 size = (1.0f + lineHeight) * 0.5f;
	for (i32 i = 0; i < string.size; i++) {
		const char32 character = string[i];
		if (character == '\n') {
			size += lineHeight;
		}
	}
	return size;
}

WString Manager::StringAddNewlines(WString string, i32 fontIndex, f32 maxWidth) const {
	if (maxWidth < 0.0f) {
		cout.PrintLn("Why are we negative???");
	}
	if (maxWidth <= 0.0f) {
		return string;
	}
	const Assets::Font *fontDesired = &sys->assets.fonts[fontIndex];
	const Assets::Font *fontFallback = &sys->assets.fonts[0];
	f32 tabWidth = CharacterWidth((char32)'_', fontDesired, fontFallback) * 4.0f;
	f32 lineSize = 0.0f;
	i32 lastSpace = -1;
	i32 charsThisLine = 0;
	for (i32 i = 0; i < string.size; i++) {
		if (string[i] == '\n') {
			lineSize = 0.0f;
			lastSpace = -1;
			charsThisLine = 0;
			continue;
		} else if (string[i] == '\t') {
			lineSize = ceil(lineSize/tabWidth+0.05f) * tabWidth;
		} else {
			lineSize += CharacterWidth(string[i], fontDesired, fontFallback);
		}
		charsThisLine++;
		if (string[i] == ' ' || string[i] == '\t') {
			lastSpace = i;
		}
		if (lineSize >= maxWidth && charsThisLine > 1) {
			if (lastSpace == -1) {
				string.Insert(i, char32('\n'));
			} else {
				string[lastSpace] = '\n';
				i = lastSpace;
			}
			lineSize = 0.0f;
			lastSpace = -1;
			charsThisLine = 0;
		}
	}
	return string;
}

void Manager::LineCursorStartAndSpaceScale(f32 &dstCursor, f32 &dstSpaceScale, f32 scale, f32 spaceWidth, i32 fontIndex, const char32 *string, f32 maxWidth, FontAlign alignH) const {
	dstSpaceScale = 1.0f;
	if (alignH != LEFT) {
		f32 lineWidth = LineWidth(string, fontIndex) * scale;
		if (alignH == RIGHT) {
			dstCursor = -lineWidth;
		} else if (alignH == MIDDLE) {
			dstCursor = -lineWidth * 0.5f;
		} else if (alignH == JUSTIFY) {
			dstCursor = 0.0f;
			i32 numSpaces = 0;
			for (i32 ii = 0; string[ii] != 0 && string[ii] != '\n'; ii++) {
				if (string[ii] == ' ') {
					numSpaces++;
				}
			}
			dstSpaceScale = 1.0f + max((maxWidth - lineWidth) / numSpaces / spaceWidth, 0.0f);
			if (dstSpaceScale > 4.0f) {
				dstSpaceScale = 1.5f;
			}
		}
	} else {
		dstCursor = 0.0f;
	}
}

void Manager::DrawCharSS(DrawingContext &context, char32 character, i32 fontIndex, vec4 color, vec2 position, vec2 scale) {
	Assets::Font *fontDesired = &sys->assets.fonts[fontIndex];
	Assets::Font *fontFallback = &sys->assets.fonts[0];
	Assets::Font *font = fontDesired;
	Rendering::PushConstants pc = Rendering::PushConstants();
	BindPipeline(context, PIPELINE_FONT_2D);
	color.rgb = sRGBToLinear(color.rgb);
	pc.frag.mat.color = color;
	i32 actualFontIndex = fontIndex;
	i32 glyphIndex = fontDesired->font.GetGlyphIndex(character);
	if (glyphIndex == 0) {
		const i32 glyphFallback = fontFallback->font.GetGlyphIndex(character);
		if (glyphFallback != 0) {
			glyphIndex = glyphFallback;
			font = fontFallback;
			actualFontIndex = 0;
		}
	}
	vec2 fullScale = vec2(aspectRatio * scale.x, scale.y);
	i32 glyphId = font->fontBuilder.indexToId[glyphIndex];
	if (glyphId == 0) {
		font->fontBuilder.AddRange(character, character);
	}
	font::Glyph& glyph = font->fontBuilder.glyphs[glyphId];
	pc.frag.tex.albedo = actualFontIndex;
	if (glyph.components.size != 0) {
		for (const font::Component& component : glyph.components) {
			i32 componentId = font->fontBuilder.indexToId[component.glyphIndex];
			pc.vert.transform = mat2::Scaler(fullScale);
			pc.font_circle.font.edge = 0.5f / (font::sdfDistance * screenSize.y * pc.vert.transform.h.y2);
			pc.vert.position = position + component.offset * fullScale;
			pc.PushFont(context.commandBuffer, this);
			vkCmdDrawIndexed(context.commandBuffer, 6, 1, 0, fontIndexOffsets[actualFontIndex] + componentId * 4, 0);
		}
	} else {
		pc.font_circle.font.edge = 0.5f / (font::sdfDistance * screenSize.y * scale.y);
		pc.vert.transform = mat2::Scaler(fullScale);
		pc.vert.position = position;
		pc.PushFont(context.commandBuffer, this);
		vkCmdDrawIndexed(context.commandBuffer, 6, 1, 0, fontIndexOffsets[actualFontIndex] + glyphId * 4, 0);
	}
}

void Manager::DrawTextSS(DrawingContext &context, WString string, i32 fontIndex, vec4 color, vec2 position, vec2 scale, FontAlign alignH, FontAlign alignV, f32 maxWidth, f32 edge, f32 bounds, Radians32 rotation) {
	if (string.size == 0) return;
	Assets::Font *fontDesired = &sys->assets.fonts[fontIndex];
	Assets::Font *fontFallback = &sys->assets.fonts[0];
	// scale.x *= aspectRatio;
	position.x /= aspectRatio;
	Rendering::PushConstants pc = Rendering::PushConstants();
	BindPipeline(context, PIPELINE_FONT_2D);
	color.rgb = sRGBToLinear(color.rgb);
	pc.frag.mat.color = color;
	// position.y += scale.y * lineHeight;
	position.y += scale.y * (lineHeight + 1.0f) * 0.5f;
	if (alignV != TOP) {
		f32 height = StringHeight(string) * scale.y;
		if (alignV == MIDDLE) {
			position.y -= height * 0.5f;
		} else {
			position.y -= height;
		}
	}
	vec2 cursor = position;
	f32 spaceScale = 1.0f;
	f32 spaceWidth = CharacterWidth((char32)' ', fontDesired, fontFallback) * scale.x;
	LineCursorStartAndSpaceScale(cursor.x, spaceScale, scale.x, spaceWidth, fontIndex, &string[0], maxWidth, alignH);
	f32 tabWidth = CharacterWidth((char32)'_', fontDesired, fontFallback) * scale.x * 4.0f;
	cursor.x += position.x;
	for (i32 i = 0; i < string.size; i++) {
		char32 character = string[i];
		if (character == '\n') {
			if (i+1 < string.size) {
				LineCursorStartAndSpaceScale(cursor.x, spaceScale, scale.x, spaceWidth, fontIndex, &string[i+1], maxWidth, alignH);
				cursor.x += position.x;
				cursor.y += scale.y * lineHeight;
			}
			continue;
		}
		if (character == '\t') {
			cursor.x = ceil((cursor.x - position.x)/tabWidth+0.05f) * tabWidth + position.x;
			continue;
		}
		pc.frag.tex.albedo = fontIndex;
		Assets::Font *font = fontDesired;
		i32 actualFontIndex = fontIndex;
		i32 glyphIndex = fontDesired->font.GetGlyphIndex(character);
		if (glyphIndex == 0) {
			const i32 glyphFallback = fontFallback->font.GetGlyphIndex(character);
			if (glyphFallback != 0) {
				glyphIndex = glyphFallback;
				font = fontFallback;
				pc.frag.tex.albedo = 0;
				actualFontIndex = 0;
			}
		}
		i32 glyphId = font->fontBuilder.indexToId[glyphIndex];
		if (glyphId == 0) {
			font->fontBuilder.AddRange(character, character);
		}
		font::Glyph& glyph = font->fontBuilder.glyphs[glyphId];

		pc.frag.tex.albedo = actualFontIndex;
		pc.font_circle.font.edge = edge / (font::sdfDistance * screenSize.y * scale.y);
		pc.font_circle.font.bounds = bounds;
		pc.vert.transform = mat2::Scaler(scale * vec2(aspectRatio, 1.0f));
		if (rotation != 0.0f) {
			pc.vert.transform = mat2::Rotation(rotation.value()) * pc.vert.transform;
		}
		if (glyph.components.size != 0) {
			for (const font::Component& component : glyph.components) {
				i32 componentId = font->fontBuilder.indexToId[component.glyphIndex];
				// const font::Glyph& componentGlyph = font->fontBuilder.glyphs[componentId];
				pc.vert.transform = component.transform * mat2::Scaler(scale * vec2(aspectRatio, 1.0f));
				if (rotation != 0.0f) {
					pc.vert.transform = mat2::Rotation(rotation.value()) * pc.vert.transform;
				}
				pc.font_circle.font.edge = edge / (font::sdfDistance * screenSize.y * abs(pc.vert.transform.h.y2));
				pc.vert.position = cursor + component.offset * scale * vec2(1.0f, -1.0f);
				if (rotation != 0.0f) {
					pc.vert.position = (pc.vert.position - position) * mat2::Rotation(rotation.value()) + position;
				}
				pc.vert.position *= vec2(aspectRatio, 1.0f);
				pc.PushFont(context.commandBuffer, this);
				vkCmdDrawIndexed(context.commandBuffer, 6, 1, 0, fontIndexOffsets[actualFontIndex] + componentId * 4, 0);
			}
		} else {
			if (character != ' ') {
				pc.vert.position = cursor;
				if (rotation != 0.0f) {
					pc.vert.position = (cursor-position) * mat2::Rotation(rotation.value()) + position;
				}
				pc.vert.position *= vec2(aspectRatio, 1.0f);
				pc.PushFont(context.commandBuffer, this);
				vkCmdDrawIndexed(context.commandBuffer, 6, 1, 0, fontIndexOffsets[actualFontIndex] + glyphId * 4, 0);
			}
		}
		if (character == ' ') {
			cursor += glyph.info.advance * spaceScale * scale;
		} else {
			cursor += glyph.info.advance * scale;
		}
	}
}

void Manager::DrawQuadSS(DrawingContext &context, vec2 position, vec2 scalePre, vec2 scalePost, vec2 origin, Radians32 rotation, PipelineIndex pipeline, Material material, TexIndices texture, f32 zShear, f32 zPos, vec2 texScale, vec2 texOffset) const {
	Rendering::PushConstants pc = Rendering::PushConstants();
	BindPipeline(context, pipeline);
	material.color.rgb = sRGBToLinear(material.color.rgb);
	pc.frag.tex = texture;
	pc.frag.mat = material;
	pc.vert.position = position;
	pc.vert.zShear = zShear;
	pc.vert.z = zPos;
	pc.vert.transform = mat2::Scaler(scalePre);
	pc.vert.texScale = texScale;
	pc.vert.texOffset = texOffset;
	if (rotation != 0.0f) {
		pc.vert.transform = pc.vert.transform * mat2::Rotation(rotation.value());
	}
	pc.vert.transform = pc.vert.transform * mat2::Scaler(scalePost);
	pc.vert.origin = origin;
	pc.Push2D(context.commandBuffer, this);
	vkCmdDrawIndexed(context.commandBuffer, 6, 1, 0, 0, 0);
}

void Manager::DrawCircleSS(DrawingContext &context, i32 texIndex, vec4 color, vec2 position, vec2 scalePre, vec2 scalePost, f32 edge, vec2 origin, Radians32 rotation) const {
	Rendering::PushConstants pc = Rendering::PushConstants();
	BindPipeline(context, PIPELINE_CIRCLE_2D);
	color.rgb = sRGBToLinear(color.rgb);
	pc.frag.mat = Material(color);
	pc.frag.tex = TexIndices(texIndex);
	pc.vert.position = position;
	pc.vert.transform = mat2::Scaler(scalePre);
	if (rotation != 0.0f) {
		pc.vert.transform = pc.vert.transform * mat2::Rotation(rotation.value());
	}
	pc.vert.transform = pc.vert.transform * mat2::Scaler(scalePost);
	pc.vert.origin = origin;
	pc.font_circle.circle.edge = edge;
	pc.PushCircle(context.commandBuffer, this);
	vkCmdDrawIndexed(context.commandBuffer, 6, 1, 0, 0, 0);
}

void Manager::DrawChar(DrawingContext &context, char32 character, i32 fontIndex, vec4 color, vec2 position, vec2 scale) {
	const vec2 screenSizeFactor = vec2(2.0f) / screenSize;
	DrawCharSS(context, character, fontIndex, color, position * screenSizeFactor + vec2(-1.0f), scale * screenSizeFactor);
}

void Manager::DrawText(DrawingContext &context, WString text, i32 fontIndex, vec4 color, vec2 position, vec2 scale, FontAlign alignH, FontAlign alignV, f32 maxWidth, f32 edge, f32 bounds) {
	const vec2 screenSizeFactor = vec2(2.0f) / screenSize;
	edge += 0.35f + clamp((scale.y - 12.0f) / 12.0f, 0.0f, 0.15f);
	bounds -= clamp((16.0f - scale.y) * 0.01f, 0.0f, 0.05f);
	DrawTextSS(context, text, fontIndex, color, position * screenSizeFactor + vec2(-1.0f), scale * screenSizeFactor.y, alignH, alignV, maxWidth * screenSizeFactor.x, edge, bounds);
}

void Manager::DrawQuad(DrawingContext &context, vec2 position, vec2 scalePre, vec2 scalePost, vec2 origin, Radians32 rotation, PipelineIndex pipeline, Material material, TexIndices texture, f32 zShear, f32 zPos, vec2 texScale, vec2 texOffset) const {
	const vec2 screenSizeFactor = vec2(2.0f) / screenSize;
	DrawQuadSS(context, position * screenSizeFactor + vec2(-1.0f), scalePre, scalePost * screenSizeFactor, origin, rotation, pipeline, material, texture, zShear, zPos*screenSizeFactor.y - 1.0f, texScale, texOffset);
}

void Manager::DrawCircle(DrawingContext &context, i32 texIndex, vec4 color, vec2 position, vec2 scalePre, vec2 scalePost, vec2 origin, Radians32 rotation) const {
	const vec2 screenSizeFactor = vec2(2.0f) / screenSize;
	DrawCircleSS(context, texIndex, color, position * screenSizeFactor + vec2(-1.0f), scalePre, scalePost * screenSizeFactor, 1.5f / scalePre.y, origin, rotation);
}

} // namespace Rendering
