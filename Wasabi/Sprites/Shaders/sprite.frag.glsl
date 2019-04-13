#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform UBO {
	float alpha;
} uboPerSprite;

layout(binding = 1) uniform sampler2D diffuseTexture;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outFragColor;

void main() {
	vec4 c = texture(diffuseTexture, inUV);
	outFragColor = vec4(c.rgb, c.a * uboPerSprite.alpha);
}