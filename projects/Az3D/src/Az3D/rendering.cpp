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

#ifdef TRANSPARENT
#undef TRANSPARENT
#endif
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

namespace Az3D::Rendering {

constexpr i32 MAX_DEBUG_VERTICES = 8192;

using GameSystems::sys;

io::Log cout("rendering.log");

String error = "No error.";

struct Frustum {
	struct Plane {
		vec3 normal;
		// distance from origin in the normal direction
		f32 dist;
	};
	Plane near;
	Plane far;
	Plane left;
	Plane right;
	Plane top;
	Plane bottom;
};

Frustum::Plane GetPlaneFromRay(vec3 point, vec3 normal) {
	Frustum::Plane result;
	result.normal = normal;
	result.dist = dot(point, normal);
	return result;
}

Frustum GetFrustumFromCamera(const Camera &camera, f32 heightOverWidth) {
	Frustum result;
	result.near  = GetPlaneFromRay(camera.pos + camera.nearClip * camera.forward, camera.forward);
	result.far   = GetPlaneFromRay(camera.pos + camera.farClip * camera.forward, -camera.forward);
	f32 tanhFOV = tan(Radians32(camera.fov).value() * 0.5f);
	f32 tanvFOV = tanhFOV * heightOverWidth;
	vec3 forward = normalize(camera.forward);
	vec3 right = normalize(cross(camera.up, forward));
	vec3 up = normalize(cross(forward, right));
	result.left  = GetPlaneFromRay(camera.pos, normalize(right + forward * tanhFOV));
	result.right = GetPlaneFromRay(camera.pos, normalize(-right + forward * tanhFOV));
	result.top    = GetPlaneFromRay(camera.pos, normalize(-up + forward * tanvFOV));
	result.bottom = GetPlaneFromRay(camera.pos, normalize(up + forward * tanvFOV));
	return result;
}

bool IsSphereAbovePlane(vec3 center, f32 radius, const Frustum::Plane &plane) {
	return dot(center, plane.normal) - plane.dist + radius > 0.0f;
}

bool IsSphereInFrustum(vec3 center, f32 radius, const Frustum &frustum) {
	return
		IsSphereAbovePlane(center, radius, frustum.near) &&
		IsSphereAbovePlane(center, radius, frustum.far) &&
		IsSphereAbovePlane(center, radius, frustum.left) &&
		IsSphereAbovePlane(center, radius, frustum.right) &&
		IsSphereAbovePlane(center, radius, frustum.bottom) &&
		IsSphereAbovePlane(center, radius, frustum.top);
}

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

void BindPipeline(GPU::Context *context, PipelineIndex pipeline) {
	Manager &r = sys->rendering;
	GPU::CmdBindPipeline(context, r.data.pipelines[pipeline]);
	switch (pipeline) {
		case PIPELINE_DEBUG_LINES: {
			GPU::CmdBindVertexBuffer(context, r.data.debugVertexBuffer);
		} break;
		case PIPELINE_BASIC_3D:
		case PIPELINE_FOLIAGE_3D: {
			GPU::CmdBindVertexBuffer(context, r.data.vertexBuffer);
		} break;
		case PIPELINE_FONT_3D: {
			GPU::CmdBindVertexBuffer(context, r.data.fontVertexBuffer);
		} break;
	}
	GPU::CmdCommitBindings(context).AzUnwrap();
}

bool Manager::Init() {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::Init)

