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

layout(set = 0, binding = 0) uniform UBOPerObject {
	mat4 worldMatrix;
	vec4 color;
	int isInstanced;
	int isTextured;
} uboPerObject;

layout(set = 1, binding = 1) uniform UBOPerFrame {
	mat4 viewMatrix;
	mat4 projectionMatrix;
} uboPerFrame;

layout(set = 0, binding = 3) uniform sampler2D instancingTexture;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outViewPos;
layout(location = 2) out vec3 outViewNorm;
layout(location = 3) flat out uint outTexIndex;

void main() {
	mat4x4 instMtx =
		uboPerObject.isInstanced == 1
		? LoadMatrixFromTexture(gl_InstanceIndex, instancingTexture, textureSize(instancingTexture, 0).x)
		: mat4x4(1.0f);

	vec4 localPos = instMtx * vec4(inPos.xyz, 1.0);
	vec4 localNorm = instMtx * vec4(inNorm.xyz, 0.0f);
	outViewPos = (uboPerFrame.viewMatrix * uboPerObject.worldMatrix * localPos).xyz;
	outViewNorm = (uboPerFrame.viewMatrix * uboPerObject.worldMatrix * localNorm).xyz;
	outUV = inUV;
	outTexIndex = inTexIndex;
	gl_Position = uboPerFrame.projectionMatrix * vec4(outViewPos, 1.0);
}