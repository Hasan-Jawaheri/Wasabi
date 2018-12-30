R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform sampler2D colorSampler;
layout(binding = 1) uniform sampler2D normalSampler;
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outFragColor;

void main() {
	vec4 c = texture(colorSampler, inUV);
	vec4 n = texture(normalSampler, inUV);
	n.rgb = (n.rgb + 1.0) / 2.0;
	outFragColor = vec4(c.rgba);
	outFragColor.rgb = mix(outFragColor.rgb, n.rgb, 0.5f);
}
)"