	{ // Device
		data.device = GPU::NewDevice();
		GPU::DeviceRequireFeatures(data.device, {
			"scalarBlockLayout",
			"uniformAndStorageBuffer8BitAccess",
			"shaderDrawParameters",
		});
	}
	{ // Window
		data.window = GPU::AddWindow(&sys->window).AzUnwrap();
		GPU::SetVSync(data.window, Settings::ReadBool(Settings::sVSync));
		// TODO: Probably support differenc color spaces
	}
	{ // Framebuffer
		data.framebuffer = GPU::NewFramebuffer(data.device, "primary");
		data.depthBuffer = GPU::NewImage(data.device, "depthBuffer");
		GPU::ImageSetFormat(data.depthBuffer, GPU::ImageBits::D32, GPU::ImageComponentType::SFLOAT);
		GPU::ImageSetSize(data.depthBuffer, sys->window.width, sys->window.height);
		if (Settings::ReadBool(Settings::sMSAA)) {
			data.msaaImage = GPU::NewImage(data.device, "msaaImage");
			GPU::ImageSetFormat(data.msaaImage, GPU::ImageBits::B8G8R8A8, GPU::ImageComponentType::SRGB);
			GPU::ImageSetSampleCount(data.msaaImage, 4);
			GPU::ImageSetSampleCount(data.depthBuffer, 4);
			GPU::ImageSetSize(data.msaaImage, sys->window.width, sys->window.height);
			GPU::FramebufferAddImageMultisampled(data.framebuffer, data.msaaImage, data.window);
		} else {
			GPU::FramebufferAddWindow(data.framebuffer, data.window);
		}
		GPU::FramebufferAddImage(data.framebuffer, data.depthBuffer);
	}
	{ // Concurrency, runtime CPU data pools
		uniforms.ambientLight = vec3(0.001f);
		if (data.concurrency < 1) {
			data.concurrency = 1;
		}
		data.drawingContexts.Resize(data.concurrency);
		data.debugVertices.Resize(MAX_DEBUG_VERTICES);
		// TODO: Make this resizeable
		data.objectShaderInfos.Resize(1000000);
	}
	{ // Context
		data.contextGraphics = GPU::NewContext(data.device);
		data.contextTransfer = GPU::NewContext(data.device);
	}
	{ // Texture Samplers
		data.textureSampler = GPU::NewSampler(data.device);
		GPU::SamplerSetAddressMode(data.textureSampler, GPU::AddressMode::REPEAT, GPU::AddressMode::REPEAT);
		GPU::SamplerSetAnisotropy(data.textureSampler, 4);
	}
	{ // Unit square mesh
		data.meshPartUnitSquare = new Assets::MeshPart;
		sys->assets.meshParts.Append(data.meshPartUnitSquare);
		data.meshPartUnitSquare->vertices = {
			{vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f)},
			{vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f)},
			{vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec2(0.0f, 1.0f)},
			{vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 1.0f)},
		};
		data.meshPartUnitSquare->indices = {0, 1, 2, 1, 3, 2};
		data.meshPartUnitSquare->material = Material::Blank();
	}
	Array<Vertex> vertices;
	Array<u32> indices;
	{ // Load 3D assets and make Vertex/Index buffers
		for (auto &part : sys->assets.meshParts) {
			part->indexStart = indices.size;
			indices.Reserve(indices.size+part->indices.size);
			for (i32 i = 0; i < part->indices.size; i++) {
				indices.Append(part->indices[i] + vertices.size);
			}
			vertices.Append(part->vertices);
		}
		data.vertexBuffer = GPU::NewVertexBuffer(data.device);
		GPU::BufferSetSize(data.vertexBuffer, vertices.size * sizeof(Vertex));
		data.indexBuffer = GPU::NewIndexBuffer(data.device, String(), 4);
		GPU::BufferSetSize(data.indexBuffer, indices.size * sizeof(u32));

		data.debugVertexBuffer = GPU::NewVertexBuffer(data.device, "DebugLines Vertex Buffer");
		GPU::BufferSetSize(data.debugVertexBuffer, data.debugVertices.size * sizeof(DebugVertex));
	}
	{ // Load textures/buffers
		data.uniformBuffer = GPU::NewUniformBuffer(data.device);
		GPU::BufferSetSize(data.uniformBuffer, sizeof(UniformBuffer));
		GPU::BufferSetShaderUsage(data.uniformBuffer, (u32)GPU::ShaderStage::VERTEX | (u32)GPU::ShaderStage::FRAGMENT);

		// TODO: Make the following buffers resizable

		data.objectBuffer = GPU::NewStorageBuffer(data.device);
		GPU::BufferSetSize(data.objectBuffer, data.objectShaderInfos.size * sizeof(ObjectShaderInfo));
		GPU::BufferSetShaderUsage(data.objectBuffer, (u32)GPU::ShaderStage::VERTEX | (u32)GPU::ShaderStage::FRAGMENT);

		data.textures.Resize(sys->assets.textures.size);
		for (i32 i = 0; i < sys->assets.textures.size; i++) {
			Image &image = sys->assets.textures[i].image;
			data.textures[i] = GPU::NewImage(data.device, Stringify("texture ", i));
			// TODO: Support HDR images
			GPU::ImageBits imageBits;
			switch (image.channels) {
				case 1: imageBits = GPU::ImageBits::R8; break;
				case 2: imageBits = GPU::ImageBits::R8G8; break;
				case 3: imageBits = GPU::ImageBits::R8G8B8; break;
				case 4: imageBits = GPU::ImageBits::R8G8B8A8; break;
				default:
					error = Stringify("Texture image ", i, " has invalid channel count (", image.channels, ")");
					return false;
			}
			GPU::ImageSetFormat(data.textures[i], imageBits, image.colorSpace == Image::LINEAR ? GPU::ImageComponentType::UNORM : GPU::ImageComponentType::SRGB);
			GPU::ImageSetSize(data.textures[i], image.width, image.height);
			GPU::ImageSetMipmapping(data.textures[i], true);
			GPU::ImageSetShaderUsage(data.textures[i], (u32)GPU::ShaderStage::FRAGMENT);
		}
		data.fontVertexBuffer = GPU::NewVertexBuffer(data.device, "Font Vertex Buffer");
		data.fontImages.Resize(sys->assets.fonts.size);
	}
	{ // Pipelines
		GPU::Shader *debugLinesVert = GPU::NewShader(data.device, "data/Az3D/shaders/DebugLines.vert.spv", GPU::ShaderStage::VERTEX);
		GPU::Shader *debugLinesFrag = GPU::NewShader(data.device, "data/Az3D/shaders/DebugLines.frag.spv", GPU::ShaderStage::FRAGMENT);
		GPU::Shader *basic3DVert = GPU::NewShader(data.device, "data/Az3D/shaders/Basic3D.vert.spv", GPU::ShaderStage::VERTEX);
		GPU::Shader *basic3DFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Basic3D.frag.spv", GPU::ShaderStage::FRAGMENT);
		GPU::Shader *font3DFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Font3D.frag.spv", GPU::ShaderStage::FRAGMENT);

		data.pipelines.Resize(PIPELINE_COUNT);

		ArrayWithBucket<GPU::ShaderValueType, 8> vertexInputs = {
			GPU::ShaderValueType::VEC3, // pos
			GPU::ShaderValueType::VEC3, // normal
			GPU::ShaderValueType::VEC3, // tangent
			GPU::ShaderValueType::VEC2, // tex
		};

		data.pipelines[PIPELINE_DEBUG_LINES] = GPU::NewGraphicsPipeline(data.device, "Debug Lines Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_DEBUG_LINES], {debugLinesVert, debugLinesFrag});
		GPU::PipelineAddVertexInputs(data.pipelines[PIPELINE_DEBUG_LINES], {
			GPU::ShaderValueType::VEC3, // pos
			GPU::ShaderValueType::VEC4, // color
		});
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_DEBUG_LINES], GPU::Topology::LINE_LIST);
		GPU::PipelineSetLineWidth(data.pipelines[PIPELINE_DEBUG_LINES], 2.0f);
		GPU::PipelineSetDepthTest(data.pipelines[PIPELINE_DEBUG_LINES], true);
		GPU::PipelineSetDepthCompareOp(data.pipelines[PIPELINE_DEBUG_LINES], GPU::CompareOp::GREATER);
		
		
		data.pipelines[PIPELINE_BASIC_3D] = GPU::NewGraphicsPipeline(data.device, "Basic 3D Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_BASIC_3D], {basic3DVert, basic3DFrag});
		GPU::PipelineAddVertexInputs(data.pipelines[PIPELINE_BASIC_3D], vertexInputs);
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_BASIC_3D], GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetMultisampleShading(data.pipelines[PIPELINE_BASIC_3D], true);
		GPU::PipelineSetDepthTest(data.pipelines[PIPELINE_BASIC_3D], true);
		GPU::PipelineSetDepthWrite(data.pipelines[PIPELINE_BASIC_3D], true);
		GPU::PipelineSetDepthCompareOp(data.pipelines[PIPELINE_BASIC_3D], GPU::CompareOp::GREATER);
		GPU::PipelineSetCullingMode(data.pipelines[PIPELINE_BASIC_3D], GPU::CullingMode::BACK);
		GPU::PipelineSetWinding(data.pipelines[PIPELINE_BASIC_3D], GPU::Winding::COUNTER_CLOCKWISE);
		
		data.pipelines[PIPELINE_FOLIAGE_3D] = GPU::NewGraphicsPipeline(data.device, "Foliage 3D Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_FOLIAGE_3D], {basic3DVert, basic3DFrag});
		GPU::PipelineAddVertexInputs(data.pipelines[PIPELINE_FOLIAGE_3D], vertexInputs);
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_FOLIAGE_3D], GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetMultisampleShading(data.pipelines[PIPELINE_FOLIAGE_3D], true);
		GPU::PipelineSetDepthTest(data.pipelines[PIPELINE_FOLIAGE_3D], true);
		GPU::PipelineSetDepthWrite(data.pipelines[PIPELINE_FOLIAGE_3D], true);
		GPU::PipelineSetDepthCompareOp(data.pipelines[PIPELINE_FOLIAGE_3D], GPU::CompareOp::GREATER);
		GPU::PipelineSetCullingMode(data.pipelines[PIPELINE_FOLIAGE_3D], GPU::CullingMode::NONE);
		GPU::PipelineSetWinding(data.pipelines[PIPELINE_FOLIAGE_3D], GPU::Winding::COUNTER_CLOCKWISE);

		data.pipelines[PIPELINE_FONT_3D] = GPU::NewGraphicsPipeline(data.device, "Font 3D Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_FONT_3D], {basic3DVert, font3DFrag});

		for (i32 i = 1; i < PIPELINE_COUNT; i++) {
			GPU::PipelineSetBlendMode(data.pipelines[i], {GPU::BlendMode::TRANSPARENT, true});
		}
	}
	{ // Shadow maps
		// data.shadowMapMemory = data.device->AddMemory();
		// data.shadowMapImage = data.shadowMapMemory->AddImage();
		// data.shadowMapImage->aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
		// data.shadowMapImage->format = VK_FORMAT_D32_SFLOAT;
		// data.shadowMapImage->usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		// data.shadowMapImage->width = data.shadowMapImage->height = 2048;
		// data.renderPassShadowMaps = data.device->AddRenderPass();
		// data.renderPassShadowMaps->initialAccessStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		// data.renderPassShadowMaps->finalAccessStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		// auto attachment = data.renderPassShadowMaps->AddAttachment();
		// attachment->bufferDepthStencil = true;
		// attachment->clearDepth = true;
		// attachment->clearDepthStencilValue.depth = 0.0f;
		// attachment->formatDepthStencil = data.shadowMapImage->format;
		// attachment->keepDepth = true;
		// auto subpass = data.renderPassShadowMaps->AddSubpass();
		// subpass->UseAttachment(attachment, vk::ATTACHMENT_DEPTH_STENCIL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
		// data.framebufferShadowMaps = data.device->AddFramebuffer();
		// data.framebufferShadowMaps->renderPass = data.renderPassShadowMaps;
		// data.framebufferShadowMaps->attachmentImages = {{data.shadowMapImage}};
		// data.framebufferShadowMaps->ownMemory = false;
		// data.framebufferShadowMaps->ownImages = false;
		// data.framebufferShadowMaps->depthMemory = data.shadowMapMemory.RawPtr();
		// data.framebufferShadowMaps->width = data.shadowMapImage->width;
		// data.framebufferShadowMaps->height = data.shadowMapImage->height;
		// data.semaphoreShadowMapsComplete = data.device->AddSemaphore();
		// data.queueSubmissionShadowMaps = data.device->AddQueueSubmission();
		// data.pipelineShadowMaps = data.device->AddPipeline();
		// data.pipelineShadowMaps->renderPass = data.renderPassShadowMaps;
		// data.pipelineShadowMaps->subpass = 0;
		// data.pipelineShadowMaps->inputAssembly = data.pipelines[PIPELINE_BASIC_3D]->inputAssembly;
		// data.pipelineShadowMaps->inputAttributeDescriptions = data.pipelines[PIPELINE_BASIC_3D]->inputAttributeDescriptions;
		// data.pipelineShadowMaps->inputBindingDescriptions = data.pipelines[PIPELINE_BASIC_3D]->inputBindingDescriptions;
		// data.pipelineShadowMaps->rasterizer = data.pipelines[PIPELINE_BASIC_3D]->rasterizer;
		// data.pipelineShadowMaps->rasterizer.rasterizerDiscardEnable = VK_TRUE;

		// auto shadersShadow = data.device->AddShaders(2);
		// shadersShadow[0].filename = "data/Az3D/shaders/ShadowMap.vert.spv";
		// shadersShadow[1].filename = "data/Az3D/shaders/ShadowMap.frag.spv";
		// vk::ShaderRef vertShadow = vk::ShaderRef(shadersShadow.GetPtr(0), VK_SHADER_STAGE_VERTEX_BIT);
		// vk::ShaderRef fragShadow = vk::ShaderRef(shadersShadow.GetPtr(1), VK_SHADER_STAGE_FRAGMENT_BIT);

		// data.pipelineShadowMaps->shaders = {vertShadow, fragShadow};
		// data.pipelineShadowMaps->depthStencil.depthTestEnable = VK_TRUE;
		// data.pipelineShadowMaps->depthStencil.depthWriteEnable = VK_TRUE;
		// data.pipelineShadowMaps->depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;
		// data.pipelineShadowMaps->descriptorLayouts = {descriptorLayout};
	}

	if (auto result = GPU::Initialize(); result.isError) {
		error = "Failed to init GPU: " + result.error;
		return false;
	}

	uniforms.lights[0].position = vec4(vec3(0.0f), 1.0f);
	uniforms.lights[0].color = vec3(0.0f);
	uniforms.lights[0].direction = vec3(0.0f, 0.0f, 1.0f);
	uniforms.lights[0].angleMin = 0.0f;
	uniforms.lights[0].angleMax = 0.0f;
	uniforms.lights[0].distMin = 0.0f;
	uniforms.lights[0].distMax = 0.0f;

	GPU::ContextBeginRecording(data.contextTransfer).AzUnwrap();
	GPU::CmdCopyDataToBuffer(data.contextTransfer, data.vertexBuffer, vertices.data).AzUnwrap();
	GPU::CmdCopyDataToBuffer(data.contextTransfer, data.indexBuffer, indices.data).AzUnwrap();
	for (i32 i = 0; i < sys->assets.textures.size; i++) {
		if (auto result = GPU::CmdCopyDataToImage(data.contextTransfer, data.textures[i], sys->assets.textures[i].image.pixels); result.isError) {
			error = Stringify("Failed to copy data to texture ", i, result.error);
			return false;
		}
	}

	GPU::ContextEndRecording(data.contextTransfer).AzUnwrap();
	GPU::SubmitCommands(data.contextTransfer).AzUnwrap();
	GPU::ContextWaitUntilFinished(data.contextTransfer).AzUnwrap();

	if (!UpdateFonts()) {
		error = "Failed to update fonts: " + error;
		return false;
	}
	UpdateBackground();

	return true;
}

