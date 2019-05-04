
mat4x4 LoadInstanceMatrix(
	in int instanceIndex,
	in sampler2D instancingTexture,
	in int textureWidth
) {
	uint baseIndex = 4 * instanceIndex;

	uint baseU = baseIndex % textureWidth;
	uint baseV = baseIndex / textureWidth;

	vec4 m1 = texelFetch(instancingTexture, ivec2(baseU + 0, baseV), 0);
	vec4 m2 = texelFetch(instancingTexture, ivec2(baseU + 1, baseV), 0);
	vec4 m3 = texelFetch(instancingTexture, ivec2(baseU + 2, baseV), 0);
	vec4 m4 = vec4(m1.w, m2.w, m3.w, 1.0f);
	m1.w = 0.0f;
	m2.w = 0.0f;
	m3.w = 0.0f;
	mat4x4 m = mat4x4(1.0);
	m[0] = m1;
	m[1] = m2;
	m[2] = m3;
	m[3] = m4;
	return m;
}

mat4x4 LoadBoneMatrix(
	in uint boneID,
	in sampler2D animationTexture,
	in int textureWidth
) {
	uint baseIndex = 4 * boneID;

	uint baseU = baseIndex % textureWidth;
	uint baseV = baseIndex / textureWidth;

	vec4 m1 = texelFetch(animationTexture, ivec2(baseU + 0, baseV), 0);
	vec4 m2 = texelFetch(animationTexture, ivec2(baseU + 1, baseV), 0);
	vec4 m3 = texelFetch(animationTexture, ivec2(baseU + 2, baseV), 0);
	vec4 m4 = vec4(m1.w, m2.w, m3.w, 1.0f);
	m1.w = 0.0f;
	m2.w = 0.0f;
	m3.w = 0.0f;
	mat4x4 m = mat4x4(1.0);
	m[0] = m1;
	m[1] = m2;
	m[2] = m3;
	m[3] = m4;
	return m;
}