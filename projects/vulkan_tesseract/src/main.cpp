/*
	File: main.cpp
	Author: Philip Haynes
	Description: High-level definition of the structure of our program.
*/

#include "AzCore/io.hpp"
#include "AzCore/gpu.hpp"
#include "AzCore/Math/Color.hpp"

using namespace AzCore;

const u32 maxVertices = 8192;

f32 scale = 1.0f;

struct Vertex {
	vec4 color;
	vec2 pos;
};

void DrawCircle(GPU::Context *context, Vertex *vertices, u32 *vertex, vec2 center, f32 radius, vec4 color, f32 aspectRatio) {
	u32 circumference = (u32)(scale * clamp(sqrt(radius*tau*1600.0f), 5.0f, 80.0f));
	center.x *= aspectRatio;
	vertices[(*vertex)++] = {color, center};
	for (u32 i = 0; i <= circumference; i++) {
		f32 angle = (f32)i * tau / (f32)circumference;
		vertices[(*vertex)++] = {color, center + vec2(sin(angle)*radius*aspectRatio, cos(angle)*radius)};
	}
	u32 vertCount = circumference+2;
	GPU::CmdDraw(context, vertCount, *vertex - vertCount);
}

void DrawQuad(GPU::Context *context, Vertex *vertices, u32 *vertex, vec2 points[4], vec4 colors[4]) {
	for (u32 i = 0; i < 4; i++) {
		vertices[(*vertex)++] = {colors[i], points[i]};
	}
	GPU::CmdDraw(context, 4, *vertex - 4);
}

void DrawLine(GPU::Context *context, Vertex *vertices, u32 *vertex, vec2 p1, vec2 p2, vec4 color) {
	vertices[(*vertex)++] = {color, p1};
	vertices[(*vertex)++] = {color, p2};
	GPU::CmdDraw(context, 2, *vertex - 2);
}