bool Manager::Deinit() {
	GPU::Deinitialize();
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

void Manager::UpdateLights() {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::UpdateLights)
	const f32 minClip = camera.nearClip;
	const f32 maxClip = camera.farClip;
	mat4 invView = uniforms.view.Inverse();
	// frustum corners
	vec3 corners[8] = {
		(vec4(-1.0f, -1.0f, minClip, 1.0f) * invView).xyz,
		(vec4( 1.0f, -1.0f, minClip, 1.0f) * invView).xyz,
		(vec4(-1.0f,  1.0f, minClip, 1.0f) * invView).xyz,
		(vec4( 1.0f,  1.0f, minClip, 1.0f) * invView).xyz,
		(vec4(-1.0f, -1.0f, maxClip, 1.0f) * invView).xyz,
		(vec4( 1.0f, -1.0f, maxClip, 1.0f) * invView).xyz,
		(vec4(-1.0f,  1.0f, maxClip, 1.0f) * invView).xyz,
		(vec4( 1.0f,  1.0f, maxClip, 1.0f) * invView).xyz
	};
	vec3 center = (vec4(0.0f, 0.0f, (minClip+maxClip) * 0.5, 1.0f) * invView).xyz;
	vec3 boundsMin(100000000.0f), boundsMax(-100000000.0f);
	uniforms.sun = mat4::Camera(center + uniforms.sunDir*100.0f, -uniforms.sunDir, camera.up);
	for (i32 i = 0; i < 8; i++) {
		corners[i] = (vec4(corners[i], 1.0f) * uniforms.sun).xyz;
		boundsMin.x = min(boundsMin.x, corners[i].x);
		boundsMin.y = min(boundsMin.y, corners[i].y);
		boundsMin.z = min(boundsMin.z, corners[i].z);
		boundsMax.x = max(boundsMax.x, corners[i].x);
		boundsMax.y = max(boundsMax.y, corners[i].y);
		boundsMax.z = max(boundsMax.z, corners[i].z);
	}
	vec3 dimensions = boundsMax - boundsMin;
	uniforms.sun = mat4::Scaler(vec4(dimensions, 1.0f)) * uniforms.sun;
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
}

