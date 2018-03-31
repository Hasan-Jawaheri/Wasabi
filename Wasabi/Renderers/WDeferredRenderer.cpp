#include "WDeferredRenderer.h"
#include "../Core/WCore.h"
#include "../Materials/WEffect.h"
#include "../Materials/WMaterial.h"
#include "../Lights/WLight.h"
#include "../Images/WRenderTarget.h"
#include "../Images/WImage.h"
#include "../Objects/WObject.h"
#include "../Sprites/WSprite.h"
#include "../Texts/WText.h"

class DeferredRendererVS : public WShader {
public:
	DeferredRendererVS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0,{
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "gProjection"), // projection
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "gWorld"), // world
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "gView"), // view
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "gAnimationTextureWidth"), // width of the animation texture
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "gInstanceTextureWidth"), // width of the instance texture
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "gAnimation"), // whether or not animation is enabled
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "gInstancing"), // whether or not instancing is enabled
			}),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 2),
		};
		m_desc.input_layouts = {W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // normal
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // tangent
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 2), // UV
		}), W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 4), // bone index
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4), // bone weight
		})};
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
			"layout(location = 4) in uvec4 boneIndex;\n"
			"layout(location = 5) in vec4 boneWeight;\n"
			""
			"layout(binding = 0) uniform UBO {\n"
			"	mat4 projectionMatrix;\n"
			"	mat4 modelMatrix;\n"
			"	mat4 viewMatrix;\n"
			"	int animationTextureWidth;\n"
			"	int instanceTextureWidth;\n"
			"	int is_animated;\n"
			"	int is_instanced;\n"
			"} ubo;\n"
			"layout(binding = 1) uniform sampler2D samplerAnimation;\n"
			"layout(binding = 2) uniform sampler2D samplerInstancing;\n"
			""
			"layout(location = 0) out vec2 outUV;\n"
			"layout(location = 1) out vec3 outWorldPos;\n"
			"layout(location = 2) out vec3 outWorldNorm;\n"
			""
			"mat4x4 LoadInstanceMatrix() {\n"
			"	if (ubo.is_instanced == 1) {\n"
			"		uint baseIndex = 4 * gl_InstanceIndex;\n"
			""
			"		uint baseU = baseIndex % ubo.instanceTextureWidth;\n"
			"		uint baseV = baseIndex / ubo.instanceTextureWidth;\n"
			""
			"		vec4 m1 = texelFetch(samplerInstancing, ivec2(baseU + 0, baseV), 0);\n"
			"		vec4 m2 = texelFetch(samplerInstancing, ivec2(baseU + 1, baseV), 0);\n"
			"		vec4 m3 = texelFetch(samplerInstancing, ivec2(baseU + 2, baseV), 0);\n"
			"		vec4 m4 = vec4(m1.w, m2.w, m3.w, 1.0f);\n"
			"		m1.w = 0.0f;\n"
			"		m2.w = 0.0f;\n"
			"		m3.w = 0.0f;\n"
			"		mat4x4 m = mat4x4(1.0);\n"
			"		m[0] = m1;\n"
			"		m[1] = m2;\n"
			"		m[2] = m3;\n"
			"		m[3] = m4;\n"
			"		return m;\n"
			"	} else\n"
			"		return mat4x4(1.0);\n"
			"}\n"
			""
			"mat4x4 LoadBoneMatrix(uint boneID)\n"
			"{\n"
			"	uint baseIndex = 4 * boneID;\n"
			""
			"	uint baseU = baseIndex % ubo.animationTextureWidth;\n"
			"	uint baseV = baseIndex / ubo.animationTextureWidth;\n"
			""
			"	vec4 m1 = texelFetch(samplerAnimation, ivec2(baseU + 0, baseV), 0);\n"
			"	vec4 m2 = texelFetch(samplerAnimation, ivec2(baseU + 1, baseV), 0);\n"
			"	vec4 m3 = texelFetch(samplerAnimation, ivec2(baseU + 2, baseV), 0);\n"
			"	vec4 m4 = vec4(m1.w, m2.w, m3.w, 1.0f);\n"
			"	m1.w = 0.0f;\n"
			"	m2.w = 0.0f;\n"
			"	m3.w = 0.0f;\n"
			"	mat4x4 m = mat4x4(1.0);\n"
			"	m[0] = m1;\n"
			"	m[1] = m2;\n"
			"	m[2] = m3;\n"
			"	m[3] = m4;\n"
			"	return m;\n"
			"}\n"
			""
			"void main() {\n"
			"	outUV = inUV;\n"
			"	mat4x4 animMtx = mat4x4(1.0) * (1-ubo.is_animated);\n"
			"	mat4x4 instMtx = LoadInstanceMatrix();\n"
			"	if (boneWeight.x * ubo.is_animated > 0.001f)\n"
			"	{\n"
			"		animMtx += boneWeight.x * LoadBoneMatrix(boneIndex.x); \n"
			"		if (boneWeight.y > 0.001f)\n"
			"		{\n"
			"			animMtx += boneWeight.y * LoadBoneMatrix(boneIndex.y); \n"
			"			if (boneWeight.z > 0.001f)\n"
			"			{\n"
			"				animMtx += boneWeight.z * LoadBoneMatrix(boneIndex.z); \n"
			"				if (boneWeight.w > 0.001f)\n"
			"				{\n"
			"					animMtx += boneWeight.w * LoadBoneMatrix(boneIndex.w); \n"
			"				}\n"
			"			}\n"
			"		}\n"
			"	}\n"
			"	vec4 localPos1 = animMtx * vec4(inPos.xyz, 1.0);\n"
			"	vec4 localPos2 = instMtx * vec4(localPos1.xyz, 1.0);\n"
			"	outWorldPos = (ubo.modelMatrix * localPos2).xyz;\n"
			"	outWorldNorm = (ubo.modelMatrix * vec4(inNorm.xyz, 0.0)).xyz;\n"
			"	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(outWorldPos, 1.0);\n"
			"}\n"
		);
	}
};