i32 main(i32 argumentCount, char** argumentValues) {

	bool enableLayers = false, enableCoreValidation = false;

	io::cout.PrintLn("\nTest program received ",  argumentCount,  " arguments:");
	for (i32 i = 0; i < argumentCount; i++) {
		io::cout.PrintLn(i,  ": ",  argumentValues[i]);
		if (equals(argumentValues[i], "--enable-layers")) {
			enableLayers = true;
		} else if (equals(argumentValues[i], "--core-validation")) {
			enableCoreValidation = true;
		}
	}

	io::cout.PrintLn("Initializing RawInput");
	io::RawInput rawInput;
	if (!rawInput.Init(io::RAW_INPUT_ENABLE_GAMEPAD_JOYSTICK)) {
		io::cout.PrintLn("Failed to Init RawInput: ",  io::error);
	}

	io::Input input;
	io::Window ioWindow;
	ioWindow.input = &input;
	ioWindow.name = "AzCore Tesseract";
	ioWindow.width = 800;
	ioWindow.height = 800;
	if (!ioWindow.Open()) {
		io::cout.PrintLn("Failed to open ioWindow: ",  io::error);
		return 1;
	}

	scale = (f32)ioWindow.GetDPI() / 96.0f;
	ioWindow.Resize(u32((f32)ioWindow.width * scale), u32((u32)ioWindow.height * scale));

	rawInput.window = &ioWindow;
	
	GPU::SetAppName("AzCore Tesseract");
	
	GPU::Window *gpuWindow = GPU::AddWindow(&ioWindow, "Main window").Unwrap();

	if (enableLayers) {
		io::cout.PrintLn("Validation layers enabled.");
		GPU::EnableValidationLayers();
	}
	
	GPU::Device *device = GPU::NewDevice();

	GPU::Buffer *vertexBuffer = GPU::NewVertexBuffer(device);
	GPU::BufferSetSize(vertexBuffer, sizeof(Vertex) * maxVertices);

	GPU::Framebuffer *framebuffer = GPU::NewFramebuffer(device);
	GPU::FramebufferAddWindow(framebuffer, gpuWindow);
	vec4 clearColor = vec4(sRGBToLinear(vec3(0.0f, 0.1f, 0.2f)), 1.0f);
	
	GPU::Shader *shaderVert = GPU::NewShader(device, "data/shaders/2D.vert.spv", GPU::ShaderStage::VERTEX);
	GPU::Shader *shaderFrag = GPU::NewShader(device, "data/shaders/2D.frag.spv", GPU::ShaderStage::FRAGMENT);
	

	GPU::Pipeline *pipelineLines = GPU::NewGraphicsPipeline(device, "Lines pipeline");
	GPU::PipelineAddShaders(pipelineLines, {shaderVert, shaderFrag});
	GPU::PipelineAddVertexInputs(pipelineLines, {
		GPU::ShaderValueType::VEC4,
		GPU::ShaderValueType::VEC2,
	});
	GPU::PipelineSetBlendMode(pipelineLines, {GPU::BlendMode::ADDITION, false});
	GPU::PipelineSetTopology(pipelineLines, GPU::Topology::LINE_LIST);
	GPU::PipelineSetLineWidth(pipelineLines, 4.0f * scale);
	
	GPU::Pipeline *pipelineTriangleFan = GPU::NewGraphicsPipeline(device, "TriangleFan pipeline");
	GPU::PipelineAddShaders(pipelineTriangleFan, {shaderVert, shaderFrag});
	GPU::PipelineAddVertexInputs(pipelineTriangleFan, {
		GPU::ShaderValueType::VEC4,
		GPU::ShaderValueType::VEC2,
	});
	GPU::PipelineSetBlendMode(pipelineTriangleFan, {GPU::BlendMode::ADDITION, false});
	GPU::PipelineSetTopology(pipelineTriangleFan, GPU::Topology::TRIANGLE_FAN);
	
	GPU::Context *contextTransfer = GPU::NewContext(device, "Transfer context");
	GPU::Context *contextDrawing = GPU::NewContext(device, "Drawing context");

	if (auto result = GPU::Initialize(); result.isError) {
		io::cout.PrintLn("Failed to initialize GPU: ", result.error);
		return 1;
	}

	if (!ioWindow.Show()) {
		io::cout.PrintLn("Failed to show ioWindow: ",  io::error);
		return 1;
	}

	Vertex *vertices = new Vertex[maxVertices];

	i32 gamepadIndex = -1; // Which gamepad we're reading input from

	f32 rotateAngle = 0.0f;
	f32 eyeWidth = 0.1f;

	vec4 offset(0.0f);
	vec2 facingAngleXY(0.0f);
	vec2 facingAngleZW(0.0f);
	bool faceMode = false;
	bool pause = false;
	bool enableStereoGraphic = false;
	bool resized = false;

	vec2i draggingOrigin[2] = {vec2i(0), vec2i(0)};
	vec2 draggingFacingAngleOrigin[2] = {vec2(0.0f), vec2(0.0f)};

	// bool first = true;
	const u32 framerate = 60;
	ClockTime frameEnd = Clock::now() + Milliseconds(1000/framerate-1);

	while (true) {
		rawInput.Update(1.0f/(f32)framerate);
		if (rawInput.AnyGP.Pressed()) {
			gamepadIndex = rawInput.AnyGPIndex;
		}
		input.Tick(1.0f/(f32)framerate);
		if (!ioWindow.Update()) {
			break;
		}
		if (ioWindow.resized) {
			resized = true;
		}
		if (input.Pressed(KC_KEY_ESC)) {
			break;
		}
		if (input.PressedChar('F')) {
			faceMode = !faceMode;
		}
		if (input.Pressed(KC_KEY_PAUSE)) {
			pause = !pause;
		}

		bool toggleStereographic = false;
		bool eyeWidthShrink = false;
		bool eyeWidthGrow = false;

		if (gamepadIndex >= 0) {
			if (rawInput.gamepads[gamepadIndex].Pressed(KC_GP_BTN_START)) {
				pause = !pause;
			}
			if (rawInput.gamepads[gamepadIndex].Pressed(KC_GP_BTN_SELECT)) {
				break;
			}
			if (rawInput.gamepads[gamepadIndex].Pressed(KC_GP_BTN_X)) {
				faceMode = !faceMode;
			}
			if (rawInput.gamepads[gamepadIndex].Pressed(KC_GP_BTN_Y)) {
				toggleStereographic = true;
			}
			if (rawInput.gamepads[gamepadIndex].Down(KC_GP_BTN_TL)) {
				eyeWidthShrink = true;
			}
			if (rawInput.gamepads[gamepadIndex].Down(KC_GP_BTN_TR)) {
				eyeWidthGrow = true;
			}
			facingAngleXY += rawInput.gamepads[gamepadIndex].axis.vec.RS * vec2(-pi, pi) / (f32)framerate;
		}

		if (input.Pressed(KC_KEY_1)) {
			toggleStereographic = true;
		}

		if (input.Down(KC_KEY_Q)) {
			eyeWidthShrink = true;
		}
		if (input.Down(KC_KEY_E)) {
			eyeWidthGrow = true;
		}

		if (input.Pressed(KC_MOUSE_LEFT)) {
			draggingOrigin[0] = input.cursor;
			draggingFacingAngleOrigin[0] = facingAngleXY;
		}
		if (input.Down(KC_MOUSE_LEFT)) {
			vec2i diff = input.cursor-draggingOrigin[0];
			facingAngleXY = draggingFacingAngleOrigin[0] + vec2((f32)diff.x, (f32)diff.y) * 0.005f / pi;
		}

		if (input.Pressed(KC_MOUSE_RIGHT)) {
			draggingOrigin[1] = input.cursor;
			draggingFacingAngleOrigin[1] = facingAngleZW;
		}
		if (input.Down(KC_MOUSE_RIGHT)) {
			vec2i diff = input.cursor-draggingOrigin[1];
			facingAngleZW = draggingFacingAngleOrigin[1] + vec2((f32)diff.x, (f32)diff.y) * 0.005f / pi;
		}

		f32 aspectRatio = (f32)ioWindow.height / (f32)ioWindow.width;

		if (toggleStereographic) {
			enableStereoGraphic = !enableStereoGraphic;
			if (enableStereoGraphic) {
				if (aspectRatio > 0.9f) {
					ioWindow.Resize(ioWindow.width * 2, ioWindow.height);
					continue;
				}
			} else {
				offset.x += eyeWidth/2.0f;
				if (abs(aspectRatio - 0.5f) < 0.05f) {
					ioWindow.Resize(ioWindow.height, ioWindow.height);
					continue;
				}
			}
		}
		if (enableStereoGraphic) {
			if (eyeWidthShrink) {
				eyeWidth -= 0.001f;
			}
			if (eyeWidthGrow) {
				eyeWidth += 0.001f;
			}
		}

		if (input.PressedChar('V')) {
			GPU::SetVSync(gpuWindow, !GPU::GetVSyncEnabled(gpuWindow));
		}

		if (auto result = GPU::WindowUpdate(gpuWindow); result.isError) {
			io::cerr.PrintLn("Failed to GPU::WindowUpdate: ", result.error);
			return 1;
		}

		u32 vertex = 0;

		if (auto result = GPU::ContextBeginRecording(contextDrawing); result.isError) {
			io::cout.PrintLn("Failed to Begin recording drawing context: ", result.error);
			return 1;
		}
		GPU::CmdBindFramebuffer(contextDrawing, framebuffer);
		GPU::CmdBindVertexBuffer(contextDrawing, vertexBuffer);
		if (auto result = GPU::CmdCommitBindings(contextDrawing); result.isError) {
			io::cerr.PrintLn("Failed to commit bindings: ", result.error);
			return 1;
		}
		GPU::CmdClearColorAttachment(contextDrawing, clearColor);

		vec5 points[16] = {
			{-1.0f, -1.0f, -1.0f, -1.0f,  1.0f},
			{ 1.0f, -1.0f, -1.0f, -1.0f,  1.0f},
			{-1.0f,  1.0f, -1.0f, -1.0f,  1.0f},
			{ 1.0f,  1.0f, -1.0f, -1.0f,  1.0f},
			{-1.0f, -1.0f,  1.0f, -1.0f,  1.0f},
			{ 1.0f, -1.0f,  1.0f, -1.0f,  1.0f},
			{-1.0f,  1.0f,  1.0f, -1.0f,  1.0f},
			{ 1.0f,  1.0f,  1.0f, -1.0f,  1.0f},
			{-1.0f, -1.0f, -1.0f,  1.0f,  1.0f},
			{ 1.0f, -1.0f, -1.0f,  1.0f,  1.0f},
			{-1.0f,  1.0f, -1.0f,  1.0f,  1.0f},
			{ 1.0f,  1.0f, -1.0f,  1.0f,  1.0f},
			{-1.0f, -1.0f,  1.0f,  1.0f,  1.0f},
			{ 1.0f, -1.0f,  1.0f,  1.0f,  1.0f},
			{-1.0f,  1.0f,  1.0f,  1.0f,  1.0f},
			{ 1.0f,  1.0f,  1.0f,  1.0f,  1.0f}
		};
		mat5 model(1.0f);
		model = mat5::RotationBasic(rotateAngle*halfpi, Plane::XZ) * mat5::RotationBasic(rotateAngle, Plane::YW);
		mat5 view(1.0f);
		view.h.v1 = offset.x;
		view.h.v2 = offset.y;
		view.h.v3 = offset.z;
		view.h.v4 = offset.w;

		view = mat5::RotationBasic(facingAngleXY.y, Plane::XW) * mat5::RotationBasic(facingAngleXY.x, Plane::YW) * view;
		view = mat5::RotationBasic(facingAngleZW.y, Plane::XZ) * mat5::RotationBasic(facingAngleZW.x, Plane::XY) * view;

		mat4 viewRotationOnly;
		for (u32 i = 0; i < 16; i++) {
			viewRotationOnly.data[i] = view.data[i+i/4];
		}
		viewRotationOnly = viewRotationOnly.Transpose();
		vec4 moveX = viewRotationOnly * vec4(0.05f,  0.0f,  0.0f,  0.0f);
		vec4 moveY = viewRotationOnly * vec4( 0.0f, 0.05f,  0.0f,  0.0f);
		vec4 moveZ = viewRotationOnly * vec4( 0.0f,  0.0f, 0.05f,  0.0f);
		vec4 moveW = viewRotationOnly * vec4( 0.0f,  0.0f,  0.0f, 0.05f);

		if (input.Down(KC_KEY_SPACE)) {
			offset += moveY;
		}
		if (input.Down(KC_KEY_LEFTCTRL)) {
			offset -= moveY;
		}
		if (input.Down(KC_KEY_LEFT)) {
			offset += moveW;
		}
		if (input.Down(KC_KEY_RIGHT)) {
			offset -= moveW;
		}
		if (input.Down(KC_KEY_D)) {
			offset -= moveX;
		}
		if (input.Down(KC_KEY_A)) {
			offset += moveX;
		}
		if (input.Down(KC_KEY_W)) {
			offset -= moveZ;
		}
		if (input.Down(KC_KEY_S)) {
			offset += moveZ;
		}

		if (gamepadIndex >=0) {
			offset -= moveX * 100.0f * rawInput.gamepads[gamepadIndex].axis.vec.LS.x / (f32)framerate;
			offset += moveZ * 100.0f * rawInput.gamepads[gamepadIndex].axis.vec.LS.y / (f32)framerate;
			offset += moveY * 100.0f * rawInput.gamepads[gamepadIndex].axis.vec.RT / (f32)framerate;
			offset -= moveY * 100.0f * rawInput.gamepads[gamepadIndex].axis.vec.LT / (f32)framerate;
			offset += moveW * 100.0f * rawInput.gamepads[gamepadIndex].axis.vec.H0.x / (f32)framerate;
		}

		if (!pause) {
			rotateAngle += 0.5f / (f32)framerate;
		}

		vec2 proj[2][16];
		f32 d[2][16];

		const f32 fov = 120.0f;
		const f32 fovFac = 1.0f / tan(fov*pi/360.0f);
		mat5 modelView = view * model;
		if (enableStereoGraphic) {
			modelView.h.v1 -= eyeWidth/2.0f;
		}

		for (u32 i = 0; i < 16; i++) {
			vec5 projected[2];
			projected[0] = modelView * points[i];
			d[0][i] = max(projected[0].z+projected[0].w+projected[0].v, 0.000001f);
			proj[0][i].x = projected[0].x / d[0][i] * aspectRatio * fovFac;
			proj[0][i].y = projected[0].y / d[0][i] * fovFac;
			if (enableStereoGraphic) {
				proj[0][i].x -= 0.5f;
				modelView.h.v1 += eyeWidth;
				projected[1] = modelView * points[i];
				modelView.h.v1 -= eyeWidth;
				d[1][i] = max(projected[1].z+projected[1].w+projected[1].v, 0.000001f);
				proj[1][i].x = projected[1].x / d[1][i] * aspectRatio * fovFac + 0.5f;
				proj[1][i].y = projected[1].y / d[1][i] * fovFac;
			};
		}
		if (faceMode) {
			GPU::CmdBindPipeline(contextDrawing, pipelineTriangleFan);
			if (auto result = GPU::CmdCommitBindings(contextDrawing); result.isError) {
				io::cerr.PrintLn("Failed to commit bindings: ", result.error);
				return 1;
			}
			u8 planes[24][4] = {
				{ 0,  1,  2,  3},
				{ 4,  5,  6,  7},
				{ 8,  9, 10, 11},
				{12, 13, 14, 15},

				{ 0,  2,  4,  6},
				{ 8, 10, 12, 14},
				{ 1,  3,  5,  7},
				{ 9, 11, 13, 15},

				{ 0,  4,  8, 12},
				{ 1,  5,  9, 13},
				{ 2,  6, 10, 14},
				{ 3,  7, 11, 15},

				{ 0,  8,  1,  9},
				{ 2, 10,  3, 11},
				{ 4, 12,  5, 13},
				{ 6, 14,  7, 15},

				{ 0,  8,  2, 10},
				{ 4, 12,  6, 14},
				{ 1,  9,  3, 11},
				{ 5, 13,  7, 15},

				{ 0,  1,  4,  5},
				{ 8,  9, 12, 13},
				{ 2,  3,  6,  7},
				{10, 11, 14, 15},

			};
			for (u32 eye = 0; eye < (enableStereoGraphic ? 2u : 1u); eye++) {
				if (enableStereoGraphic) {
					GPU::CmdSetScissor(contextDrawing, ioWindow.width/2, ioWindow.height, eye * ioWindow.width / 2);
				}
				for (u32 i = 0; i < 24; i++) {
					const u32 index[4] = {0, 1, 3, 2};
					vec2 facePoints[4];
					vec4 colors[4];
					for (u32 ii = 0; ii < 4; ii++) {
						facePoints[ii] = proj[eye][planes[i][index[ii]]];
						colors[ii].a = 0.25f;
						colors[ii].rgb = hsvToRgb(vec3(
							(f32)planes[i][index[ii]] / 16.0f,
							clamp01(1.0f/d[eye][planes[i][index[ii]]]*4.0f),
							1.0f
						));
					}
					DrawQuad(contextDrawing, vertices, &vertex, facePoints, colors);
				}
			}
		}

		GPU::CmdBindPipeline(contextDrawing, pipelineLines);
		if (auto result = GPU::CmdCommitBindings(contextDrawing); result.isError) {
			io::cerr.PrintLn("Failed to commit bindings: ", result.error);
			return 1;
		}
		for (u32 eye = 0; eye < (enableStereoGraphic ? 2u : 1u); eye++) {
			if (enableStereoGraphic) {
				GPU::CmdSetScissor(contextDrawing, ioWindow.width/2, ioWindow.height, eye * ioWindow.width / 2);
			}
			for (u32 i = 0; i < 16; i++) {
				for (u32 ii = i+1; ii < 16; ii++) {
					f32 a = norm(points[i]-points[ii]);
					const f32 epsilon = 0.0001f;
					if (a == median(a, 2.0f-epsilon, 2.0f+epsilon)) {
						vec4 color1(0.5f);
						color1.rgb = hsvToRgb(vec3((f32)i / 16.0f, clamp01(1.0f/d[eye][i]*4.0f), 1.0f));
						vec4 color2(0.5f);
						color2.rgb = hsvToRgb(vec3((f32)ii / 16.0f, clamp01(1.0f/d[eye][ii]*4.0f), 1.0f));
						vertices[vertex++] = {color1, proj[eye][i]};
						vertices[vertex++] = {color2, proj[eye][ii]};
						GPU::CmdDraw(contextDrawing, 2, vertex-2);
					}
				}
			}
		}
		GPU::CmdBindPipeline(contextDrawing, pipelineTriangleFan);
		if (auto result = GPU::CmdCommitBindings(contextDrawing); result.isError) {
			io::cerr.PrintLn("Failed to commit bindings: ", result.error);
			return 1;
		}

		for (u32 eye = 0; eye < (enableStereoGraphic ? 2u : 1u); eye++) {
			if (enableStereoGraphic) {
				GPU::CmdSetScissor(contextDrawing, ioWindow.width/2, ioWindow.height, eye * ioWindow.width / 2);
			}
			for (u32 i = 0; i < 16; i++) {
				if (d[eye][i] > 0.001f) {
					DrawCircle(contextDrawing, vertices, &vertex, proj[eye][i]/vec2(aspectRatio, 1.0f), 0.05f/d[eye][i], vec4(1.0f), aspectRatio);
				}
			}
		}

		GPU::ContextEndRecording(contextDrawing);
		
		GPU::ContextBeginRecording(contextTransfer);
		GPU::CmdCopyDataToBuffer(contextTransfer, vertexBuffer, vertices, 0, sizeof(Vertex) * vertex);
		GPU::ContextEndRecording(contextTransfer);
		
		if (auto result = GPU::SubmitCommands(contextTransfer, true); result.isError) {
			io::cerr.PrintLn("Failed to Submit Transfer Commands: ", result.error);
			return 1;
		}
		if (auto result = GPU::SubmitCommands(contextDrawing, true, {contextTransfer}); result.isError) {
			io::cerr.PrintLn("Failed to Submit Drawing Commands: ", result.error);
			return 1;
		}

		if (auto result = GPU::WindowPresent(gpuWindow, {contextDrawing}); result.isError) {
			io::cout.PrintLn("Failed to present: ", result.error);
			return 1;
		}

		ClockTime now = Clock::now();
		if (std::chrono::duration_cast<Milliseconds>(frameEnd-now).count() > 2) {
			Thread::Sleep(frameEnd-now);
			frameEnd += Nanoseconds(1000000000/framerate);
		} else {
			frameEnd = now + Milliseconds(1000/framerate-1);
		}
	}
	GPU::Deinitialize();
	ioWindow.Close();
	io::cout.PrintLn("Last io::error was \"",  io::error,  "\"");

	delete[] vertices;

	return 0;
}
