R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 1) uniform sampler2D samplerColor;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inWorldNorm;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormals;

void main() {
	outColor = texture(samplerColor, inUV);
	outNormals = (vec4(inWorldNorm, 1.0) + 1) / 2;
}
)"