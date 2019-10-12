#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "../../Common/Shaders/lighting_utils.glsl"

struct Light {
	vec4 color;
	vec4 dir;
	vec4 pos;
	int type;
};

layout(set = 0, binding = 0) uniform UBO {
	mat4 worldMatrix;
	vec4 color;
	int isInstanced;
	int isTextured;
} uboPerObject;

layout(set = 1, binding = 1) uniform LUBO {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	vec3 camPosW;
	int numLights;
	Light lights[16];
} uboPerFrame;

layout(set = 0, binding = 4) uniform sampler2D diffuseTexture[8];

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inWorldNorm;
layout(location = 3) flat in uint inTexIndex;

layout(location = 0) out vec4 outFragColor;

void main() {
	outFragColor = texture(diffuseTexture[inTexIndex], inUV) * uboPerObject.isTextured + uboPerObject.color;
	vec3 camDir = normalize(uboPerFrame.camPosW - inWorldPos);
	vec3 lighting = vec3(0,0,0);
	for (int i = 0; i < uboPerFrame.numLights; i++) {
		if (uboPerFrame.lights[i].type == 0) {
			vec4 light = DirectionalLight(
				inWorldPos,
				inWorldNorm,
				camDir,
				uboPerFrame.lights[i].dir.xyz,
				uboPerFrame.lights[i].color.rgb,
				uboPerFrame.lights[i].color.a
			);
			lighting += light.rgb * light.a;
		} else if (uboPerFrame.lights[i].type == 1) {
			vec4 light = PointLight(
				inWorldPos,
				inWorldNorm,
				camDir,
				uboPerFrame.lights[i].pos.xyz,
				uboPerFrame.lights[i].color.rgb,
				uboPerFrame.lights[i].color.a,
				uboPerFrame.lights[i].dir.a
			);
			lighting += light.rgb * light.a;
		} else if (uboPerFrame.lights[i].type == 2) {
			vec4 light = SpotLight(
				inWorldPos,
				inWorldNorm,
				camDir,
				uboPerFrame.lights[i].pos.xyz,
				uboPerFrame.lights[i].dir.xyz,
				uboPerFrame.lights[i].color.rgb,
				uboPerFrame.lights[i].color.a,
				uboPerFrame.lights[i].dir.a, // light range
				uboPerFrame.lights[i].pos.a // min cosine angle
			);
			lighting += light.rgb * light.a;
		}
	}
	outFragColor.rgb = outFragColor.rgb * 0.2f + outFragColor.rgb * 0.8f * lighting; // ambient 20%
}
