#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "../../Common/Shaders/object_utils.glsl"

// per-vertex data
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inTang;
layout(location = 2) in vec3 inNorm;
layout(location = 3) in vec2 inUV;
layout(location = 4) in uint inTexIndex;

struct Light {
	vec4 color;
	vec4 dir;
	vec4 pos;
	int type;
};

layout(set = 0, binding = 0) uniform UBO {
	mat4 worldMatrix;
} uboPerTerrain;

layout(set = 1, binding = 1) uniform LUBO {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	vec3 camPosW;
	int numLights;
	Light lights[16];
} uboPerFrame;

layout(push_constant) uniform PushConstant {
	int offsetInTexture;
} pcPerGeometry;

layout(set = 0, binding = 2) uniform sampler2D instancingTexture;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outWorldNorm;
layout(location = 3) flat out uint outTexIndex;

void main() {
	vec4 data = LoadVector4FromTexture(gl_InstanceIndex + pcPerGeometry.offsetInTexture, instancingTexture, 64);
	vec3 instanceOffset = vec3(data.x, 0, data.y);
	int level = int(data.z);
	float instanceScale = pow(2, max(0, level-1));
	float orientation = data.w;
	vec3 pos = vec3(inPos.x * (1 - orientation) + inPos.z * orientation, inPos.y, inPos.z * (1 - orientation) + inPos.x * orientation) * vec3(((1 - orientation) * 2 - 1), 1, 1);

	outUV = inUV;
	outWorldPos = (uboPerTerrain.worldMatrix * vec4(pos * instanceScale + instanceOffset, 1.0)).xyz;
	outWorldNorm = (uboPerTerrain.worldMatrix * vec4(inNorm.xyz, 0.0)).xyz;
	outTexIndex = level;
	gl_Position = uboPerFrame.projectionMatrix * uboPerFrame.viewMatrix * vec4(outWorldPos, 1.0);
}