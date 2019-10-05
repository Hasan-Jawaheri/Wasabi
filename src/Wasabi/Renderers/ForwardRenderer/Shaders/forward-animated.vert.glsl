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
layout(location = 5) in uvec4 inBoneIndex;
layout(location = 6) in vec4 inBoneWeight;

struct Light {
	vec4 color;
	vec4 dir;
	vec4 pos;
	int type;
};

layout(set = 0, binding = 0) uniform UBO {
	mat4 worldMatrix;
	int animationTextureWidth;
	int instanceTextureWidth;
	int isInstanced;
	vec4 color;
	int isTextured;
} uboPerObject;

layout(set = 1, binding = 1) uniform LUBO {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	vec3 camPosW;
	int numLights;
	Light lights[16];
} uboPerFrame;

layout(set = 0, binding = 2) uniform sampler2D animationTexture;
layout(set = 0, binding = 3) uniform sampler2D instancingTexture;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outWorldNorm;
layout(location = 3) flat out uint outTexIndex;
void main() {
	mat4x4 animMtx = mat4x4(0.0f);
	mat4x4 instMtx =
		uboPerObject.isInstanced == 1
		? LoadMatrixFromTexture(gl_InstanceIndex, instancingTexture, uboPerObject.instanceTextureWidth)
		: mat4x4(1.0f);
	if (inBoneWeight.x > 0.001f) {
		animMtx += inBoneWeight.x * LoadMatrixFromTexture(int(inBoneIndex.x), animationTexture, uboPerObject.animationTextureWidth);
		if (inBoneWeight.y > 0.001f) {
			animMtx += inBoneWeight.y * LoadMatrixFromTexture(int(inBoneIndex.y), animationTexture, uboPerObject.animationTextureWidth);
			if (inBoneWeight.z > 0.001f) {
				animMtx += inBoneWeight.z * LoadMatrixFromTexture(int(inBoneIndex.z), animationTexture, uboPerObject.animationTextureWidth);
				if (inBoneWeight.w > 0.001f) {
					animMtx += inBoneWeight.w * LoadMatrixFromTexture(int(inBoneIndex.w), animationTexture, uboPerObject.animationTextureWidth);
				}
			}
		}
	}

	vec4 localPos1 = animMtx * vec4(inPos.xyz, 1.0);
	vec4 localPos2 = instMtx * vec4(localPos1.xyz, 1.0);
	vec4 localNorm1 = animMtx * vec4(inNorm.xyz, 0.0f);
	vec4 localNorm2 = instMtx * vec4(localNorm1.xyz, 0.0f);
	outWorldPos = (uboPerObject.worldMatrix * localPos2).xyz;
	outWorldNorm = (uboPerObject.worldMatrix * localNorm2).xyz;
	outUV = inUV;
	outTexIndex = inTexIndex;
	gl_Position = uboPerFrame.projectionMatrix * uboPerFrame.viewMatrix * vec4(outWorldPos, 1.0);
}