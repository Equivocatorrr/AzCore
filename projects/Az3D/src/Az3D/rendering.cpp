/*
	File: rendering.cpp
	Author: Philip Haynes
*/

#include "rendering.hpp"
#include "game_systems.hpp"
#include "settings.hpp"
#include "assets.hpp"
// #include "gui_basics.hpp"
#include "profiling.hpp"
// #include "entity_basics.hpp"

#include "AzCore/IO/Log.hpp"
#include "AzCore/io.hpp"
#include "AzCore/font.hpp"
#include "AzCore/QuickSort.hpp"

namespace Az3D::Rendering {

constexpr i32 MAX_DEBUG_VERTICES = 8192;

using GameSystems::sys;

io::Log cout("rendering.log");

String error = "No error.";

void AddPointLight(vec3 pos, vec3 color, f32 distMin, f32 distMax) {
	AzAssert(distMin < distMax, "distMin must be < distMax, else shit breaks");
	Light light;
	light.position = vec4(pos, 1.0f);
	light.color = color;
	light.distMin = distMin;
	light.distMax = distMax;

	light.direction = vec3(0.0f, 0.0f, -1.0f);
	light.angleMin = pi;
	light.angleMax = tau;
	sys->rendering.lightsMutex.Lock();
	sys->rendering.lights.Append(light);
	sys->rendering.lightsMutex.Unlock();
}

void AddLight(vec3 pos, vec3 color, vec3 direction, f32 angleMin, f32 angleMax, f32 distMin, f32 distMax) {
	AzAssert(angleMin < angleMax, "angleMin must be < angleMax, else shit breaks");
	AzAssert(distMin < distMax, "distMin must be < distMax, else shit breaks");
	Light light;
	light.position = vec4(pos, 1.0f);
	light.color = color;
	light.direction = direction;
	light.angleMin = angleMin;
	light.angleMax = angleMax;
	light.distMin = distMin;
	light.distMax = distMax;
	sys->rendering.lightsMutex.Lock();
	sys->rendering.lights.Append(light);
	sys->rendering.lightsMutex.Unlock();
}

void BindPipeline(VkCommandBuffer cmdBuf, PipelineIndex pipeline) {
	Manager &r = sys->rendering;
	r.data.pipelines[pipeline]->Bind(cmdBuf);
	switch (pipeline) {
		case PIPELINE_DEBUG_LINES: {
			vk::CmdBindVertexBuffer(cmdBuf, 0, r.data.debugVertexBuffer);
		} break;
		case PIPELINE_BASIC_3D: {
			vk::CmdBindVertexBuffer(cmdBuf, 0, r.data.vertexBuffer);
		} break;
		case PIPELINE_FONT_3D: {
			vk::CmdBindVertexBuffer(cmdBuf, 0, r.data.fontVertexBuffer);
		} break;
	}
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, r.data.pipelines[pipeline]->data.layout, 0, 1, &r.data.descriptorSet->data.set, 0, nullptr);
}

