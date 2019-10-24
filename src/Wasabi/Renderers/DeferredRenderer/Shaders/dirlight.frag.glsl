#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "../../Common/Shaders/lighting_utils.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outFragColor;

layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D depthTexture;

layout(set = 0, binding = 0) uniform UBOPerLight {
	mat4 wvp;
	vec3 lightDir;
	float range;
	vec3 lightColor;
	float intensity;
	vec3 position;
	float minCosAngle;
} uboPerLight;

layout(set = 1, binding = 3) uniform UBO {
	mat4 projInv;
} uboPerFrame;

void main() {
	float z = texture(depthTexture, inUV).r;
	float x = inUV.x * 2.0f - 1.0f;
	float y = inUV.y * 2.0f - 1.0f;
	vec4 vPositionVS = uboPerFrame.projInv * vec4 (x, y, z, 1.0f);
	vec3 pixelPosition = vPositionVS.xyz / vPositionVS.w;

	vec4 normalT = texture(normalTexture, inUV); //rgb norm, a spec

	//clip(normalT.x + normalT.y + normalT.z - 0.01); // reject pixel
	vec3 pixelNormal = normalize((normalT.xyz * 2.0f) - 1.0f);
	vec3 camDir = vec3(0, 0, 1); // since pixelPosition is in view space
	vec4 light = DirectionalLight(
		uboPerLight.intensity,
		pixelPosition,
		pixelNormal,
		normalT.a,
		camDir,
		uboPerLight.lightDir,
		uboPerLight.lightColor
	);
	outFragColor = vec4(light.rgb + light.rgb * light.a, 1);
}