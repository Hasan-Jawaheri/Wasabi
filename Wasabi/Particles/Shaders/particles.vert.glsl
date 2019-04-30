#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inSize;
layout(location = 2) in vec4 inUVs;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec3 outSize;
layout(location = 1) out vec4 outUVs;
layout(location = 2) out vec4 outColor;

layout(binding = 0) uniform UBOPerParticles {
	mat4 worldMatrix;
	mat4 viewMatrix;
	mat4 projMatrix;
} uboPerParticles;

void main() {
	gl_Position = uboPerParticles.worldMatrix * vec4(inPos.xyz, 1.0);
	outSize = inSize;
	outUVs = inUVs;
	outColor = inColor;
}