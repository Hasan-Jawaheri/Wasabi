#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "utils.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNorm;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;

layout(set = 0, binding = 0) uniform UBO {
    float explosionRange;
    float percentage;
} uboPerObject;

layout(set = 0, binding = 1) uniform sampler2D diffuseTexture;

void main() {
	vec4 color = vec4(0.5, 0.5, 0.5, 1);//texture(diffuseTexture, inUV).rgba;
    color.a *= 1.0f - uboPerObject.percentage;
	outColor = color;
    outNormal.rg = WasabiPackNormalSpheremapTransform(inNorm);
    outNormal.ba = vec2(2.0f, 1.0f);
}