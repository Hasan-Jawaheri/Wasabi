R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) out vec2 outUV;

layout(binding = 0) uniform UBOPerParticles {
	mat4 worldView;
	mat4 projection;
} uboPerParticles;

void main() {
    // gl_in.length() == 1

	vec3 inPosV = gl_in[0].gl_Position.xyz;

	float size = 0.1;
	gl_Position = uboPerParticles.projection * vec4(inPosV.xyz + vec3(-size,  size, 0), 1.0);
	outUV = vec2(0.0, 0.0);
    EmitVertex();
	gl_Position = uboPerParticles.projection * vec4(inPosV.xyz + vec3(-size, -size, 0), 1.0);
	outUV = vec2(0.0, 1.0);
    EmitVertex();
	gl_Position = uboPerParticles.projection * vec4(inPosV.xyz + vec3( size,  size, 0), 1.0);
	outUV = vec2(1.0, 0.0);
    EmitVertex();
	gl_Position = uboPerParticles.projection * vec4(inPosV.xyz + vec3( size, -size, 0), 1.0);
	outUV = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}
)"