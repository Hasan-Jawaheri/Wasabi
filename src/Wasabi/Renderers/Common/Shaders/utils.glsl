
vec2 WasabiPackNormalSpheremapTransform(vec3 norm) {
    vec2 packed = normalize(norm.xy) * (sqrt(-norm.z*0.5+0.5));
    packed = packed * 0.5 + 0.5;
    return packed;
}

vec3 WasabiUnpackNormalSpheremapTransform(vec2 packed2) {
	vec4 packed = vec4(packed2, 0, 0);
    vec4 nn = packed * vec4(2,2,0,0) + vec4(-1,-1,1,-1);
    float l = dot(nn.xyz, -nn.xyw);
    nn.z = l;
    nn.xy *= sqrt(l);
    vec3 unpacked = nn.xyz * 2 + vec3(0,0,-1);
    return unpacked;
}

vec4 WasabiDirectionalLight(
	in vec3 pixelPos,
	in vec3 pixelNorm,
	in vec3 camDir,
	in float specularPower,
	in vec3 lightDir,
	in vec3 lightColor
) {
	vec3 lDir = normalize(lightDir);
	// N dot L lighting term
	float nl = max(0, dot(pixelNorm, -lDir));

	// Calculate specular term
    vec3 reflection = normalize(2.0f * dot(-lDir, pixelNorm) * pixelNorm + lDir);
	float spec = pow(clamp(dot(-camDir, reflection), 0, 1), specularPower);

	return vec4(lightColor * nl, spec);
}

vec4 WasabiPointLight(
	in vec3 pixelPos,
	in vec3 pixelNorm,
	in vec3 camDir,
	in float specularPower,
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
	float spec = pow(clamp(dot(-camDir, reflection), 0, 1), specularPower);

	float xVal = max(0, (1.0f - d / lightRange));
	return vec4(lightColor * nl * xVal, spec);
}

vec4 WasabiSpotLight(
	in vec3 pixelPos,
	in vec3 pixelNorm,
	in vec3 camDir,
	in float specularPower,
	in vec3 lightPos,
	in vec3 lightDir,
	in vec3 lightColor,
	in float lightRange,
	in float minCosAngle
) {
	vec4 color = WasabiPointLight(pixelPos, pixelNorm, camDir, specularPower, lightPos, lightColor, lightRange);

	// The vector from the surface to the light.
	vec3 lightVec = normalize(lightPos - pixelPos);

	float cosAngle = dot(-lightVec, lightDir);
	float angleFactor = max((cosAngle - minCosAngle) / (1.0f - minCosAngle), 0);
	color *= angleFactor;

	// Scale color by spotlight factor.
	return color;
}
