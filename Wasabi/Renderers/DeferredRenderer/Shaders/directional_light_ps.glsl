R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outFragColor;

layout(binding = 1) uniform sampler2D normalSampler;
layout(binding = 2) uniform sampler2D depthSampler;

layout(binding = 0) uniform UBOPerLight {
	mat4 wvp;
	vec3 lightDir;
	float lightSpec;
	vec3 lightColor;
	float intensity;
	vec3 position;
	float range;
	float minCosAngle;
} uboPerLight;

layout(binding = 3) uniform UBO {
	mat4 projInv;
} uboPerFrame;

vec4 DirectionalLight(vec3 pos, vec3 norm)
{
	vec3 lDir = -normalize(uboPerLight.lightDir);
	// N dot L lighting term
	float nl = dot(norm, lDir);
	if (nl < 0)
		return vec4(0, 0, 0, 0);

	vec3 camDir = normalize(-pos); // since pos is in view space

	// Calculate specular term
	vec3 h = normalize(lDir + camDir);
	float spec = pow(clamp(dot(norm, h), 0, 1), uboPerLight.lightSpec);

	// Fill the light buffer:
	// R: Color.r * N.L
	// G: Color.g * N.L
	// B: Color.b * N.L
	// A: Specular Term
	return vec4 (uboPerLight.lightColor * nl, spec);
}

void main() {
	float z = texture(depthSampler, inUV).r;
	float x = inUV.x * 2.0f - 1.0f;
	float y = inUV.y * 2.0f - 1.0f;
	vec4 vPositionVS = uboPerFrame.projInv * vec4 (x, y, z, 1.0f);
	vec3 pixelPosition = vPositionVS.xyz / vPositionVS.w;

	vec4 normalT = texture(normalSampler, inUV); //rgb norm, a spec

	//clip(normalT.x + normalT.y + normalT.z - 0.01); // reject pixel
	vec3 pixelNormal = normalize((normalT.xyz * 2.0f) - 1.0f);
	vec4 light = DirectionalLight(pixelPosition, pixelNormal);
	outFragColor = light * uboPerLight.intensity; //scale by intensity
}
)"