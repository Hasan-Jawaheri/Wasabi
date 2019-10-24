#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "../../Common/Shaders/utils.glsl"

layout(set = 0, binding = 0) uniform UBOPerObject {
	mat4 worldMatrix;
	vec4 color;
	float specularPower;
	float specularIntensity;
	int isInstanced;
	int isTextured;
} uboPerObject;

layout(set = 0, binding = 4) uniform sampler2D diffuseTexture[8];

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inViewPos;
layout(location = 2) in vec3 inViewNorm;
layout(location = 3) flat in uint inTexIndex;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormals;

void main() {
	outColor = texture(diffuseTexture[inTexIndex], inUV) * uboPerObject.isTextured + uboPerObject.color;
    outNormals.rg = packNormalSpheremapTransform(inViewNorm);
	outNormals.ba = vec2(uboPerObject.specularPower, uboPerObject.specularIntensity);
}