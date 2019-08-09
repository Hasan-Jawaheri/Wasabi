#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(points) in;

layout(location = 0) in vec3 inSize[];
layout(location = 1) in vec4 inUVs[];
layout(location = 2) in vec4 inColor[];

layout(triangle_strip, max_vertices = 4) out;
layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

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
	vec2 uv_topleft = inUVs[0].xy;
	vec2 uv_botright = inUVs[0].zw;

	gl_Position = uboPerParticles.viewMatrix * vec4(inPosW.xyz + vec3(-sizeFlat,  0.0, sizeFlat), 1.0);
	gl_Position = uboPerParticles.projMatrix * vec4(gl_Position + vec4(-sizeBillboard, sizeBillboard, 0.0, 1.0));
	outUV = vec2(inUVs[0].x, inUVs[0].y);
	outColor = inColor[0];
    EmitVertex();

	gl_Position = uboPerParticles.viewMatrix * vec4(inPosW.xyz + vec3(-sizeFlat, 0.0, -sizeFlat), 1.0);
	gl_Position = uboPerParticles.projMatrix * vec4(gl_Position + vec4(-sizeBillboard, -sizeBillboard, 0.0, 1.0));
	outUV = vec2(inUVs[0].x, inUVs[0].w);
	outColor = inColor[0];
    EmitVertex();

	gl_Position = uboPerParticles.viewMatrix * vec4(inPosW.xyz + vec3( sizeFlat,  0.0, sizeFlat), 1.0);
	gl_Position = uboPerParticles.projMatrix * vec4(gl_Position + vec4( sizeBillboard,  sizeBillboard, 0.0, 1.0));
	outUV = vec2(inUVs[0].z, inUVs[0].y);
	outColor = inColor[0];
    EmitVertex();

	gl_Position = uboPerParticles.viewMatrix * vec4(inPosW.xyz + vec3( sizeFlat, 0.0, -sizeFlat), 1.0);
	gl_Position = uboPerParticles.projMatrix * vec4(gl_Position + vec4( sizeBillboard, -sizeBillboard, 0.0, 1.0));
	outUV = vec2(inUVs[0].z, inUVs[0].w);
	outColor = inColor[0];
    EmitVertex();

    EndPrimitive();
}