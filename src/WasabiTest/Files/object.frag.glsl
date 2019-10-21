#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inWorldNorm;

layout(location = 0) out vec4 outFragColor;

layout(binding = 3) uniform sampler2D diffuseTexture;

layout(binding = 4) uniform UBO {
    vec4 customColor;
} ubo;

void main() {
    outFragColor = texture(diffuseTexture, inUV) + ubo.customColor;
}
