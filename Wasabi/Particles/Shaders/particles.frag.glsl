#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 inUV;
layout(location = 1) in float inAlpha;
layout(location = 0) out vec4 outFragColor;

layout(binding = 1) uniform sampler2D diffuseTexture;

void main() {
	vec4 color = texture(diffuseTexture, inUV).rgba;
	color.a *= inAlpha;
	outFragColor = color;
}