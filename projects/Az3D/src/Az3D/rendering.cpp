/*
	File: rendering.cpp
	Author: Philip Haynes
*/

#include "rendering.hpp"
#include "game_systems.hpp"
#include "settings.hpp"
#include "assets.hpp"
#include "AzCore/Profiling.hpp"

#include "AzCore/IO/Log.hpp"
#include "AzCore/io.hpp"
#include "AzCore/font.hpp"
#include "AzCore/QuickSort.hpp"

namespace Az3D::Rendering {

constexpr i32 MAX_DEBUG_VERTICES = 8192;

i32 numNewtonIterations = 10;
i32 numBinarySearchIterations = 50;

using GameSystems::sys;

io::Log cout("rendering.log");

String error = "No error.";

Plane GetPlaneFromRay(vec3 point, vec3 normal) {
	Plane result;
	result.normal = normal;
	result.dist = dot(point, normal);
	return result;
}

void GetBasisFromCamera(const Camera &camera, vec3 &dstForward, vec3 &dstRight, vec3 &dstUp) {
	dstForward = normalize(camera.forward);
	dstRight = normalize(cross(camera.up, dstForward));
	dstUp = normalize(cross(dstForward, dstRight));
}

Frustum GetFrustumFromCamera(const Camera &camera, f32 heightOverWidth) {
	Frustum result;
	f32 tanhFOV = tan(Radians32(camera.fov).value() * 0.5f);
	f32 tanvFOV = tanhFOV * heightOverWidth;
	vec3 forward, right, up;
	GetBasisFromCamera(camera, forward, right, up);
	result.near  = GetPlaneFromRay(camera.pos + camera.nearClip * camera.forward, camera.forward);
	result.far   = GetPlaneFromRay(camera.pos + camera.farClip * camera.forward, -camera.forward);
	result.left  = GetPlaneFromRay(camera.pos, normalize(right + forward * tanhFOV));
	result.right = GetPlaneFromRay(camera.pos, normalize(-right + forward * tanhFOV));
	result.top    = GetPlaneFromRay(camera.pos, normalize(-up + forward * tanvFOV));
	result.bottom = GetPlaneFromRay(camera.pos, normalize(up + forward * tanvFOV));
	return result;
}

// pos is the center of the near plane, pos+forward is the center of the far plane
// right and up are half of the width and height of the frustum such that pos+right is on the right plane, etc.
Frustum GetOrtho(vec3 pos, vec3 forward, vec3 right, vec3 up) {
	Frustum result;
	vec3 forwardNormal = normalize(forward);
	vec3 rightNormal = normalize(right);
	// What's upNormal?
	// Nothing, what's up wit u?
	vec3 upNormal = normalize(up);
	result.near = GetPlaneFromRay(pos, forwardNormal);
	result.far = GetPlaneFromRay(pos + forward, -forwardNormal);
	result.left = GetPlaneFromRay(pos - right, rightNormal);
	result.right = GetPlaneFromRay(pos + right, -rightNormal);
	result.top = GetPlaneFromRay(pos - up, upNormal);
	result.bottom = GetPlaneFromRay(pos + up, -upNormal);
	return result;
}

bool IsSphereAbovePlane(vec3 center, f32 radius, const Plane &plane) {
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
		case PIPELINE_DEBUG_LINES:
			GPU::CmdBindVertexBuffer(context, r.data.debugVertexBuffer);
			break;
		case PIPELINE_BASIC_3D:
		case PIPELINE_BASIC_3D_VSM:
		case PIPELINE_FOLIAGE_3D:
			GPU::CmdBindVertexBuffer(context, r.data.vertexBuffer);
			break;
		case PIPELINE_FONT_3D:
		case PIPELINE_FONT_3D_VSM:
			GPU::CmdBindUniformBufferArray(context, r.data.fontBuffers, 0, 5);
			GPU::CmdBindStorageBuffer(context, r.data.textBuffer, 0, 6);
			break;
	}
	GPU::CmdCommitBindings(context).AzUnwrap();
}