bool Manager::UpdateFonts() {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::UpdateFonts)
	/*
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
*/
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

bool Manager::UpdateUniforms(GPU::Context *context) {
	// Update camera matrix
	uniforms.view = mat4::Camera(camera.pos, camera.forward, camera.up);
	uniforms.proj = mat4::Perspective(camera.fov, screenSize.x / screenSize.y, camera.nearClip, camera.farClip);
	uniforms.viewProj = uniforms.view * uniforms.proj;
	uniforms.eyePos = camera.pos;
	UpdateLights();

	GPU::CmdCopyDataToBuffer(context, data.uniformBuffer, &uniforms).AzUnwrap();

	return true;
}

bool Manager::UpdateObjects(GPU::Context *context) {
	i64 copySize = min(data.objectShaderInfos.size, 1000000) * sizeof(ObjectShaderInfo);
	if (copySize) {
		GPU::CmdCopyDataToBuffer(context, data.objectBuffer, data.objectShaderInfos.data, 0, copySize).AzUnwrap();
	}

	return true;
}

bool Manager::UpdateDebugLines(GPU::Context *context) {
	data.debugVertices.ClearSoft();
	for (DrawingContext &context : data.drawingContexts) {
		data.debugVertices.Append(context.debugLines);
	}
	if (data.debugVertices.size < 2) return true;

	BindPipeline(data.contextGraphics, PIPELINE_DEBUG_LINES);

	GPU::CmdDraw(data.contextGraphics, data.debugVertices.size, 0);

	GPU::CmdCopyDataToBuffer(context, data.debugVertexBuffer, data.debugVertices.data, 0, min(data.debugVertices.size, MAX_DEBUG_VERTICES) * sizeof(DebugVertex)).AzUnwrap();
	return true;
}

