R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct Light {
	vec4 color;
	vec4 dir;
	vec4 pos;
	int type;
};

layout(set = 0, binding = 3) uniform sampler2D diffuseTexture;
layout(set = 0, binding = 4) uniform UBO {
	vec4 color;
	int isTextured;
} uboPerObject;
layout(set = 1, binding = 5) uniform LUBO {
	int numLights;
	vec3 camPosW;
	Light lights[~~~~maxLights~~~~];
} uboPerFrame;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inWorldNorm;

layout(location = 0) out vec4 outFragColor;

// DIRECTIONAL LIGHT CODE
vec3 DirectionalLight(in vec3 dir, in vec3 col, in float intensity) {
	// N dot L lighting term
	vec3 lDir = -normalize(dir); 
	float nl = clamp(dot(inWorldNorm, lDir), 0, 1);
	vec3 camDir = normalize(uboPerFrame.camPosW - inWorldPos);
	// Calculate specular term
	float spec = max (dot(reflect(-lDir, inWorldNorm), camDir), 0.0f);
	vec3 unspecced = vec3(col * nl) * intensity;
	return unspecced * spec;
}

// POINT LIGHT CODE
vec3 PointLight(in vec3 pos, in vec3 col, in float intensity, in float range) {
	// The distance from surface to light
	vec3 lightVec = pos - inWorldPos;
	float d = length (lightVec);
	// N dot L lighting term
	vec3 lDir = normalize(lightVec);
	float nl = clamp(dot(inWorldNorm, lDir), 0, 1);
	// Calculate specular term
	vec3 camDir = normalize(uboPerFrame.camPosW - inWorldPos);
	vec3 h = normalize(lDir + camDir);
	float spec = clamp(dot(inWorldNorm, h), 0, 1);

	float xVal = clamp((range-d)/range, 0, 1);
	return vec3(col * xVal * spec * nl * intensity);
}

// SPOT LIGHT CODE
vec3 SpotLight(in vec3 pos, in vec3 dir, in vec3 col, in float intensity, in float range, float minCos) {
	vec3 color = PointLight(pos, col, intensity, range);
	// The vector from the surface to the light
	vec3 lightVec = normalize(pos - inWorldPos);

	float cosAngle = dot(-lightVec, dir);
	color *= max ((cosAngle - minCos) / (1.0f-minCos), 0); // Scale color by spotlight factor
	return color;
}

void main() {
	if (uboPerObject.isTextured == 1)
		outFragColor = texture(diffuseTexture, inUV);
	else
		outFragColor = uboPerObject.color;
	vec3 lighting = vec3(0,0,0);
	for (int i = 0; i < uboPerFrame.numLights; i++) {
		if (uboPerFrame.lights[i].type == 0)
			lighting += DirectionalLight(uboPerFrame.lights[i].dir.xyz,
										 uboPerFrame.lights[i].color.rgb,
										 uboPerFrame.lights[i].color.a);
		else if (uboPerFrame.lights[i].type == 1)
			lighting += PointLight( uboPerFrame.lights[i].pos.xyz,
									uboPerFrame.lights[i].color.rgb,
									uboPerFrame.lights[i].color.a,
									uboPerFrame.lights[i].dir.a);
		else if (uboPerFrame.lights[i].type == 2)
			lighting += SpotLight(  uboPerFrame.lights[i].pos.xyz,
									uboPerFrame.lights[i].dir.xyz,
									uboPerFrame.lights[i].color.rgb,
									uboPerFrame.lights[i].color.a,
									uboPerFrame.lights[i].dir.a,
									uboPerFrame.lights[i].pos.a);
	}
	outFragColor.rgb = outFragColor.rgb * 0.2f + outFragColor.rgb * 0.8f * lighting; // ambient 20%
}
)"