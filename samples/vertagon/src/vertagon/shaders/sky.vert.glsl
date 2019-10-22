#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inTang;
layout(location = 2) in vec3 inNorm;
layout(location = 3) in vec2 inUV;
layout(location = 4) in uint inTexIndex;

layout(set = 0, binding = 0) uniform UBOPerObject {
	mat4 wvp;
} uboPerObject;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNorm;

void main() {
	outUV = inUV;
    outNorm = inNorm;
	gl_Position = uboPerObject.wvp * vec4(inPos.xyz, 1.0);
}