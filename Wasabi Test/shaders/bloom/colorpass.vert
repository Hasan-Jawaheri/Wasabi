#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 glowColor;
} ubo;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;

void main() 
{
	outUV = inUV;
/*
	if (inColor.r >= 0.9) 
	{
		outColor = ubo.glowColor.rgb;
	}
	else*/
	{
		outColor = inColor;
	}
	gl_Position = ubo.projection * ubo.model * inPos;
}