class DeferredRendererPS : public WShader {
public:
	DeferredRendererPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
		};
		LoadCodeGLSL(
			"#version 450\n"
			""
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			""
			"layout(binding = 1) uniform sampler2D samplerColor;\n"
			""
			"layout(location = 0) in vec2 inUV;\n"
			"layout(location = 1) in vec3 inWorldPos;\n"
			"layout(location = 2) in vec3 inWorldNorm;\n"
			""
			"layout(location = 0) out vec4 outColor;\n"
			"layout(location = 1) out vec4 outNormals;\n"
			""
			"void main() {\n"
			"	outColor = texture(samplerColor, inUV);\n"
			"	outNormals = vec4(inWorldNorm, 1.0);\n"
			"}\n"
		);
	}
};

class SceneCompositionPS : public WShader {
public:
	SceneCompositionPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 0),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
		};
		LoadCodeGLSL(
			"#version 450\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			""
			"layout(binding = 0) uniform sampler2D colorSampler;\n"
			"layout(binding = 1) uniform sampler2D normalSampler;\n"
			"layout(location = 0) in vec2 inUV;\n"
			"layout(location = 0) out vec4 outFragColor;\n"
			""
			"void main() {\n"
			"	vec4 c = texture(colorSampler, inUV);\n"
			"   vec4 n = texture(normalSampler, inUV);\n"
			"   n.rgb = (n.rgb + 1.0) / 2.0;\n"
			"	outFragColor = vec4(c.rgba);\n"
			"   outFragColor.rgb = mix(outFragColor.rgb, n.rgb, 0.5f);\n"
			"}\n"
		);
	}
};

WDeferredRenderer::WDeferredRenderer(Wasabi* const app) : WRenderer(app) {
	m_sampler = VK_NULL_HANDLE;
	m_default_fx = nullptr;
	m_GBuffer = nullptr;
	m_GBufferColor = nullptr;
	m_GBufferNormal = nullptr;
	m_masterRenderSprite = nullptr;
	m_compositionMaterial = nullptr;
}

WError WDeferredRenderer::Initiailize() {
	Cleanup();

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
	if (err != VK_SUCCESS) {
		Cleanup();
		return WError(W_OUTOFMEMORY);
	}

	WError werr;

	//
	// Create the GBuffer
	//
	m_GBufferColor = new WImage(m_app);
	werr = m_GBufferColor->CreateFromPixelsArray(nullptr, m_width, m_height, false, 4, VK_FORMAT_R8G8B8A8_UNORM, 4);
	if (werr == W_SUCCEEDED) {
		m_GBufferNormal = new WImage(m_app);
		werr = m_GBufferNormal->CreateFromPixelsArray(nullptr, m_width, m_height, false, 4, VK_FORMAT_R16G16B16A16_SFLOAT, 4);
		if (werr == W_SUCCEEDED) {
			m_GBuffer = new WRenderTarget(m_app);
			werr = m_GBuffer->Create(m_width, m_height, vector<WImage*>({ m_GBufferColor, m_GBufferNormal }));
		}
	}

	//
	// Create the default effect for rendering
	//
	if (werr == W_SUCCEEDED) {
		WShader* vs = new DeferredRendererVS(m_app);
		vs->Load();
		WShader* ps = new DeferredRendererPS(m_app);
		ps->Load();
		m_default_fx = new WEffect(m_app);
		m_default_fx->BindShader(vs);
		m_default_fx->BindShader(ps);
		werr = m_default_fx->BuildPipeline(m_GBuffer);
		vs->RemoveReference();
		ps->RemoveReference();
	}

	if (werr != W_SUCCEEDED)
		Cleanup();

	return werr;
}

