#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "../../Common/Shaders/utils.glsl"

struct Light {
	vec4 color;
	vec4 dir;
	vec4 pos;
	int type;
};

layout(set = 0, binding = 0) uniform UBO {
	mat4 worldMatrix;
	float specularPower;
	float specularIntensity;
} uboPerObject;

layout(set = 1, binding = 1) uniform LUBO {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	vec3 camDirW;
	int numLights;
	Light lights[16];
} uboPerFrame;

layout(set = 0, binding = 4) uniform sampler2DArray diffuseTexture;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inWorldNorm;
layout(location = 3) flat in int inLevel;
layout(location = 4) in float inAlpha;

layout(location = 0) out vec4 outFragColor;

void main() {
	// outFragColor = vec4(inTexIndex == 0 || inTexIndex == 4 || inTexIndex == 5 ? 1 : 0, inTexIndex == 1 || inTexIndex == 4 || inTexIndex == 3 ? 1 : 0, inTexIndex == 2 || inTexIndex == 3 || inTexIndex == 5 ? 1 : 0, 1);// texture(diffuseTexture[inTexIndex], inUV);
	// outFragColor.rgb *= inAlpha * min(max(inWorldPos.y / 50.0f, 0.2f), 2.0f);
	float heightAlpha = min(max((inWorldPos.y + 70) / 150.0f, 0.5f), 2.0f);
	vec4 color = texture(diffuseTexture, vec3(inWorldPos.xz / 50.0f, heightAlpha)) * heightAlpha;
	vec3 totalLighting = vec3(0,0,0);
	for (int i = 0; i < uboPerFrame.numLights; i++) {
		float lightIntensity = uboPerFrame.lights[i].color.a;
		vec4 light;
		if (uboPerFrame.lights[i].type == 0) {
			light = WasabiDirectionalLight(
				inWorldPos,
				inWorldNorm,
				uboPerFrame.camDirW,
				uboPerObject.specularPower,
				uboPerFrame.lights[i].dir.xyz,
				uboPerFrame.lights[i].color.rgb
			);
		} else if (uboPerFrame.lights[i].type == 1) {
			light = WasabiPointLight(
				inWorldPos,
				inWorldNorm,
				uboPerFrame.camDirW,
				uboPerObject.specularPower,
				uboPerFrame.lights[i].pos.xyz,
				uboPerFrame.lights[i].color.rgb,
				uboPerFrame.lights[i].dir.a
			);
		} else if (uboPerFrame.lights[i].type == 2) {
			light = WasabiSpotLight(
				inWorldPos,
				inWorldNorm,
				uboPerFrame.camDirW,
				uboPerObject.specularPower,
				uboPerFrame.lights[i].pos.xyz,
				uboPerFrame.lights[i].dir.xyz,
				uboPerFrame.lights[i].color.rgb,
				uboPerFrame.lights[i].dir.a, // light range
				uboPerFrame.lights[i].pos.a // min cosine angle
			);
		}
		totalLighting += light.rgb * lightIntensity + light.rgb * light.a * uboPerObject.specularIntensity;
	}
	vec3 ambientLight = color.rgb * 0.2f;
	vec3 lit = color.rgb * totalLighting.rgb;
	outFragColor = vec4(ambientLight + lit, color.a);
}
