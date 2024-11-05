/*
	File: main.cpp
	Author: Philip Haynes
*/

#include "Az3D/game_systems.hpp"
#include "Az3D/settings.hpp"
#include "AzCore/Profiling.hpp"
#include "Az3D/rendering.hpp"

using namespace AzCore;
using namespace Az3D;
using GameSystems::sys;

Settings::Name sLookSmoothing;
Settings::Name sFlickTilting;

struct Test : public GameSystems::System {
	vec3 pos = vec3(1.0f, 1.0f, 1.0f);
	quat facingDir = quat(1.0f);
	// Angle32 pitch = 0.0f, yaw = 0.0f;
	quat objectOrientation = quat(1.0f);
	vec2 facingDiff = 0.0f;
	Degrees32 targetFOV = 90.0f;
	Angle32 sunAngle = Degrees32(20.0f);
	bool mouseLook = false;
	bool sunTurning = true;
	Assets::MeshIndex meshes[3];
	static constexpr i32 meshesCount = 3;
	Assets::MeshIndex meshGround;
	Assets::MeshIndex meshTree;
	Assets::MeshIndex meshGrass;
	Assets::MeshIndex meshFence;
	Assets::MeshIndex meshShitman;
	Assets::ActionIndex actionJump;
	Assets::MeshIndex meshTube;
	Assets::ActionIndex actionWiggle;
	Array<Vector<f32>> ikParametersShitman;
	Array<Vector<f32>> ikParametersTube;
	f32 jumpT = 0.0f;
	f32 rate = 1.0f;
	bool pause = false;
	i32 currentMesh = 0;
	Angle32 hover = 0.0f;
	virtual void EventAssetsRequest() override {
		meshes[0]   = sys->assets.RequestMesh("suzanne.az3d");
		meshes[1]   = sys->assets.RequestMesh("F-232 Eagle.az3d");
		meshes[2]   = sys->assets.RequestMesh("C-1 Transport.az3d");
		meshShitman = sys->assets.RequestMesh("shitman.az3d");
		actionJump  = sys->assets.RequestAction("shitman.az3d/Jump");
		meshTube    = sys->assets.RequestMesh("Tube.az3d");
		actionWiggle= sys->assets.RequestAction("Tube.az3d/Wiggle");
		meshTree    = sys->assets.RequestMesh("Tree.az3d");
		meshGrass   = sys->assets.RequestMesh("Grass_Patch.az3d");
		meshFence   = sys->assets.RequestMesh("Weathered Metal Fence.az3d");
		meshGround  = sys->assets.RequestMesh("ground.az3d");
	}
	virtual void EventSync() override {
		pos.z = 1.5f + sin(hover);
		hover += Degrees32(sys->timestep * 9.0f);
		if (!pause)
			jumpT += sys->timestep * rate;
		f32 speed = sys->Down(KC_KEY_LEFTSHIFT) ? 8.0f : 2.0f;
		if (sys->Pressed(KC_KEY_ESC, true)) sys->exit = true;
		if (sys->Pressed(KC_KEY_T)) sunTurning = !sunTurning;
		if (sys->Pressed(KC_KEY_SPACE)) {
			pause = !pause;
		}
		if (sys->Repeated(KC_KEY_UP)) {
			if (sys->Down(KC_KEY_LEFTSHIFT)) {
				Az3D::Rendering::numNewtonIterations++;
			} else if (sys->Down(KC_KEY_LEFTCTRL)) {
				Az3D::Rendering::numBinarySearchIterations++;
			} else {
				rate *= 2.0f;
			}
		}
		if (sys->Repeated(KC_KEY_DOWN)) {
			if (sys->Down(KC_KEY_LEFTSHIFT)) {
				Az3D::Rendering::numNewtonIterations--;
			} else if (sys->Down(KC_KEY_LEFTCTRL)) {
				Az3D::Rendering::numBinarySearchIterations--;
			} else {
				rate /= 2.0f;
			}
		}
		if (sys->Down(KC_KEY_LEFT)) {
			jumpT -= sys->timestep * (0.5f + rate);
		}
		if (sys->Down(KC_KEY_RIGHT)) {
			jumpT += sys->timestep * (rate + 0.5f);
		}
		if (sunTurning) {
			sunAngle += Radians32(tau * sys->timestep / 60.0f / 60.0f);
		}
		Rendering::Camera &camera = sys->rendering.camera;
		vec3 camRight = normalize(cross(camera.forward, camera.up));
		vec3 camUp = normalize(cross(camera.forward, camRight));
		{
			vec2i center = vec2i(sys->window.width / 2, sys->window.height / 2);
			if (mouseLook && !sys->rendering.IsInDebugFlyCam()) {
				facingDiff.x += f32(sys->input.cursor.x - center.x) * camera.fov.value() / 60.0f / sys->rendering.screenSize.x;
				facingDiff.y -= f32(sys->input.cursor.y - center.y) * camera.fov.value() / 60.0f / sys->rendering.screenSize.x;
				sys->window.MoveCursor(center.x, center.y);
			}
			if (sys->Pressed(KC_KEY_TAB)) {
				mouseLook = !mouseLook;
				sys->window.HideCursor(mouseLook);
				sys->input.cursor = center;
			}
		}
		{
			f32 factor = 1.0f;
			if (Settings::ReadBool(sLookSmoothing)) {
				factor = decayFactor(0.015f, sys->timestep);
			}
			vec2 diff = facingDiff * factor;
			facingDiff -= diff;
			// pitch += Radians32(diff.y);
			// yaw += Radians32(diff.x);
			quat zRot = quat::Rotation(
				diff.x,
				facingDir.Conjugate().RotatePoint(camUp)
			);
			quat xRot = quat::Rotation(
				diff.y,
				facingDir.Conjugate().RotatePoint(camRight)
			);
			facingDir *= zRot * xRot;
			facingDir = normalize(facingDir);
		}
		if (sys->Down(KC_MOUSE_RIGHT)) {
			// vec3 right = objectOrientation.RotatePoint(vec3(1.0f, 0.0f, 0.0f));
			// vec3 up = objectOrientation.RotatePoint(vec3(0.0f, 0.0f, 1.0f));
			quat zRot = quat::Rotation(
				f32(sys->input.cursor.x - sys->input.cursorPrevious.x) * 4.0f / sys->rendering.screenSize.x,
				camUp
			);
			quat xRot = quat::Rotation(
				f32(sys->input.cursor.y - sys->input.cursorPrevious.y) * 4.0f / sys->rendering.screenSize.x,
				camRight
			);
			objectOrientation *= zRot * xRot;
			objectOrientation = normalize(objectOrientation);
		}
		// camera.up = vec3(0.0f, 0.0f, 1.0f);
		// {
		// 	f32 sYaw = sin(yaw);
		// 	f32 cYaw = cos(yaw);
		// 	f32 sPitch = sin(pitch);
		// 	f32 cPitch = cos(pitch);
		// 	camera.forward = vec3(
		// 		sYaw * cPitch,
		// 		cYaw * cPitch,
		// 		sPitch
		// 	);
		// }
		camera.forward = facingDir.RotatePoint(vec3(0.0f, 1.0f, 0.0f));
		camera.up = facingDir.RotatePoint(vec3(0.0f, 0.0f, 1.0f));
		{
			vec3 camUp = facingDir.Conjugate().RotatePoint(camera.up);
			vec3 camForward = facingDir.Conjugate().RotatePoint(camera.forward);
			vec3 trueUp = facingDir.Conjugate().RotatePoint(vec3(0.0f, 0.0f, 1.0f));
			vec3 targetUp = orthogonalize(trueUp, camForward);
			// You have to clamp because if there's the slightest error that bring the magnitudes above 1 acos gives you NaN
			f32 theta = acos(clamp(dot(camUp, targetUp), -1.0f, 1.0f));
			if (theta > 1.0e-14f) {
				vec3 axis = normalize(cross(camUp, targetUp));
				f32 factor = 1.0f;
				if (Settings::ReadBool(sFlickTilting)) {
					factor = decayFactor(0.025f, sys->timestep);
				}
				quat adjust = quat::Rotation(theta * factor, axis);
				facingDir *= adjust;
			}
		}
		camera.forward = facingDir.RotatePoint(vec3(0.0f, 1.0f, 0.0f));
		camera.up = facingDir.RotatePoint(vec3(0.0f, 0.0f, 1.0f));
		if (sys->Down(KC_KEY_W)) {
			camera.pos += speed * sys->timestep * camera.forward;
		}
		if (sys->Down(KC_KEY_S)) {
			camera.pos -= speed * sys->timestep * camera.forward;
		}
		if (sys->Down(KC_KEY_D)) {
			camera.pos += speed * sys->timestep * camRight;
		}
		if (sys->Down(KC_KEY_A)) {
			camera.pos -= speed * sys->timestep * camRight;
		}
		for (i32 i = 0; i < meshesCount; i++) {
			if (sys->Pressed(KC_KEY_1 + i)) currentMesh = i;
		}
		targetFOV = clamp(targetFOV.value() - sys->input.scroll.y*5.0f, 5.0f, 90.0f);
		camera.fov = decay(camera.fov.value(), targetFOV.value(), 0.2f, sys->timestep);
	}
	virtual void EventDraw(Array<Rendering::DrawingContext> &contexts) override {
		for (i32 y = -5; y <= 5; y++) {
			for (i32 x = -5; x <= 5; x++) {
				const f32 scale = 5.0f;
				mat4 transform = Rendering::GetTransform(vec3((f32)x * scale * 10.0f, (f32)y * scale * 10.0f, 0.0f), quat(1.0f), vec3(vec2(scale), 1.0f));
				Rendering::DrawMesh(contexts[0], meshGround, {transform}, true, true);
			}
		}
		{
			Assets::Material material = Assets::Material::Blank();
			material.color.rgb = sRGBToLinear(vec3(0.5f, 0.05f, 0.05f));
			material.roughness = 0.2f;
			material.metalness = 0.0f;
			// material.emit = sRGBToLinear(vec3(1.0f, 0.5f, 0.2f));
			Rendering::DrawText(contexts[0], 0, vec2(0.5f, 1.0f), ToWString("Hello, you beautiful thing!\nWhat the dog doin?\nキスミー"), Rendering::GetTransform(vec3(1.0f, 6.0f, 0.0f), quat::Rotation(hover.value(), vec3(0.0f, 0.0f, 1.0f)) * quat::Rotation(-halfpi, vec3(1.0f, 0.0f, 0.0f)), vec3(2.0f)), true, material);
		}
		Rendering::DrawMesh(contexts[0], meshTree, {Rendering::GetTransform(vec3(-2.0f, 0.0f, 0.0f), quat(1.0f), vec3(1.0f))}, true, true);
		Rendering::DrawMesh(contexts[0], meshFence, {Rendering::GetTransform(vec3(0.0f, 8.0f, 0.0f), quat(1.0f), vec3(1.0f))}, true, true);
		Rendering::DrawMeshAnimated(contexts[0], meshShitman, actionJump, jumpT, {Rendering::GetTransform(vec3(6.0f, 0.0f, 0.0f), quat(1.0f), vec3(1.0f))}, true, true, &ikParametersShitman);
		Rendering::DrawMeshAnimated(contexts[0], meshTube, actionWiggle, jumpT, {Rendering::GetTransform(vec3(6.0f, 6.0f, 0.0f), quat(1.0f), vec3(1.0f))}, true, true, &ikParametersTube);
		mat4 transform = Rendering::GetTransform(pos, objectOrientation, vec3(1.0f));
		// mat4 transform = Rendering::GetTransform(pos, objectOrientation, vec3(3.0f, 0.5f, 1.0f));
		Rendering::DrawMesh(contexts[0], meshes[currentMesh], {transform}, true, true);
		for (i32 i = -10; i <= 10; i++) {
			f32 p = i;
			f32 f = (p+10.0f)/20.0f;
			Rendering::DrawDebugLine(contexts[0],
				{vec3(p, -10.0f, 0.0f), vec4(f, 0.0f, 0.5f, 0.5f)},
				{vec3(p,  10.0f, 0.0f), vec4(f, 1.0f, 0.5f, 0.5f)}
			);
			Rendering::DrawDebugLine(contexts[0],
				{vec3(-10.0f, p, 0.0f), vec4(0.0f, f, 0.5f, 0.5f)},
				{vec3( 10.0f, p, 0.0f), vec4(1.0f, f, 0.5f, 0.5f)}
			);
		}
		RandomNumberGenerator rng(69420);
		const i32 patchCount = 14;
		const f32 patchDimension = 2.0f;
		ArrayWithBucket<mat4, 1> transforms(square(patchCount));
		const f32 grassDimensions = 2.0f - patchDimension/2.0f;
		for (f32 y = -grassDimensions; y <= grassDimensions; y += 2.0f) {
			for (f32 x = -grassDimensions; x <= grassDimensions; x += 2.0f) {
				for (i32 yy = -patchCount/2; yy < patchCount/2; yy++) {
					for (i32 xx = -patchCount/2; xx < patchCount/2; xx++) {
						mat4 &transform = transforms[(yy+patchCount/2) * patchCount + (xx+patchCount/2)];
						transform = mat4::RotationBasic(random(0.0f, tau, &rng), Axis::Z);
						transform[3][0] = x + float(xx) * patchDimension / float(patchCount);
						transform[3][1] = y + float(yy) * patchDimension / float(patchCount);
					}
				}
				Rendering::DrawMesh(contexts[0], meshGrass, transforms, true, false);
			}
		}
		// Rendering::DrawDebugSphere(contexts[0], vec3(0.0f), 1.0f, vec4(1.0f));
		sys->rendering.worldInfo.sunDir = vec3(0.0f, vec2::UnitVecFromAngle(sunAngle.value()));
	}
};

i32 main(i32 argumentCount, char** argumentValues) {

	bool enableLayers = false;
	Test test;

	sLookSmoothing = "lookSmoothing";
	sFlickTilting = "flickTilting";

	for (i32 i = 0; i < argumentCount; i++) {
		io::cout.PrintLn(i, ": ", argumentValues[i]);
		if (equals(argumentValues[i], "--validation")) {
			io::cout.PrintLn("Enabling validation layers");
			enableLayers = true;
		} else if (equals(argumentValues[i], "--profiling")) {
			io::cout.PrintLn("Enabling profiling");
			az::Profiling::Enable();
		}
	}

	Settings::Add(sLookSmoothing, Settings::Setting(true));
	Settings::Add(sFlickTilting, Settings::Setting(true));

	if (!GameSystems::Init("Az3D Example", {&test}, enableLayers)) {
		io::cerr.PrintLn("Failed to Init: ", GameSystems::sys->error);
		return 1;
	}

	sys->rendering.camera.pos = vec3(0.0f, -3.0f, 3.0f);

	GameSystems::UpdateLoop();

	GameSystems::Deinit();
	return 0;
}
