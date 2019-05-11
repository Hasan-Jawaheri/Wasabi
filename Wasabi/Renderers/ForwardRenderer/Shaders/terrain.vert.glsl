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
layout(location = 4) out float outAlpha;

void main() {
	vec4 instanceData = LoadVector4FromTexture(gl_InstanceIndex + pcPerGeometry.geometryOffsetInTexture, instancingTexture, textureSize(instancingTexture, 0).x);
	vec3 instanceOffset = vec3(instanceData.x, 0, instanceData.y);
	int level = int(instanceData.z);
	float instanceScale = pow(2, max(0, level-1));
	float orientation = instanceData.w;
	vec4 levelData = LoadVector4FromTexture(level, instancingTexture, 64);
	vec2 levelCenter = levelData.xy;
	int N = int(levelData.z);
	int M = (N + 1) / 4;
	float localScale = levelData.w;

	vec3 posLocal = vec3(inPos.x * (1 - orientation) + inPos.z * orientation, inPos.y, inPos.z * (1 - orientation) + inPos.x * orientation) * vec3(((1 - orientation) * 2 - 1), 1, 1);
	vec3 posScaled = posLocal * instanceScale + instanceOffset;
	vec2 _uvHeightmap = vec2(1,-1) * (posScaled.xz - levelCenter) / (instanceScale * localScale) + N / 2;
	ivec2 uvHeightmap = ivec2(int(_uvHeightmap.x), int(_uvHeightmap.y));

	float heights = texelFetch(heightTexture, ivec3(uvHeightmap.xy, max(0, level - 1)), 0).x;
	float height = floor(heights) / 100.0f;
	float coarserHeight = height + (fract(heights) - 0.5f) * 1000.0f;
	float fineAlpha = min(1.0f, max(0.0f, 3.5f - max(
		4.0f * abs(posScaled.x - levelCenter.x) / (instanceScale * localScale * (N / 2.0f + 0.25f)),
		4.0f * abs(posScaled.z - levelCenter.y) / (instanceScale * localScale * (N / 2.0f + 0.25f))
	)));

	outWorldPos = posScaled;
	outWorldPos.y = height * fineAlpha + coarserHeight * (1.0f-fineAlpha);
	outUV = inUV;
	outWorldNorm = vec4(inNorm.xyz, 0.0).xyz;
	outTexIndex = level;
	outAlpha = fineAlpha;
	gl_Position = uboPerFrame.projectionMatrix * uboPerFrame.viewMatrix * vec4(outWorldPos, 1.0);
}