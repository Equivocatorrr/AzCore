/*
	File: rendering.cpp
	Author: Philip Haynes
*/

#include "rendering.hpp"
#include "game_systems.hpp"
#include "settings.hpp"
#include "assets.hpp"
#include "animation.hpp"

#include "AzCore/Profiling.hpp"
#include "AzCore/IO/Log.hpp"
#include "AzCore/font.hpp"
#include "AzCore/QuickSort.hpp"

namespace Az3D::Rendering {

constexpr i32 MAX_DEBUG_VERTICES = 8192;

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

Frustum GetFrustumFromCamera(const Camera &camera) {
	Frustum result;
	f32 tanhFOV = tan(Radians32(camera.fov).value() * 0.5f);
	f32 tanvFOV = tanhFOV * camera.aspectRatio;
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
			GPU::CmdBindIndexBuffer(context, nullptr);
			GPU::CmdBindVertexBuffer(context, r.data.debugVertexBuffer);
			GPU::CmdClearDescriptors(context);
			GPU::CmdBindUniformBuffer(context, r.data.worldInfoBuffer, 0, 0);
			break;
		case PIPELINE_BASIC_3D:
		case PIPELINE_BASIC_3D_VSM:
		case PIPELINE_FOLIAGE_3D:
		case PIPELINE_FOLIAGE_3D_VSM:
			GPU::CmdBindVertexBuffer(context, r.data.vertexBuffer);
			GPU::CmdBindIndexBuffer(context, r.data.indexBuffer);
			GPU::CmdClearDescriptors(context);
			GPU::CmdBindUniformBuffer(context, r.data.worldInfoBuffer, 0, 0);
			GPU::CmdBindStorageBuffer(context, r.data.objectBuffer, 0, 1);
			GPU::CmdBindStorageBuffer(context, r.data.bonesBuffer, 0, 2);
			GPU::CmdBindImageArraySampler(context, r.data.textures, r.data.textureSampler, 0, 3);
			GPU::CmdBindImageSampler(context, r.data.aoSmoothedImage, r.data.aoImageSampler, 0, 4);
			GPU::CmdBindImageSampler(context, r.data.shadowMapImage, r.data.shadowMapSampler, 0, 5);
			break;
		case PIPELINE_FONT_3D:
		case PIPELINE_FONT_3D_VSM:
			GPU::CmdBindVertexBuffer(context, nullptr);
			GPU::CmdBindIndexBuffer(context, nullptr);
			GPU::CmdClearDescriptors(context);
			GPU::CmdBindUniformBuffer(context, r.data.worldInfoBuffer, 0, 0);
			GPU::CmdBindStorageBuffer(context, r.data.objectBuffer, 0, 1);
			GPU::CmdBindStorageBuffer(context, r.data.bonesBuffer, 0, 2);
			GPU::CmdBindImageArraySampler(context, r.data.textures, r.data.textureSampler, 0, 3);
			GPU::CmdBindImageSampler(context, r.data.aoSmoothedImage, r.data.aoImageSampler, 0, 4);
			GPU::CmdBindImageSampler(context, r.data.shadowMapImage, r.data.shadowMapSampler, 0, 5);
			GPU::CmdBindUniformBufferArray(context, r.data.fontBuffers, 1, 0);
			GPU::CmdBindStorageBuffer(context, r.data.textBuffer, 1, 1);
			break;
	}
	GPU::CmdCommitBindings(context).AzUnwrap();
}

