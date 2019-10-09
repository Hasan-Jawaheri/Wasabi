#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

layout(push_constant) uniform PushConstant {
	mat4x4 projection;
} pcPerParticles;

layout(binding = 0) uniform sampler2D instancingTexture;

const uint pixelsPerInstance = 5;

ivec2 getTextureUV(uint index, uint textureWidth) {
	return ivec2(index % textureWidth, index / textureWidth);
}

void main() {
	uint instancingTextureWidth = textureSize(instancingTexture, 0).x;
	uint baseIndex = pixelsPerInstance * gl_InstanceIndex;
	vec4 m1 = texelFetch(instancingTexture, getTextureUV(baseIndex + 0, instancingTextureWidth), 0);
	vec4 m2 = texelFetch(instancingTexture, getTextureUV(baseIndex + 1, instancingTextureWidth), 0);
	vec4 m3 = texelFetch(instancingTexture, getTextureUV(baseIndex + 2, instancingTextureWidth), 0);
	vec4 colAndUvs = texelFetch(instancingTexture, getTextureUV(baseIndex + 3, instancingTextureWidth), 0);
	vec4 sizes = texelFetch(instancingTexture, getTextureUV(baseIndex + 4, instancingTextureWidth), 0);
	vec4 m4 = vec4(m1.w, m2.w, m3.w, 1.0f);
	m1.w = 0.0f;
	m2.w = 0.0f;
	m3.w = 0.0f;

	mat4x4 worldView;
	worldView[0] = m1;
	worldView[1] = m2;
	worldView[2] = m3;
	worldView[3] = m4;

	vec4 color255 = vec4(floor(colAndUvs.x), floor(colAndUvs.y), floor(colAndUvs.z), floor(colAndUvs.w));
	vec4 UVs = ((colAndUvs - color255) - 0.01f) * (1.0f / 0.98f);

	vec4 viewPos = worldView * vec4(inPos * sizes.x, 1.0f);
	viewPos.x += sizes.y * ((gl_VertexIndex % 2) * 2 - 1.0f);
	viewPos.y += -sizes.y * ((gl_VertexIndex / 2) * 2 - 1.0f);

	gl_Position = pcPerParticles.projection * viewPos;
	outUV.x = (gl_VertexIndex % 2) * UVs.x + (1 - (gl_VertexIndex % 2)) * UVs.z;
	outUV.y = (gl_VertexIndex / 2) * UVs.y + (1 - (gl_VertexIndex / 2)) * UVs.w;
	outColor = color255 / 255.0f;
}