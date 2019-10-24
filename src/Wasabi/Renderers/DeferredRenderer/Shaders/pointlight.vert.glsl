#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;
layout(location = 0) out vec4 outPos;

layout(set = 0, binding = 0) uniform UBOPerLight {
	mat4 wvp;
	vec3 lightDir;
	float range;
	vec3 lightColor;
	float intensity;
	vec3 position;
} uboPerLight;

void main() {
	gl_Position = uboPerLight.wvp * vec4(inPos.xyz * uboPerLight.range * 1.05f, 1.0); // scale a bit more to make the sphere big enough so edges don't make a seam
	outPos = gl_Position;
}