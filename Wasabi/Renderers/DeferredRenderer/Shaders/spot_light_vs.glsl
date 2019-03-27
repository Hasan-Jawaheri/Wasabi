R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;
layout(location = 0) out vec4 outPos;

layout(set = 0, binding = 0) uniform UBOPerLight {
	mat4 wvp;
	vec3 lightDir;
	float lightSpec;
	vec3 lightColor;
	float intensity;
	vec3 position;
	float range;
	float minCosAngle;
	float spotRadius;
} uboPerLight;

void main() {
	vec3 localPos = inPos.xyz * vec3(uboPerLight.spotRadius, uboPerLight.spotRadius, uboPerLight.range);
	gl_Position = uboPerLight.wvp * vec4(localPos, 1.0);
	outPos = gl_Position;
}
)"