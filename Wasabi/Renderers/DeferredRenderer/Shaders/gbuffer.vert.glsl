#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "../../Common/Shaders/object_utils.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inTang;
layout(location = 2) in vec3 inNorm;
layout(location = 3) in vec2 inUV;
layout(location = 4) in uint inTexIndex;
layout(location = 5) in uvec4 boneIndex;
layout(location = 6) in vec4 boneWeight;

layout(set = 0, binding = 0) uniform UBOPerObject {
	mat4 worldMatrix;
	int animationTextureWidth;
	int instanceTextureWidth;
	int isAnimated;
	int isInstanced;
	vec4 color;
	int isTextured;
} uboPerObject;

layout(set = 1, binding = 1) uniform UBOPerFrame {
	mat4 viewMatrix;
	mat4 projectionMatrix;
} uboPerFrame;

layout(set = 0, binding = 2) uniform sampler2D animationTexture;
layout(set = 0, binding = 3) uniform sampler2D instancingTexture;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outViewPos;
layout(location = 2) out vec3 outViewNorm;
layout(location = 3) flat out uint outTexIndex;

void main() {
	mat4x4 animMtx = mat4x4(1.0) * (1-uboPerObject.isAnimated);
	mat4x4 instMtx =
		uboPerObject.isInstanced == 1
		? LoadInstanceMatrix(gl_InstanceIndex, instancingTexture, uboPerObject.instanceTextureWidth)
		: mat4x4(1.0f);
	if (boneWeight.x * uboPerObject.isAnimated > 0.001f) {
		animMtx += boneWeight.x * LoadBoneMatrix(boneIndex.x, animationTexture, uboPerObject.animationTextureWidth);
		if (boneWeight.y > 0.001f) {
			animMtx += boneWeight.y * LoadBoneMatrix(boneIndex.y, animationTexture, uboPerObject.animationTextureWidth);
			if (boneWeight.z > 0.001f) {
				animMtx += boneWeight.z * LoadBoneMatrix(boneIndex.z, animationTexture, uboPerObject.animationTextureWidth);
				if (boneWeight.w > 0.001f) {
					animMtx += boneWeight.w * LoadBoneMatrix(boneIndex.w, animationTexture, uboPerObject.animationTextureWidth);
				}
			}
		}
	}

	vec4 localPos1 = animMtx * vec4(inPos.xyz, 1.0);
	vec4 localPos2 = instMtx * vec4(localPos1.xyz, 1.0);
	vec4 localNorm1 = animMtx * vec4(inNorm.xyz, 0.0f);
	vec4 localNorm2 = instMtx * vec4(localNorm1.xyz, 0.0f);
	outViewPos = (uboPerFrame.viewMatrix * uboPerObject.worldMatrix * localPos2).xyz;
	outViewNorm = (uboPerFrame.viewMatrix * uboPerObject.worldMatrix * localNorm2).xyz;
	outUV = inUV;
	outTexIndex = inTexIndex;
	gl_Position = uboPerFrame.projectionMatrix * vec4(outViewPos, 1.0);
}