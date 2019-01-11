R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;
layout(location = 0) out vec4 outPos;

layout(binding = 0) uniform UBOPerParticles {
	mat4 worldView;
	mat4 projection;
} uboPerParticles;

void main() {
	gl_Position = uboPerParticles.worldView * vec4(inPos.xyz, 1.0);
	outPos = gl_Position;
}
)"