bool Manager::Init() {
	AZCORE_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::Init)

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
		worldInfo.ambientLight = vec3(0.001f);
		if (data.concurrency < 1) {
			data.concurrency = 1;
		}
		data.drawingContexts.Resize(data.concurrency);
		data.debugVertices.Resize(MAX_DEBUG_VERTICES);
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
		data.meshPartUnitSquare->material = Assets::Material::Blank();
	}
	// We'll be loading all the textures and meshes at once, so wait until they're loaded.
	AZCORE_PROFILING_EXCEPTION_START()
	sys->assets.fileManager.WaitUntilDone();
	AZCORE_PROFILING_EXCEPTION_END()
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
		GPU::BufferSetSize(data.vertexBuffer, vertices.size * sizeof(Vertex)).AzUnwrap();
		data.indexBuffer = GPU::NewIndexBuffer(data.device, String(), 4);
		GPU::BufferSetSize(data.indexBuffer, indices.size * sizeof(u32)).AzUnwrap();

		data.debugVertexBuffer = GPU::NewVertexBuffer(data.device, "DebugLines Vertex Buffer");
		GPU::BufferSetSize(data.debugVertexBuffer, data.debugVertices.size * sizeof(DebugVertex)).AzUnwrap();
	}
	{ // Load textures/buffers
		data.worldInfoBuffer = GPU::NewUniformBuffer(data.device);
		GPU::BufferSetSize(data.worldInfoBuffer, sizeof(WorldInfoBuffer)).AzUnwrap();
		GPU::BufferSetShaderUsage(data.worldInfoBuffer, (u32)GPU::ShaderStage::VERTEX | (u32)GPU::ShaderStage::FRAGMENT);

		data.objectShaderInfos.Resize(1000);
		data.objectBuffer = GPU::NewStorageBuffer(data.device);
		GPU::BufferSetSize(data.objectBuffer, data.objectShaderInfos.size * sizeof(ObjectShaderInfo)).AzUnwrap();
		GPU::BufferSetShaderUsage(data.objectBuffer, (u32)GPU::ShaderStage::VERTEX | (u32)GPU::ShaderStage::FRAGMENT);

		data.bones.Resize(100, mat4(1.0f));
		data.bonesBuffer = GPU::NewStorageBuffer(data.device);
		GPU::BufferSetSize(data.bonesBuffer, data.bones.size * sizeof(mat4)).AzUnwrap();
		GPU::BufferSetShaderUsage(data.bonesBuffer, (u32)GPU::ShaderStage::VERTEX);

		data.textShaderInfos.Resize(100);
		data.textBuffer = GPU::NewStorageBuffer(data.device);
		GPU::BufferSetSize(data.textBuffer, data.textShaderInfos.size * sizeof(TextShaderInfo)).AzUnwrap();
		GPU::BufferSetShaderUsage(data.textBuffer, (u32)GPU::ShaderStage::VERTEX);

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
	}
	ArrayWithBucket<GPU::ShaderValueType, 8> vertexInputs = {
		GPU::ShaderValueType::VEC3, // pos
		GPU::ShaderValueType::VEC3, // normal
		GPU::ShaderValueType::VEC3, // tangent
		GPU::ShaderValueType::VEC2, // tex
		GPU::ShaderValueType::U32,  // boneIDs (4 u8s packed into a u32)
		GPU::ShaderValueType::U32,  // boneWeights (4 u8s packed into a u32)
	};
	{ // Pipelines
		GPU::Shader *debugLinesVert = GPU::NewShader(data.device, "data/Az3D/shaders/DebugLines.vert.spv", GPU::ShaderStage::VERTEX);
		GPU::Shader *debugLinesFrag = GPU::NewShader(data.device, "data/Az3D/shaders/DebugLines.frag.spv", GPU::ShaderStage::FRAGMENT);
		GPU::Shader *basic3DVert = GPU::NewShader(data.device, "data/Az3D/shaders/Basic3D.vert.spv", GPU::ShaderStage::VERTEX);
		GPU::Shader *basic3DFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Basic3D.frag.spv", GPU::ShaderStage::FRAGMENT);
		GPU::Shader *font3DVert = GPU::NewShader(data.device, "data/Az3D/shaders/Font3D.vert.spv", GPU::ShaderStage::VERTEX);
		GPU::Shader *font3DFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Font3D.frag.spv", GPU::ShaderStage::FRAGMENT);

		data.pipelines.Resize(PIPELINE_COUNT);

		data.pipelines[PIPELINE_DEBUG_LINES] = GPU::NewGraphicsPipeline(data.device, "Debug Lines Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_DEBUG_LINES], {debugLinesVert, debugLinesFrag});
		GPU::PipelineAddVertexInputs(data.pipelines[PIPELINE_DEBUG_LINES], {
			GPU::ShaderValueType::VEC3, // pos
			GPU::ShaderValueType::VEC4, // color
		});
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_DEBUG_LINES], GPU::Topology::LINE_LIST);
		GPU::PipelineSetLineWidth(data.pipelines[PIPELINE_DEBUG_LINES], 2.0f);
		GPU::PipelineSetDepthTest(data.pipelines[PIPELINE_DEBUG_LINES], Settings::ReadBool(Settings::sDebugLinesDepthTest));
		GPU::PipelineSetDepthCompareOp(data.pipelines[PIPELINE_DEBUG_LINES], GPU::CompareOp::LESS);


		data.pipelines[PIPELINE_BASIC_3D] = GPU::NewGraphicsPipeline(data.device, "Basic 3D Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_BASIC_3D], {basic3DVert, basic3DFrag});
		GPU::PipelineAddVertexInputs(data.pipelines[PIPELINE_BASIC_3D], vertexInputs);
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_BASIC_3D], GPU::Topology::TRIANGLE_LIST);
		// GPU::PipelineSetMultisampleShading(data.pipelines[PIPELINE_BASIC_3D], true);
		GPU::PipelineSetDepthTest(data.pipelines[PIPELINE_BASIC_3D], true);
		GPU::PipelineSetDepthWrite(data.pipelines[PIPELINE_BASIC_3D], true);
		GPU::PipelineSetDepthCompareOp(data.pipelines[PIPELINE_BASIC_3D], GPU::CompareOp::LESS);
		GPU::PipelineSetCullingMode(data.pipelines[PIPELINE_BASIC_3D], GPU::CullingMode::BACK);
		GPU::PipelineSetWinding(data.pipelines[PIPELINE_BASIC_3D], GPU::Winding::COUNTER_CLOCKWISE);

		data.pipelines[PIPELINE_FOLIAGE_3D] = GPU::NewGraphicsPipeline(data.device, "Foliage 3D Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_FOLIAGE_3D], {basic3DVert, basic3DFrag});
		GPU::PipelineAddVertexInputs(data.pipelines[PIPELINE_FOLIAGE_3D], vertexInputs);
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_FOLIAGE_3D], GPU::Topology::TRIANGLE_LIST);
		// GPU::PipelineSetMultisampleShading(data.pipelines[PIPELINE_FOLIAGE_3D], true);
		GPU::PipelineSetDepthTest(data.pipelines[PIPELINE_FOLIAGE_3D], true);
		GPU::PipelineSetDepthWrite(data.pipelines[PIPELINE_FOLIAGE_3D], true);
		GPU::PipelineSetDepthCompareOp(data.pipelines[PIPELINE_FOLIAGE_3D], GPU::CompareOp::LESS);
		GPU::PipelineSetCullingMode(data.pipelines[PIPELINE_FOLIAGE_3D], GPU::CullingMode::NONE);
		GPU::PipelineSetWinding(data.pipelines[PIPELINE_FOLIAGE_3D], GPU::Winding::COUNTER_CLOCKWISE);

		data.pipelines[PIPELINE_FONT_3D] = GPU::NewGraphicsPipeline(data.device, "Font 3D Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_FONT_3D], {font3DVert, font3DFrag});
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_FONT_3D], GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetDepthTest(data.pipelines[PIPELINE_FONT_3D], true);
		GPU::PipelineSetDepthWrite(data.pipelines[PIPELINE_FONT_3D], false);
		GPU::PipelineSetDepthCompareOp(data.pipelines[PIPELINE_FONT_3D], GPU::CompareOp::LESS);
		GPU::PipelineSetCullingMode(data.pipelines[PIPELINE_FONT_3D], GPU::CullingMode::NONE);
		GPU::PipelineSetMultisampleShading(data.pipelines[PIPELINE_FONT_3D], true);

		for (i32 i = 1; i < PIPELINE_COUNT; i++) {
			if (i == PIPELINE_BASIC_3D_VSM || i == PIPELINE_FONT_3D_VSM) continue;
			GPU::PipelineSetBlendMode(data.pipelines[i], {GPU::BlendMode::TRANSPARENT, true});
		}
	}
	{ // Shadow maps
		constexpr i32 dims = 1024;
		data.contextShadowMap = GPU::NewContext(data.device, "VSM Context");

		GPU::Image *shadowMapMSAAImage = GPU::NewImage(data.device, "VSM MSAA Image");
		data.shadowMapImage = GPU::NewImage(data.device, "VSM Image");
		GPU::ImageSetFormat(shadowMapMSAAImage, GPU::ImageBits::R32G32, GPU::ImageComponentType::SFLOAT);
		GPU::ImageSetSize(shadowMapMSAAImage, dims, dims);
		GPU::ImageSetSampleCount(shadowMapMSAAImage, 4);

		GPU::ImageSetFormat(data.shadowMapImage, GPU::ImageBits::R32G32, GPU::ImageComponentType::SFLOAT);
		GPU::ImageSetSize(data.shadowMapImage, dims, dims);
		GPU::ImageSetShaderUsage(data.shadowMapImage, (u32)GPU::ShaderStage::FRAGMENT);
		GPU::ImageSetMipmapping(data.shadowMapImage, true);

		data.framebufferShadowMaps = GPU::NewFramebuffer(data.device, "VSM Framebuffer");
		GPU::FramebufferAddImageMultisampled(data.framebufferShadowMaps, shadowMapMSAAImage, data.shadowMapImage);

		GPU::Shader *vsmVert = GPU::NewShader(data.device, "data/Az3D/shaders/Basic3D_VSM.vert.spv", GPU::ShaderStage::VERTEX, "VSM Vertex Shader");
		GPU::Shader *vsmFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Basic3D_VSM.frag.spv", GPU::ShaderStage::FRAGMENT, "VSM Fragment Shader");
		data.pipelines[PIPELINE_BASIC_3D_VSM] = GPU::NewGraphicsPipeline(data.device, "VSM Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_BASIC_3D_VSM], {vsmVert, vsmFrag});
		GPU::PipelineAddVertexInputs(data.pipelines[PIPELINE_BASIC_3D_VSM], vertexInputs);
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_BASIC_3D_VSM], GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetBlendMode(data.pipelines[PIPELINE_BASIC_3D_VSM], GPU::BlendMode::MAX);

		GPU::Shader *vsmFontVert = GPU::NewShader(data.device, "data/Az3D/shaders/Font3D_VSM.vert.spv", GPU::ShaderStage::VERTEX, "VSM Font Vertex Shader");
		GPU::Shader *vsmFontFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Font3D_VSM.frag.spv", GPU::ShaderStage::FRAGMENT, "VSM Font Fragment Shader");
		data.pipelines[PIPELINE_FONT_3D_VSM] = GPU::NewGraphicsPipeline(data.device, "VSM Font Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_FONT_3D_VSM], {vsmFontVert, vsmFontFrag});
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_FONT_3D_VSM], GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetBlendMode(data.pipelines[PIPELINE_FONT_3D_VSM], GPU::BlendMode::MAX);
		GPU::PipelineSetCullingMode(data.pipelines[PIPELINE_FONT_3D_VSM], GPU::CullingMode::NONE);
		GPU::PipelineSetMultisampleShading(data.pipelines[PIPELINE_FONT_3D_VSM], true);

		data.shadowMapConvolutionImage = GPU::NewImage(data.device, "VSM Convolution Image");
		GPU::ImageSetFormat(data.shadowMapConvolutionImage, GPU::ImageBits::R32G32, GPU::ImageComponentType::SFLOAT);
		GPU::ImageSetSize(data.shadowMapConvolutionImage, dims, dims);
		GPU::ImageSetShaderUsage(data.shadowMapConvolutionImage, (u32)GPU::ShaderStage::FRAGMENT);

		data.framebufferConvolution[0] = GPU::NewFramebuffer(data.device, "VSM Convolution Framebuffer 0");
		data.framebufferConvolution[1] = GPU::NewFramebuffer(data.device, "VSM Convolution Framebuffer 1");
		GPU::FramebufferAddImage(data.framebufferConvolution[0], data.shadowMapConvolutionImage);
		GPU::FramebufferAddImage(data.framebufferConvolution[1], data.shadowMapImage);

		GPU::Shader *convolutionVert = GPU::NewShader(data.device, "data/Az3D/shaders/Convolution.vert.spv", GPU::ShaderStage::VERTEX);
		GPU::Shader *convolutionFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Convolution.frag.spv", GPU::ShaderStage::FRAGMENT);

		data.pipelineShadowMapConvolution = GPU::NewGraphicsPipeline(data.device, "VSM Convolution Pipeline");
		GPU::PipelineAddShaders(data.pipelineShadowMapConvolution, {convolutionVert, convolutionFrag});
		GPU::PipelineSetTopology(data.pipelineShadowMapConvolution, GPU::Topology::TRIANGLE_FAN);
		GPU::PipelineAddPushConstantRange(data.pipelineShadowMapConvolution, 0, sizeof(vec2), (u32)GPU::ShaderStage::FRAGMENT);

		data.shadowMapSampler = GPU::NewSampler(data.device, "VSM Sampler");
		GPU::SamplerSetAddressMode(data.shadowMapSampler, GPU::AddressMode::CLAMP_TO_BORDER, GPU::AddressMode::CLAMP_TO_BORDER);
		GPU::SamplerSetBorderColor(data.shadowMapSampler, true, false, true);
	}

	if (auto result = GPU::Initialize(); result.isError) {
		error = "Failed to init GPU: " + result.error;
		return false;
	}

	worldInfo.lights[0].position = vec4(vec3(0.0f), 1.0f);
	worldInfo.lights[0].color = vec3(0.0f);
	worldInfo.lights[0].direction = vec3(0.0f, 0.0f, 1.0f);
	worldInfo.lights[0].angleMin = 0.0f;
	worldInfo.lights[0].angleMax = 0.0f;
	worldInfo.lights[0].distMin = 0.0f;
	worldInfo.lights[0].distMax = 0.0f;

	GPU::ContextBeginRecording(data.contextTransfer).AzUnwrap();
	GPU::CmdCopyDataToBuffer(data.contextTransfer, data.bonesBuffer, data.bones.data).AzUnwrap();
	GPU::CmdCopyDataToBuffer(data.contextTransfer, data.vertexBuffer, vertices.data).AzUnwrap();
	GPU::CmdCopyDataToBuffer(data.contextTransfer, data.indexBuffer, indices.data).AzUnwrap();
	for (i32 i = 0; i < sys->assets.textures.size; i++) {
		if (auto result = GPU::CmdCopyDataToImage(data.contextTransfer, data.textures[i], sys->assets.textures[i].image.pixels); result.isError) {
			error = Stringify("Failed to copy data to texture ", i, result.error);
			return false;
		}
	}

	if (!UpdateFonts(data.contextTransfer)) {
		error = "Failed to update fonts: " + error;
		return false;
	}

	GPU::ContextEndRecording(data.contextTransfer).AzUnwrap();
	GPU::SubmitCommands(data.contextTransfer).AzUnwrap();
	GPU::ContextWaitUntilFinished(data.contextTransfer).AzUnwrap();
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

