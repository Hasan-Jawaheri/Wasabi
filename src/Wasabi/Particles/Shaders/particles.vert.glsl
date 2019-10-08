#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inViewPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

layout(binding = 0) uniform UBOPerParticles {
	mat4 projMatrix;
} uboPerParticles;

void main() {
	gl_Position = uboPerParticles.projMatrix * vec4(inViewPos.xyz, 1.0);
	outUV = inUV;
	outColor = inColor;
}