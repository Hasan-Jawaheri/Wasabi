#include "WForwardRenderer.h"

class ForwardRendererVS : public WShader {
public:
	ForwardRendererVS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, {
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "gProjection"), // projection
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "gWorld"), // world
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "gView"), // view
			}),
		};
		m_desc.input_layout = W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // normal
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // tangent
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 2), // UV
		});
		LoadCodeGLSL(
			"#version 450\n"
			""
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			""
			"layout(location = 0) in vec3 inPos;\n"
			"layout(location = 1) in vec3 inNorm;\n"
			"layout(location = 2) in vec3 inTang;\n"
			"layout(location = 3) in vec2 inUV;\n"
			""
			"layout(binding = 0) uniform UBO {\n"
			"	mat4 projectionMatrix;\n"
			"	mat4 modelMatrix;\n"
			"	mat4 viewMatrix;\n"
			"} ubo;\n"
			""
			"layout(location = 0) out vec2 outUV;\n"
			"layout(location = 1) out vec3 outWorldNorm;\n"
			""
			"void main() {\n"
			"	outUV = inUV;\n"
			"	outWorldNorm = (ubo.modelMatrix * vec4(inNorm.xyz, 0.0)).xyz;\n"
			"	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);\n"
			"}\n"
		);
	}
};

class ForwardRendererPS : public WShader {
public:
	ForwardRendererPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
			W_BOUND_RESOURCE(W_TYPE_UBO, 2, {
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4, "color"),
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "isTextured"),
			}),
		};
		LoadCodeGLSL(
			"#version 450\n"
			""
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			""
			"layout(binding = 1) uniform sampler2D samplerColor;\n"
			"layout(binding = 2) uniform UBO {\n"
			"	vec4 color;\n"
			"	int isTextured;\n"
			"} ubo;\n"
			""
			"layout(location = 0) in vec2 inUV;\n"
			"layout(location = 1) in vec3 inWorldNorm;\n"
			""
			"layout(location = 0) out vec4 outFragColor;\n"
			""
			"void main() {\n"
			"	if (ubo.isTextured == 1)\n"
			"		outFragColor = texture(samplerColor, inUV);\n"
			"	else\n"
			"		outFragColor = ubo.color;\n"
			"	float L = clamp(dot(inWorldNorm, vec3(0,1,0)) + 0.2, 0, 1);\n"
			"	outFragColor *= L;\n"
			"}\n"
		);
	}
};

WForwardRenderer::WForwardRenderer(Wasabi* const app) : WRenderer(app) {
	m_sampler = VK_NULL_HANDLE;
	m_default_fx = nullptr;
}

WError WForwardRenderer::Initiailize() {
	//
	// Create the texture sampler
	//
	VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	// Max level-of-detail should match mip level count
	sampler.maxLod = 1;
	// Enable anisotropic filtering
	sampler.maxAnisotropy = 8;
	sampler.anisotropyEnable = VK_TRUE;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VkResult err = vkCreateSampler(m_device, &sampler, nullptr, &m_sampler);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	//
	// Create the default effect for rendering
	//
	WShader* vs = new ForwardRendererVS(m_app);
	vs->Load();
	WShader* ps = new ForwardRendererPS(m_app);
	ps->Load();
	m_default_fx = new WEffect(m_app);
	m_default_fx->BindShader(vs);
	m_default_fx->BindShader(ps);
	WError werr = m_default_fx->BuildPipeline(m_renderTarget);
	vs->RemoveReference();
	ps->RemoveReference();
	if (!werr)
		return werr;

	return WError(W_SUCCEEDED);
}

WError WForwardRenderer::Resize(unsigned int width, unsigned int height) {
	return WRenderer::Resize(width, height);
}

WError WForwardRenderer::Render(WRenderTarget* rt) {
	WError err = rt->Begin();
	if (!err)
		return err;

	m_app->ObjectManager->Render(rt);

	m_app->SpriteManager->Render(rt);

	m_app->TextComponent->Render(rt);

	return rt->End();
}

void WForwardRenderer::Cleanup() {
	if (m_sampler)
		vkDestroySampler(m_device, m_sampler, nullptr);
	m_sampler = VK_NULL_HANDLE;

	W_SAFE_REMOVEREF(m_default_fx);
}

class WMaterial* WForwardRenderer::CreateDefaultMaterial() {
	WFRMaterial* mat = new WFRMaterial(m_app);
	mat->SetEffect(m_default_fx);
	float r = (float)(rand() % 224 + 32) / 256.0f;
	float g = (float)(rand() % 224 + 32) / 256.0f;
	float b = (float)(rand() % 224 + 32) / 256.0f;
	mat->SetColor(WColor(r, g, b, 1.0f));
	return mat;
}

VkSampler WForwardRenderer::GetDefaultSampler() const {
	return m_sampler;
}

WFRMaterial::WFRMaterial(Wasabi* const app, unsigned int ID) : WMaterial(app, ID) {

}

WError WFRMaterial::Texture(class WImage* img) {
	int isTextured = 1;
	SetVariableInt("isTextured", isTextured);
	return SetTexture(1, img);
}

WError WFRMaterial::SetColor(WColor col) {
	int isTextured = 0;
	SetVariableInt("isTextured", isTextured);
	return SetVariableColor("color", col);
}