vec3 InvViewProj(vec3 point, const mat4 &invViewProj) {
	vec4 inter = invViewProj * vec4(point, 1.0f);
	return inter.xyz / inter.w;
}

void Manager::UpdateLights() {
	AZCORE_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::UpdateLights)
	mat4 invView = worldInfo.viewProj.Inverse();
	// frustum corners
	// TODO: This is a very non-linear way to limit the range of shadows. Do something more direct pls.
	f32 shadowMaxDist = 0.995f;
	vec3 corners[8] = {
		InvViewProj(vec3(-1.0f, -1.0f, 0.0f), invView),
		InvViewProj(vec3( 1.0f, -1.0f, 0.0f), invView),
		InvViewProj(vec3(-1.0f,  1.0f, 0.0f), invView),
		InvViewProj(vec3( 1.0f,  1.0f, 0.0f), invView),
		InvViewProj(vec3(-1.0f, -1.0f, shadowMaxDist), invView),
		InvViewProj(vec3( 1.0f, -1.0f, shadowMaxDist), invView),
		InvViewProj(vec3(-1.0f,  1.0f, shadowMaxDist), invView),
		InvViewProj(vec3( 1.0f,  1.0f, shadowMaxDist), invView),
	};
	vec3 center = 0.0f;
	vec3 boundsMin(100000000.0f), boundsMax(-100000000.0f);
	worldInfo.sun = mat4::Camera(-worldInfo.sunDir, worldInfo.sunDir, vec3(0.0f, 0.0f, 1.0f));
	for (i32 i = 0; i < 8; i++) {
		center += corners[i];
		corners[i] = (worldInfo.sun * vec4(corners[i], 1.0f)).xyz;
		boundsMin.x = min(boundsMin.x, corners[i].x);
		boundsMin.y = min(boundsMin.y, corners[i].y);
		boundsMin.z = min(boundsMin.z, corners[i].z);
		boundsMax.x = max(boundsMax.x, corners[i].x);
		boundsMax.y = max(boundsMax.y, corners[i].y);
		boundsMax.z = max(boundsMax.z, corners[i].z);
	}
	center /= 8.0f;
	// if (sys->Pressed(KC_KEY_C)) {
	// 	io::cout.PrintLn("Center: [", center.x, ", ", center.y, ", ", center.z, "]");
	// }
	// DrawDebugSphere(data.drawingContexts[0], center, 0.1f, vec4(1.0f));
	vec3 dimensions = boundsMax - boundsMin;
	// center gives us an implicit 0.5
	worldInfo.sun = mat4::Camera(center + worldInfo.sunDir * dimensions.z * 9.5f, -worldInfo.sunDir, vec3(0.0f, 0.0f, 1.0f));
	worldInfo.sun = mat4::Ortho(max(0.1f, dimensions.x), max(0.1f, dimensions.y), 0.0f, max(0.1f, dimensions.z*10.0f)) * worldInfo.sun;
	sunFrustum = GetOrtho(
		worldInfo.sun.Col<3>().xyz,
		worldInfo.sun.Col<2>().xyz,
		worldInfo.sun.Col<1>().xyz,
		worldInfo.sun.Col<0>().xyz
	);
// 	i32 lightCounts[LIGHT_BIN_COUNT] = {0};
// 	i32 totalLights = 1;
// 	// By default, they all point to the default light which has no light at all
// 	memset(worldInfo.lightBins, 0, sizeof(worldInfo.lightBins));
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
// 		worldInfo.lights[lightIndex] = light;
// 		bool atLeastOne = false;
// 		for (i32 y = binMin.y; y <= binMax.y; y++) {
// 			for (i32 x = binMin.x; x <= binMax.x; x++) {
// 				i32 i = LightBinIndex({x, y});
// 				LightBin &bin = worldInfo.lightBins[i];
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
// 			worldInfo.lightBins[i].lightIndices[0] = i;
// 	}
// #endif
}

static void GrowBuffer(GPU::Buffer *buffer, i64 size, i64 alignment, i64 numerator, i64 denominator) {
	AzAssert(numerator > denominator, Stringify("Couldn't possibly grow with a factor of ", numerator, "/", denominator));
	i64 newSize = max(GPU::BufferGetSize(buffer), (i64)1);
	if (newSize >= size) return;
	while (newSize < size) {
		newSize *= numerator;
		newSize /= denominator;
		newSize = alignNonPowerOfTwo(newSize, alignment);
	}
	GPU::BufferSetSize(buffer, newSize).AzUnwrap();
}

bool Manager::UpdateFonts(GPU::Context *context) {
	AZCORE_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::UpdateFonts)
	// Will be done on-the-fly

	// FontBuffer
	data.fontBufferDatas.Resize(max(data.fontBufferDatas.size, sys->assets.fonts.size));
	data.textures.Resize(max(data.textures.size, sys->assets.textures.size + sys->assets.fonts.size), nullptr);
	const i32 texIndexOffset = sys->assets.textures.size;
	for (i32 i = 0; i < sys->assets.fonts.size; i++) {
		static constexpr u32 maxFontImageMipLevels = 3;
		Assets::Font &font = sys->assets.fonts[i];
		FontBuffer &fontBuffer = data.fontBufferDatas[i];
		fontBuffer.texAtlas = i + sys->assets.textures.size;
		GPU::Image *&image = data.textures[texIndexOffset + i];
		if (image == nullptr) {
			image = GPU::NewImage(data.device, Stringify("Font atlas ", i));
			GPU::ImageSetFormat(image, GPU::ImageBits::R8, GPU::ImageComponentType::UNORM);
			GPU::ImageSetSize(image, font.fontBuilder.dimensions.x, font.fontBuilder.dimensions.y);
			GPU::ImageSetMipmapping(image, true, maxFontImageMipLevels);
			GPU::ImageSetShaderUsage(image, (u32)GPU::ShaderStage::FRAGMENT);
			if (auto result = GPU::ImageRecreate(image); result.isError) {
				error = result.error;
				return false;
			}
		}
		if (fontBuffer.glyphs.size == font.fontBuilder.glyphs.size) continue;
		fontBuffer.glyphs.ClearSoft();
		for (font::Glyph& glyph : font.fontBuilder.glyphs) {
			if (glyph.info.size.x == 0.0f || glyph.info.size.y == 0.0f) {
				continue;
			}
			const f32 boundSquare = font.fontBuilder.boundSquare;
			f32 posTop = -glyph.info.offset.y * boundSquare;
			f32 posLeft = -glyph.info.offset.x * boundSquare;
			f32 posBot = -glyph.info.size.y * boundSquare + posTop;
			f32 posRight = glyph.info.size.x * boundSquare + posLeft;
			f32 texLeft = glyph.info.pos.x;
			f32 texBot = glyph.info.pos.y;
			f32 texRight = (glyph.info.pos.x + glyph.info.size.x);
			f32 texTop = (glyph.info.pos.y + glyph.info.size.y);
			GlyphInfo glyphInfo;
			glyphInfo.uvs[0] = vec2(texLeft, texTop);
			glyphInfo.uvs[1] = vec2(texRight, texBot);
			glyphInfo.offsets[0] = vec2(posLeft, posTop);
			glyphInfo.offsets[1] = vec2(posRight, posBot);
			fontBuffer.glyphs.Append(glyphInfo);
		}
		GPU::Image *fontTexture = data.textures[fontBuffer.texAtlas];
		if (GPU::ImageSetSize(fontTexture, font.fontBuilder.dimensions.x, font.fontBuilder.dimensions.y, maxFontImageMipLevels)) {
			if (auto result = GPU::ImageRecreate(image); result.isError) {
				error = result.error;
				return false;
			}
		}
		if (auto result = GPU::CmdCopyDataToImage(context, fontTexture, font.fontBuilder.pixels.data); result.isError) {
			error = result.error;
			return false;
		}
	}

	for (i32 i = 0; i < data.fontBufferDatas.size; i++) {
		if (i >= data.fontBuffers.size) {
			data.fontBuffers.Append(GPU::NewUniformBuffer(data.device, Stringify("Font buffer ", i)));
			GPU::BufferSetShaderUsage(data.fontBuffers.Back(), (u32)GPU::ShaderStage::VERTEX);
		}
		GPU::Buffer *buffer = data.fontBuffers[i];
		FontBuffer &bufferData = data.fontBufferDatas[i];
		i64 copySize = bufferData.TotalSize();
		GrowBuffer(buffer, copySize, sizeof(GlyphInfo) * 10, 4, 3);
		u32 *dst;
		if (auto result = GPU::BufferMapHostMemory(buffer); result.isError) {
			error = result.error;
			return false;
		} else {
			dst = (u32*)result.value;
		}
		*dst = bufferData.texAtlas;
		memcpy(&dst[2], bufferData.glyphs.data, bufferData.glyphs.size * sizeof(bufferData.glyphs[0]));
		GPU::BufferUnmapHostMemory(buffer);
		GPU::CmdCopyHostBufferToDeviceBuffer(context, buffer, 0, copySize).AzUnwrap();
	}
	return true;
}