bool Manager::Init() {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::Init)
	data.device = data.instance.AddDevice();
	data.device->data.vk12FeaturesRequired.scalarBlockLayout = VK_TRUE;
	data.device->data.vk12FeaturesRequired.uniformAndStorageBuffer8BitAccess = VK_TRUE;
	data.device->data.vk11FeaturesRequired.shaderDrawParameters = VK_TRUE; // For gl_BaseInstance
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
	attachment->bufferDepthStencil = true;
	if (msaa) {
		attachment->sampleCount = VK_SAMPLE_COUNT_4_BIT;
		attachment->resolveColor = true;
	}
	auto subpass = data.renderPass->AddSubpass();
	subpass->UseAttachment(attachment, vk::AttachmentType::ATTACHMENT_ALL,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
	data.framebuffer->renderPass = data.renderPass;
	// attachment->initialLayoutColor = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// attachment->loadColor = true;
	// attachment->clearColor = true;
	attachment->clearDepth = true;
	attachment->clearDepthStencilValue = {0.0f, 0};
	// attachment->clearColorValue = {1.0, 1.0, 1.0, 1.0};
	// attachment->clearColorValue = {0.0f, 0.1f, 0.2f, 1.0f}; // AzCore blue
	if (data.concurrency < 1) {
		data.concurrency = 1;
	}
	data.drawingContexts.Resize(data.concurrency);
	data.debugVertices.Resize(MAX_DEBUG_VERTICES);
	// Allow up to 10000 objects at once
	// TODO: Make this resizeable
	data.objectShaderInfos.Resize(10000);
	data.commandPoolGraphics = data.device->AddCommandPool(data.queueGraphics);
	data.commandPoolGraphics->resettable = true;

	data.semaphoreRenderComplete = data.device->AddSemaphore();

	for (i32 i = 0; i < 2; i++) {
		data.commandBufferGraphics[i] = data.commandPoolGraphics->AddCommandBuffer();
		data.queueSubmission[i] = data.device->AddQueueSubmission();
		data.queueSubmission[i]->commandBuffers = {data.commandBufferGraphics[i]};
		data.queueSubmission[i]->signalSemaphores = {data.semaphoreRenderComplete};
		data.queueSubmission[i]->waitSemaphores = {vk::SemaphoreWait(data.swapchain, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)};
		data.queueSubmission[i]->noAutoConfig = true;
	}
	data.commandBufferGraphicsTransfer = data.commandPoolGraphics->AddCommandBuffer();

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

	vk::Buffer baseStagingBuffer = vk::Buffer();
	baseStagingBuffer.size = 1;
	baseStagingBuffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vk::Buffer baseBuffer = vk::Buffer();
	baseBuffer.size = 1;
	baseBuffer.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	data.uniformStagingBuffer = data.stagingMemory->AddBuffer(baseStagingBuffer);
	data.uniformStagingBuffer->size = sizeof(UniformBuffer);
	data.uniformBuffer = data.bufferMemory->AddBuffer(baseBuffer);
	data.uniformBuffer->usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	data.uniformBuffer->size = data.uniformStagingBuffer->size;
	
	// TODO: Make the following buffers resizable

	data.objectStagingBuffer = data.stagingMemory->AddBuffer(baseStagingBuffer);
	data.objectStagingBuffer->size = data.objectShaderInfos.size * sizeof(ObjectShaderInfo);
	data.objectBuffer = data.bufferMemory->AddBuffer(baseBuffer);
	data.objectBuffer->usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	data.objectBuffer->size = data.objectStagingBuffer->size;

	data.meshPartUnitSquare = new Assets::MeshPart;
	sys->assets.meshParts.Append(data.meshPartUnitSquare);
	data.meshPartUnitSquare->vertices = {
		{vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(0.0f, 0.0f)},
		{vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(1.0f, 0.0f)},
		{vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(0.0f, 1.0f)},
		{vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(1.0f, 1.0f)},
	};
	data.meshPartUnitSquare->indices = {0, 1, 2, 1, 3, 2};
	data.meshPartUnitSquare->material = Material::Blank();

	Array<Vertex> vertices;
	Array<u32> indices;
	for (auto &part : sys->assets.meshParts) {
		part->indexStart = indices.size;
		indices.Reserve(indices.size+part->indices.size);
		for (i32 i = 0; i < part->indices.size; i++) {
			indices.Append(part->indices[i] + vertices.size);
		}
		vertices.Append(part->vertices);
	}
	data.vertexStagingBuffer = data.stagingMemory->AddBuffer(baseStagingBuffer);
	data.vertexStagingBuffer->size = vertices.size * sizeof(Vertex);
	data.indexStagingBuffer = data.stagingMemory->AddBuffer(baseStagingBuffer);
	data.indexStagingBuffer->size = indices.size * sizeof(u32);

	data.debugVertexStagingBuffer = data.stagingMemory->AddBuffer(baseStagingBuffer);
	data.debugVertexStagingBuffer->size = data.debugVertices.size * sizeof(DebugVertex);

	data.vertexBuffer = data.bufferMemory->AddBuffer(baseBuffer);
	data.indexBuffer = data.bufferMemory->AddBuffer(baseBuffer);
	data.vertexBuffer->usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	data.vertexBuffer->size = data.vertexStagingBuffer->size;
	data.indexBuffer->usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	data.indexBuffer->size = data.indexStagingBuffer->size;

	data.debugVertexBuffer = data.bufferMemory->AddBuffer(baseBuffer);
	data.debugVertexBuffer->usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	data.debugVertexBuffer->size = data.debugVertexStagingBuffer->size;

	auto texStagingBuffers = data.stagingMemory->AddBuffers(sys->assets.textures.size, baseStagingBuffer);

	data.fontStagingVertexBuffer = data.fontStagingMemory->AddBuffer(baseStagingBuffer);
	data.fontStagingImageBuffers = data.fontStagingMemory->AddBuffers(sys->assets.fonts.size, baseStagingBuffer);

	data.fontVertexBuffer = data.fontBufferMemory->AddBuffer(baseBuffer);
	data.fontVertexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	vk::Image baseImage = vk::Image();
	baseImage.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	baseImage.format = VK_FORMAT_R8G8B8A8_SRGB;
	auto texImages = data.textureMemory->AddImages(sys->assets.textures.size, baseImage);
	for (i32 i = 0; i < sys->assets.textures.size; i++) {
		Image &image = sys->assets.textures[i].image;
		switch (image.channels) {
		case 1:
			data.textureMemory->data.images[i].format = image.colorSpace == Image::LINEAR ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8_SRGB;
			break;
		case 2:
			data.textureMemory->data.images[i].format = image.colorSpace == Image::LINEAR ? VK_FORMAT_R8G8_UNORM : VK_FORMAT_R8G8_SRGB;
			break;
		case 3:
			data.textureMemory->data.images[i].format = image.colorSpace == Image::LINEAR ? VK_FORMAT_R8G8B8_UNORM : VK_FORMAT_R8G8B8_SRGB;
			break;
		case 4:
			data.textureMemory->data.images[i].format = image.colorSpace == Image::LINEAR ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_SRGB;
			break;
		default:
			error = Stringify("Texture image ", i, " has invalid channel count (", image.channels, ")");
			return false;
		}
	}

	baseImage.format = VK_FORMAT_R8_UNORM;
	baseImage.width = 1;
	baseImage.height = 1;
	data.fontImages = data.fontImageMemory->AddImages(sys->assets.fonts.size, baseImage);

	for (i32 i = 0; i < texImages.size; i++) {
		const i32 channels = sys->assets.textures[i].image.channels;
		texImages[i].width = sys->assets.textures[i].image.width;
		texImages[i].height = sys->assets.textures[i].image.height;
		texImages[i].mipLevels = (u32)floor(log2((f32)max(texImages[i].width, texImages[i].height))) + 1;

		texStagingBuffers[i].size = channels * texImages[i].width * texImages[i].height;
	}

	data.descriptors = data.device->AddDescriptors();
	Ptr<vk::DescriptorLayout> descriptorLayout = data.descriptors->AddLayout();
	descriptorLayout->bindings.Resize(3);
	// Binding 0 is WorldInfo
	descriptorLayout->bindings[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorLayout->bindings[0].stage = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
	descriptorLayout->bindings[0].binding = 0;
	descriptorLayout->bindings[0].count = 1;
	// Binding 1 is ObjectInfo
	descriptorLayout->bindings[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorLayout->bindings[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
	descriptorLayout->bindings[1].binding = 1;
	descriptorLayout->bindings[1].count = 1;
	// Binding 2 is textures
	descriptorLayout->bindings[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorLayout->bindings[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorLayout->bindings[2].binding = 2;
	descriptorLayout->bindings[2].count = sys->assets.textures.size;

	data.descriptorSet = data.descriptors->AddSet(descriptorLayout);
	if (!data.descriptorSet->AddDescriptor(data.uniformBuffer, 0)) {
		error = "Failed to add Uniform Buffer Descriptor: " + vk::error;
		return false;
	}
	if (!data.descriptorSet->AddDescriptor(data.objectBuffer, 1)) {
		error = "Failed to add Storage Buffer Descriptor: " + vk::error;
		return false;
	}
	if (!data.descriptorSet->AddDescriptor(texImages, data.textureSampler, 2)) {
		error = "Failed to add Texture Descriptor: " + vk::error;
		return false;
	}

	Range<vk::Shader> shaders = data.device->AddShaders(5);
	shaders[0].filename = "data/Az3D/shaders/DebugLines.vert.spv";
	shaders[1].filename = "data/Az3D/shaders/DebugLines.frag.spv";
	shaders[2].filename = "data/Az3D/shaders/Basic3D.vert.spv";
	shaders[3].filename = "data/Az3D/shaders/Basic3D.frag.spv";
	shaders[4].filename = "data/Az3D/shaders/Font3D.frag.spv";

	vk::ShaderRef shaderRefDebugLinesVert = vk::ShaderRef(shaders.GetPtr(0), VK_SHADER_STAGE_VERTEX_BIT);
	vk::ShaderRef shaderRefDebugLinesFrag = vk::ShaderRef(shaders.GetPtr(1), VK_SHADER_STAGE_FRAGMENT_BIT);
	vk::ShaderRef shaderRefVert = vk::ShaderRef(shaders.GetPtr(2), VK_SHADER_STAGE_VERTEX_BIT);
	vk::ShaderRef shaderRefBasic3D = vk::ShaderRef(shaders.GetPtr(3), VK_SHADER_STAGE_FRAGMENT_BIT);
	vk::ShaderRef shaderRefFont3D = vk::ShaderRef(shaders.GetPtr(4), VK_SHADER_STAGE_FRAGMENT_BIT);

	data.pipelines.Resize(PIPELINE_COUNT);
	
	data.pipelines[PIPELINE_DEBUG_LINES] = data.device->AddPipeline();
	data.pipelines[PIPELINE_DEBUG_LINES]->renderPass = data.renderPass;
	data.pipelines[PIPELINE_DEBUG_LINES]->subpass = 0;
	data.pipelines[PIPELINE_DEBUG_LINES]->shaders.Append(shaderRefDebugLinesVert);
	data.pipelines[PIPELINE_DEBUG_LINES]->shaders.Append(shaderRefDebugLinesFrag);
	data.pipelines[PIPELINE_DEBUG_LINES]->inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	data.pipelines[PIPELINE_DEBUG_LINES]->rasterizer.lineWidth = 2.0f;
	data.pipelines[PIPELINE_DEBUG_LINES]->depthStencil.depthTestEnable = VK_TRUE;
	data.pipelines[PIPELINE_DEBUG_LINES]->depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;
	data.pipelines[PIPELINE_DEBUG_LINES]->descriptorLayouts.Append(descriptorLayout);
	
	data.pipelines[PIPELINE_BASIC_3D] = data.device->AddPipeline();
	data.pipelines[PIPELINE_BASIC_3D]->renderPass = data.renderPass;
	data.pipelines[PIPELINE_BASIC_3D]->subpass = 0;
	data.pipelines[PIPELINE_BASIC_3D]->shaders.Append(shaderRefVert);
	data.pipelines[PIPELINE_BASIC_3D]->shaders.Append(shaderRefBasic3D);
	data.pipelines[PIPELINE_BASIC_3D]->rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	// data.pipelines[PIPELINE_BASIC_3D]->rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	data.pipelines[PIPELINE_BASIC_3D]->depthStencil.depthTestEnable = VK_TRUE;
	data.pipelines[PIPELINE_BASIC_3D]->depthStencil.depthWriteEnable = VK_TRUE;
	data.pipelines[PIPELINE_BASIC_3D]->depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;
	data.pipelines[PIPELINE_BASIC_3D]->descriptorLayouts.Append(descriptorLayout);

	data.pipelines[PIPELINE_BASIC_3D]->dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	data.pipelines[PIPELINE_DEBUG_LINES]->dynamicStates = data.pipelines[PIPELINE_BASIC_3D]->dynamicStates;

	data.pipelines[PIPELINE_FONT_3D] = data.device->AddPipeline();
	data.pipelines[PIPELINE_FONT_3D]->renderPass = data.renderPass;
	data.pipelines[PIPELINE_FONT_3D]->subpass = 0;
	data.pipelines[PIPELINE_FONT_3D]->shaders.Append(shaderRefVert);
	data.pipelines[PIPELINE_FONT_3D]->shaders.Append(shaderRefFont3D);
	data.pipelines[PIPELINE_FONT_3D]->descriptorLayouts.Append(descriptorLayout);

	data.pipelines[PIPELINE_FONT_3D]->dynamicStates = data.pipelines[PIPELINE_BASIC_3D]->dynamicStates;

	VkVertexInputAttributeDescription vertexInputAttributeDescription = {};
	vertexInputAttributeDescription.binding = 0;
	vertexInputAttributeDescription.location = 0;
	vertexInputAttributeDescription.offset = offsetof(DebugVertex, pos);
	vertexInputAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	data.pipelines[PIPELINE_DEBUG_LINES]->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
	vertexInputAttributeDescription.location = 1;
	vertexInputAttributeDescription.offset = offsetof(DebugVertex, color);
	vertexInputAttributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	data.pipelines[PIPELINE_DEBUG_LINES]->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
	
	vertexInputAttributeDescription.binding = 0;
	vertexInputAttributeDescription.location = 0;
	vertexInputAttributeDescription.offset = offsetof(Vertex, pos);
	vertexInputAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	for (i32 i = PIPELINE_3D_RANGE_START; i < PIPELINE_3D_RANGE_END; i++) {
		data.pipelines[i]->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
	}
	vertexInputAttributeDescription.location = 1;
	vertexInputAttributeDescription.offset = offsetof(Vertex, normal);
	vertexInputAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	for (i32 i = PIPELINE_3D_RANGE_START; i < PIPELINE_3D_RANGE_END; i++) {
		data.pipelines[i]->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
	}
	vertexInputAttributeDescription.location = 2;
	vertexInputAttributeDescription.offset = offsetof(Vertex, tex);
	vertexInputAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
	for (i32 i = PIPELINE_3D_RANGE_START; i < PIPELINE_3D_RANGE_END; i++) {
		data.pipelines[i]->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
	}
	VkVertexInputBindingDescription vertexInputBindingDescription = {};
	vertexInputBindingDescription.binding = 0;
	vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexInputBindingDescription.stride = sizeof(DebugVertex);
	data.pipelines[PIPELINE_DEBUG_LINES]->inputBindingDescriptions.Append(vertexInputBindingDescription);
	
	vertexInputBindingDescription.binding = 0;
	vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexInputBindingDescription.stride = sizeof(Vertex);
	for (i32 i = PIPELINE_3D_RANGE_START; i < PIPELINE_3D_RANGE_END; i++) {
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

	for (i32 i = 1; i < PIPELINE_COUNT; i++) {
		data.pipelines[i]->colorBlendAttachments.Append(colorBlendAttachment);
	}

	if (!data.instance.Init()) {
		error = "Failed to init vk::instance: " + vk::error;
		return false;
	}
	
	uniforms.lights[0].position = vec4(vec3(0.0f), 1.0f);
	uniforms.lights[0].color = vec3(0.0f);
	uniforms.lights[0].direction = vec3(0.0f, 0.0f, 1.0f);
	uniforms.lights[0].angleMin = 0.0f;
	uniforms.lights[0].angleMax = 0.0f;
	uniforms.lights[0].distMin = 0.0f;
	uniforms.lights[0].distMax = 0.0f;

	data.vertexStagingBuffer->CopyData(vertices.data);
	data.indexStagingBuffer->CopyData(indices.data);
	for (i32 i = 0; i < texStagingBuffers.size; i++) {
		texStagingBuffers[i].CopyData(sys->assets.textures[i].image.pixels);
	}

	VkCommandBuffer cmdBufCopy = data.commandBufferGraphicsTransfer->Begin();
	data.vertexBuffer->Copy(cmdBufCopy, data.vertexStagingBuffer);
	data.indexBuffer->Copy(cmdBufCopy, data.indexStagingBuffer);

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

// using Entities::AABB;
// AABB GetAABB(const Light &light) {
// 	AABB result;
// 	vec2 center = {light.position.x, light.position.y};
	
// 	result.minPos = center;
// 	result.maxPos = center;
	
// 	f32 dist = light.distMax;// * sqrt(1.0f - square(light.direction.z));
// 	Angle32 cardinalDirs[4] = {0.0f, halfpi, pi, halfpi * 3.0f};
// 	vec2 cardinalVecs[4] = {
// 		{dist, 0.0f},
// 		{0.0f, dist},
// 		{-dist, 0.0f},
// 		{0.0f, -dist},
// 	};
// 	if (light.direction.x != 0.0f || light.direction.y != 0.0f) {
// 		Angle32 dir = atan2(light.direction.y, light.direction.x);
// 		Angle32 dirMin = dir + -light.angleMax;
// 		Angle32 dirMax = dir + light.angleMax;
// 		result.Extend(center + vec2(cos(dirMin), sin(dirMin)) * dist);
// 		result.Extend(center + vec2(cos(dirMax), sin(dirMax)) * dist);
// 		for (i32 i = 0; i < 4; i++) {
// 			if (abs(cardinalDirs[i] - dir) < light.angleMax) {
// 				result.Extend(center + cardinalVecs[i]);
// 			}
// 		}
// 	} else {
// 		for (i32 i = 0; i < 4; i++) {
// 			result.Extend(center + cardinalVecs[i]);
// 		}
// 	}
// 	return result;
// }

// vec2i GetLightBin(vec2 point, vec2 screenSize) {
// 	vec2i result;
// 	result.x = (point.x) / screenSize.x * LIGHT_BIN_COUNT_X;
// 	result.y = (point.y) / screenSize.y * LIGHT_BIN_COUNT_Y;
// 	return result;
// }

// i32 LightBinIndex(vec2i bin) {
// 	return bin.y * LIGHT_BIN_COUNT_X + bin.x;
// }

// void Manager::UpdateLights() {
// 	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::UpdateLights)
// 	i32 lightCounts[LIGHT_BIN_COUNT] = {0};
// 	i32 totalLights = 1;
// 	// By default, they all point to the default light which has no light at all
// 	memset(uniforms.lightBins, 0, sizeof(uniforms.lightBins));
// #if 1
// 	for (const Light &light : lights) {
// 		if (totalLights >= MAX_LIGHTS) break;
// 		AABB lightAABB = GetAABB(light);
// 		vec2i binMin = GetLightBin(lightAABB.minPos, screenSize);
// 		if (binMin.x >= LIGHT_BIN_COUNT_X || binMin.y >= LIGHT_BIN_COUNT_Y) continue;
// 		vec2i binMax = GetLightBin(lightAABB.maxPos, screenSize);
// 		if (binMax.x < 0 || binMax.y < 0) continue;
// 		binMin.x = max(binMin.x, 0);
// 		binMin.y = max(binMin.y, 0);
// 		binMax.x = min(binMax.x, LIGHT_BIN_COUNT_X-1);
// 		binMax.y = min(binMax.y, LIGHT_BIN_COUNT_Y-1);
// 		i32 lightIndex = totalLights;
// 		uniforms.lights[lightIndex] = light;
// 		bool atLeastOne = false;
// 		for (i32 y = binMin.y; y <= binMax.y; y++) {
// 			for (i32 x = binMin.x; x <= binMax.x; x++) {
// 				i32 i = LightBinIndex({x, y});
// 				LightBin &bin = uniforms.lightBins[i];
// 				if (lightCounts[i] >= MAX_LIGHTS_PER_BIN) continue;
// 				atLeastOne = true;
// 				bin.lightIndices[lightCounts[i]] = lightIndex;
// 				lightCounts[i]++;
// 			}
// 		}
// 		if (atLeastOne) {
// 			totalLights++;
// 		}
// 	}
// #else
// 	for (i32 i = 0; i < LIGHT_BIN_COUNT; i++) {
// 		// for (i32 l = 0; l < MAX_LIGHTS_PER_BIN; l++)
// 			uniforms.lightBins[i].lightIndices[0] = i;
// 	}
// #endif
// }

bool Manager::UpdateFonts() {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::UpdateFonts)
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
			quad[0].pos = vec3(posLeft, posTop, 0.0f);
			quad[0].normal = vec3(0.0f, 0.0f, -1.0f);
			quad[0].tex = vec2(texLeft, texTop);
			quad[1].pos = vec3(posLeft, posBot, 0.0f);
			quad[1].normal = vec3(0.0f, 0.0f, -1.0f);
			quad[1].tex = vec2(texLeft, texBot);
			quad[2].pos = vec3(posRight, posBot, 0.0f);
			quad[2].normal = vec3(0.0f, 0.0f, -1.0f);
			quad[2].tex = vec2(texRight, texBot);
			quad[3].pos = vec3(posRight, posTop, 0.0f);
			quad[3].normal = vec3(0.0f, 0.0f, -1.0f);
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

String ToString(mat4 mat, i32 precision=2) {
	return Stringify(
		"| ", FormatFloat(mat.v.x1, 10, precision), ", ", FormatFloat(mat.v.y1, 10, precision), ", ", FormatFloat(mat.v.z1, 10, precision), ", ", FormatFloat(mat.v.w1, 10, precision), " |\n", 
		"| ", FormatFloat(mat.v.x2, 10, precision), ", ", FormatFloat(mat.v.y2, 10, precision), ", ", FormatFloat(mat.v.z2, 10, precision), ", ", FormatFloat(mat.v.w2, 10, precision), " |\n", 
		"| ", FormatFloat(mat.v.x3, 10, precision), ", ", FormatFloat(mat.v.y3, 10, precision), ", ", FormatFloat(mat.v.z3, 10, precision), ", ", FormatFloat(mat.v.w3, 10, precision), " |\n", 
		"| ", FormatFloat(mat.v.x4, 10, precision), ", ", FormatFloat(mat.v.y4, 10, precision), ", ", FormatFloat(mat.v.z4, 10, precision), ", ", FormatFloat(mat.v.w4, 10, precision), " |");
}

String ToString(vec4 vec, i32 precision=2) {
	return Stringify("[ ", FormatFloat(vec.x, 10, precision), ", ", FormatFloat(vec.y, 10, precision), ", ", FormatFloat(vec.z, 10, 1), ", ", FormatFloat(vec.w, 10, precision), " ]");
}

vec4 PerspectiveNormalize(vec4 point) {
	return point / point.w;
}

bool Manager::UpdateUniforms() {
	// Update camera matrix
	uniforms.view = mat4::Camera(camera.pos, camera.forward, camera.up);
	uniforms.proj = mat4::Perspective(camera.fov, screenSize.x / screenSize.y, camera.nearClip, camera.farClip);
	uniforms.viewProj = uniforms.view * uniforms.proj;
	// UpdateLights();
	
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

bool Manager::UpdateObjects() {
	data.objectStagingBuffer->CopyData(data.objectShaderInfos.data, min(data.objectShaderInfos.size, 10000) * sizeof(ObjectShaderInfo));
	VkCommandBuffer cmdBuf = data.commandBufferTransfer->Begin();
	data.objectBuffer->Copy(cmdBuf, data.objectStagingBuffer);
	if (!data.commandBufferTransfer->End()) {
		error = "Failed to copy from objects staging buffer: " + vk::error;
		return false;
	}
	if (!data.device->SubmitCommandBuffers(data.queueTransfer, {data.queueSubmissionTransfer})) {
		error = "Failed to submit transer command buffer for objects: " + vk::error;
		return false;
	}
	// TODO: Synchronize this with the graphics queue using a semaphore
	vk::QueueWaitIdle(data.queueTransfer);

	return true;
}

bool Manager::UpdateDebugLines(VkCommandBuffer cmdBuf) {
	data.debugVertices.ClearSoft();
	for (DrawingContext &context : data.drawingContexts) {
		data.debugVertices.Append(context.debugLines);
	}
	if (data.debugVertices.size < 2) return true;
	
	BindPipeline(cmdBuf, PIPELINE_DEBUG_LINES);
	
	vkCmdDraw(cmdBuf, data.debugVertices.size, 1, 0, 0);
	
	data.debugVertexStagingBuffer->CopyData(data.debugVertices.data, min(data.debugVertices.size, MAX_DEBUG_VERTICES) * sizeof(DebugVertex));
	VkCommandBuffer cmdBufTransfer = data.commandBufferTransfer->Begin();
	data.debugVertexBuffer->Copy(cmdBufTransfer, data.debugVertexStagingBuffer);
	if (!data.commandBufferTransfer->End()) {
		error = "Failed to copy from debug vertex staging buffer: " + vk::error;
		return false;
	}
	if (!data.device->SubmitCommandBuffers(data.queueTransfer, {data.queueSubmissionTransfer})) {
		error = "Failed to submit transer command buffer for debug vertices: " + vk::error;
		return false;
	}
	// TODO: Synchronize this with the graphics queue using a semaphore
	vk::QueueWaitIdle(data.queueTransfer);

	return true;
}

bool Manager::Draw() {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::Draw)
	if (vk::hadValidationError) {
		error = "Quitting due to vulkan validation error.";
		return false;
	}
	if (sys->window.resized || data.resized || data.zeroExtent) {
		AZ3D_PROFILING_EXCEPTION_START();
		vk::DeviceWaitIdle(data.device);
		AZ3D_PROFILING_EXCEPTION_END();
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
		AZ3D_PROFILING_EXCEPTION_START();
		vk::DeviceWaitIdle(data.device);
		AZ3D_PROFILING_EXCEPTION_END();
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
		AZ3D_PROFILING_EXCEPTION_START();
		vk::DeviceWaitIdle(data.device);
		AZ3D_PROFILING_EXCEPTION_END();
		if (!UpdateFonts()) {
			return false;
		}
	}

	static Az3D::Profiling::AString sAcquisition("Swapchain::AcquireNextImage");
	Az3D::Profiling::Timer timerAcquisition(sAcquisition);
	timerAcquisition.Start();
	AZ3D_PROFILING_EXCEPTION_START();
	VkResult acquisitionResult = data.swapchain->AcquireNextImage();
	timerAcquisition.End();
	AZ3D_PROFILING_EXCEPTION_END();

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

	Ptr<vk::CommandBuffer> commandBuffer = data.commandBufferGraphics[data.buffer];
	VkCommandBuffer cmdBuf = commandBuffer->Begin();
	if (cmdBuf == VK_NULL_HANDLE) {
		error = "Failed to Begin recording graphics command buffer: " + vk::error;
		return false;
	}
	data.renderPass->Begin(cmdBuf, data.framebuffer, true);
	vk::CmdSetViewportAndScissor(cmdBuf, sys->window.width, sys->window.height);
	vk::CmdBindIndexBuffer(cmdBuf, data.indexBuffer, VK_INDEX_TYPE_UINT32);
	/*{ // Fade
		DrawQuadSS(commandBuffersSecondary[0], texBlank, vec4(backgroundRGB, 0.2f), vec2(-1.0), vec2(2.0), vec2(1.0));
	}*/
	{ // Clear
		vk::CmdClearColorAttachment(
			cmdBuf,
			data.renderPass->data.subpasses[0].data.referencesColor[0].attachment,
			vec4(sRGBToLinear(backgroundRGB), 1.0f),
			sys->window.width,
			sys->window.height
		);
	}
	// Clear lights so we get new ones this frame
	lights.size = 0;
	
	for (DrawingContext &context : data.drawingContexts) {
		context.thingsToDraw.ClearSoft();
		context.debugLines.ClearSoft();
	}

	sys->Draw(data.drawingContexts);
	
	data.objectShaderInfos.ClearSoft();
	{ // Sorting draw calls
		Array<DrawCallInfo> allDrawCalls;
		for (DrawingContext &context : data.drawingContexts) {
			allDrawCalls.Append(context.thingsToDraw);
		}
		QuickSort(allDrawCalls, [](const DrawCallInfo &lhs, const DrawCallInfo &rhs) -> bool {
			if (lhs.opaque != rhs.opaque) return lhs.opaque;
			if (lhs.pipeline < rhs.pipeline) return true;
			// We want opaque objects sorted front to back
			// and transparent objects sorted back to front
			if (lhs.depth < rhs.depth) return lhs.opaque;
			return false;
		});
		PipelineIndex currentPipeline = PIPELINE_NONE;
		for (DrawCallInfo &drawCall : allDrawCalls) {
			if (drawCall.culled) continue;
			if (drawCall.pipeline != currentPipeline) {
				BindPipeline(cmdBuf, drawCall.pipeline);
				currentPipeline = drawCall.pipeline;
			}
			drawCall.instanceStart = data.objectShaderInfos.size;
			data.objectShaderInfos.Append(ObjectShaderInfo{drawCall.transform, drawCall.material});
			vkCmdDrawIndexed(cmdBuf, drawCall.indexCount, drawCall.instanceCount, drawCall.indexStart, 0, drawCall.instanceStart);
		}
	}

	// { // Debug info
	// 	if (Settings::ReadBool(Settings::sDebugInfo)) {
	// 		f32 msAvg = sys->frametimes.Average();
	// 		f32 msMax = sys->frametimes.Max();
	// 		f32 msMin = sys->frametimes.Min();
	// 		f32 msDiff = msMax - msMin;
	// 		f32 fps = 1000.0f / msAvg;
	// 		DrawQuad2D(0.0f, vec2(500.0f, 20.0f) * Gui::guiBasic->scale, 1.0f, 0.0f, 0.0f, PIPELINE_BASIC_2D, vec4(vec3(0.0f), 0.5f));
	// 		WString strings[] = {
	// 			ToWString(Stringify("fps: ", FormatFloat(fps, 10, 1))),
	// 			ToWString(Stringify("avg: ", FormatFloat(msAvg, 10, 1), "ms")),
	// 			ToWString(Stringify("max: ", FormatFloat(msMax, 10, 1), "ms")),
	// 			ToWString(Stringify("min: ", FormatFloat(msMin, 10, 1), "ms")),
	// 			ToWString(Stringify("diff: ", FormatFloat(msDiff, 10, 1), "ms")),
	// 			ToWString(Stringify("timestep: ", FormatFloat(sys->timestep * 1000.0f, 10, 1), "ms")),
	// 		};
	// 		for (i32 i = 0; i < (i32)(sizeof(strings)/sizeof(WString)); i++) {
	// 			vec2 pos = vec2(4.0f + f32(i*80), 4.0f) * Gui::guiBasic->scale;
	// 			DrawText(commandBuffersSecondary.Back(), strings[i], 0, vec4(1.0f), pos, vec2(12.0f * Gui::guiBasic->scale), LEFT, TOP);
	// 		}
	// 	}
	// }

	static Az3D::Profiling::AString sWaitIdle("vk::DeviceWaitIdle()");
	Az3D::Profiling::Timer timerWaitIdle(sWaitIdle);
	timerWaitIdle.Start();
	AZ3D_PROFILING_EXCEPTION_START();
	vk::DeviceWaitIdle(data.device);
	AZ3D_PROFILING_EXCEPTION_END();
	timerWaitIdle.End();
	
	if (!UpdateUniforms()) return false;
	if (!UpdateObjects()) return false;
	if (!UpdateDebugLines(cmdBuf)) return false;
	
	vkCmdEndRenderPass(cmdBuf);

	if (!commandBuffer->End()) {
		error = Stringify("Failed to End graphics command buffer: ", vk::error);
		return false;
	}

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
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::Present)
	if (!data.swapchain->Present(data.queuePresent, {data.semaphoreRenderComplete->semaphore})) {
		error = "Failed to present: " + vk::error;
		return false;
	}
	return true;
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

void DrawMeshPart(DrawingContext &context, Assets::MeshPart *meshPart, const mat4 &transform, bool opaque, bool castsShadows) {
	DrawCallInfo draw;
	draw.transform = transform;
	draw.boundingSphereCenter = transform.Col4().xyz;
	draw.boundingSphereRadius = meshPart->boundingSphereRadius * sqrt(max(
		normSqr(transform.Col1().xyz),
		normSqr(transform.Col2().xyz),
		normSqr(transform.Col3().xyz)
	));
	draw.depth = dot(sys->rendering.camera.forward, draw.boundingSphereCenter - sys->rendering.camera.pos);
	draw.indexStart = meshPart->indexStart;
	draw.indexCount = meshPart->indices.size;
	draw.instanceCount = 1;
	draw.material = meshPart->material;
	draw.pipeline = PIPELINE_BASIC_3D;
	draw.opaque = opaque;
	draw.castsShadows = castsShadows;
	draw.culled = false;
	// We don't need synchronization because each thread gets their own array.
	context.thingsToDraw.Append(draw);
}

void DrawMesh(DrawingContext &context, Assets::Mesh mesh, const mat4 &transform, bool opaque, bool castsShadows) {
	for (Assets::MeshPart *meshPart : mesh.parts) {
		DrawMeshPart(context, meshPart, transform, opaque, castsShadows);
	}
}

/*
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
*/

} // namespace Rendering
