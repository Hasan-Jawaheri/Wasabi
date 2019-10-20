#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inTang;
layout(location = 2) in vec3 inNorm;
layout(location = 3) in vec2 inUV;
layout(location = 4) in uint inTexIndex;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNorm;

layout(set = 0, binding = 0) uniform UBO {
    float explosionRange;
    float percentage;
} uboPerObject;

layout(push_constant) uniform PushConstant {
	mat4 world;
    mat4 view;
    mat4 projection;
} pcPerObject;

void main() {
    vec3 direction = inNorm.xyz;
    vec3 pos = inPos.xyz;
    // pos *= 1.0f - uboPerObject.percentage;
    pos += uboPerObject.percentage * direction * uboPerObject.explosionRange;
	gl_Position = pcPerObject.projection * pcPerObject.view * pcPerObject.world * vec4(pos, 1.0);
	outUV = inUV;
    outNorm = (pcPerObject.view * pcPerObject.world * vec4(inNorm, 0.0)).xyz;
}