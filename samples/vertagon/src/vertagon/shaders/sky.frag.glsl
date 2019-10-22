#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNorm;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;

void main() {
	vec4 color = vec4(1.0, 0.5, 0.5, 1);
	outColor = color;
    outNormal = (vec4(inNorm.xyz, 1.0f) + 1) / 2;
}