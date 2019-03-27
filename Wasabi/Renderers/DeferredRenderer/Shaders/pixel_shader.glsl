R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform UBOPerObject {
	mat4 worldMatrix;
	int animationTextureWidth;
	int instanceTextureWidth;
	int isAnimated;
	int isInstanced;
	vec4 color;
} uboPerObject;

layout(set = 0, binding = 4) uniform sampler2D diffuseTexture;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inViewPos;
layout(location = 2) in vec3 inViewNorm;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormals;

void main() {
	outColor = texture(diffuseTexture, inUV) + uboPerObject.color;
	outNormals = (vec4(inViewNorm, 1.0) + 1) / 2;
}
)"