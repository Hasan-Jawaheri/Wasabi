
// Return values:
// R: Color.r * N.L
// G: Color.g * N.L
// B: Color.b * N.L
// A: Specular Term

vec4 DirectionalLight(
	in float intensity,
	in vec3 pixelPos,
	in vec3 pixelNorm,
	in float pixelSpec,
	in vec3 camDir,
	in vec3 lightDir,
	in vec3 lightColor
) {
	vec3 lDir = normalize(lightDir);
	// N dot L lighting term
	float nl = max(0, dot(pixelNorm, -lDir));

	// Calculate specular term
	vec3 h = normalize(-lDir + camDir);
	float spec = pow(clamp(dot(pixelNorm, h), 0, 1), pixelSpec);

	return vec4(lightColor * nl, spec);
}

vec4 PointLight(
	in float intensity,
	in vec3 pixelPos,
	in vec3 pixelNorm,
	in float pixelSpec,
	in vec3 camDir,
	in vec3 lightPos,
	in vec3 lightColor,
	in float lightRange
) {
	vec3 lightVec = pixelPos - lightPos;
	float d = length(lightVec); // The distance from surface to light.

	vec3 lDir = normalize(lightVec);

	// N dot L lighting term
	float nl = max(0, dot(pixelNorm, -lDir));

	// Calculate specular term
    vec3 reflection = normalize(2.0f * dot(-lDir, pixelNorm) * pixelNorm + lDir);
	float spec = pow(clamp(dot(-camDir, reflection), 0, 1), pixelSpec);

	float xVal = max(0, (1.0f - d / lightRange));
	return vec4(lightColor * nl * xVal, spec);
}

vec4 SpotLight(
	in float intensity,
	in vec3 pixelPos,
	in vec3 pixelNorm,
	in float pixelSpec,
	in vec3 camDir,
	in vec3 lightPos,
	in vec3 lightDir,
	in vec3 lightColor,
	in float lightRange,
	in float minCosAngle
) {
	vec4 color = PointLight(intensity, pixelPos, pixelNorm, pixelSpec, camDir, lightPos, lightColor, lightRange);

	// The vector from the surface to the light.
	vec3 lightVec = normalize(lightPos - pixelPos);

	float cosAngle = dot(-lightVec, lightDir);
	float angleFactor = max((cosAngle - minCosAngle) / (1.0f - minCosAngle), 0);
	color *= angleFactor;

	// Scale color by spotlight factor.
	return color;
}