WError WDeferredRenderer::LoadDependantResources() {
	//
	// Create the final composition material
	//

	WShader* compositionVS = m_app->SpriteManager->GetSpriteVertexShader();
	WShader* compositionPS = new SceneCompositionPS(m_app);
	compositionPS->Load();
	WEffect* compositionFX = new WEffect(m_app);
	WError werr = compositionFX->BindShader(compositionVS);
	if (werr == W_SUCCEEDED) {
		werr = compositionFX->BindShader(compositionPS);
		if (werr == W_SUCCEEDED) {
			VkPipelineColorBlendAttachmentState bs;
			bs.colorWriteMask = 0xf;
			bs.blendEnable = VK_FALSE;
			bs.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			compositionFX->SetBlendingState(bs);

			VkPipelineDepthStencilStateCreateInfo dss = {};
			dss.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			dss.depthTestEnable = VK_FALSE;
			dss.depthWriteEnable = VK_FALSE;
			dss.depthBoundsTestEnable = VK_FALSE;
			dss.stencilTestEnable = VK_FALSE;
			dss.front = dss.back;
			compositionFX->SetDepthStencilState(dss);

			werr = compositionFX->BuildPipeline(m_renderTarget);

			if (werr == W_SUCCEEDED) {
				m_compositionMaterial = new WMaterial(m_app);
				werr = m_compositionMaterial->SetEffect(compositionFX);
				if (werr == W_SUCCEEDED) {
					werr = m_compositionMaterial->SetTexture(0, m_GBufferColor);
					if (werr == W_SUCCEEDED) {
						werr = m_compositionMaterial->SetTexture(1, m_GBufferNormal);
						if (werr == W_SUCCEEDED) {
							m_masterRenderSprite = new WSprite(m_app);
							m_masterRenderSprite->SetMaterial(m_compositionMaterial);
							m_masterRenderSprite->SetSize(m_width, m_height);
						}
					}
				}
			}
		}
	}

	compositionFX->RemoveReference();
	compositionPS->RemoveReference();

	if (werr != W_SUCCEEDED)
		Cleanup();

	return werr;
}

WError WDeferredRenderer::Resize(unsigned int width, unsigned int height) {
	WError werr = WRenderer::Resize(width, height);

	if (werr == W_SUCCEEDED && m_GBuffer) {
		werr = m_GBufferColor->CreateFromPixelsArray(nullptr, width, height, false, 4, VK_FORMAT_R32G32B32A32_SFLOAT, 4);
		if (werr == W_SUCCEEDED) {
			werr = m_GBufferNormal->CreateFromPixelsArray(nullptr, width, height, false, 4, VK_FORMAT_R32G32B32A32_SFLOAT, 4);
			if (werr == W_SUCCEEDED) {
				werr = m_GBuffer->Create(width, height, vector<WImage*>({ m_GBufferColor, m_GBufferNormal }));
			}
		}
		if (werr == W_SUCCEEDED)
			m_masterRenderSprite->SetSize(width, height);
	}

	return werr;
}

void WDeferredRenderer::Render(WRenderTarget* rt, unsigned int filter) {
	WError werr;

	//
	// Render to the GBuffer
	//
	m_GBuffer->SetCamera(rt->GetCamera());
	werr = m_GBuffer->Begin();
	if (werr != W_SUCCEEDED)
		return;

	if (filter & RENDER_FILTER_OBJECTS)
		m_app->ObjectManager->Render(m_GBuffer);

	werr = m_GBuffer->End();
	if (werr != W_SUCCEEDED)
		return;

	//
	// Compose the GBuffer onto the final render target
	//
	werr = rt->Begin();
	if (werr != W_SUCCEEDED)
		return;

	m_masterRenderSprite->Show();
	m_masterRenderSprite->Render(rt);
	m_masterRenderSprite->Hide();

	if (filter & RENDER_FILTER_SPRITES) {
		m_app->SpriteManager->Render(rt);
	}

	if (filter & RENDER_FILTER_TEXT)
		m_app->TextComponent->Render(rt);

	rt->End(false);
}

void WDeferredRenderer::Cleanup() {
	if (m_sampler)
		vkDestroySampler(m_device, m_sampler, nullptr);
	m_sampler = VK_NULL_HANDLE;

	W_SAFE_REMOVEREF(m_default_fx);
	W_SAFE_REMOVEREF(m_GBuffer);
	W_SAFE_REMOVEREF(m_GBufferColor);
	W_SAFE_REMOVEREF(m_GBufferNormal);
	W_SAFE_REMOVEREF(m_masterRenderSprite);
	W_SAFE_REMOVEREF(m_compositionMaterial);
}

class WMaterial* WDeferredRenderer::CreateDefaultMaterial() {
	WMaterial* mat = new WMaterial(m_app);
	mat->SetEffect(m_default_fx);
	return mat;
}

VkSampler WDeferredRenderer::GetDefaultSampler() const {
	return m_sampler;
}