String ToString(mat4 mat, i32 precision=2) {
	return Stringify(
		"| ", FormatFloat(mat[0][0], 10, precision), ", ", FormatFloat(mat[1][0], 10, precision), ", ", FormatFloat(mat[2][0], 10, precision), ", ", FormatFloat(mat[3][0], 10, precision), " |\n",
		"| ", FormatFloat(mat[0][1], 10, precision), ", ", FormatFloat(mat[1][1], 10, precision), ", ", FormatFloat(mat[2][1], 10, precision), ", ", FormatFloat(mat[3][1], 10, precision), " |\n",
		"| ", FormatFloat(mat[0][2], 10, precision), ", ", FormatFloat(mat[1][2], 10, precision), ", ", FormatFloat(mat[2][2], 10, precision), ", ", FormatFloat(mat[3][2], 10, precision), " |\n",
		"| ", FormatFloat(mat[0][3], 10, precision), ", ", FormatFloat(mat[1][3], 10, precision), ", ", FormatFloat(mat[2][3], 10, precision), ", ", FormatFloat(mat[3][3], 10, precision), " |");
}

String ToString(vec4 vec, i32 precision=2) {
	return Stringify("[ ", FormatFloat(vec.x, 10, precision), ", ", FormatFloat(vec.y, 10, precision), ", ", FormatFloat(vec.z, 10, 1), ", ", FormatFloat(vec.w, 10, precision), " ]");
}

vec4 PerspectiveNormalize(vec4 point) {
	return point / point.w;
}

bool ArmatureAction::operator==(const ArmatureAction &other) const {
	return meshIndex == other.meshIndex && actionIndex == other.actionIndex && other.actionTime == other.actionTime;
}

bool Manager::UpdateWorldInfo(GPU::Context *context) {
	// Update camera matrix
	worldInfo.view = mat4::Camera(camera.pos, camera.forward, camera.up);
	// worldInfo.proj = mat4::Ortho(10.0f, 10.0f * screenSize.y / screenSize.x, camera.nearClip, camera.farClip);
	worldInfo.proj = mat4::Perspective(camera.fov, screenSize.x / screenSize.y, camera.nearClip, camera.farClip);
	worldInfo.viewProj = worldInfo.proj * worldInfo.view;
	worldInfo.eyePos = camera.pos;
	worldInfo.fogColor = sRGBToLinear(backgroundRGB);
	worldInfo.ambientLight = worldInfo.fogColor * 0.5f;
	UpdateLights();

	GPU::CmdCopyDataToBuffer(context, data.worldInfoBuffer, &worldInfo).AzUnwrap();

	return true;
}

bool Manager::UpdateObjects(GPU::Context *context) {
	i64 copySize = data.objectShaderInfos.size * sizeof(ObjectShaderInfo);
	GrowBuffer(data.objectBuffer, copySize, sizeof(ObjectShaderInfo) * 100, 3, 2);
	if (copySize) {
		GPU::CmdCopyDataToBuffer(context, data.objectBuffer, data.objectShaderInfos.data, 0, copySize).AzUnwrap();
	}
	copySize = data.bones.size * sizeof(mat4);
	GrowBuffer(data.bonesBuffer, copySize, sizeof(mat4) * 100, 3, 2);
	if (copySize) {
		GPU::CmdCopyDataToBuffer(context, data.bonesBuffer, data.bones.data, 0, copySize).AzUnwrap();
	}
	copySize = data.textShaderInfos.size * sizeof(TextShaderInfo);
	GrowBuffer(data.textBuffer, copySize, sizeof(TextShaderInfo) * 100, 3, 2);
	if (copySize) {
		GPU::CmdCopyDataToBuffer(context, data.textBuffer, data.textShaderInfos.data, 0, copySize);
	}
	return true;
}

