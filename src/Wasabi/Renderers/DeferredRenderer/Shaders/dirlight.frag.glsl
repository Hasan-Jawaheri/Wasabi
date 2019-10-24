#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "../../Common/Shaders/lighting_utils.glsl"
#include "../../Common/Shaders/utils.glsl"

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
	vec3 pixelPositionV = vPositionVS.xyz / vPositionVS.w;

	vec4 normalAndSpec = texture(normalTexture, inUV); //rg=packed-normal, b=specPower, a=specIntensityy
	vec3 pixelNormalV = unpackNormalSpheremapTransform(normalAndSpec.xy);
	float specularPower = normalAndSpec.b;
	float specularIntensity = normalAndSpec.a;
	vec3 camDirV = vec3(0, 0, 1); // since pixelPositionV is in view space

	vec4 light = DirectionalLight(
		pixelPositionV,
		pixelNormalV,
		camDirV,
		specularPower,
		uboPerLight.lightDir,
		uboPerLight.lightColor
	);
	outFragColor = vec4(light.rgb * uboPerLight.intensity + light.rgb * light.a * specularIntensity, 1);
}