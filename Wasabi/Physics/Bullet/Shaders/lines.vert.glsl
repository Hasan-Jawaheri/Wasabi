#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inCol;

layout(location = 0) out vec4 outCol;

layout(binding = 0) uniform UBO{
	mat4x4 proj;
	mat4x4 view;
} uboPerFrame;

void main() {
	outCol = inCol;
	gl_Position = uboPerFrame.proj * uboPerFrame.view * vec4(inPos.xyz, 1.0);
}