/*
	File: animation.cpp
	Author: Philip Haynes
*/

#include "animation.hpp"
#include "game_systems.hpp"
#include "rendering.hpp"
#include "settings.hpp"

#include "AzCore/Profiling.hpp"

namespace Az3D::Animation {

using namespace AzCore;

using GameSystems::sys;

i32 numNewtonIterations = 10;
i32 numBinarySearchIterations = 50;

bool ArmatureAction::operator==(const ArmatureAction &other) const {
	return meshIndex == other.meshIndex && actionIndex == other.actionIndex && other.actionTime == other.actionTime;
}

struct BoneEvalMetadata {
	mat4 restTransformLocal;
	mat4 restTransformModel;
	quat animOrientation = quat(1.0f);
	vec3 animOffset = vec3(0.0f);
	bool evaluated = false;
};

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
		vec3 tip = vec3(0.0f);
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
				DrawDebugLine(sys->rendering.data.drawingContexts.Back(), Rendering::DebugVertex{tip, vec4(0.2f, 0.0f, 0.0f, 1.0f)}, Rendering::DebugVertex{tip + pDerivative, vec4(1.0f, 0.0f, 0.0f, 1.0f)});
			}
			p++;
		}
		if (!ik.locked.y) {
			vec3 &pDerivative = jacobian.Col(p).AsVec3();
			pDerivative = rotationEval * ((ikChain[i].rotationX * ikChain[i].localTip).RotatedYPos90() * vec3(1.0f, 0.0f, 1.0f));
			pDerivative *= (1.0f - stiffness[p]);
			if (showDerivatives) {
				DrawDebugLine(sys->rendering.data.drawingContexts.Back(), Rendering::DebugVertex{tip, vec4(0.0f, 0.2f, 0.0f, 1.0f)}, Rendering::DebugVertex{tip + pDerivative, vec4(0.0f, 1.0f, 0.0f, 1.0f)});
			}
			p++;
		}
		if (!ik.locked.z) {
			vec3 &pDerivative = jacobian.Col(p).AsVec3();
			pDerivative = rotationEval * ((ikChain[i].rotationXY * ikChain[i].localTip).RotatedZPos90() * vec3(1.0f, 1.0f, 0.0f));
			pDerivative *= (1.0f - stiffness[p]);
			if (showDerivatives) {
				DrawDebugLine(sys->rendering.data.drawingContexts.Back(), Rendering::DebugVertex{tip, vec4(0.0f, 0.0f, 0.2f, 1.0f)}, Rendering::DebugVertex{tip + pDerivative, vec4(0.0f, 0.0f, 1.0f, 1.0f)});
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
		DrawDebugLine(sys->rendering.data.drawingContexts.Back(), Rendering::DebugVertex{tip, vec4(0.2f, 0.0f, 0.2f, 1.0f)}, Rendering::DebugVertex{tip - (ikChain.Back().modelTip - ikTargetPos.xyz), vec4(1.0f, 0.0f, 1.0f, 1.0f)});
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

	transform = Rendering::GetMat4(meta.animOrientation, meta.animOffset);

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
				Rendering::DebugVertex p1, p2;
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

} // namespace Az3D