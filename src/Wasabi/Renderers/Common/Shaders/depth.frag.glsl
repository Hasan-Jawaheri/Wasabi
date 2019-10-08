#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inViewPos;
layout(location = 2) in vec3 inViewNorm;
layout(location = 3) flat in uint inTexIndex;

layout(binding = 0) uniform sampler2D colorTexture;

void main() {
}