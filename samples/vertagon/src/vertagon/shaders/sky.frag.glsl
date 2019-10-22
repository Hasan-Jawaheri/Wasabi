#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNorm;

layout(location = 0) out vec4 outColor;

void main() {
	float l = min(1.0f, abs(dot(inNorm, vec3(0, 1, 0)) + 0.1f));
	vec4 color = vec4(vec3(0.01, 0.37, 0.59) * l, 1);
	outColor = color;
}