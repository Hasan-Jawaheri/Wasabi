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
	int geometryOffsetInTexture;
} pcPerGeometry;

layout(set = 0, binding = 2) uniform sampler2D instancingTexture;
layout(set = 0, binding = 3) uniform sampler2DArray heightTexture;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outWorldNorm;
layout(location = 3) flat out uint outTexIndex;

void main() {
	vec4 instanceData = LoadVector4FromTexture(gl_InstanceIndex + pcPerGeometry.geometryOffsetInTexture, instancingTexture, textureSize(instancingTexture, 0).x);
	vec3 instanceOffset = vec3(instanceData.x, 0, instanceData.y);
	int level = int(instanceData.z);
	float instanceScale = pow(2, max(0, level-1));
	float orientation = instanceData.w;
	vec4 levelData = LoadVector4FromTexture(level, instancingTexture, 64);
	vec2 levelCenter = levelData.xy;
	int N = int(levelData.z);
	float localScale = levelData.w;

	vec3 posLocal = vec3(inPos.x * (1 - orientation) + inPos.z * orientation, inPos.y, inPos.z * (1 - orientation) + inPos.x * orientation) * vec3(((1 - orientation) * 2 - 1), 1, 1);
	vec3 posScaled = posLocal * instanceScale + instanceOffset;
	vec2 _uvHeightmap = (posScaled.xz - levelCenter) / (instanceScale * localScale) + N / 2;
	ivec2 uvHeightmap = ivec2(int(_uvHeightmap.x), int(N) - 1 - int(_uvHeightmap.y));

	posScaled.y = texelFetch(heightTexture, ivec3(uvHeightmap.xy, max(0, level - 1)), 0).x;

	outUV = inUV;
	outWorldPos = (uboPerTerrain.worldMatrix * vec4(posScaled, 1.0)).xyz;
	outWorldNorm = (uboPerTerrain.worldMatrix * vec4(inNorm.xyz, 0.0)).xyz;
	outTexIndex = level;
	gl_Position = uboPerFrame.projectionMatrix * uboPerFrame.viewMatrix * vec4(outWorldPos, 1.0);
}