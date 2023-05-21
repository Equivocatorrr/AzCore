/*
	File: main.cpp
	Author: Philip Haynes
	Description: High-level definition of the structure of our program.
*/

#include "AzCore/io.hpp"
#include "AzCore/gpu.hpp"
#include "AzCore/Image.hpp"

using namespace AzCore;

i32 main(i32 argumentCount, char** argumentValues) {

	f32 scale = 1.0f;

	for (i32 i = 0; i < argumentCount; i++) {
		if (equals(argumentValues[i], "--enable-layers")) {
			GPU::EnableValidationLayers();
		} else if (equals(argumentValues[i], "--trace")) {
			io::logLevel = io::LogLevel::TRACE;
		}
	}

	Image image("data/icon.png");
	if (image.pixels == nullptr) {
		io::cerr.PrintLn("Failed to load image!");
		return 1;
	}

	GPU::SetAppName("AzCore GPU Test Program");

	GPU::Device* device = GPU::NewDevice();

	io::Window window;
	io::Input input;
	window.input = &input;
	window.width = 480;
	window.height = 480;
	if (!window.Open()) {
		io::cerr.PrintLn("Failed to open Window: ", io::error);
		return 1;
	}

	scale = (f32)window.GetDPI() / 96.0f;
	window.Resize(u32((f32)window.width * scale), u32((u32)window.height * scale));

	GPU::Window *gpuWindow = GPU::AddWindow(&window, "main").Unwrap();
	GPU::SetVSync(gpuWindow, false);
	
	GPU::Framebuffer *framebuffer = GPU::NewFramebuffer(device, "main");

	GPU::FramebufferAddWindow(framebuffer, gpuWindow);

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
	
	GPU::Buffer *vertexBuffer = GPU::NewVertexBuffer(device, "main");
	if (auto result = GPU::BufferSetSize(vertexBuffer, vertices.size * sizeof(Vertex)); result.isError) {
		io::cerr.PrintLn("Failed to set vertex buffer size: ", result.error);
		return 1;
	}
	
	GPU::Buffer *indexBuffer = GPU::NewIndexBuffer(device, "main");
	if (auto result = GPU::BufferSetSize(indexBuffer, indices.size * sizeof(Vertex)); result.isError) {
		io::cerr.PrintLn("Failed to set index buffer size: ", result.error);
		return 1;
	}
	
	GPU::Image *gpuImage = GPU::NewImage(device, "tex");
	if (auto result = GPU::ImageSetFormat(gpuImage, image.channels, 8, GPU::ImageComponentType::SRGB); result.isError) {
		io::cerr.PrintLn("Failed to set image format: ", result.error);
		return 1;
	}
	if (auto result = GPU::ImageSetSize(gpuImage, image.width, image.height); result.isError) {
		io::cerr.PrintLn("Failed to set image size: ", result.error);
		return 1;
	}
	GPU::ImageSetMipmapping(gpuImage, true, 16);
	GPU::ImageSetUsageSampled(gpuImage, (u32)GPU::ShaderStage::FRAGMENT);

	GPU::Pipeline *pipeline = GPU::NewGraphicsPipeline(device, "test");
	GPU::PipelineAddShader(pipeline, "data/shaders/test.vert.spv", GPU::ShaderStage::VERTEX);
	GPU::PipelineAddShader(pipeline, "data/shaders/test.frag.spv", GPU::ShaderStage::FRAGMENT);
	GPU::PipelineAddImage(pipeline, gpuImage);
	GPU::PipelineAddVertexInputs(pipeline, {
		GPU::ShaderValueType::VEC2,
		GPU::ShaderValueType::VEC2
	});
	GPU::PipelineSetBlendMode(pipeline, GPU::BlendMode::TRANSPARENT);
	
	GPU::Context* context = GPU::NewContext(device, "main");
	
	if (auto result = GPU::Initialize(); result.isError) {
		io::cerr.PrintLn("Failed to initialize GPU: ", result.error);
		return 1;
	}

	if (auto result = GPU::ContextBeginRecording(context); result.isError) {
		io::cerr.PrintLn("Failed to begin Context Recording: ", result.error);
		return 1;
	}
	if (auto result = GPU::CmdCopyDataToBuffer(context, vertexBuffer, vertices.data); result.isError) {
		io::cerr.PrintLn("Failed to copy data to Vertex Buffer: ", result.error);
		return 1;
	}
	if (auto result = GPU::CmdCopyDataToBuffer(context, indexBuffer, indices.data); result.isError) {
		io::cerr.PrintLn("Failed to copy data to Index Buffer: ", result.error);
		return 1;
	}
	if (auto result = GPU::CmdCopyDataToImage(context, gpuImage, image.pixels); result.isError) {
		io::cerr.PrintLn("Failed to copy data to Image: ", result.error);
		return 1;
	}
	if (auto result = GPU::ContextEndRecording(context); result.isError) {
		io::cerr.PrintLn("Failed to record data copies: ", result.error);
		return 1;
	}
	if (auto result = GPU::SubmitCommands(context); result.isError) {
		io::cerr.PrintLn("Failed to submit data copies: ", result.error);
		return 1;
	}
	if (auto result = GPU::ContextWaitUntilFinished(context); result.isError) {
		io::cerr.PrintLn("Failed to wait on transfer: ", result.error);
		return 1;
	}

	if(!window.Show()) {
		io::cerr.PrintLn("Failed to show Window: ", io::error);
		return 1;
	}
	RandomNumberGenerator rng;
	bool resize = false;
	do {
		for (i32 i = 0; i < 256; i++) {
			if (input.inputs[i].Pressed()) {
				io::cout.PrintLn("Pressed   HID 0x", FormatInt(i, 16), "\t", window.InputName(i));
			}
			if (input.inputs[i].Released()) {
				io::cout.PrintLn("Released  HID 0x", FormatInt(i, 16), "\t", window.InputName(i));
			}
		}
		input.Tick(1.0f/60.0f);

		if (auto result = GPU::WindowUpdate(gpuWindow); result.isError) {
			io::cerr.PrintLn("UpdateWindow error: ", result.error);
			return 1;
		}

		// Begin recording commands

		if (auto result = GPU::ContextBeginRecording(context); result.isError) {
			io::cerr.PrintLn("Failed to begin Context Recording: ", result.error);
			return 1;
		}

		GPU::CmdBindFramebuffer(context, framebuffer);
		GPU::CmdBindPipeline(context, pipeline);
		GPU::CmdBindVertexBuffer(context, vertexBuffer);
		GPU::CmdBindIndexBuffer(context, indexBuffer);
		GPU::CmdBindImageSampler(context, gpuImage, 0, 0);
		if (auto result = GPU::CmdCommitBindings(context); result.isError) {
			io::cerr.PrintLn("Failed to commit bindings: ", result.error);
			return 1;
		}

		GPU::CmdDrawIndexed(context, 6, 0, 0);

		if (auto result = GPU::ContextEndRecording(context); result.isError) {
			io::cerr.PrintLn("Failed to record frame draw: ", result.error);
			return 1;
		}

		if (auto result = GPU::SubmitCommands(context); result.isError) {
			io::cerr.PrintLn("Failed to submit data copies: ", result.error);
			return 1;
		}

		if (auto result = GPU::WindowPresent(gpuWindow); result.isError) {
			io::cerr.PrintLn("Failed to present window surface: ", result.error);
			return 1;
		}
	} while (window.Update());
	// This should be all you need to call to clean everything up
	GPU::Deinitialize();
	if (!window.Close()) {
		io::cout.PrintLn("Failed to close Window: ", io::error);
		return 1;
	}

	return 0;
}
