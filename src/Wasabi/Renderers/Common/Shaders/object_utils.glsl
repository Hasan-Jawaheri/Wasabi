
// Loads a matrix from an instance texture that was packed in 3 consecutive 4-component texels
// The packing is using the last component of every texel as x, y, and z (respectively) of the
// 4th matrix row
mat4x4 LoadMatrixFromTexture(
	in int index,
	in sampler2D matrixTexture,
	in int textureWidth
) {
	uint baseIndex = 4 * index;

	uint baseU = baseIndex % textureWidth;
	uint baseV = baseIndex / textureWidth;

	vec4 m1 = texelFetch(matrixTexture, ivec2(baseU + 0, baseV), 0);
	vec4 m2 = texelFetch(matrixTexture, ivec2(baseU + 1, baseV), 0);
	vec4 m3 = texelFetch(matrixTexture, ivec2(baseU + 2, baseV), 0);
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

vec4 LoadVector4FromTexture(
	in int index,
	in sampler2D matrixTexture,
	in int textureWidth
) {
	uint baseU = index % textureWidth;
	uint baseV = index / textureWidth;

	return texelFetch(matrixTexture, ivec2(baseU, baseV), 0);
}
