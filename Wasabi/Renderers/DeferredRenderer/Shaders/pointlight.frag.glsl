#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec4 inPos;
layout(location = 0) out vec4 outFragColor;

layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D depthTexture;

layout(set = 0, binding = 0) uniform UBOPerLight {
	mat4 wvp;
	vec3 lightDir;
	float lightSpec;
	vec3 lightColor;
	float intensity;
	vec3 position;
	float range;
} uboPerLight;

layout(set = 1, binding = 3) uniform UBOPerFrame {
	mat4 projInv;
} uboPerFrame;

vec4 PointLight(vec3 pos, vec3 norm) {
	vec3 lightVec = uboPerLight.position - pos;
	float d = length(lightVec); // The distance from surface to light.
	
	if (d > uboPerLight.range)
		return vec4(0.0f, 0.0f, 0.0f, 0.0f);
	
	vec3 lDir = normalize(lightVec);
	
	// N dot L lighting term
	float nl = dot(norm, lDir);
	if (nl <= 0.0f)
		return vec4(0, 0, 0, 0);
	
	vec3 camDir = normalize(-pos); // since pos is in view space
	
	// Calculate specular term
	vec3 h = normalize(lDir + camDir);
	float spec = pow(clamp(dot(norm, h), 0, 1), uboPerLight.lightSpec);
	
	float xVal = (1.0f - d/uboPerLight.range);
	return vec4(uboPerLight.lightColor * nl, spec) * xVal;
}

void main() {
	vec2 uv = (inPos.xy / inPos.w + 1) / 2;
	float z = texture(depthTexture, uv).r;
	float x = uv.x * 2.0f - 1.0f;
	float y = uv.y * 2.0f - 1.0f;
	vec4 vPositionVS = uboPerFrame.projInv * vec4 (x, y, z, 1.0f);
	vec3 pixelPosition = vPositionVS.xyz / vPositionVS.w;

	vec4 normalT = texture(normalTexture, uv); //rgb norm, a spec

	//clip(normalT.x + normalT.y + normalT.z - 0.01); // reject pixel
	vec3 pixelNormal = normalize((normalT.xyz * 2.0f) - 1.0f);
	vec4 light = PointLight(pixelPosition, pixelNormal);
	outFragColor = light * uboPerLight.intensity; //scale by intensity
}