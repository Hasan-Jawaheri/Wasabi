R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inTang;
layout(location = 2) in vec3 inNorm;
layout(location = 3) in vec2 inUV;
layout(location = 4) in uvec4 boneIndex;
layout(location = 5) in vec4 boneWeight;

struct Light {
	vec4 color;
	vec4 dir;
	vec4 pos;
	int type;
};

layout(set = 0, binding = 0) uniform UBO {
	mat4 worldMatrix;
	int animationTextureWidth;
	int instanceTextureWidth;
	int isAnimated;
	int isInstanced;
	vec4 color;
} uboPerObject;

layout(set = 1, binding = 1) uniform LUBO {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	vec3 camPosW;
	int numLights;
	Light lights[~~~~maxLights~~~~];
} uboPerFrame;

layout(set = 0, binding = 2) uniform sampler2D animationTexture;
layout(set = 0, binding = 3) uniform sampler2D instancingTexture;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outWorldNorm;

mat4x4 LoadInstanceMatrix() {
	if (uboPerObject.isInstanced == 1) {
		uint baseIndex = 4 * gl_InstanceIndex;

		uint baseU = baseIndex % uboPerObject.instanceTextureWidth;
		uint baseV = baseIndex / uboPerObject.instanceTextureWidth;

		vec4 m1 = texelFetch(instancingTexture, ivec2(baseU + 0, baseV), 0);
		vec4 m2 = texelFetch(instancingTexture, ivec2(baseU + 1, baseV), 0);
		vec4 m3 = texelFetch(instancingTexture, ivec2(baseU + 2, baseV), 0);
		vec4 m4 = vec4(m1.w, m2.w, m3.w, 1.0f);
		m1.w = 0.0f;
		m2.w = 0.0f;
		m3.w = 0.0f;
		mat4x4 m = mat4x4(1.0);
		m[0] = m1;
		m[1] = m2;
		m[2] = m3;
		m[3] = m4;
		return m;
	} else
		return mat4x4(1.0);
}

mat4x4 LoadBoneMatrix(uint boneID)
{
	uint baseIndex = 4 * boneID;

	uint baseU = baseIndex % uboPerObject.animationTextureWidth;
	uint baseV = baseIndex / uboPerObject.animationTextureWidth;

	vec4 m1 = texelFetch(animationTexture, ivec2(baseU + 0, baseV), 0);
	vec4 m2 = texelFetch(animationTexture, ivec2(baseU + 1, baseV), 0);
	vec4 m3 = texelFetch(animationTexture, ivec2(baseU + 2, baseV), 0);
	vec4 m4 = vec4(m1.w, m2.w, m3.w, 1.0f);
	m1.w = 0.0f;
	m2.w = 0.0f;
	m3.w = 0.0f;
	mat4x4 m = mat4x4(1.0);
	m[0] = m1;
	m[1] = m2;
	m[2] = m3;
	m[3] = m4;
	return m;
}

void main() {
	outUV = inUV;
	mat4x4 animMtx = mat4x4(1.0) * (1-uboPerObject.isAnimated);
	mat4x4 instMtx = LoadInstanceMatrix();
	if (boneWeight.x * uboPerObject.isAnimated > 0.001f)
	{
		animMtx += boneWeight.x * LoadBoneMatrix(boneIndex.x); 
		if (boneWeight.y > 0.001f)
		{
			animMtx += boneWeight.y * LoadBoneMatrix(boneIndex.y); 
			if (boneWeight.z > 0.001f)
			{
				animMtx += boneWeight.z * LoadBoneMatrix(boneIndex.z); 
				if (boneWeight.w > 0.001f)
				{
					animMtx += boneWeight.w * LoadBoneMatrix(boneIndex.w); 
				}
			}
		}
	}
	vec4 localPos1 = animMtx * vec4(inPos.xyz, 1.0);
	vec4 localPos2 = instMtx * vec4(localPos1.xyz, 1.0);
	outWorldPos = (uboPerObject.worldMatrix * localPos2).xyz;
	outWorldNorm = (uboPerObject.worldMatrix * vec4(inNorm.xyz, 0.0)).xyz;
	gl_Position = uboPerFrame.projectionMatrix * uboPerFrame.viewMatrix * vec4(outWorldPos, 1.0);
}
)"