#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
} ubo;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;

void main() 
{
	outColor = inColor;
	outUV = inUV;
	outNormal = inNormal;
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
}
