R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform sampler2D colorSampler;
layout(binding = 1) uniform sampler2D lightSampler;
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outFragColor;

layout(binding = 2) uniform UBO {
	vec4 ambient;
} ubo;

void main() {
	vec4 color = texture(colorSampler, inUV);
	vec4 light = texture(lightSampler, inUV);

	outFragColor = vec4(color.rgb * ubo.ambient.rgb + color.rgb * light.rgb * light.a, 1);
}
)"