// Translates the main rendering pipeline into the version for the depth prepass
void BindPipelineDepthPrepass(GPU::Context *context, PipelineIndex pipeline) {
	Manager &r = sys->rendering;
	switch (pipeline) {
		case PIPELINE_BASIC_3D:
			GPU::CmdBindPipeline(context, r.data.pipelineBasic3DDepthPrepass);
			break;
		case PIPELINE_FOLIAGE_3D:
			GPU::CmdBindPipeline(context, r.data.pipelineFoliage3DDepthPrepass);
			break;
		case PIPELINE_FONT_3D:
			GPU::CmdBindPipeline(context, r.data.pipelineFont3DDepthPrepass);
			break;
		default: break;
	}
	switch (pipeline) {
		case PIPELINE_BASIC_3D:
		case PIPELINE_FOLIAGE_3D:
			GPU::CmdBindVertexBuffer(context, r.data.vertexBuffer);
			GPU::CmdBindIndexBuffer(context, r.data.indexBuffer);
			GPU::CmdClearDescriptors(context);
			GPU::CmdBindUniformBuffer(context, r.data.worldInfoBuffer, 0, 0);
			GPU::CmdBindStorageBuffer(context, r.data.objectBuffer, 0, 1);
			GPU::CmdBindStorageBuffer(context, r.data.bonesBuffer, 0, 2);
			GPU::CmdBindImageArraySampler(context, r.data.textures, r.data.textureSampler, 0, 3);
			break;
		case PIPELINE_FONT_3D:
			GPU::CmdBindVertexBuffer(context, nullptr);
			GPU::CmdBindIndexBuffer(context, nullptr);
			GPU::CmdClearDescriptors(context);
			GPU::CmdBindUniformBuffer(context, r.data.worldInfoBuffer, 0, 0);
			GPU::CmdBindStorageBuffer(context, r.data.objectBuffer, 0, 1);
			GPU::CmdBindStorageBuffer(context, r.data.bonesBuffer, 0, 2);
			GPU::CmdBindImageArraySampler(context, r.data.textures, r.data.textureSampler, 0, 3);
			GPU::CmdBindUniformBufferArray(context, r.data.fontBuffers, 1, 0);
			GPU::CmdBindStorageBuffer(context, r.data.textBuffer, 1, 1);
			break;
		default: break;
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
	{ // Framebuffers
		data.depthPrepassFramebuffer = GPU::NewFramebuffer(data.device, "depthPrepass");
		data.rawFramebuffer = GPU::NewFramebuffer(data.device, "raw");
		data.depthImage = GPU::NewImage(data.device, "depthImage");
		vec2i ssaaNumerator = vec2i(Settings::ReadInt(Settings::sSupersamplingNumerator));
		vec2i ssaaDenominator = vec2i(Settings::ReadInt(Settings::sSupersamplingDenominator));
		GPU::ImageSetFormat(data.depthImage, GPU::ImageBits::D32, GPU::ImageComponentType::SFLOAT);
		GPU::ImageSetSizeToWindow(data.depthImage, data.window, ssaaNumerator, ssaaDenominator);
		data.rawImage = GPU::NewImage(data.device, "rawImage");
		GPU::ImageSetFormat(data.rawImage, GPU::ImageBits::R16G16B16A16, GPU::ImageComponentType::SFLOAT);
		GPU::ImageSetSizeToWindow(data.rawImage, data.window, ssaaNumerator, ssaaDenominator);
		GPU::ImageSetShaderUsage(data.rawImage, GPU::ShaderStage::FRAGMENT);
		i64 msaaSamples = Settings::ReadInt(Settings::sMultisamplingSamples);
		if (msaaSamples > 1) {
			data.msaaRawImage = GPU::NewImage(data.device, "msaaRawImage");
			GPU::ImageSetFormat(data.msaaRawImage, GPU::ImageBits::R16G16B16A16, GPU::ImageComponentType::SFLOAT);
			GPU::ImageSetSampleCount(data.msaaRawImage, msaaSamples);
			GPU::ImageSetSizeToWindow(data.msaaRawImage, data.window, ssaaNumerator, ssaaDenominator);
			data.msaaDepthImage = GPU::NewImage(data.device, "msaaDepthImage");
			GPU::ImageSetFormat(data.msaaDepthImage, GPU::ImageBits::D32, GPU::ImageComponentType::SFLOAT);
			GPU::ImageSetSampleCount(data.msaaDepthImage, msaaSamples);
			GPU::ImageSetSizeToWindow(data.msaaDepthImage, data.window, ssaaNumerator, ssaaDenominator);

			// We use MAX here to minimize edge artifacts involving our surface-aware AO denoise.
			GPU::FramebufferAddImageMultisampled(data.depthPrepassFramebuffer, data.msaaDepthImage, data.depthImage, false, true, GPU::ResolveMode::MAX);
			GPU::FramebufferAddImageMultisampled(data.rawFramebuffer, data.msaaRawImage, data.rawImage);
			GPU::FramebufferAddImage(data.rawFramebuffer, data.msaaDepthImage, true, false);
		} else {
			GPU::FramebufferAddImage(data.depthPrepassFramebuffer, data.depthImage);
			GPU::FramebufferAddImage(data.rawFramebuffer, data.rawImage);
			GPU::FramebufferAddImage(data.rawFramebuffer, data.depthImage, true, false);
		}

		data.windowFramebuffer = GPU::NewFramebuffer(data.device, "window");
		GPU::FramebufferAddWindow(data.windowFramebuffer, data.window);
	}
	{ // Concurrency, runtime CPU data pools
		worldInfo.ambientLightUp = vec3(0.001f);
		worldInfo.ambientLightDown = vec3(0.001f);
		if (data.concurrency < 1) {
			data.concurrency = 1;
		}
		data.drawingContexts.Resize(data.concurrency);
		data.debugVertices.Resize(MAX_DEBUG_VERTICES);
	}
	{ // Context
		data.contextMainRender = GPU::NewContext(data.device);
		data.contextDepthPrepass = GPU::NewContext(data.device);
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
		GPU::BufferSetShaderUsage(data.worldInfoBuffer, GPU::ShaderStage::VERTEX | GPU::ShaderStage::FRAGMENT);

		data.objectShaderInfos.Resize(1000);
		data.objectBuffer = GPU::NewStorageBuffer(data.device);
		GPU::BufferSetSize(data.objectBuffer, data.objectShaderInfos.size * sizeof(ObjectShaderInfo)).AzUnwrap();
		GPU::BufferSetShaderUsage(data.objectBuffer, GPU::ShaderStage::VERTEX | GPU::ShaderStage::FRAGMENT);

		data.bones.Resize(100, mat4(1.0f));
		data.bonesBuffer = GPU::NewStorageBuffer(data.device);
		GPU::BufferSetSize(data.bonesBuffer, data.bones.size * sizeof(mat4)).AzUnwrap();
		GPU::BufferSetShaderUsage(data.bonesBuffer, GPU::ShaderStage::VERTEX);

		data.textShaderInfos.Resize(100);
		data.textBuffer = GPU::NewStorageBuffer(data.device);
		GPU::BufferSetSize(data.textBuffer, data.textShaderInfos.size * sizeof(TextShaderInfo)).AzUnwrap();
		GPU::BufferSetShaderUsage(data.textBuffer, GPU::ShaderStage::VERTEX);

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
			GPU::ImageSetShaderUsage(data.textures[i], GPU::ShaderStage::FRAGMENT);
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
	GPU::Shader *fullscreenQuadVert = GPU::NewShader(data.device, "data/Az3D/shaders/FullscreenQuad.vert.spv", GPU::ShaderStage::VERTEX, "fullscreenQuadVert");
	{ // Pipelines
		GPU::Shader *debugLinesVert = GPU::NewShader(data.device, "data/Az3D/shaders/DebugLines.vert.spv", GPU::ShaderStage::VERTEX);
		GPU::Shader *debugLinesFrag = GPU::NewShader(data.device, "data/Az3D/shaders/DebugLines.frag.spv", GPU::ShaderStage::FRAGMENT);
		GPU::Shader *basic3DVert = GPU::NewShader(data.device, "data/Az3D/shaders/Basic3D.vert.spv", GPU::ShaderStage::VERTEX);
		GPU::Shader *basic3DFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Basic3D.frag.spv", GPU::ShaderStage::FRAGMENT);
		// TODO: If we decide we need texture-alpha-based pixel discards, we need one of these
		// GPU::Shader *basic3DDepthPrepassFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Basic3DDepthPrepass.frag.spv", GPU::ShaderStage::FRAGMENT);
		GPU::Shader *font3DVert = GPU::NewShader(data.device, "data/Az3D/shaders/Font3D.vert.spv", GPU::ShaderStage::VERTEX);
		GPU::Shader *font3DFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Font3D.frag.spv", GPU::ShaderStage::FRAGMENT);
		GPU::Shader *font3DDepthPrepassFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Font3DDepthPrepass.frag.spv", GPU::ShaderStage::FRAGMENT);

		data.pipelines.Resize(PIPELINE_COUNT);

		data.pipelines[PIPELINE_DEBUG_LINES] = GPU::NewGraphicsPipeline(data.device, "Debug Lines Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_DEBUG_LINES], {debugLinesVert, debugLinesFrag});
		GPU::PipelineAddVertexInputs(data.pipelines[PIPELINE_DEBUG_LINES], {
			GPU::ShaderValueType::VEC3, // pos
			GPU::ShaderValueType::VEC4, // color
		});
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_DEBUG_LINES], GPU::Topology::LINE_LIST);
		GPU::PipelineSetLineWidth(data.pipelines[PIPELINE_DEBUG_LINES], 2.0f);
		GPU::PipelineSetDepthCompareOp(data.pipelines[PIPELINE_DEBUG_LINES], GPU::CompareOp::LESS);
		GPU::PipelineAddPushConstantRange(data.pipelines[PIPELINE_DEBUG_LINES], 0, sizeof(f32), GPU::ShaderStage::FRAGMENT);


		data.pipelines[PIPELINE_BASIC_3D] = GPU::NewGraphicsPipeline(data.device, "Basic 3D Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_BASIC_3D], {basic3DVert, basic3DFrag});
		GPU::PipelineAddVertexInputs(data.pipelines[PIPELINE_BASIC_3D], vertexInputs);
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_BASIC_3D], GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetDepthTest(data.pipelines[PIPELINE_BASIC_3D], true);
		GPU::PipelineSetDepthWrite(data.pipelines[PIPELINE_BASIC_3D], false);
		GPU::PipelineSetDepthCompareOp(data.pipelines[PIPELINE_BASIC_3D], GPU::CompareOp::LESS_OR_EQUAL);
		GPU::PipelineSetCullingMode(data.pipelines[PIPELINE_BASIC_3D], GPU::CullingMode::BACK);
		GPU::PipelineSetWinding(data.pipelines[PIPELINE_BASIC_3D], GPU::Winding::COUNTER_CLOCKWISE);

		data.pipelineBasic3DDepthPrepass = GPU::NewGraphicsPipeline(data.device, "Basic 3D Depth Prepass Pipeline");
		GPU::PipelineAddShaders(data.pipelineBasic3DDepthPrepass, {basic3DVert /*, basic3DDepthPrepassFrag */});
		GPU::PipelineAddVertexInputs(data.pipelineBasic3DDepthPrepass, vertexInputs);
		GPU::PipelineSetTopology(data.pipelineBasic3DDepthPrepass, GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetDepthTest(data.pipelineBasic3DDepthPrepass, true);
		GPU::PipelineSetDepthWrite(data.pipelineBasic3DDepthPrepass, true);
		GPU::PipelineSetDepthCompareOp(data.pipelineBasic3DDepthPrepass, GPU::CompareOp::LESS);
		GPU::PipelineSetCullingMode(data.pipelineBasic3DDepthPrepass, GPU::CullingMode::BACK);
		GPU::PipelineSetWinding(data.pipelineBasic3DDepthPrepass, GPU::Winding::COUNTER_CLOCKWISE);

		data.pipelines[PIPELINE_FOLIAGE_3D] = GPU::NewGraphicsPipeline(data.device, "Foliage 3D Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_FOLIAGE_3D], {basic3DVert, basic3DFrag});
		GPU::PipelineAddVertexInputs(data.pipelines[PIPELINE_FOLIAGE_3D], vertexInputs);
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_FOLIAGE_3D], GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetDepthTest(data.pipelines[PIPELINE_FOLIAGE_3D], true);
		GPU::PipelineSetDepthWrite(data.pipelines[PIPELINE_FOLIAGE_3D], false);
		GPU::PipelineSetDepthCompareOp(data.pipelines[PIPELINE_FOLIAGE_3D], GPU::CompareOp::LESS_OR_EQUAL);
		GPU::PipelineSetCullingMode(data.pipelines[PIPELINE_FOLIAGE_3D], GPU::CullingMode::NONE);
		GPU::PipelineSetWinding(data.pipelines[PIPELINE_FOLIAGE_3D], GPU::Winding::COUNTER_CLOCKWISE);

		data.pipelineFoliage3DDepthPrepass = GPU::NewGraphicsPipeline(data.device, "Foliage 3D Depth Prepass Pipeline");
		GPU::PipelineAddShaders(data.pipelineFoliage3DDepthPrepass, {basic3DVert/*, basic3DDepthPrepassFrag */});
		GPU::PipelineAddVertexInputs(data.pipelineFoliage3DDepthPrepass, vertexInputs);
		GPU::PipelineSetTopology(data.pipelineFoliage3DDepthPrepass, GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetDepthTest(data.pipelineFoliage3DDepthPrepass, true);
		GPU::PipelineSetDepthWrite(data.pipelineFoliage3DDepthPrepass, true);
		GPU::PipelineSetDepthCompareOp(data.pipelineFoliage3DDepthPrepass, GPU::CompareOp::LESS_OR_EQUAL);
		GPU::PipelineSetCullingMode(data.pipelineFoliage3DDepthPrepass, GPU::CullingMode::NONE);
		GPU::PipelineSetWinding(data.pipelineFoliage3DDepthPrepass, GPU::Winding::COUNTER_CLOCKWISE);

		data.pipelines[PIPELINE_FONT_3D] = GPU::NewGraphicsPipeline(data.device, "Font 3D Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_FONT_3D], {font3DVert, font3DFrag});
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_FONT_3D], GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetDepthTest(data.pipelines[PIPELINE_FONT_3D], true);
		GPU::PipelineSetDepthWrite(data.pipelines[PIPELINE_FONT_3D], false);
		GPU::PipelineSetDepthCompareOp(data.pipelines[PIPELINE_FONT_3D], GPU::CompareOp::LESS_OR_EQUAL);
		GPU::PipelineSetCullingMode(data.pipelines[PIPELINE_FONT_3D], GPU::CullingMode::NONE);
		GPU::PipelineSetMultisampleShading(data.pipelines[PIPELINE_FONT_3D], true);

		data.pipelineFont3DDepthPrepass = GPU::NewGraphicsPipeline(data.device, "Font 3D Depth Prepass Pipeline");
		GPU::PipelineAddShaders(data.pipelineFont3DDepthPrepass, {font3DVert, font3DDepthPrepassFrag});
		GPU::PipelineSetTopology(data.pipelineFont3DDepthPrepass, GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetDepthTest(data.pipelineFont3DDepthPrepass, true);
		GPU::PipelineSetDepthWrite(data.pipelineFont3DDepthPrepass, true);
		GPU::PipelineSetDepthCompareOp(data.pipelineFont3DDepthPrepass, GPU::CompareOp::LESS);
		GPU::PipelineSetCullingMode(data.pipelineFont3DDepthPrepass, GPU::CullingMode::NONE);
		GPU::PipelineSetMultisampleShading(data.pipelineFont3DDepthPrepass, true);

		for (i32 i = 1; i < PIPELINE_COUNT; i++) {
			if (i == PIPELINE_BASIC_3D_VSM || i == PIPELINE_FONT_3D_VSM || i == PIPELINE_FOLIAGE_3D_VSM) continue;
			GPU::PipelineSetBlendMode(data.pipelines[i], {GPU::BlendMode::TRANSPARENT, true});
		}
	}
	{ // Shadow maps
		constexpr i32 dims = 2048;
		data.contextShadowMap = GPU::NewContext(data.device, "VSM Context");

		GPU::Image *shadowMapMSAAImage = GPU::NewImage(data.device, "VSM MSAA Image");
		data.shadowMapImage = GPU::NewImage(data.device, "VSM Image");
		GPU::ImageSetFormat(shadowMapMSAAImage, GPU::ImageBits::R32G32, GPU::ImageComponentType::SFLOAT);
		GPU::ImageSetSize(shadowMapMSAAImage, dims, dims);
		GPU::ImageSetSampleCount(shadowMapMSAAImage, 4);

		GPU::ImageSetFormat(data.shadowMapImage, GPU::ImageBits::R32G32, GPU::ImageComponentType::SFLOAT);
		GPU::ImageSetSize(data.shadowMapImage, dims, dims);
		GPU::ImageSetShaderUsage(data.shadowMapImage, GPU::ShaderStage::FRAGMENT);
		GPU::ImageSetMipmapping(data.shadowMapImage, true);

		data.framebufferShadowMaps = GPU::NewFramebuffer(data.device, "VSM Framebuffer");
		GPU::FramebufferAddImageMultisampled(data.framebufferShadowMaps, shadowMapMSAAImage, data.shadowMapImage);

		GPU::Shader *vsmVert = GPU::NewShader(data.device, "data/Az3D/shaders/Basic3D_VSM.vert.spv", GPU::ShaderStage::VERTEX, "VSM Vertex Shader");
		GPU::Shader *vsmFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Basic3D_VSM.frag.spv", GPU::ShaderStage::FRAGMENT, "VSM Fragment Shader");
		data.pipelines[PIPELINE_BASIC_3D_VSM] = GPU::NewGraphicsPipeline(data.device, "VSM Basic Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_BASIC_3D_VSM], {vsmVert, vsmFrag});
		GPU::PipelineAddVertexInputs(data.pipelines[PIPELINE_BASIC_3D_VSM], vertexInputs);
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_BASIC_3D_VSM], GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetCullingMode(data.pipelines[PIPELINE_BASIC_3D_VSM], GPU::CullingMode::BACK);
		GPU::PipelineSetWinding(data.pipelines[PIPELINE_BASIC_3D_VSM], GPU::Winding::COUNTER_CLOCKWISE);
		GPU::PipelineSetBlendMode(data.pipelines[PIPELINE_BASIC_3D_VSM], GPU::BlendMode::MAX);

		data.pipelines[PIPELINE_FOLIAGE_3D_VSM] = GPU::NewGraphicsPipeline(data.device, "VSM Foliage Pipeline");
		GPU::PipelineAddShaders(data.pipelines[PIPELINE_FOLIAGE_3D_VSM], {vsmVert, vsmFrag});
		GPU::PipelineAddVertexInputs(data.pipelines[PIPELINE_FOLIAGE_3D_VSM], vertexInputs);
		GPU::PipelineSetTopology(data.pipelines[PIPELINE_FOLIAGE_3D_VSM], GPU::Topology::TRIANGLE_LIST);
		GPU::PipelineSetBlendMode(data.pipelines[PIPELINE_FOLIAGE_3D_VSM], GPU::BlendMode::MAX);

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
		GPU::ImageSetShaderUsage(data.shadowMapConvolutionImage, GPU::ShaderStage::FRAGMENT);

		data.framebufferConvolution[0] = GPU::NewFramebuffer(data.device, "VSM Convolution Framebuffer 0");
		data.framebufferConvolution[1] = GPU::NewFramebuffer(data.device, "VSM Convolution Framebuffer 1");
		GPU::FramebufferAddImage(data.framebufferConvolution[0], data.shadowMapConvolutionImage);
		GPU::FramebufferAddImage(data.framebufferConvolution[1], data.shadowMapImage);

		GPU::Shader *convolutionVert = fullscreenQuadVert;
		GPU::Shader *convolutionFrag = GPU::NewShader(data.device, "data/Az3D/shaders/VSMConvolution.frag.spv", GPU::ShaderStage::FRAGMENT);

		data.pipelineShadowMapConvolution = GPU::NewGraphicsPipeline(data.device, "VSM Convolution Pipeline");
		GPU::PipelineAddShaders(data.pipelineShadowMapConvolution, {convolutionVert, convolutionFrag});
		GPU::PipelineSetTopology(data.pipelineShadowMapConvolution, GPU::Topology::TRIANGLE_FAN);
		GPU::PipelineAddPushConstantRange(data.pipelineShadowMapConvolution, 0, sizeof(vec2), GPU::ShaderStage::FRAGMENT);

		data.shadowMapSampler = GPU::NewSampler(data.device, "VSM Sampler");
		GPU::SamplerSetAddressMode(data.shadowMapSampler, GPU::AddressMode::CLAMP_TO_BORDER, GPU::AddressMode::CLAMP_TO_BORDER);
		GPU::SamplerSetBorderColor(data.shadowMapSampler, true, false, true);
	}
	{ // Ambient Occlusion
		vec2i sizeNumerator = Settings::ReadInt(Settings::sSSAONumerator);
		vec2i sizeDenominator = Settings::ReadInt(Settings::sSSAODenominator);

		GPU::ImageSetShaderUsage(data.depthImage, GPU::ShaderStage::FRAGMENT);
		data.aoDepthImageSampler = GPU::NewSampler(data.device, "aoDepthImageSampler");
		GPU::SamplerSetFiltering(data.aoDepthImageSampler, GPU::Filter::NEAREST, GPU::Filter::NEAREST);
		GPU::SamplerSetAddressMode(data.aoDepthImageSampler, GPU::AddressMode::CLAMP_TO_EDGE, GPU::AddressMode::CLAMP_TO_EDGE);

		data.aoImage = GPU::NewImage(data.device, "aoImage");
		GPU::ImageSetFormat(data.aoImage, GPU::ImageBits::R8, GPU::ImageComponentType::UNORM);
		GPU::ImageSetSizeToWindow(data.aoImage, data.window, sizeNumerator, sizeDenominator);
		GPU::ImageSetShaderUsage(data.aoImage, GPU::ShaderStage::FRAGMENT);

		data.aoFramebuffer = GPU::NewFramebuffer(data.device, "aoFramebuffer");
		GPU::FramebufferAddImage(data.aoFramebuffer, data.aoImage);

		data.aoSmoothedImage = GPU::NewImage(data.device, "aoSmoothedImage");
		GPU::ImageSetFormat(data.aoSmoothedImage, GPU::ImageBits::R8, GPU::ImageComponentType::UNORM);
		GPU::ImageSetSizeToWindow(data.aoSmoothedImage, data.window, sizeNumerator, sizeDenominator);
		GPU::ImageSetShaderUsage(data.aoSmoothedImage, GPU::ShaderStage::FRAGMENT);

		data.aoSmoothedFramebuffer = GPU::NewFramebuffer(data.device, "aoSmoothedFramebuffer");
		GPU::FramebufferAddImage(data.aoSmoothedFramebuffer, data.aoSmoothedImage);

		data.aoImageSampler = GPU::NewSampler(data.device, "aoImageSampler");
		GPU::SamplerSetAddressMode(data.aoImageSampler, GPU::AddressMode::CLAMP_TO_BORDER, GPU::AddressMode::CLAMP_TO_BORDER);
		GPU::SamplerSetBorderColor(data.aoImageSampler, false, true, true);

		GPU::Shader *aoFromDepthVert = fullscreenQuadVert;
		GPU::Shader *aoFromDepthFrag = GPU::NewShader(data.device, "data/Az3D/shaders/AOFromDepth.frag.spv", GPU::ShaderStage::FRAGMENT);
		data.pipelineAOFromDepth = GPU::NewGraphicsPipeline(data.device, "AO From Depth Pipeline");
		GPU::PipelineAddShaders(data.pipelineAOFromDepth, {aoFromDepthVert, aoFromDepthFrag});
		GPU::PipelineSetTopology(data.pipelineAOFromDepth, GPU::Topology::TRIANGLE_FAN);

		GPU::Shader *aoConvolutionVert = fullscreenQuadVert;
		GPU::Shader *aoConvolutionFrag = GPU::NewShader(data.device, "data/Az3D/shaders/AOConvolution.frag.spv", GPU::ShaderStage::FRAGMENT);
		data.pipelineAOConvolution = GPU::NewGraphicsPipeline(data.device, "AO Convolution Pipeline");
		GPU::PipelineAddShaders(data.pipelineAOConvolution, {aoConvolutionVert, aoConvolutionFrag});
		GPU::PipelineSetTopology(data.pipelineAOConvolution, GPU::Topology::TRIANGLE_FAN);
	}
	{ // Bloom
		constexpr i32 startingHeight = 900;
		vec2i baseSize = vec2i(startingHeight * sys->window.width / sys->window.height, startingHeight);
		for (i32 layer = 0; layer < bloomLayers; layer++) {
			for (i32 j = 0; j < 2; j++) {
				i32 i = layer * 2 + j;
				data.bloomImage[i] = GPU::NewImage(data.device, Stringify("bloomImg", layer, "[", j, "]"));
				GPU::ImageSetSize(data.bloomImage[i], baseSize.x >> layer, baseSize.y >> layer);
				GPU::ImageSetFormat(data.bloomImage[i], GPU::ImageBits::B10G11R11, GPU::ImageComponentType::UFLOAT);
				GPU::ImageSetShaderUsage(data.bloomImage[i], GPU::ShaderStage::FRAGMENT);
				data.bloomFramebuffer[i] = GPU::NewFramebuffer(data.device, Stringify("bloomFB", layer, "[", j, "]"));
				GPU::FramebufferAddImage(data.bloomFramebuffer[i], data.bloomImage[i], true);
			}
			GPU::ImageSetTransferUsage(data.bloomImage[layer * 2], true, true);
		}
		GPU::ImageSetTransferUsage(data.rawImage, None, true);
		data.bloomSampler = GPU::NewSampler(data.device, "Bloom Sampler");

		GPU::Shader *bloomBlurVert = fullscreenQuadVert;
		GPU::Shader *bloomBlurFrag = GPU::NewShader(data.device, "data/Az3D/shaders/BloomConvolution.frag.spv", GPU::ShaderStage::FRAGMENT);
		data.pipelineBloomConvolution = GPU::NewGraphicsPipeline(data.device, "Bloom Convolution Pipeline");
		GPU::PipelineAddShaders(data.pipelineBloomConvolution, {bloomBlurVert, bloomBlurFrag});
		GPU::PipelineSetTopology(data.pipelineBloomConvolution, GPU::Topology::TRIANGLE_FAN);
		GPU::PipelineAddPushConstantRange(data.pipelineBloomConvolution, 0, sizeof(vec2), GPU::ShaderStage::FRAGMENT);

		GPU::Shader *bloomCombineVert = fullscreenQuadVert;
		GPU::Shader *bloomCombineFrag = GPU::NewShader(data.device, "data/Az3D/shaders/BloomCombine.frag.spv", GPU::ShaderStage::FRAGMENT);
		data.pipelineBloomCombine = GPU::NewGraphicsPipeline(data.device, "Bloom Combine Pipeline");
		GPU::PipelineAddShaders(data.pipelineBloomCombine, {bloomCombineVert, bloomCombineFrag});
		GPU::PipelineSetTopology(data.pipelineBloomCombine, GPU::Topology::TRIANGLE_FAN);
		GPU::PipelineAddPushConstantRange(data.pipelineBloomCombine, 0, sizeof(u32), GPU::ShaderStage::FRAGMENT);
		GPU::PipelineSetBlendMode(data.pipelineBloomCombine, GPU::BlendMode::ADDITION);
	}
	{ // Final image composition
		GPU::Shader *compositionVert = fullscreenQuadVert;
		GPU::Shader *compositionFrag = GPU::NewShader(data.device, "data/Az3D/shaders/Composition.frag.spv", GPU::ShaderStage::FRAGMENT, "compositionFrag");
		data.pipelineCompositing = GPU::NewGraphicsPipeline(data.device, "Composition Pipeline");
		GPU::PipelineAddShaders(data.pipelineCompositing, {compositionVert, compositionFrag});
		GPU::PipelineSetTopology(data.pipelineCompositing, GPU::Topology::TRIANGLE_FAN);
		GPU::PipelineAddPushConstantRange(data.pipelineCompositing, 0, sizeof(float), GPU::ShaderStage::FRAGMENT);

		data.rawSampler = GPU::NewSampler(data.device, "Raw Image Sampler");
		GPU::SamplerSetAddressMode(data.rawSampler, GPU::AddressMode::CLAMP_TO_BORDER, GPU::AddressMode::CLAMP_TO_BORDER);
		GPU::SamplerSetBorderColor(data.rawSampler, true, false, true);
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
	vec3 corners[8];
	{
		f32 prevFarClip = camera.farClip;
		camera.farClip *= 0.5f;
		GetCameraFrustumCorners(camera, corners, corners+4);
		camera.farClip = prevFarClip;
	}
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
			GPU::ImageSetShaderUsage(image, GPU::ShaderStage::FRAGMENT);
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
		if (GPU::ImageSetSize(fontTexture, font.fontBuilder.dimensions.x, font.fontBuilder.dimensions.y)) {
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
			GPU::BufferSetShaderUsage(data.fontBuffers.Back(), GPU::ShaderStage::VERTEX);
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

bool Manager::UpdateWorldInfo(GPU::Context *context) {
	Camera &activeCam = debugCameraActive ? debugCamera : camera;
	// Update camera matrix
	worldInfo.view = mat4::Camera(activeCam.pos, activeCam.forward, activeCam.up);
	// worldInfo.proj = mat4::Ortho(10.0f, 10.0f * screenSize.y / screenSize.x, camera.nearClip, camera.farClip);
	worldInfo.proj = mat4::Perspective(activeCam.fov, 1.0f / camera.aspectRatio, activeCam.nearClip, activeCam.farClip);
	worldInfo.viewProj = worldInfo.proj * worldInfo.view;
	// Use regular cam to see the lighting as though it was done from the normal camera's POV
	worldInfo.eyePos = camera.pos;
	worldInfo.fogColor = sRGBToLinear(backgroundRGB);
	worldInfo.ambientLightUp = worldInfo.fogColor * 0.5f;
	worldInfo.ambientLightDown = vec3(0.491f, 0.357f, 0.205f) * 0.5f;
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
		GPU::CmdCopyDataToBuffer(context, data.textBuffer, data.textShaderInfos.data, 0, copySize).AzUnwrap();
	}
	return true;
}

bool Manager::UpdateDebugLines(GPU::Context *context) {
	if (!Settings::ReadBool(Settings::sDebugLines)) return true;
	data.debugVertices.ClearSoft();
	for (DrawingContext &context : data.drawingContexts) {
		data.debugVertices.Append(context.debugLines);
	}
	if (data.debugVertices.size < 2) return true;

	BindPipeline(data.contextMainRender, PIPELINE_DEBUG_LINES);

	GPU::CmdSetDepthTestEnable(data.contextMainRender, false);
	f32 alpha = 0.2f;
	GPU::CmdPushConstants(data.contextMainRender, &alpha, 0, sizeof(f32));
	GPU::CmdDraw(data.contextMainRender, data.debugVertices.size, 0);
	GPU::CmdSetDepthTestEnable(data.contextMainRender, true);
	alpha = 1.0f;
	GPU::CmdPushConstants(data.contextMainRender, &alpha, 0, sizeof(f32));
	GPU::CmdDraw(data.contextMainRender, data.debugVertices.size, 0);

	GPU::CmdCopyDataToBuffer(context, data.debugVertexBuffer, data.debugVertices.data, 0, min(data.debugVertices.size, MAX_DEBUG_VERTICES) * sizeof(DebugVertex)).AzUnwrap();
	return true;
}

void Manager::UpdateDebugCamera() {
	if (sys->Pressed(KC_KEY_F3, true)) {
		debugCameraActive = !debugCameraActive;
		if (debugCameraActive) {
			debugCamera = camera;
			debugCamera.farClip *= 2.0f;
			debugCameraFacingDiff = vec2(0.0f);
		}
	}
	if (!debugCameraActive) return;
	DrawCamera(data.drawingContexts[0], camera, vec4(vec3(0.0f), 1.0f));

	vec2i center = vec2i(sys->window.width / 2, sys->window.height / 2);
	if (sys->Pressed(KC_KEY_TAB, true)) {
		debugCameraFly = !debugCameraFly;
		sys->window.HideCursor(debugCameraFly);
		sys->input.cursor = center;
	}
	if (!debugCameraFly) return;
	f32 speed = sys->Down(KC_KEY_LEFTSHIFT, true) ? 10.0f : 2.0f;
	vec3 camRight = normalize(cross(debugCamera.forward, vec3(0.0f, 0.0f, 1.0f)));
	debugCamera.up = orthogonalize(vec3(0.0f, 0.0f, 1.0f), debugCamera.forward);
	{
		f32 sensitivity = debugCamera.fov.value() / 60.0f / screenSize.x;
		debugCameraFacingDiff.x -= f32(sys->input.cursor.x - center.x) * sensitivity;
		debugCameraFacingDiff.y -= f32(sys->input.cursor.y - center.y) * sensitivity;
		sys->window.MoveCursor(center.x, center.y);
	}
	{
		vec2 diff = debugCameraFacingDiff * decayFactor(0.015f, sys->timestep);
		debugCameraFacingDiff -= diff;
		quat zRot = quat::Rotation(
			diff.x,
			debugCamera.up
		);
		quat xRot = quat::Rotation(
			diff.y,
			camRight
		);
		quat rot = zRot * xRot;
		debugCamera.forward = rot.RotatePoint(debugCamera.forward);
		camRight = normalize(cross(debugCamera.forward, vec3(0.0f, 0.0f, 1.0f)));
		debugCamera.up = orthogonalize(vec3(0.0f, 0.0f, 1.0f), debugCamera.forward);
	}
	if (sys->Down(KC_KEY_W, true)) {
		debugCamera.pos += speed * sys->timestep * debugCamera.forward;
	}
	if (sys->Down(KC_KEY_S, true)) {
		debugCamera.pos -= speed * sys->timestep * debugCamera.forward;
	}
	if (sys->Down(KC_KEY_D, true)) {
		debugCamera.pos += speed * sys->timestep * camRight;
	}
	if (sys->Down(KC_KEY_A, true)) {
		debugCamera.pos -= speed * sys->timestep * camRight;
	}
	if (sys->Down(KC_KEY_SPACE, true)) {
		debugCamera.pos += speed * sys->timestep * debugCamera.up;
	}
	if (sys->Down(KC_KEY_V, true)) {
		debugCamera.pos -= speed * sys->timestep * debugCamera.up;
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

	GPU::PipelineSetLineWidth(data.pipelines[PIPELINE_DEBUG_LINES], 2.0f / 96.0f * (f32)sys->window.dpi);

	GPU::Context *cmr = data.contextMainRender;

	screenSize = vec2((f32)max((u16)1, sys->window.width), (f32)max((u16)1, sys->window.height));
	aspectRatio = screenSize.y / screenSize.x;
	camera.aspectRatio = aspectRatio;
	debugCamera.aspectRatio = aspectRatio;

	{ // Bloom images aspect ratio
		i32 scaleDenominator = 1;
		for (i32 layer = 0; layer < bloomLayers; layer++) {
			for (i32 j = 0; j < 2; j++) {
				i32 i = layer * 2 + j;
				if (GPU::ImageSetSize(
					data.bloomImage[i],
					900 * sys->window.width / sys->window.height / scaleDenominator,
					900 / scaleDenominator
				)) {
					GPU::ImageRecreate(data.bloomImage[i]).AzUnwrap();
				}
			}
			scaleDenominator *= 2;
		}
	}

	// Clear lights so we get new ones this frame
	lights.size = 0;

	for (DrawingContext &context : data.drawingContexts) {
		context.thingsToDraw.ClearSoft();
		context.debugLines.ClearSoft();
	}
	// Gather all the thingsToDraw
	sys->Draw(data.drawingContexts);

	UpdateDebugCamera();

	GPU::ContextBeginRecording(data.contextTransfer).AzUnwrap();
	if (updateFontMemory) {
		if (!UpdateFonts(data.contextTransfer)) return false;
	}
	if (!UpdateWorldInfo(data.contextTransfer)) return false;

	data.objectShaderInfos.ClearSoft();
	data.bones.ClearSoft();
	data.textShaderInfos.ClearSoft();
	az::HashMap<Animation::ArmatureAction, u32> actions;
	{ // Sorting draw calls
		Array<DrawCallInfo> allDrawCalls;
		for (DrawingContext &context : data.drawingContexts) {
			allDrawCalls.Append(context.thingsToDraw);
		}
		// Do frustum-based culling
		Frustum frustum = GetFrustumFromCamera(camera);
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
		// Actually draw them

		// Shadow Maps
		GPU::ContextBeginRecording(data.contextShadowMap).AzUnwrap();
		GPU::CmdImageTransitionLayout(data.contextShadowMap, data.shadowMapImage, GPU::ImageLayout::UNDEFINED, GPU::ImageLayout::ATTACHMENT);
		GPU::CmdBindFramebuffer(data.contextShadowMap, data.framebufferShadowMaps);
		BindPipeline(data.contextShadowMap, PIPELINE_BASIC_3D_VSM);
		GPU::CmdClearColorAttachment(data.contextShadowMap, vec4(0.0));

		// Depth Prepass
		GPU::ContextBeginRecording(data.contextDepthPrepass).AzUnwrap();
		GPU::CmdImageTransitionLayout(data.contextDepthPrepass, data.depthImage, GPU::ImageLayout::UNDEFINED, GPU::ImageLayout::ATTACHMENT);
		GPU::CmdBindFramebuffer(data.contextDepthPrepass, data.depthPrepassFramebuffer);
		BindPipelineDepthPrepass(data.contextDepthPrepass, PIPELINE_BASIC_3D);
		vec2 viewport = GPU::ImageGetSize(data.depthImage);
		GPU::CmdSetViewportAndScissor(data.contextDepthPrepass, viewport.x, viewport.y);
		GPU::CmdClearDepthAttachment(data.contextDepthPrepass, 1.0f);

		GPU::ContextBeginRecording(cmr).AzUnwrap();
		// Ambient Occlusion
		GPU::CmdImageTransitionLayout(cmr, data.aoImage, GPU::ImageLayout::UNDEFINED, GPU::ImageLayout::ATTACHMENT);
		GPU::CmdImageTransitionLayout(cmr, data.depthImage, GPU::ImageLayout::ATTACHMENT, GPU::ImageLayout::SHADER_READ);
		GPU::CmdBindFramebuffer(cmr, data.aoFramebuffer);
		GPU::CmdBindUniformBuffer(cmr, data.worldInfoBuffer, 0, 0);
		GPU::CmdBindImageSampler(cmr, data.depthImage, data.aoDepthImageSampler, 0, 1);
		GPU::CmdBindPipeline(cmr, data.pipelineAOFromDepth);
		GPU::CmdCommitBindings(cmr).AzUnwrap();
		GPU::CmdDraw(cmr, 4, 0);
		GPU::CmdFinishFramebuffer(cmr);
		// AO Denoise
		GPU::CmdImageTransitionLayout(cmr, data.aoSmoothedImage, GPU::ImageLayout::UNDEFINED, GPU::ImageLayout::ATTACHMENT);
		GPU::CmdImageTransitionLayout(cmr, data.aoImage, GPU::ImageLayout::ATTACHMENT, GPU::ImageLayout::SHADER_READ);
		GPU::CmdBindFramebuffer(cmr, data.aoSmoothedFramebuffer);
		GPU::CmdBindUniformBuffer(cmr, data.worldInfoBuffer, 0, 0);
		GPU::CmdBindImageSampler(cmr, data.depthImage, data.aoDepthImageSampler, 0, 1);
		GPU::CmdBindImageSampler(cmr, data.aoImage, data.aoImageSampler, 0, 2);
		GPU::CmdBindPipeline(cmr, data.pipelineAOConvolution);
		GPU::CmdCommitBindings(cmr).AzUnwrap();
		GPU::CmdDraw(cmr, 4, 0);
		GPU::CmdFinishFramebuffer(cmr);

		// Full scene render
		GPU::CmdImageTransitionLayout(cmr, data.depthImage, GPU::ImageLayout::SHADER_READ, GPU::ImageLayout::ATTACHMENT);
		if (data.msaaDepthImage) {
			// We're in TRANSFER_SRC because the depth pre-pass resolves the multisampled image
			GPU::CmdImageTransitionLayout(cmr, data.msaaDepthImage, GPU::ImageLayout::TRANSFER_SRC, GPU::ImageLayout::ATTACHMENT);
		}
		GPU::CmdImageTransitionLayout(cmr, data.rawImage, GPU::ImageLayout::UNDEFINED, GPU::ImageLayout::ATTACHMENT);
		GPU::CmdImageTransitionLayout(cmr, data.aoSmoothedImage, GPU::ImageLayout::ATTACHMENT, GPU::ImageLayout::SHADER_READ);
		GPU::CmdBindFramebuffer(cmr, data.rawFramebuffer);
		BindPipeline(cmr, PIPELINE_BASIC_3D);
		GPU::CmdSetViewportAndScissor(cmr, viewport.x, viewport.y);
		GPU::CmdClearColorAttachment(cmr, vec4(sRGBToLinear(backgroundRGB), 1.0f));
		// We specifically DON'T clear the depth attachment, because the values from the depth prepass help reduce fragment overdraw.

		PipelineIndex currentPipelineVSM = PIPELINE_BASIC_3D;
		PipelineIndex currentPipelineDepthPrepass = PIPELINE_BASIC_3D;
		PipelineIndex currentPipeline = PIPELINE_BASIC_3D;
		for (DrawCallInfo &drawCall : allDrawCalls) {
			if (drawCall.culled && !drawCall.castsShadows) continue;
			u32 bonesOffset = 0;
			if (drawCall.armatureAction.Exists()) {
				u32 &dstOffset = actions.ValueOf(drawCall.armatureAction.ValueUnchecked(), (u32)data.bones.size);
				if (dstOffset == (u32)data.bones.size) {
					// Actually make the bones!
					Animation::AnimateArmature(data.bones, drawCall.armatureAction.ValueUnchecked(), drawCall.transforms[0], drawCall.ikParameters);
				}
				bonesOffset = dstOffset;
			}
			if (!drawCall.culled && drawCall.pipeline != currentPipeline) {
				BindPipeline(cmr, drawCall.pipeline);
				currentPipeline = drawCall.pipeline;
			}
			if (!drawCall.culled && drawCall.opaque && drawCall.pipeline != currentPipelineDepthPrepass) {
				BindPipelineDepthPrepass(data.contextDepthPrepass, drawCall.pipeline);
				currentPipelineDepthPrepass = drawCall.pipeline;
			}
			if (drawCall.castsShadows && drawCall.pipeline != currentPipelineVSM) {
				BindPipeline(data.contextShadowMap, drawCall.pipeline+1);
				currentPipelineVSM = drawCall.pipeline;
			}
			if (drawCall.textsToDraw.size > 0) {
				i32 objectIndex = data.objectShaderInfos.size;
				data.objectShaderInfos.Append(ObjectShaderInfo{drawCall.transforms[0], drawCall.material, 0});
				for (DrawTextInfo &info : drawCall.textsToDraw) {
					info.shaderInfo.objectIndex = objectIndex;
					i32 textIndex = data.textShaderInfos.size;
					data.textShaderInfos.Append(info.shaderInfo);
					if (!drawCall.culled) {
						if (drawCall.opaque) {
							GPU::CmdDraw(data.contextDepthPrepass, info.glyphCount * 6, 0, 1, textIndex);
						}
						GPU::CmdDraw(cmr, info.glyphCount * 6, 0, 1, textIndex);
					}
					if (drawCall.castsShadows) {
						GPU::CmdDraw(data.contextShadowMap, info.glyphCount * 6, 0, 1, textIndex);
					}
				}
			} else {
				drawCall.instanceStart = data.objectShaderInfos.size;
				i32 prevSize = data.objectShaderInfos.size;
				data.objectShaderInfos.Resize(data.objectShaderInfos.size + drawCall.transforms.size);
				for (i32 i = 0; i < drawCall.transforms.size; i++) {
					data.objectShaderInfos[prevSize+i] = ObjectShaderInfo{drawCall.transforms[i], drawCall.material, bonesOffset};
				}
				if (!drawCall.culled) {
					if (drawCall.opaque) {
						GPU::CmdDrawIndexed(data.contextDepthPrepass, drawCall.indexCount, drawCall.indexStart, 0, drawCall.instanceCount, drawCall.instanceStart);
					}
					GPU::CmdDrawIndexed(cmr, drawCall.indexCount, drawCall.indexStart, 0, drawCall.instanceCount, drawCall.instanceStart);
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
			waitSemaphores.Append(GPU::ContextGetPreviousSemaphore(cmr, 1));
		}
		// Signal 2 semaphores, one for the shadow map, and the second for the depth prepass
		if (auto result = GPU::SubmitCommands(data.contextTransfer, 2, waitSemaphores); result.isError) {
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

	// Signal 1 semaphore for the main render
	// Wait on 1st transfer semaphore
	if (auto result = GPU::SubmitCommands(data.contextShadowMap, 1, {GPU::ContextGetCurrentSemaphore(data.contextTransfer)}); result.isError) {
		error = "Failed to submit shadow map commands: " + result.error;
		return false;
	}

	GPU::ContextEndRecording(data.contextDepthPrepass).AzUnwrap();

	// Signal 1 semaphore for the main render
	// Wait on the 2nd transfer semaphore
	if (auto result = GPU::SubmitCommands(data.contextDepthPrepass, 1, {GPU::ContextGetCurrentSemaphore(data.contextTransfer, 1)}); result.isError) {
		error = "Failed to draw depth prepass: " + result.error;
		return false;
	}

	GPU::CmdFinishFramebuffer(cmr);

	{ // Bloom
		vec2 right = vec2(1.0f, 0.0f);
		vec2 down = vec2(0.0f, 1.0f);
		GPU::Image *lastImage = data.rawImage;
		// Blit and blur all the layers
		for (i32 layer = 0; layer < bloomLayers; layer++) {
			i32 i0 = layer * 2;
			i32 i1 = layer * 2 + 1;
			GPU::CmdImageBlit(cmr, data.bloomImage[i0], 0, GPU::ImageLayout::UNDEFINED, GPU::ImageLayout::SHADER_READ, lastImage, 0, GPU::ImageLayout::ATTACHMENT, GPU::ImageLayout::SHADER_READ);
			GPU::CmdImageTransitionLayout(cmr, data.bloomImage[i1], GPU::ImageLayout::UNDEFINED, GPU::ImageLayout::ATTACHMENT);
			GPU::CmdBindFramebuffer(cmr, data.bloomFramebuffer[i1]);
			GPU::CmdBindPipeline(cmr, data.pipelineBloomConvolution);
			GPU::CmdBindImageSampler(cmr, data.bloomImage[i0], data.bloomSampler, 0, 0);
			GPU::CmdCommitBindings(cmr).AzUnwrap();
			GPU::CmdPushConstants(cmr, &right, 0, sizeof(vec2));
			GPU::CmdDraw(cmr, 4, 0);
			GPU::CmdFinishFramebuffer(cmr);
			GPU::CmdImageTransitionLayout(cmr, data.bloomImage[i0], GPU::ImageLayout::SHADER_READ, GPU::ImageLayout::ATTACHMENT);
			GPU::CmdImageTransitionLayout(cmr, data.bloomImage[i1], GPU::ImageLayout::ATTACHMENT, GPU::ImageLayout::SHADER_READ);
			GPU::CmdBindFramebuffer(cmr, data.bloomFramebuffer[i0]);
			GPU::CmdBindPipeline(cmr, data.pipelineBloomConvolution);
			GPU::CmdBindImageSampler(cmr, data.bloomImage[i1], data.bloomSampler, 0, 0);
			GPU::CmdCommitBindings(cmr).AzUnwrap();
			GPU::CmdPushConstants(cmr, &down, 0, sizeof(vec2));
			GPU::CmdDraw(cmr, 4, 0);
			GPU::CmdFinishFramebuffer(cmr);
			lastImage = data.bloomImage[i0];
		}
		// Combine images up from the smallest so we can sample one image in Composition
		for (i32 layer = bloomLayers-2; layer >= 0; layer--) {
			i32 i0 = layer * 2;
			i32 i2 = (layer + 1) * 2;
			GPU::CmdImageTransitionLayout(cmr, data.bloomImage[i2], GPU::ImageLayout::ATTACHMENT, GPU::ImageLayout::SHADER_READ);
			GPU::CmdImageTransitionLayout(cmr, data.bloomImage[i0], GPU::ImageLayout::SHADER_READ, GPU::ImageLayout::ATTACHMENT);
			GPU::CmdBindFramebuffer(cmr, data.bloomFramebuffer[i0]);
			GPU::CmdBindPipeline(cmr, data.pipelineBloomCombine);
			GPU::CmdBindImageSampler(cmr, data.bloomImage[i2], data.bloomSampler, 0, 0);
			GPU::CmdCommitBindings(cmr).AzUnwrap();
			GPU::CmdDraw(cmr, 4, 0);
			GPU::CmdFinishFramebuffer(cmr);
		}
		GPU::CmdImageTransitionLayout(cmr, data.bloomImage[0], GPU::ImageLayout::ATTACHMENT, GPU::ImageLayout::SHADER_READ);
	}
	{ // Composition
		f32 bloomIntensity = (f32)Settings::ReadReal(Settings::sBloomIntensity) * 0.1f;
		GPU::CmdBindFramebuffer(cmr, data.windowFramebuffer);
		GPU::CmdBindPipeline(cmr, data.pipelineCompositing);
		GPU::CmdBindImageSampler(cmr, data.rawImage, data.rawSampler, 0, 0);
		GPU::CmdBindImageSampler(cmr, data.bloomImage[0], data.bloomSampler, 0, 1);
		GPU::CmdCommitBindings(cmr).AzUnwrap();
		GPU::CmdPushConstants(cmr, &bloomIntensity, 0, sizeof(f32));
		GPU::CmdDraw(cmr, 4, 0);
	}

	GPU::ContextEndRecording(cmr).AzUnwrap();

	// Signal 2 semaphores: one for present and the other for the next frame's data transfer
	// Wait on the depth prepass and shadow map semaphores
	if (auto result = GPU::SubmitCommands(cmr, 2, {GPU::ContextGetCurrentSemaphore(data.contextDepthPrepass), GPU::ContextGetCurrentSemaphore(data.contextShadowMap)}); result.isError) {
		error = "Failed to draw commands: " + result.error;
		return false;
	}
	return true;
}

bool Manager::Present() {
	AZCORE_PROFILING_SCOPED_TIMER(Az3D::Rendering::Manager::Present)
	if (auto result = GPU::WindowPresent(data.window, {GPU::ContextGetCurrentSemaphore(data.contextMainRender)}); result.isError) {
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

vec2 StringSize(const WString &string, Assets::FontIndex fontIndex) {
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

f32 StringWidth(const WString &string, Assets::FontIndex fontIndex) {
	return StringSize(string, fontIndex).x;
}

f32 StringHeight(const WString &string) {
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
	drawCallInfo.opaque = material.color.a >= 1.0f;
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

	vec2 cursor = vec2(0.0f, lineHeight);
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

void DrawCamera(DrawingContext &context, const Camera &camera, vec4 color) {
	vec3 pointsNear[4], pointsFar[4];
	GetCameraFrustumCorners(camera, pointsNear, pointsFar);
	DrawDebugLine(context, DebugVertex(pointsNear[0], color), DebugVertex(pointsFar[0], color));
	DrawDebugLine(context, DebugVertex(pointsNear[1], color), DebugVertex(pointsFar[1], color));
	DrawDebugLine(context, DebugVertex(pointsNear[2], color), DebugVertex(pointsFar[2], color));
	DrawDebugLine(context, DebugVertex(pointsNear[3], color), DebugVertex(pointsFar[3], color));
	DrawDebugLine(context, DebugVertex(pointsFar[0], color), DebugVertex(pointsFar[1], color));
	DrawDebugLine(context, DebugVertex(pointsFar[1], color), DebugVertex(pointsFar[2], color));
	DrawDebugLine(context, DebugVertex(pointsFar[2], color), DebugVertex(pointsFar[3], color));
	DrawDebugLine(context, DebugVertex(pointsFar[3], color), DebugVertex(pointsFar[0], color));
	DrawDebugLine(context, DebugVertex(pointsNear[0], color), DebugVertex(pointsNear[1], color));
	DrawDebugLine(context, DebugVertex(pointsNear[1], color), DebugVertex(pointsNear[2], color));
	DrawDebugLine(context, DebugVertex(pointsNear[2], color), DebugVertex(pointsNear[3], color));
	DrawDebugLine(context, DebugVertex(pointsNear[3], color), DebugVertex(pointsNear[0], color));
}

void GetCameraFrustumCorners(const Camera &camera, vec3 dstPointsNear[4], vec3 dstPointsFar[4]) {
	f32 fovX = tan(camera.fov * 0.5f);
	f32 fovY = fovX * camera.aspectRatio;
	vec3 right = normalize(cross(camera.forward, camera.up)) * fovX;
	vec3 up = orthogonalize(camera.up, camera.forward) * fovY;
	vec3 points[4] = {
		camera.forward + right + up,
		camera.forward + right - up,
		camera.forward - right - up,
		camera.forward - right + up,
	};
	for (i32 i = 0; i < 4; i++) {
		dstPointsNear[i] = points[i] * camera.nearClip + camera.pos;
		dstPointsFar[i] = points[i] * camera.farClip + camera.pos;
	}
}

void DrawMeshPart(DrawingContext &context, Assets::MeshPart *meshPart, const ArrayWithBucket<mat4, 1> &transforms, bool opaque, bool castsShadows, Optional<Animation::ArmatureAction> action) {
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
		DrawMeshPart(context, meshPart, *finalTransforms, opaque && meshPart->material.color.a == 1.0f, castsShadows && meshPart->material.color.a >= 0.5f, Animation::ArmatureAction{mesh, actionIndex, time});
		context.thingsToDraw.Back().ikParameters = ikParameters;
	}
}

} // namespace Rendering
