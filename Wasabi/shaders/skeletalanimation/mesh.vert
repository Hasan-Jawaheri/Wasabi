#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;
layout (location = 4) in vec4 inBoneWeights;
layout (location = 5) in ivec4 inBoneIDs;

#define MAX_BONES 128

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 bones[MAX_BONES];	
	vec4 lightPos;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outEyePos;
layout (location = 4) out vec3 outLightVec;

void main() 
{
    mat4 boneTransform = ubo.bones[inBoneIDs[0]] * inBoneWeights[0];
    boneTransform     += ubo.bones[inBoneIDs[1]] * inBoneWeights[1];
    boneTransform     += ubo.bones[inBoneIDs[2]] * inBoneWeights[2];
    boneTransform     += ubo.bones[inBoneIDs[3]] * inBoneWeights[3];	

	outNormal = inNormal;
	outColor = inColor;
	outUV = inUV;

	gl_Position = ubo.projection * ubo.model * boneTransform * vec4(inPos.xyz, 1.0);

	outEyePos = (gl_Position).xyz;
	
	vec4 lightPos = ubo.lightPos;
	outLightVec = normalize(lightPos.xyz - outEyePos);	
}