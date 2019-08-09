
// Return values:
// R: Color.r * N.L
// G: Color.g * N.L
// B: Color.b * N.L
// A: Specular Term

vec4 DirectionalLight(
	in vec3 pixelPos,
	in vec3 pixelNorm,
	in vec3 camDir,
	in vec3 lightDir,
	in vec3 lightColor,
	in float lightSpec
) {
	vec3 lDir = -normalize(lightDir);
	// N dot L lighting term
	float nl = max(0, dot(pixelNorm, lDir));

	// Calculate specular term
	vec3 h = normalize(lDir + camDir);
	float spec = pow(clamp(dot(pixelNorm, h), 0, 1), lightSpec);

	return vec4(lightColor * nl, spec);
}

vec4 PointLight(
	in vec3 pixelPos,
	in vec3 pixelNorm,
	in vec3 camDir,
	in vec3 lightPos,
	in vec3 lightColor,
	in float lightSpec,
	in float lightRange
) {
	vec3 lightVec = lightPos - pixelPos;
	float d = length(lightVec); // The distance from surface to light.

	vec3 lDir = normalize(lightVec);

	// N dot L lighting term
	float nl = max(0, dot(pixelNorm, lDir));

	// Calculate specular term
	vec3 h = normalize(lDir + camDir);
	float spec = pow(clamp(dot(pixelNorm, h), 0, 1), lightSpec);

	float xVal = max(0, (1.0f - d / lightRange));
	return vec4(lightColor * nl, spec) * xVal;
}

vec4 SpotLight(
	in vec3 pixelPos,
	in vec3 pixelNorm,
	in vec3 camDir,
	in vec3 lightPos,
	in vec3 lightDir,
	in vec3 lightColor,
	in float lightSpec,
	in float lightRange,
	in float minCosAngle
) {
	vec4 color = PointLight(pixelPos, pixelNorm, camDir, lightPos, lightColor, lightSpec, lightRange);

	// The vector from the surface to the light.
	vec3 lightVec = normalize(lightPos - pixelPos);

	float cosAngle = dot(-lightVec, lightDir);
	float angleFactor = max((cosAngle - minCosAngle) / (1.0f - minCosAngle), 0);
	color *= angleFactor;

	// Scale color by spotlight factor.
	return color;
}
