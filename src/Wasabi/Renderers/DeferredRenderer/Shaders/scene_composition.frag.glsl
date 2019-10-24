#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "../../Common/Shaders/utils.glsl"

layout(set = 1, binding = 2) uniform sampler2D diffuseTexture;
layout(set = 1, binding = 3) uniform sampler2D lightTexture;
layout(set = 1, binding = 4) uniform sampler2D normalTexture;
layout(set = 1, binding = 5) uniform sampler2D depthTexture;
layout(set = 1, binding = 6) uniform sampler2D backfaceDepthTexture;
layout(set = 1, binding = 7) uniform sampler2D randomTexture;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform UBOPerFrame {
	mat4x4 projInv;
} uboPerFrame;

layout(set = 1, binding = 1) uniform UBOParams {
	vec4 ambient;
	float SSAOSampleRadius;
	float SSAOIntensity;
	float SSAODistanceScale;
	float SSAOAngleBias;
	float camFarClip;
} uboParams;

vec3 getPosition_depth(vec2 uv, float z) {
	float x = uv.x * 2.0f - 1.0f;
	float y = uv.y * 2.0f - 1.0f;
	vec4 vPositionVS = uboPerFrame.projInv * vec4 (x, y, z, 1.0f);
	return vPositionVS.xyz / vPositionVS.w;
}

vec3 getPosition(vec2 uv) {
	float z = texture(depthTexture, uv).r;
	return getPosition_depth(uv, z);
}

vec3 getPositionBackface(vec2 uv) {
	float z = texture(backfaceDepthTexture, uv).r;
	float x = uv.x * 2.0f - 1.0f;
	float y = uv.y * 2.0f - 1.0f;
	vec4 vPositionVS = uboPerFrame.projInv * vec4 (x, y, z, 1.0f);
	return vPositionVS.xyz / vPositionVS.w;
}

vec3 getNormal(vec2 uv) {
	vec4 normalAndSpec = texture(normalTexture, uv); //rg is packed norm
	return WasabiUnpackNormalSpheremapTransform(normalAndSpec.xy);
}

vec2 getRandom(vec2 uv) {
	return vec2(0,0);//texture(randomTexture, vec2(2500,1000) * uv / 100).xy * 2.0f - 1.0f;
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
	vec4 color = texture(diffuseTexture, inUV);
	vec4 light = texture(lightTexture, inUV);

	vec2 occludersUVs[4] = {vec2(1, 0), vec2(-1, 0), vec2(0, 1), vec2(0, -1)};
	float depth = texture(depthTexture, inUV).r;
	vec3 occluderPos = getPosition_depth(inUV, depth);
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

	ao /= iterations * 8.0f;

	vec3 ambientLight = max(vec3(0,0,0), color.rgb * uboParams.ambient.rgb - vec3(ao));
	vec3 lit = color.rgb * light.rgb;
	outFragColor = vec4(ambientLight + lit, color.a);
	gl_FragDepth = depth;
}