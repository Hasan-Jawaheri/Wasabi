#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

layout(binding = 0) uniform UBOPerParticles {
	mat4x4 projection;
} uboPerParticles;

layout(binding = 1) uniform sampler2D instancingTexture;

void main() {
	uint instancingTextureWidth = textureSize(instancingTexture, 0).x;
	uint baseIndex = 4 * gl_InstanceIndex;

	uint baseU = baseIndex % instancingTextureWidth;
	uint baseV = baseIndex / instancingTextureWidth;

	vec4 m1 = texelFetch(instancingTexture, ivec2(baseU + 0, baseV), 0);
	vec4 m2 = texelFetch(instancingTexture, ivec2(baseU + 1, baseV), 0);
	vec4 m3 = texelFetch(instancingTexture, ivec2(baseU + 2, baseV), 0);
	vec4 colAndUvs = texelFetch(instancingTexture, ivec2(baseU + 3, baseV), 0);
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

	gl_Position = uboPerParticles.projection * worldView * vec4(inPos, 1.0f);
	outUV = vec2(gl_VertexIndex % 2 == 0 ? UVs.x : UVs.z, gl_VertexIndex / 2 == 0 ? UVs.y : UVs.w);
	outColor = color255 / 255.0f;
}