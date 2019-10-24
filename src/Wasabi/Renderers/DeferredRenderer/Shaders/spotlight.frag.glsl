#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "../../Common/Shaders/lighting_utils.glsl"
#include "../../Common/Shaders/utils.glsl"

layout(location = 0) in vec4 inPos;
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
	float spotRadius;
} uboPerLight;

layout(set = 1, binding = 3) uniform UBOPerFrame {
	mat4 projInv;
} uboPerFrame;

void main() {
	vec2 uv = (inPos.xy/inPos.w + 1) / 2;
	float z = texture(depthTexture, uv).r;
	float x = uv.x * 2.0f - 1.0f;
	float y = uv.y * 2.0f - 1.0f;
	vec4 vPositionVS = uboPerFrame.projInv * vec4 (x, y, z, 1.0f);
	vec3 pixelPositionV = vPositionVS.xyz / vPositionVS.w;

	vec4 normalAndSpec = texture(normalTexture, uv); //rg=packed-normal, b=specPower, a=specIntensityy
	vec3 pixelNormalV = unpackNormalSpheremapTransform(normalAndSpec.xy);
	float specularPower = normalAndSpec.b;
	float specularIntensity = normalAndSpec.a;
	vec3 camDirV = vec3(0, 0, 1); // since pixelPositionV is in view space

	vec4 light = SpotLight(
		pixelPositionV,
		pixelNormalV,
		camDirV,
		specularPower,
		uboPerLight.position,
		uboPerLight.lightDir,
		uboPerLight.lightColor,
		uboPerLight.range,
		uboPerLight.minCosAngle
	);
	outFragColor = vec4(light.rgb * uboPerLight.intensity + light.rgb * light.a * specularIntensity, 1);
}