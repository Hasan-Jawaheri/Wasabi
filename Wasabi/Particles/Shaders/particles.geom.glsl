#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(points) in;

layout(location = 0) in vec3 inSize[];
layout(location = 1) in float inAlpha[];

layout(triangle_strip, max_vertices = 4) out;
layout(location = 0) out vec2 outUV;
layout(location = 1) out float outAlpha;

layout(binding = 0) uniform UBOPerParticles {
	mat4 worldMatrix;
	mat4 viewMatrix;
	mat4 projMatrix;
} uboPerParticles;

void main() {
    // gl_in.length() == 1

	vec3 inPosW = gl_in[0].gl_Position.xyz;
	float sizeFlat = inSize[0].x;
	float sizeBillboard = inSize[0].y;

	// pass-thru
	outAlpha = inAlpha[0];

	gl_Position = uboPerParticles.viewMatrix * vec4(inPosW.xyz + vec3(-sizeFlat,  0.0, sizeFlat), 1.0);
	gl_Position = uboPerParticles.projMatrix * vec4(gl_Position + vec4(-sizeBillboard, sizeBillboard, 0.0, 1.0));
	outUV = vec2(0.0, 0.0);
    EmitVertex();

	gl_Position = uboPerParticles.viewMatrix * vec4(inPosW.xyz + vec3(-sizeFlat, 0.0, -sizeFlat), 1.0);
	gl_Position = uboPerParticles.projMatrix * vec4(gl_Position + vec4(-sizeBillboard, -sizeBillboard, 0.0, 1.0));
	outUV = vec2(0.0, 1.0);
    EmitVertex();

	gl_Position = uboPerParticles.viewMatrix * vec4(inPosW.xyz + vec3( sizeFlat,  0.0, sizeFlat), 1.0);
	gl_Position = uboPerParticles.projMatrix * vec4(gl_Position + vec4( sizeBillboard,  sizeBillboard, 0.0, 1.0));
	outUV = vec2(1.0, 0.0);
    EmitVertex();

	gl_Position = uboPerParticles.viewMatrix * vec4(inPosW.xyz + vec3( sizeFlat, 0.0, -sizeFlat), 1.0);
	gl_Position = uboPerParticles.projMatrix * vec4(gl_Position + vec4( sizeBillboard, -sizeBillboard, 0.0, 1.0));
	outUV = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}