bool Manager::Draw() {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::Draw)
	// if (vk::hadValidationError) {
	// 	error = "Quitting due to vulkan validation error.";
	// 	return false;
	// }
	// if (sys->window.resized || data.resized || data.zeroExtent) {
	// 	AZ3D_PROFILING_EXCEPTION_START();
	// 	GPU::
	// 	AZ3D_PROFILING_EXCEPTION_END();
	// 	data.swapchain->UpdateSurfaceCapabilities();
	// 	VkExtent2D extent = data.swapchain->data.surfaceCapabilities.currentExtent;
	// 	if (extent.width == 0 || extent.height == 0) {
	// 		data.zeroExtent = true;
	// 		return true;
	// 	}
	// 	data.zeroExtent = false;
	// 	if (!data.swapchain->Resize()) {
	// 		error = "Failed to resize swapchain: " + vk::error;
	// 		return false;
	// 	}
	// 	data.resized = false;
	// }
	// if (Settings::ReadBool(Settings::sVSync) != data.swapchain->vsync) {
	// 	AZ3D_PROFILING_EXCEPTION_START();
	// 	vk::DeviceWaitIdle(data.device);
	// 	AZ3D_PROFILING_EXCEPTION_END();
	// 	data.swapchain->vsync = Settings::ReadBool(Settings::sVSync);
	// 	if (!data.swapchain->Reconfigure()) {
	// 		error = "Failed to set VSync: " + vk::error;
	// 		return false;
	// 	}
	// }

	// bool updateFontMemory = false;
	// for (i32 i = 0; i < sys->assets.fonts.size; i++) {
	// 	Assets::Font& font = sys->assets.fonts[i];
	// 	if (font.fontBuilder.indicesToAdd.size != 0) {
	// 		font.fontBuilder.Build();
	// 		updateFontMemory = true;
	// 	}
	// }
	// if (updateFontMemory) {
	// 	AZ3D_PROFILING_EXCEPTION_START();
	// 	GPU::ContextWaitUntilFinished(data.contextGraphics).AzUnwrap();
	// 	AZ3D_PROFILING_EXCEPTION_END();
	// 	if (!UpdateFonts()) {
	// 		return false;
	// 	}
	// }

	GPU::SetVSync(data.window, Settings::ReadBool(Settings::sVSync));
	static Az3D::Profiling::AString sWindowUpdate("GPU::WindowUpdate");
	Az3D::Profiling::Timer timerWindowUpdate(sWindowUpdate);
	timerWindowUpdate.Start();
	AZ3D_PROFILING_EXCEPTION_START();
	if (auto result = GPU::WindowUpdate(data.window); result.isError) {
		error = "Failed to update GPU window: " + result.error;
		return false;
	}
	timerWindowUpdate.End();
	AZ3D_PROFILING_EXCEPTION_END();

	// data.buffer = !data.buffer;

	screenSize = vec2((f32)sys->window.width, (f32)sys->window.height);
	aspectRatio = screenSize.y / screenSize.x;

	GPU::ContextBeginRecording(data.contextGraphics).AzUnwrap();
	GPU::CmdBindFramebuffer(data.contextGraphics, data.framebuffer);
	GPU::CmdSetViewportAndScissor(data.contextGraphics, (f32)sys->window.width, (f32)sys->window.height);
	GPU::CmdBindIndexBuffer(data.contextGraphics, data.indexBuffer);
	GPU::CmdBindUniformBuffer(data.contextGraphics, data.uniformBuffer, 0, 0);
	GPU::CmdBindStorageBuffer(data.contextGraphics, data.objectBuffer, 0, 1);
	GPU::CmdBindImageArraySampler(data.contextGraphics, data.textures, data.textureSampler, 0, 2);
	GPU::CmdCommitBindings(data.contextGraphics).AzUnwrap();
	/*{ // Fade
		DrawQuadSS(commandBuffersSecondary[0], texBlank, vec4(backgroundRGB, 0.2f), vec2(-1.0), vec2(2.0), vec2(1.0));
	}*/
	{ // Clear
		GPU::CmdClearColorAttachment(data.contextGraphics, vec4(sRGBToLinear(backgroundRGB), 1.0f));
		GPU::CmdClearDepthAttachment(data.contextGraphics, 0.0f);
	}
	// Clear lights so we get new ones this frame
	lights.size = 0;

	for (DrawingContext &context : data.drawingContexts) {
		context.thingsToDraw.ClearSoft();
		context.debugLines.ClearSoft();
	}

	sys->Draw(data.drawingContexts);
	
	GPU::ContextBeginRecording(data.contextTransfer).AzUnwrap();
	if (!UpdateUniforms(data.contextTransfer)) return false;

	data.objectShaderInfos.ClearSoft();
	{ // Sorting draw calls
		Array<DrawCallInfo> allDrawCalls;
		for (DrawingContext &context : data.drawingContexts) {
			allDrawCalls.Append(context.thingsToDraw);
		}
		// Do frustum-based culling
		Frustum frustum = GetFrustumFromCamera(camera, screenSize.y / screenSize.x);
		for (DrawCallInfo &drawCall : allDrawCalls) {
			drawCall.culled = !IsSphereInFrustum(drawCall.boundingSphereCenter, drawCall.boundingSphereRadius, frustum);
			#if 0
			DrawDebugSphere(data.drawingContexts[0], drawCall.boundingSphereCenter, drawCall.boundingSphereRadius*2.0f, drawCall.culled ? vec4(1.0f, 0.0f, 0.0f, 1.0f) : vec4(1.0f));
			#endif
		}
		QuickSort(allDrawCalls, [](const DrawCallInfo &lhs, const DrawCallInfo &rhs) -> bool {
			// Place culled objects at the end
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
				BindPipeline(data.contextGraphics, drawCall.pipeline);
				currentPipeline = drawCall.pipeline;
			}
			drawCall.instanceStart = data.objectShaderInfos.size;
			i32 prevSize = data.objectShaderInfos.size;
			data.objectShaderInfos.Resize(data.objectShaderInfos.size + drawCall.transforms.size);
			for (i32 i = 0; i < drawCall.transforms.size; i++) {
				data.objectShaderInfos[prevSize+i] = ObjectShaderInfo{drawCall.transforms[i], drawCall.material};
			}
			GPU::CmdDrawIndexed(data.contextGraphics, drawCall.indexCount, drawCall.indexStart, 0, drawCall.instanceCount, drawCall.instanceStart);
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


	if (!UpdateObjects(data.contextTransfer)) return false;
	if (!UpdateDebugLines(data.contextTransfer)) return false;
	GPU::ContextEndRecording(data.contextTransfer).AzUnwrap();
	{
		// Because the first frame won't have a signaled semaphore, only wait for it if we're not on the first frame.
		ArrayWithBucket<GPU::Semaphore*, 4> waitSemaphores;
		static bool once = true;
		if (once) {
			once = false;
		} else {
			waitSemaphores.Append(GPU::ContextGetPreviousSemaphore(data.contextGraphics, 1));
		}
		if (auto result = GPU::SubmitCommands(data.contextTransfer, 1, waitSemaphores); result.isError) {
			error = "Failed to submit transfer commands: " + result.error;
			return false;
		}
	}

	GPU::ContextEndRecording(data.contextGraphics).AzUnwrap();

	if (auto result = GPU::SubmitCommands(data.contextGraphics, 2, {GPU::ContextGetCurrentSemaphore(data.contextTransfer)}); result.isError) {
		error = "Failed to SubmitCommandBuffers: " + result.error;
		return false;
	}
	return true;
}

bool Manager::Present() {
	AZ3D_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::Present)
	if (auto result = GPU::WindowPresent(data.window, {GPU::ContextGetCurrentSemaphore(data.contextGraphics)}); result.isError) {
		error = "Failed to present: " + result.error;
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

void DrawDebugSphere(DrawingContext &context, vec3 center, f32 radius, vec4 color) {
	f32 angleDelta = tau / 32.0f;
	for (f32 angle = 0.0f; angle < tau; angle += angleDelta) {
		f32 x1 = sin(angle) * radius;
		f32 y1 = cos(angle) * radius;
		f32 x2 = sin(angle+angleDelta) * radius;
		f32 y2 = cos(angle+angleDelta) * radius;
		DrawDebugLine(context, {center + vec3(x1, y1, 0.0f), color}, {center + vec3(x2, y2, 0.0f), color});
		DrawDebugLine(context, {center + vec3(x1, 0.0f, y1), color}, {center + vec3(x2, 0.0f, y2), color});
		DrawDebugLine(context, {center + vec3(0.0f, x1, y1), color}, {center + vec3(0.0f, x2, y2), color});
	}
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

void DrawMeshPart(DrawingContext &context, Assets::MeshPart *meshPart, const ArrayWithBucket<mat4, 1> &transforms, bool opaque, bool castsShadows) {
	DrawCallInfo draw;
	draw.transforms = transforms;
	draw.boundingSphereCenter = vec3(0.0f);
	for (i32 i = 0; i < transforms.size; i++) {
		draw.boundingSphereCenter += transforms[i].Row4().xyz;
	}
	draw.boundingSphereCenter /= (f32)transforms.size;
	draw.boundingSphereRadius = 0.0f;
	for (i32 i = 0; i < transforms.size; i++) {
		f32 myRadius = meshPart->boundingSphereRadius * sqrt(max(
			normSqr(transforms[i].Row1().xyz),
			normSqr(transforms[i].Row2().xyz),
			normSqr(transforms[i].Row3().xyz)
		));
		myRadius += norm(draw.boundingSphereCenter - transforms[i].Row4().xyz);
		if (myRadius > draw.boundingSphereRadius) {
			draw.boundingSphereRadius = myRadius;
		}
	}
	draw.depth = dot(sys->rendering.camera.forward, draw.boundingSphereCenter - sys->rendering.camera.pos);
	draw.indexStart = meshPart->indexStart;
	draw.indexCount = meshPart->indices.size;
	draw.instanceCount = transforms.size;
	draw.material = meshPart->material;
	draw.pipeline = meshPart->material.isFoliage ? PIPELINE_FOLIAGE_3D : PIPELINE_BASIC_3D;
	draw.opaque = opaque;
	draw.castsShadows = castsShadows;
	draw.culled = false;
	// We don't need synchronization because each thread gets their own array.
	context.thingsToDraw.Append(draw);
}

void DrawMesh(DrawingContext &context, Assets::Mesh mesh, const ArrayWithBucket<mat4, 1> &transforms, bool opaque, bool castsShadows) {
	for (Assets::MeshPart *meshPart : mesh.parts) {
		DrawMeshPart(context, meshPart, transforms, opaque && meshPart->material.color.a == 1.0f, castsShadows && meshPart->material.color.a >= 0.5f);
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
