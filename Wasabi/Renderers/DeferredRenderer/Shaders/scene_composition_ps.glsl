R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 2) uniform sampler2D colorSampler;
layout(binding = 3) uniform sampler2D lightSampler;
layout(binding = 4) uniform sampler2D normalSampler;
layout(binding = 5) uniform sampler2D depthSampler;
layout(binding = 6) uniform sampler2D backfaceDepthSampler;
layout(binding = 7) uniform sampler2D randomSampler;
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outFragColor;

layout(binding = 0) uniform UBO {
	mat4x4 projInv;
} ubo;

layout(binding=1) uniform UBOParams {
	vec4 ambient;
	float SSAOSampleRadius;
	float SSAOIntensity;
	float SSAODistanceScale;
	float SSAOAngleBias;
	float camFarClip;
} uboParams;

vec3 getPosition(vec2 uv) {
	float z = texture(depthSampler, uv).r;
	float x = uv.x * 2.0f - 1.0f;
	float y = uv.y * 2.0f - 1.0f;
	vec4 vPositionVS = ubo.projInv * vec4 (x, y, z, 1.0f);
	return vPositionVS.xyz / vPositionVS.w;
}

vec3 getPositionBackface(vec2 uv) {
	float z = texture(backfaceDepthSampler, uv).r;
	float x = uv.x * 2.0f - 1.0f;
	float y = uv.y * 2.0f - 1.0f;
	vec4 vPositionVS = ubo.projInv * vec4 (x, y, z, 1.0f);
	return vPositionVS.xyz / vPositionVS.w;
}

vec3 getNormal(vec2 uv) {
	vec4 normalT = texture(normalSampler, uv); //rgb norm, a spec
	return normalize((normalT.xyz * 2.0f) - 1.0f);
}

vec2 getRandom(vec2 uv) {
	return texture(randomSampler, vec2(2500,1000) * uv / 100).xy * 2.0f - 1.0f;
}

float getAmbientOcclusion(vec2 tcoord, vec2 uv, vec3 p, vec3 cnorm) {
	float intensity = 2;
	float scale = 1;
	float bias = 0.2;

	vec3 diff = getPosition(tcoord + uv) - p;
	vec3 v = normalize(diff);
	float d = length(diff) * uboParams.SSAODistanceScale;
	return max(0.0, dot(cnorm,v)-uboParams.SSAOAngleBias) * (1.0/(1.0+d)) * uboParams.SSAOIntensity;
}

float getBackfaceAmbientOcclusion(vec2 tcoord, vec2 uv, vec3 p, vec3 cnorm) {
	float intensity = 2;
	float scale = 1;
	float bias = 0.2;

	vec3 diff = getPositionBackface(tcoord + uv) - p;
	vec3 v = normalize(diff);
	float d = length(diff) * uboParams.SSAODistanceScale;
	return max(0.0, dot(cnorm,v)-uboParams.SSAOAngleBias) * (1.0/(1.0+d)) * uboParams.SSAOIntensity;
}

void main() {
	vec4 color = texture(colorSampler, inUV);
	vec4 light = texture(lightSampler, inUV);

	vec2 occludersUVs[4] = {vec2(1, 0), vec2(-1, 0), vec2(0, 1), vec2(0, -1)};
	vec3 occluderPos = getPosition(inUV);
	vec3 occluderNorm = getNormal(inUV);
	vec2 rand = getRandom(inUV);
	float rad = uboParams.SSAOSampleRadius / occluderPos.z;

	float ao = 0.0f;
	int iterations = int(mix(6.0, 2.0, occluderPos.z / uboParams.camFarClip));
	for (int i = 0; i < iterations; i++) {
		vec2 coord1 = reflect(occludersUVs[i], rand) * rad;
		vec2 coord2 = vec2(coord1.x*0.707 - coord1.y*0.707, coord1.x*0.707 + coord1.y*0.707);

		ao += getAmbientOcclusion(inUV, coord1*0.25, occluderPos, occluderNorm);
		ao += getAmbientOcclusion(inUV, coord2*0.5, occluderPos, occluderNorm);
		ao += getAmbientOcclusion(inUV, coord1*0.75, occluderPos, occluderNorm);
		ao += getAmbientOcclusion(inUV, coord2*1.0, occluderPos, occluderNorm);

		ao += getBackfaceAmbientOcclusion(inUV, coord1*(0.25+0.125), occluderPos, occluderNorm);
		ao += getBackfaceAmbientOcclusion(inUV, coord2*(0.5+0.125), occluderPos, occluderNorm);
		ao += getBackfaceAmbientOcclusion(inUV, coord1*(0.75), occluderPos, occluderNorm);
		ao += getBackfaceAmbientOcclusion(inUV, coord2*(1.0+0.125), occluderPos, occluderNorm);
	}

	ao /= iterations * 4.0f;

	//outFragColor = vec4(vec3(1-(min(1, ao))), 1);
	outFragColor = vec4((color.rgb * uboParams.ambient.rgb - vec3(max(0, ao))) + color.rgb * light.rgb * light.a, 1);
}
)"
