R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(points) in;
in vs_out {
	vec3 size;
	float alpha;
} gs_in[];

layout(triangle_strip, max_vertices = 4) out;
layout(location = 0) out vec2 outUV;
layout(location = 1) out float outAlpha;

layout(binding = 0) uniform UBOPerParticles {
	mat4 world;
	mat4 view;
	mat4 proj;
} uboPerParticles;

void main() {
    // gl_in.length() == 1

	vec3 inPosW = gl_in[0].gl_Position.xyz;
	float sizeFlat = gs_in[0].size.x;
	float sizeBillboard = gs_in[0].size.y;

	// pass-thru
	outAlpha = gs_in[0].alpha;

	gl_Position = uboPerParticles.view * vec4(inPosW.xyz + vec3(-sizeFlat,  0.0, sizeFlat), 1.0);
	gl_Position = uboPerParticles.proj * vec4(gl_Position + vec4(-sizeBillboard, sizeBillboard, 0.0, 1.0));
	outUV = vec2(0.0, 0.0);
    EmitVertex();

	gl_Position = uboPerParticles.view * vec4(inPosW.xyz + vec3(-sizeFlat, 0.0, -sizeFlat), 1.0);
	gl_Position = uboPerParticles.proj * vec4(gl_Position + vec4(-sizeBillboard, -sizeBillboard, 0.0, 1.0));
	outUV = vec2(0.0, 1.0);
    EmitVertex();

	gl_Position = uboPerParticles.view * vec4(inPosW.xyz + vec3( sizeFlat,  0.0, sizeFlat), 1.0);
	gl_Position = uboPerParticles.proj * vec4(gl_Position + vec4( sizeBillboard,  sizeBillboard, 0.0, 1.0));
	outUV = vec2(1.0, 0.0);
    EmitVertex();

	gl_Position = uboPerParticles.view * vec4(inPosW.xyz + vec3( sizeFlat, 0.0, -sizeFlat), 1.0);
	gl_Position = uboPerParticles.proj * vec4(gl_Position + vec4( sizeBillboard, -sizeBillboard, 0.0, 1.0));
	outUV = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}
)"