#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in  vec2 inPos;
layout(location = 1) in  vec2 inUV;
layout(location = 2) in  vec4 inCol;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outCol;

void main() {
	outUV = inUV;
	outCol = inCol;
	gl_Position = vec4(inPos.xy, 0.0, 1.0);
}