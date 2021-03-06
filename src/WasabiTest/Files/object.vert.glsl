#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inTang;
layout(location = 2) in vec3 inNorm;
layout(location = 3) in vec2 inUV;
layout(location = 4) in int textureIndex;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outWorldNorm;

layout(binding = 0) uniform UBO {
    mat4 worldMatrix;
    mat4 viewMatrix;
	mat4 projectionMatrix;
} ubo;

void main() {
    outUV = inUV;
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.worldMatrix * vec4(inPos.xyz, 1.0);
	outWorldNorm = (ubo.worldMatrix * vec4(inNorm, 0.0f)).xyz;
}
