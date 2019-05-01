#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform sampler2D textureFont;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inCol;

layout(location = 0) out vec4 outFragColor;

void main() {
	float c = texture(textureFont, inUV).r;
	outFragColor = vec4(c) * inCol;
}