bool Manager::UpdateDebugLines(GPU::Context *context) {
	if (!Settings::ReadBool(Settings::sDebugLines)) return true;
	GPU::PipelineSetDepthTest(data.pipelines[PIPELINE_DEBUG_LINES], Settings::ReadBool(Settings::sDebugLinesDepthTest));
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

struct BoneEvalMetadata {
	mat4 restTransformLocal;
	mat4 restTransformModel;
	quat animOrientation = quat(1.0f);
	vec3 animOffset = vec3(0.0f);
	bool evaluated = false;
};

mat4 GetMat4(quat orientation, vec3 offset) {
	mat3 rotation = normalize(orientation).ToMat3();
	mat4 result = mat4(rotation);
	result[3].xyz = offset;
	return result;
}

struct IkEvalMetadata {
	Az3DObj::Bone* bone;
	// rest pose rotation matrix relative to parent
	mat3 localRotation;
	// rest pose offset from parent relative to parent
	vec3 localOffset;
	mat3 rotationX, rotationXY;
	// Our transform relative to parent with evaluated joint positions
	mat4 transformEval;
	// Our transform with evaluated joint positions in model space
	mat4 transformEvalAccum;
	// position of the target relative to our current pose root bone
	// Tip of the outermost bone in model space (only valid for the tip bone)
	vec3 modelTip;
	// Tip of the outermost bone relative to this bone
	vec3 localTip;
	// Evaluated parameters
	f32 stretch = 1.0f;
	vec3 axisAngles = 0.0f;
};

void EvaluateBone(SimpleRange<mat4> transforms, SimpleRange<BoneEvalMetadata> metadatas, SimpleRange<Az3DObj::Bone> bones, i32 boneIndex, Az3DObj::Action &action, f32 time, mat4 &modelTransform, Array<Vector<f32>>& ikParameters, i32& ikIndex);

void EvaluateParameters(Array<IkEvalMetadata> &ikChain, Vector<f32> &parameters, SimpleRange<mat4> transforms, SimpleRange<BoneEvalMetadata> metadatas, SimpleRange<Az3DObj::Bone> bones, mat4 &modelTransform) {
	AZCORE_PROFILING_FUNC_TIMER()
	for (i32 i = 0, p = 0; i < ikChain.size; i++) {
		auto *b = ikChain[i].bone;
		auto &ik = b->ikInfo;
		mat3 allRotation = ikChain[i].localRotation;
		if (ik.stretch != 0.0f) {
			ikChain[i].stretch = parameters[p];
			p++;
		}
		if (!ik.locked.x) {
			ikChain[i].axisAngles.x = parameters[p];
			ikChain[i].rotationX = mat3::RotationBasic(parameters[p], Axis::X);
			allRotation = allRotation * ikChain[i].rotationX;
			p++;
		} else {
			ikChain[i].axisAngles.x = 0.0f;
			ikChain[i].rotationX = mat3::Identity();
		}
		if (!ik.locked.y) {
			ikChain[i].axisAngles.y = parameters[p];
			mat3 rotationY = mat3::RotationBasic(parameters[p], Axis::Y);
			ikChain[i].rotationXY = ikChain[i].rotationX * rotationY;
			allRotation = allRotation * rotationY;
			p++;
		} else {
			ikChain[i].axisAngles.y = 0.0f;
			ikChain[i].rotationXY = ikChain[i].rotationX;
		}
		if (!ik.locked.z) {
			ikChain[i].axisAngles.z = parameters[p];
			allRotation = allRotation * mat3::RotationBasic(parameters[p], Axis::Z);
			p++;
		} else {
			ikChain[i].axisAngles.z = 0.0f;
		}
		ikChain[i].transformEval = mat4(allRotation);
		ikChain[i].transformEval[3].xyz = ikChain[i].localOffset;
		if (i > 0) {
			ikChain[i].transformEvalAccum = ikChain[i-1].transformEvalAccum * ikChain[i].transformEval;
		} else {
			if (b->parent != 255) {
				ikChain[i].transformEvalAccum = transforms[b->parent] * ikChain[i].transformEval;
			} else {
				ikChain[i].transformEvalAccum = ikChain[i].transformEval;
			}
		}
	}
	ikChain.Back().modelTip = ikChain.Back().transformEvalAccum[1].xyz * ikChain.Back().bone->length + ikChain.Back().transformEvalAccum[3].xyz;
	for (i32 i = ikChain.size-1; i >= 0; i--) {
		ikChain[i].localTip = (ikChain[i].transformEvalAccum.InverseUnscaledTransform() * vec4(ikChain.Back().modelTip, 1.0f)).xyz;
	}
}

void EvaluateJacobian(Matrix<f32> &jacobian, Array<IkEvalMetadata> &ikChain, Vector<f32> &parameters, Vector<f32> &stiffness, SimpleRange<mat4> transforms, SimpleRange<BoneEvalMetadata> metadatas, SimpleRange<Az3DObj::Bone> bones, mat4 &modelTransform, bool showDerivatives) {
	AZCORE_PROFILING_FUNC_TIMER()
	for (i32 i = 0, p = 0; i < ikChain.size; i++) {
		auto *b = ikChain[i].bone;
		auto &ik = b->ikInfo;
		vec3 tip;
		if (showDerivatives) {
			tip = (modelTransform * ikChain[i].transformEvalAccum * vec4(0.0f, b->length, 0.0f, 1.0f)).xyz;
		}
		mat3 rotationEval = ikChain[i].transformEvalAccum.TrimmedMat3();
		if (ik.stretch != 0.0f) {
			vec3 &pDerivative = jacobian.Col(p).AsVec3();
			// Same as transformEval * vec3(0, length, 0)
			pDerivative = ikChain[i].transformEvalAccum.Col<1>().xyz * b->length;
			p++;
		}
		if (!ik.locked.x) {
			vec3 &pDerivative = jacobian.Col(p).AsVec3();
			pDerivative = rotationEval.Col<2>() * ikChain[i].localTip.y;
			pDerivative *= (1.0f - stiffness[p]);
			if (showDerivatives) {
				DrawDebugLine(sys->rendering.data.drawingContexts.Back(), DebugVertex{tip, vec4(0.2f, 0.0f, 0.0f, 1.0f)}, DebugVertex{tip + pDerivative, vec4(1.0f, 0.0f, 0.0f, 1.0f)});
			}
			p++;
		}
		if (!ik.locked.y) {
			vec3 &pDerivative = jacobian.Col(p).AsVec3();
			pDerivative = rotationEval * ((ikChain[i].rotationX * ikChain[i].localTip).RotatedYPos90() * vec3(1.0f, 0.0f, 1.0f));
			pDerivative *= (1.0f - stiffness[p]);
			if (showDerivatives) {
				DrawDebugLine(sys->rendering.data.drawingContexts.Back(), DebugVertex{tip, vec4(0.0f, 0.2f, 0.0f, 1.0f)}, DebugVertex{tip + pDerivative, vec4(0.0f, 1.0f, 0.0f, 1.0f)});
			}
			p++;
		}
		if (!ik.locked.z) {
			vec3 &pDerivative = jacobian.Col(p).AsVec3();
			pDerivative = rotationEval * ((ikChain[i].rotationXY * ikChain[i].localTip).RotatedZPos90() * vec3(1.0f, 1.0f, 0.0f));
			pDerivative *= (1.0f - stiffness[p]);
			if (showDerivatives) {
				DrawDebugLine(sys->rendering.data.drawingContexts.Back(), DebugVertex{tip, vec4(0.0f, 0.0f, 0.2f, 1.0f)}, DebugVertex{tip + pDerivative, vec4(0.0f, 0.0f, 1.0f, 1.0f)});
			}
			p++;
		}
	}
}

void LimitParameters(Vector<f32> &parameters, Vector<f32> &parameterMinimums, Vector<f32> &parameterMaximums) {
	for (i32 i = 0; i < parameters.Count(); i++) {
		parameters[i] = clamp(parameters[i], parameterMinimums[i], parameterMaximums[i]);
	}
}

void EvaluateIK(SimpleRange<mat4> transforms, SimpleRange<BoneEvalMetadata> metadatas, SimpleRange<Az3DObj::Bone> bones, i32 boneIndex, Az3DObj::Action &action, f32 time, mat4 &modelTransform, Array<Vector<f32>> &ikParameters, i32 &ikIndex) {
	AZCORE_PROFILING_FUNC_TIMER()
	// cout.PrintLn(bone.name, " has target ", bones[bone.ikTarget].name);
	mat4 &transform = transforms[boneIndex];
	BoneEvalMetadata &meta = metadatas[boneIndex];
	Az3DObj::Bone &bone = bones[boneIndex];
	EvaluateBone(transforms, metadatas, bones, bone.ikTarget, action, time, modelTransform, ikParameters, ikIndex);

	Vector<f32> &parameters = ikParameters[ikIndex];

	static thread_local Array<IkEvalMetadata> ikChain;

	i32 degreesOfFreedom = 0;
	vec4 ikTargetPos = transforms[bone.ikTarget].Col<3>();
	ikChain.ClearSoft();
	for (Az3DObj::Bone *b = &bone; b->isInIkChain; b = &bones[b->parent]) {
		auto &ik = b->ikInfo;
		if (ik.stretch != 0.0f) degreesOfFreedom++;
		if (!ik.locked.x) degreesOfFreedom++;
		if (!ik.locked.y) degreesOfFreedom++;
		if (!ik.locked.z) degreesOfFreedom++;
		i32 chainBoneIndex = b - &bones[0];
		mat4 &t = transforms[chainBoneIndex];
		mat4 &rest = metadatas[chainBoneIndex].restTransformLocal;
		ikChain.Insert(0, {b, rest.TrimmedMat3(), rest[3].xyz});
		if (b->parent == 255) break;
		// break;
	}

	bool uninitialized = parameters.Count() == 0;
	parameters.Resize(degreesOfFreedom);

	// Do one allocation for everything, and partition the resulting matrix.
	Matrix<f32> allInfo = Matrix<f32>::Filled(degreesOfFreedom, 9, 0.0f);
	Matrix<f32> configuration = allInfo.SubMatrix(0, 0, degreesOfFreedom, 5);
	Matrix<f32> jacobian = allInfo.SubMatrix(0, 5, degreesOfFreedom, 3);

	Vector<f32> newParameters = configuration.Row(0);
	Vector<f32> parameterDelta = configuration.Row(1);
	Vector<f32> parameterMinimums = configuration.Row(2);
	Vector<f32> parameterMaximums = configuration.Row(3);
	Vector<f32> parameterStiffness = configuration.Row(4);
	constexpr f32 parameterPersistence = 0.9f;
	for (i32 i = 0, p = 0; i < ikChain.size; i++) {
		auto *b = ikChain[i].bone;
		auto &ik = b->ikInfo;
		if (ik.stretch != 0.0f) {
			if (uninitialized) {
				parameters[p] = 1.0f; // Default to no stretch or squash applied.
			} else {
				parameters[p] = lerp(parameters[p], 1.0f, 1.0f - parameterPersistence);
			}
			parameterMinimums[p] = 0.0f;
			parameterMaximums[p] = INFINITY;
			parameterStiffness[p] = 1.0f - ik.stretch;
			p++;
		}
		if (!ik.locked.x) {
			if (uninitialized) {
				parameters[p] = 0.0f;
			} else {
				parameters[p] = lerp(parameters[p], 0.0f, 1.0f - parameterPersistence);
			}
			parameterMinimums[p] = ik.min.x;
			parameterMaximums[p] = ik.max.x;
			parameterStiffness[p] = ik.stiffness.x;
			p++;
		}
		if (!ik.locked.y) {
			if (uninitialized) {
				parameters[p] = 0.0f;
			} else {
				parameters[p] = lerp(parameters[p], 0.0f, 1.0f - parameterPersistence);
			}
			parameterMinimums[p] = ik.min.y;
			parameterMaximums[p] = ik.max.y;
			parameterStiffness[p] = ik.stiffness.y;
			p++;
		}
		if (!ik.locked.z) {
			if (uninitialized) {
				parameters[p] = 0.0f;
			} else {
				parameters[p] = lerp(parameters[p], 0.0f, 1.0f - parameterPersistence);
			}
			parameterMinimums[p] = ik.min.z;
			parameterMaximums[p] = ik.max.z;
			parameterStiffness[p] = ik.stiffness.z;
			p++;
		}
	}
	if (sys->input.Pressed(KC_KEY_P)) {
		io::cout.PrintLn("\nIK Bone: ", ikChain.Back().bone->name);
	}
	vec3 error, nextError;
	for (i32 i = 0; i < numNewtonIterations; i++) {
		// Do some newton-raphson iteration to reduce error.
		EvaluateParameters(ikChain, parameters, transforms, metadatas, bones, modelTransform);
		error = ikChain.Back().modelTip - ikTargetPos.xyz;
		if (normSqr(error) < square(0.001f)) break;
		EvaluateJacobian(jacobian, ikChain, parameters, parameterStiffness, transforms, metadatas, bones, modelTransform, false);
		Vector<f32> err(error.data, 3, 1);
		// io::cout.PrintLn("error: ", err);
		Matrix<f32> inv = transpose(&jacobian);
		// Matrix<f32> inv = jacobian.PseudoInverse(10, 0.01f, 0.1f);
		parameterDelta = inv * err;
		parameterDelta /= max(norm(err), 0.1f);
		f32 highestParameter = 0.0f;
		for (i32 i = 0; i < parameterDelta.Count(); i++) {
			// Because our Jacobian is a bunch of sinusoidal first derivatives, our pseudoinverse will give us the tangent of the actual change in angle we want.
			// TODO: Be smarter because stretch is not an angle.
			parameterDelta[i] = atan(parameterDelta[i]);
			if (parameterDelta[i] > highestParameter) {
				highestParameter = parameterDelta[i];
			}
		}
		if (sys->input.Pressed(KC_KEY_P)) {
			io::cout.PrintLn("Iteration: ", i, "\nJacobian:\n", jacobian, "PseudoInverse:\n", inv, "Error: ", err, "\nParameter Delta: ", parameterDelta);
		}
		f32 normSqrLastError;
		f32 scale = min(halfpi / highestParameter, 1.0f) / ikChain.size;
		f32 step = scale;
		nextError = error;
		for (i32 j = 0; j < numBinarySearchIterations; j++) {
			newParameters = parameters;
			newParameters -= parameterDelta * scale;
			LimitParameters(newParameters, parameterMinimums, parameterMaximums);
			EvaluateParameters(ikChain, newParameters, transforms, metadatas, bones, modelTransform);
			normSqrLastError = norm(nextError);
			nextError = ikChain.Back().modelTip - ikTargetPos.xyz;
			f32 delta = norm(nextError) - normSqrLastError;
			if (abs(delta) < 0.0001f) break;
			step *= -0.5f * sign(delta);
			scale += step;
		}
		parameters -= parameterDelta * scale;
	}
	EvaluateParameters(ikChain, parameters, transforms, metadatas, bones, modelTransform);
	EvaluateJacobian(jacobian, ikChain, parameters, parameterStiffness, transforms, metadatas, bones, modelTransform, true);

	{
		vec3 tip = (modelTransform * ikChain.Back().transformEvalAccum * vec4(0.0f, ikChain.Back().bone->length, 0.0f, 1.0f)).xyz;
		DrawDebugLine(sys->rendering.data.drawingContexts.Back(), DebugVertex{tip, vec4(0.2f, 0.0f, 0.2f, 1.0f)}, DebugVertex{tip - (ikChain.Back().modelTip - ikTargetPos.xyz), vec4(1.0f, 0.0f, 1.0f, 1.0f)});
	}
	// io::cout.PrintLn("Parameters: ", parameters, "\nParameter Mins: ", parameterMinimums, "\nParameter Maxs: ", parameterMaximums, "\nParameter Stiffness: ", parameterStiffness, "\nJacobian:\n", Indent(), jacobian);
	for (i32 i = 0; i < ikChain.size; i++) {
		i32 boneIndex = ikChain[i].bone - &bones[0];
		transforms[boneIndex] = ikChain[i].transformEvalAccum;
	}
}

void EvaluateBone(SimpleRange<mat4> transforms, SimpleRange<BoneEvalMetadata> metadatas, SimpleRange<Az3DObj::Bone> bones, i32 boneIndex, Az3DObj::Action &action, f32 time, mat4 &modelTransform, Array<Vector<f32>> &ikParameters, i32 &ikIndex) {
	AZCORE_PROFILING_FUNC_TIMER()
	mat4 &transform = transforms[boneIndex];
	BoneEvalMetadata &meta = metadatas[boneIndex];
	if (meta.evaluated) {
		return;
	}
	Az3DObj::Bone &bone = bones[boneIndex];

	meta.restTransformLocal = mat4::FromCols(
		vec4(bone.basis.Col<0>(), 0.0f),
		vec4(bone.basis.Col<1>(), 0.0f),
		vec4(bone.basis.Col<2>(), 0.0f),
		vec4(bone.offset        , 1.0f)
	);
	meta.animOrientation = quat(1.0f);
	meta.animOffset = vec3(0.0f);

	for (auto &curve : action.curves) {
		if (curve.boneName != bone.name) continue;
		// io::cout.PrintLn("Found curve for bone \"", bone.name, "\" which is a ", curve.isOffset ? "location" : "orientation", " with index ", curve.index);
		if (curve.isOffset) {
			meta.animOffset[curve.index] = curve.Evaluate(time);
		} else {
			meta.animOrientation[curve.index] = curve.Evaluate(time);
		}
	}

	transform = GetMat4(meta.animOrientation, meta.animOffset);

	// transform = mat4::RotationBasic(tau * 0.02f, Axis::X);
	if (bone.parent != 255) {
		EvaluateBone(transforms, metadatas, bones, bone.parent, action, time, modelTransform, ikParameters, ikIndex);
		meta.restTransformModel = metadatas[bone.parent].restTransformModel * meta.restTransformLocal;
		transform = transforms[bone.parent] * meta.restTransformLocal * transform;
	} else {
		meta.restTransformModel = meta.restTransformLocal;
		transform = meta.restTransformLocal * transform;
	}
	if (bone.ikTarget != 255) {
		if (ikParameters.size <= ikIndex) {
			ikParameters.Resize(ikIndex+1);
		}
		EvaluateIK(transforms, metadatas, bones, boneIndex, action, time, modelTransform, ikParameters, ikIndex);
		ikIndex++;
	}
	meta.evaluated = true;
	return;
}

// Appends the animated bones to the end of dstBones
void AnimateArmature(Array<mat4> &dstBones, ArmatureAction armatureAction, mat4 &modelTransform, Array<Vector<f32>> *ikParameters) {
	AZCORE_PROFILING_FUNC_TIMER()
	Array<Vector<f32>> ikParametersFallback;
	if (ikParameters == nullptr) {
		ikParameters = &ikParametersFallback;
	}
	Assets::Mesh &mesh = sys->assets.meshes[armatureAction.meshIndex];
	Az3DObj::Action &action = sys->assets.actions[armatureAction.actionIndex].action;
	i32 ikIndex = 0;
	for (auto &armature : mesh.armatures) {
		i32 boneStart = dstBones.size;
		dstBones.Resize(dstBones.size + armature.bones.size, mat4::Identity());
		SimpleRange<mat4> transforms(&dstBones[boneStart], armature.bones.size);
		Array<BoneEvalMetadata> metadatas(armature.bones.size);
		// Evaluate the hierarchy in bone space, also getting the model-space rest transforms
		for (i32 i = 0; i < transforms.size; i++) {
			EvaluateBone(transforms, metadatas, armature.bones, i, action, armatureAction.actionTime, modelTransform, *ikParameters, ikIndex);
		}
		// THEN go from model space to bone space
		for (i32 i = 0; i < transforms.size; i++) {
			if (Settings::ReadBool(Settings::sDebugLines)) {
				DebugVertex p1, p2;
				p1.color = vec4(0.0f, 0.0f, 1.0f, 0.4f);
				p2.color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
				p1.pos = (modelTransform * transforms[i] * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
				p2.pos = (modelTransform * transforms[i] * vec4(0.0f, armature.bones[i].length, 0.0f, 1.0f)).xyz;
				DrawDebugLine(sys->rendering.data.drawingContexts[0], p1, p2);
			}
			transforms[i] = transforms[i] * metadatas[i].restTransformModel.Inverse();
		}
	}
}

bool Manager::Draw() {
	AZCORE_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::Draw)

	bool updateFontMemory = false;
	for (i32 i = 0; i < sys->assets.fonts.size; i++) {
		Assets::Font& font = sys->assets.fonts[i];
		if (font.fontBuilder.indicesToAdd.size != 0) {
			font.fontBuilder.Build();
			updateFontMemory = true;
		}
	}

	GPU::SetVSync(data.window, Settings::ReadBool(Settings::sVSync));
	static az::Profiling::AString sWindowUpdate("GPU::WindowUpdate");
	az::Profiling::Timer timerWindowUpdate(sWindowUpdate);
	timerWindowUpdate.Start();
	AZCORE_PROFILING_EXCEPTION_START();
	if (auto result = GPU::WindowUpdate(data.window); result.isError) {
		error = "Failed to update GPU window: " + result.error;
		return false;
	}
	timerWindowUpdate.End();
	AZCORE_PROFILING_EXCEPTION_END();

	screenSize = vec2((f32)max((u16)1, sys->window.width), (f32)max((u16)1, sys->window.height));
	aspectRatio = screenSize.y / screenSize.x;

	{ // Shadow Map
		GPU::ContextBeginRecording(data.contextShadowMap).AzUnwrap();
		GPU::CmdImageTransitionLayout(data.contextShadowMap, data.shadowMapImage, GPU::ImageLayout::UNDEFINED, GPU::ImageLayout::ATTACHMENT);
		GPU::CmdBindFramebuffer(data.contextShadowMap, data.framebufferShadowMaps);
		GPU::CmdBindPipeline(data.contextShadowMap, data.pipelines[PIPELINE_BASIC_3D_VSM]);
		GPU::CmdBindIndexBuffer(data.contextShadowMap, data.indexBuffer);
		GPU::CmdBindVertexBuffer(data.contextShadowMap, data.vertexBuffer);
		GPU::CmdBindUniformBuffer(data.contextShadowMap, data.worldInfoBuffer, 0, 0);
		GPU::CmdBindStorageBuffer(data.contextShadowMap, data.objectBuffer, 0, 1);
		GPU::CmdBindStorageBuffer(data.contextShadowMap, data.bonesBuffer, 0, 2);
		GPU::CmdBindImageArraySampler(data.contextShadowMap, data.textures, data.textureSampler, 0, 3);
		GPU::CmdCommitBindings(data.contextShadowMap).AzUnwrap();

		GPU::CmdClearColorAttachment(data.contextShadowMap, vec4(0.0));
	}

	GPU::ContextBeginRecording(data.contextGraphics).AzUnwrap();
	GPU::CmdBindFramebuffer(data.contextGraphics, data.framebuffer);
	GPU::CmdSetViewportAndScissor(data.contextGraphics, (f32)sys->window.width, (f32)sys->window.height);
	GPU::CmdBindIndexBuffer(data.contextGraphics, data.indexBuffer);
	GPU::CmdBindUniformBuffer(data.contextGraphics, data.worldInfoBuffer, 0, 0);
	GPU::CmdBindStorageBuffer(data.contextGraphics, data.objectBuffer, 0, 1);
	GPU::CmdBindStorageBuffer(data.contextGraphics, data.bonesBuffer, 0, 2);
	GPU::CmdBindImageArraySampler(data.contextGraphics, data.textures, data.textureSampler, 0, 3);
	GPU::CmdBindImageSampler(data.contextGraphics, data.shadowMapImage, data.shadowMapSampler, 0, 4);
	GPU::CmdCommitBindings(data.contextGraphics).AzUnwrap();
	/*{ // Fade
		DrawQuadSS(commandBuffersSecondary[0], texBlank, vec4(backgroundRGB, 0.2f), vec2(-1.0), vec2(2.0), vec2(1.0));
	}*/
	{ // Clear
		GPU::CmdClearColorAttachment(data.contextGraphics, vec4(sRGBToLinear(backgroundRGB), 1.0f));
		GPU::CmdClearDepthAttachment(data.contextGraphics, 1.0f);
	}
	// Clear lights so we get new ones this frame
	lights.size = 0;

	for (DrawingContext &context : data.drawingContexts) {
		context.thingsToDraw.ClearSoft();
		context.debugLines.ClearSoft();
	}

	sys->Draw(data.drawingContexts);

	GPU::ContextBeginRecording(data.contextTransfer).AzUnwrap();
	if (updateFontMemory) {
		if (!UpdateFonts(data.contextTransfer)) return false;
	}
	if (!UpdateWorldInfo(data.contextTransfer)) return false;

	data.objectShaderInfos.ClearSoft();
	data.bones.ClearSoft();
	data.textShaderInfos.ClearSoft();
	az::HashMap<ArmatureAction, u32> actions;
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
			// if (drawCall.castsShadows) {
			// 	drawCall.castsShadows = IsSphereInFrustum(drawCall.boundingSphereCenter, drawCall.boundingSphereRadius, sunFrustum);
			// }
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
		bool shadowPipelineIsForFonts = false;
		for (DrawCallInfo &drawCall : allDrawCalls) {
			if (drawCall.culled && !drawCall.castsShadows) continue;
			u32 bonesOffset = 0;
			if (drawCall.armatureAction.Exists()) {
				u32 &dstOffset = actions.ValueOf(drawCall.armatureAction.ValueUnchecked(), (u32)data.bones.size);
				if (dstOffset == (u32)data.bones.size) {
					// Actually make the bones!
					AnimateArmature(data.bones, drawCall.armatureAction.ValueUnchecked(), drawCall.transforms[0], drawCall.ikParameters);
				}
				bonesOffset = dstOffset;
			}
			if (!drawCall.culled && drawCall.pipeline != currentPipeline) {
				BindPipeline(data.contextGraphics, drawCall.pipeline);
				currentPipeline = drawCall.pipeline;
			}
			if (drawCall.textsToDraw.size > 0) {
				if (drawCall.castsShadows && !shadowPipelineIsForFonts) {
					BindPipeline(data.contextShadowMap, PIPELINE_FONT_3D_VSM);
					shadowPipelineIsForFonts = true;
				}
				i32 objectIndex = data.objectShaderInfos.size;
				data.objectShaderInfos.Append(ObjectShaderInfo{drawCall.transforms[0], drawCall.material, 0});
				for (DrawTextInfo &info : drawCall.textsToDraw) {
					info.shaderInfo.objectIndex = objectIndex;
					i32 textIndex = data.textShaderInfos.size;
					data.textShaderInfos.Append(info.shaderInfo);
					if (!drawCall.culled) {
						GPU::CmdDraw(data.contextGraphics, info.glyphCount * 6, 0, 1, textIndex);
					}
					if (drawCall.castsShadows) {
						GPU::CmdDraw(data.contextShadowMap, info.glyphCount * 6, 0, 1, textIndex);
					}
				}
			} else {
				if (drawCall.castsShadows && shadowPipelineIsForFonts) {
					BindPipeline(data.contextShadowMap, PIPELINE_BASIC_3D_VSM);
					shadowPipelineIsForFonts = false;
				}
				drawCall.instanceStart = data.objectShaderInfos.size;
				i32 prevSize = data.objectShaderInfos.size;
				data.objectShaderInfos.Resize(data.objectShaderInfos.size + drawCall.transforms.size);
				for (i32 i = 0; i < drawCall.transforms.size; i++) {
					data.objectShaderInfos[prevSize+i] = ObjectShaderInfo{drawCall.transforms[i], drawCall.material, bonesOffset};
				}
				if (!drawCall.culled) {
					GPU::CmdDrawIndexed(data.contextGraphics, drawCall.indexCount, drawCall.indexStart, 0, drawCall.instanceCount, drawCall.instanceStart);
				}
				if (drawCall.castsShadows) {
					GPU::CmdDrawIndexed(data.contextShadowMap, drawCall.indexCount, drawCall.indexStart, 0, drawCall.instanceCount, drawCall.instanceStart);
				}
			}
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

	GPU::CmdFinishFramebuffer(data.contextShadowMap, false);
	GPU::CmdImageTransitionLayout(data.contextShadowMap, data.shadowMapImage, GPU::ImageLayout::ATTACHMENT, GPU::ImageLayout::SHADER_READ);

	GPU::CmdImageTransitionLayout(data.contextShadowMap, data.shadowMapConvolutionImage, GPU::ImageLayout::UNDEFINED, GPU::ImageLayout::ATTACHMENT);
	GPU::CmdBindFramebuffer(data.contextShadowMap, data.framebufferConvolution[0]);
	GPU::CmdBindPipeline(data.contextShadowMap, data.pipelineShadowMapConvolution);
	GPU::CmdBindImageSampler(data.contextShadowMap, data.shadowMapImage, data.shadowMapSampler, 0, 0);
	GPU::CmdCommitBindings(data.contextShadowMap).AzUnwrap();
	vec2 convolutionDirection = vec2(1.0f, 0.0f);
	GPU::CmdPushConstants(data.contextShadowMap, &convolutionDirection, 0, sizeof(vec2));
	GPU::CmdDraw(data.contextShadowMap, 4, 0);
	GPU::CmdFinishFramebuffer(data.contextShadowMap);
	GPU::CmdImageTransitionLayout(data.contextShadowMap, data.shadowMapConvolutionImage, GPU::ImageLayout::ATTACHMENT, GPU::ImageLayout::SHADER_READ);
	GPU::CmdImageTransitionLayout(data.contextShadowMap, data.shadowMapImage, GPU::ImageLayout::UNDEFINED, GPU::ImageLayout::ATTACHMENT);

	GPU::CmdBindFramebuffer(data.contextShadowMap, data.framebufferConvolution[1]);
	GPU::CmdBindPipeline(data.contextShadowMap, data.pipelineShadowMapConvolution);
	GPU::CmdBindImageSampler(data.contextShadowMap, data.shadowMapConvolutionImage, data.shadowMapSampler, 0, 0);
	GPU::CmdCommitBindings(data.contextShadowMap).AzUnwrap();
	convolutionDirection = vec2(0.0f, 1.0f);
	GPU::CmdPushConstants(data.contextShadowMap, &convolutionDirection, 0, sizeof(vec2));
	GPU::CmdDraw(data.contextShadowMap, 4, 0);
	GPU::CmdFinishFramebuffer(data.contextShadowMap);
	GPU::CmdImageGenerateMipmaps(data.contextShadowMap, data.shadowMapImage, GPU::ImageLayout::ATTACHMENT, GPU::ImageLayout::SHADER_READ);

	GPU::ContextEndRecording(data.contextShadowMap).AzUnwrap();

	if (auto result = GPU::SubmitCommands(data.contextShadowMap, 1, {GPU::ContextGetCurrentSemaphore(data.contextTransfer)}); result.isError) {
		error = "Failed to submit shadow map commands: " + result.error;
		return false;
	}

	GPU::ContextEndRecording(data.contextGraphics).AzUnwrap();

	if (auto result = GPU::SubmitCommands(data.contextGraphics, 2, {GPU::ContextGetCurrentSemaphore(data.contextShadowMap)}); result.isError) {
		error = "Failed to draw commands: " + result.error;
		return false;
	}
	return true;
}

bool Manager::Present() {
	AZCORE_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::Present)
	if (auto result = GPU::WindowPresent(data.window, {GPU::ContextGetCurrentSemaphore(data.contextGraphics)}); result.isError) {
		error = "Failed to present: " + result.error;
		return false;
	}
	return true;
}

void Manager::UpdateBackground() {
	backgroundRGB = hsvToRgb(backgroundHSV);
}

f32 CharacterWidth(char32 character, const Assets::Font *fontDesired, const Assets::Font *fontFallback) {
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

f32 LineWidth(const char32 *string, Assets::FontIndex fontIndex) {
	const Assets::Font *fontDesired = &sys->assets.fonts[fontIndex];
	const Assets::Font *fontFallback = &sys->assets.fonts[0];
	f32 size = 0.0f;
	for (i32 i = 0; string[i] != 0 && string[i] != '\n'; i++) {
		size += CharacterWidth(string[i], fontDesired, fontFallback);
	}
	return size;
}

vec2 StringSize(WString string, Assets::FontIndex fontIndex) {
	const Assets::Font *fontDesired = &sys->assets.fonts[fontIndex];
	const Assets::Font *fontFallback = &sys->assets.fonts[0];
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

f32 StringWidth(WString string, Assets::FontIndex fontIndex) {
	return StringSize(string, fontIndex).x;
}

f32 StringHeight(WString string) {
	f32 size = (1.0f + lineHeight) * 0.5f;
	for (i32 i = 0; i < string.size; i++) {
		const char32 character = string[i];
		if (character == '\n') {
			size += lineHeight;
		}
	}
	return size;
}

// TODO: Maybe just mutate a reference instead? Check if we depend on this anywhere.
WString StringAddNewlines(WString string, Assets::FontIndex fontIndex, f32 maxWidth) {
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

void LineCursorStartAndSpaceScale(f32 &dstCursor, f32 &dstSpaceScale, f32 textOrigin, f32 spaceWidth, Assets::FontIndex fontIndex, const char32 *string, TextJustify justify) {
	f32 lineWidth = LineWidth(string, fontIndex);
	dstCursor = -lineWidth * textOrigin;
	if (justify) {
		i32 numSpaces = 0;
		for (i32 i = 0; string[i] != 0 && string[i] != '\n'; i++) {
			if (string[i] == ' ') {
				numSpaces++;
			}
		}
		dstSpaceScale = 1.0f + max((justify.MaxWidth() - lineWidth) / numSpaces / spaceWidth, 0.0f);
		if (dstSpaceScale > 4.0f) {
			dstSpaceScale = 1.5f;
		}
	} else {
		dstSpaceScale = 1.0f;
	}
}

void DrawText(DrawingContext &context, Assets::FontIndex fontIndex, vec2 textOrigin, const WString &string, mat4 transform, bool castsShadows, Assets::Material material, TextJustify justify) {
	if (string.size == 0) return;
	DrawCallInfo drawCallInfo;
	drawCallInfo.transforms = {transform};
	drawCallInfo.boundingSphereCenter = transform.Col<3>().xyz;
	drawCallInfo.boundingSphereRadius = 0.0f;
	drawCallInfo.pipeline = PIPELINE_FONT_3D;
	drawCallInfo.material = material;
	// treat text as transparent since the edges will do blending
	drawCallInfo.opaque = false;
	drawCallInfo.castsShadows = castsShadows;
	drawCallInfo.culled = false;

	DrawTextInfo textInfo;
	textInfo.shaderInfo.fontIndex = (u32)fontIndex;
	textInfo.shaderInfo.objectIndex = (u32)context.thingsToDraw.size; // TODO: These get set later, yeah? Remove this.
	textInfo.glyphCount = 0;

	DrawTextInfo textInfoFallback;
	textInfoFallback.shaderInfo.fontIndex = 0;
	textInfoFallback.shaderInfo.objectIndex = (u32)context.thingsToDraw.size; // TODO: These get set later, yeah? Remove this.
	textInfoFallback.glyphCount = 0;

	Assets::Font *fontDesired = &sys->assets.fonts[fontIndex];
	Assets::Font *fontFallback = &sys->assets.fonts[0];

	vec2 cursor = vec2(0.0f);
	if (textOrigin.y != 0.0f) {
		f32 height = StringHeight(string);
		cursor.y -= height * textOrigin.y;
	}
	f32 spaceWidth = CharacterWidth((char32)' ', fontDesired, fontFallback);
	f32 tabWidth = CharacterWidth((char32)'_', fontDesired, fontFallback) * 4.0f;
	f32 spaceScale = 1.0f;
	LineCursorStartAndSpaceScale(cursor.x, spaceScale, textOrigin.x, spaceWidth, fontIndex, &string[0], justify);
	for (i32 i = 0; i < string.size; i++) {
		drawCallInfo.boundingSphereRadius = max(normSqr(cursor), drawCallInfo.boundingSphereRadius);
		char32 character = string[i];
		if (character == '\n') {
			if (i+1 < string.size) {
				LineCursorStartAndSpaceScale(cursor.x, spaceScale, textOrigin.x, spaceWidth, fontIndex, &string[i+1], justify);
				cursor.y += lineHeight;
			}
			continue;
		}
		if (character == '\t') {
			cursor.x = ceil(cursor.x / tabWidth + 0.05f) * tabWidth;
			continue;
		}

		Assets::Font *font = fontDesired;
		DrawTextInfo *text = &textInfo;
		i32 glyphIndex = fontDesired->font.GetGlyphIndex(character);
		if (glyphIndex == 0) {
			const i32 glyphFallback = fontFallback->font.GetGlyphIndex(character);
			if (glyphFallback != 0) {
				glyphIndex = glyphFallback;
				font = fontFallback;
				text = &textInfoFallback;
			}
		}
		i32 glyphId = font->fontBuilder.indexToId[glyphIndex];
		if (glyphId == 0) {
			font->fontBuilder.AddRange(character, character);
		}
		font::Glyph& glyph = font->fontBuilder.glyphs[glyphId];

		if (glyph.components.size != 0) {
			for (const font::Component& component : glyph.components) {
				i32 componentId = font->fontBuilder.indexToId[component.glyphIndex];
				text->shaderInfo.glyphTransforms[text->glyphCount] = component.transform;
				text->shaderInfo.glyphOffsets[text->glyphCount] = cursor + component.offset * vec2(1.0f, -1.0f);
				text->shaderInfo.glyphIndices[text->glyphCount] = componentId; // shader glyphIndex maps to fontBuilder glyphId
				text->glyphCount++;
				if (text->glyphCount == TextShaderInfo::MAX_GLYPHS) {
					drawCallInfo.textsToDraw.Append(*text);
					text->glyphCount = 0;
				}
			}
		} else {
			if (character != ' ') {
				text->shaderInfo.glyphTransforms[text->glyphCount] = mat2::Identity();
				text->shaderInfo.glyphOffsets[text->glyphCount] = cursor;
				text->shaderInfo.glyphIndices[text->glyphCount] = glyphId; // shader glyphIndex maps to fontBuilder glyphId
				text->glyphCount++;
				if (text->glyphCount == TextShaderInfo::MAX_GLYPHS) {
					drawCallInfo.textsToDraw.Append(*text);
					text->glyphCount = 0;
				}
			}
		}
		if (character == ' ') {
			cursor += glyph.info.advance * spaceScale;
		} else {
			cursor += glyph.info.advance;
		}
	}
	if (textInfo.glyphCount > 0) {
		drawCallInfo.textsToDraw.Append(textInfo);
	}
	if (textInfoFallback.glyphCount > 0) {
		drawCallInfo.textsToDraw.Append(textInfoFallback);
	}
	if (drawCallInfo.textsToDraw.size > 0) {
		// Add 1.5em to account for not bothering to handle glyph size and offset. This should work in 99% of cases minimum.
		drawCallInfo.boundingSphereRadius = (sqrt(drawCallInfo.boundingSphereRadius) + 1.5f)
		* sqrt(max(
			normSqr(transform.Col<0>().xyz),
			normSqr(transform.Col<1>().xyz),
			normSqr(transform.Col<2>().xyz)
		));
		context.thingsToDraw.Append(std::move(drawCallInfo));
	}
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

void DrawMeshPart(DrawingContext &context, Assets::MeshPart *meshPart, const ArrayWithBucket<mat4, 1> &transforms, bool opaque, bool castsShadows, az::Optional<ArmatureAction> action) {
	DrawCallInfo draw;
	draw.transforms = transforms;
	draw.boundingSphereCenter = vec3(0.0f);
	for (i32 i = 0; i < transforms.size; i++) {
		draw.boundingSphereCenter += transforms[i].Col<3>().xyz;
	}
	draw.boundingSphereCenter /= (f32)transforms.size;
	draw.boundingSphereRadius = 0.0f;
	for (i32 i = 0; i < transforms.size; i++) {
		f32 myRadius = meshPart->boundingSphereRadius * sqrt(max(
			normSqr(transforms[i].Col<0>().xyz),
			normSqr(transforms[i].Col<1>().xyz),
			normSqr(transforms[i].Col<2>().xyz)
		));
		myRadius += norm(draw.boundingSphereCenter - transforms[i].Col<3>().xyz);
		if (myRadius > draw.boundingSphereRadius) {
			draw.boundingSphereRadius = myRadius;
		}
	}
	if (action.Exists()) {
		// TODO: Maybe work out a better upper bound as animations can do more than this (but probably won't?)
		draw.boundingSphereRadius *= 2.0f;
	}
	draw.depth = dot(sys->rendering.camera.forward, draw.boundingSphereCenter - sys->rendering.camera.pos);
	draw.indexStart = meshPart->indexStart;
	draw.indexCount = meshPart->indices.size;
	draw.instanceCount = transforms.size;
	draw.material = meshPart->material;
	draw.pipeline = meshPart->material.isFoliage ? PIPELINE_FOLIAGE_3D : PIPELINE_BASIC_3D;
	draw.armatureAction = action;
	draw.ikParameters = nullptr;
	draw.opaque = opaque;
	draw.castsShadows = castsShadows;
	draw.culled = false;
	// We don't need synchronization because each thread gets their own array.
	context.thingsToDraw.Append(draw);
}

void DrawMesh(DrawingContext &context, Assets::MeshIndex mesh, const ArrayWithBucket<mat4, 1> &transforms, bool opaque, bool castsShadows) {
	for (Assets::MeshPart *meshPart : sys->assets.meshes[mesh].parts) {
		DrawMeshPart(context, meshPart, transforms, opaque && meshPart->material.color.a == 1.0f, castsShadows && meshPart->material.color.a >= 0.5f);
	}
}

void DrawMeshAnimated(DrawingContext &context, Assets::MeshIndex mesh, Assets::ActionIndex actionIndex, f32 time, const ArrayWithBucket<mat4, 1> &transforms, bool opaque, bool castsShadows, Array<Vector<f32>> *ikParameters) {
	const ArrayWithBucket<mat4, 1> *finalTransforms = &transforms;
	Az3DObj::Action &action = sys->assets.actions[actionIndex].action;
	bool usesModelTransform = false;
	quat orientation = quat(1.0f);
	vec3 offset = vec3(0.0f);
	for (auto &curve : action.curves) {
		if (curve.boneName.size == 0) {
			usesModelTransform = true;
			if (curve.isOffset) {
				offset[curve.index] = curve.Evaluate(time);
			} else {
				orientation[curve.index] = curve.Evaluate(time);
			}
		}
	}
	ArrayWithBucket<mat4, 1> newTransforms;
	if (usesModelTransform) {
		newTransforms.Reserve(transforms.size);
		mat4 transform = GetMat4(orientation, offset);
		for (const mat4 &oldTransform : transforms) {
			newTransforms.Append(oldTransform * transform);
		}
		finalTransforms = &newTransforms;
	}
	for (Assets::MeshPart *meshPart : sys->assets.meshes[mesh].parts) {
		DrawMeshPart(context, meshPart, *finalTransforms, opaque && meshPart->material.color.a == 1.0f, castsShadows && meshPart->material.color.a >= 0.5f, ArmatureAction{mesh, actionIndex, time});
		context.thingsToDraw.Back().ikParameters = ikParameters;
	}
}

} // namespace Rendering
