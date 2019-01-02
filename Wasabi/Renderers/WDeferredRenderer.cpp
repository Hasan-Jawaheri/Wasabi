#include "WDeferredRenderer.h"
#include "../Core/WCore.h"
#include "../Cameras/WCamera.h"
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
			#include "Shaders/Deferred/vertex_shader.glsl"
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
			#include "Shaders/Deferred/pixel_shader.glsl"
		);
	}
};

class DirectionalLightPS : public WShader {
public:
	DirectionalLightPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 0),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
			W_BOUND_RESOURCE(W_TYPE_UBO, 2, {
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "viewProjInv"), // view * projection
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3, "eyePosW"), // camera position
			}),
			W_BOUND_RESOURCE(W_TYPE_UBO, 3, {
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3, "lightDir"), // light direction (L vector)
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3, "lightColor"), // light color
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 1, "lightSpec"), // light specular
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 1, "intensity"), // light intensity
			}),
		};
		LoadCodeGLSL(
			#include "Shaders/Deferred/directional_light_ps.glsl"
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
			#include "Shaders/Deferred/scene_composition_ps.glsl"
		);
	}
};

WDeferredRenderer::WDeferredRenderer(Wasabi* const app) : WRenderer(app), m_GBuffer(this), m_LightBuffer(this) {
	m_sampler = VK_NULL_HANDLE;
	m_default_fx = nullptr;
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

	//
	// Create the GBuffer
	//
	WError werr = m_GBuffer.Initialize(m_width, m_height);
	if (!werr) {
		Cleanup();
		return werr;
	}

	//
	// Create the default effect for rendering
	//
	WShader* vs = new DeferredRendererVS(m_app);
	vs->Load();
	WShader* ps = new DeferredRendererPS(m_app);
	ps->Load();
	m_default_fx = new WEffect(m_app);
	m_default_fx->BindShader(vs);
	m_default_fx->BindShader(ps);
	werr = m_default_fx->BuildPipeline(m_GBuffer.m_renderTarget);
	vs->RemoveReference();
	ps->RemoveReference();
	if (!werr)
		Cleanup();

	return werr;
}

WError WDeferredRenderer::LoadDependantResources() {
	VkPipelineColorBlendAttachmentState bs = {};
	VkPipelineDepthStencilStateCreateInfo dss = {};

	//
	// Create the LightBuffer
	//
	WError werr = m_LightBuffer.Initialize(m_width, m_height);
	if (!werr)
		goto error;

	//
	// Create the final composition material
	//

	WShader* compositionVS = m_app->SpriteManager->GetSpriteVertexShader();
	WShader* compositionPS = new SceneCompositionPS(m_app);
	compositionPS->Load();
	WEffect* compositionFX = new WEffect(m_app);
	werr = compositionFX->BindShader(compositionVS);
	if (!werr)
		goto error;

	werr = compositionFX->BindShader(compositionPS);
	if (!werr)
		goto error;

	bs.colorWriteMask = 0xf;
	bs.blendEnable = VK_FALSE;
	bs.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	compositionFX->SetBlendingState(bs);

	dss.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	dss.depthTestEnable = VK_FALSE;
	dss.depthWriteEnable = VK_FALSE;
	dss.depthBoundsTestEnable = VK_FALSE;
	dss.stencilTestEnable = VK_FALSE;
	dss.front = dss.back;
	compositionFX->SetDepthStencilState(dss);

	werr = compositionFX->BuildPipeline(m_renderTarget);
	if (!werr)
		goto error;

	m_compositionMaterial = new WMaterial(m_app);
	werr = m_compositionMaterial->SetEffect(compositionFX);
	if (!werr)
		goto error;

	werr = m_compositionMaterial->SetTexture(0, m_GBuffer.m_colorTarget);
	if (!werr)
		goto error;

	werr = m_compositionMaterial->SetTexture(1, m_LightBuffer.m_outputTarget);
	if (!werr)
		goto error;

	m_masterRenderSprite = new WSprite(m_app);
	m_masterRenderSprite->SetMaterial(m_compositionMaterial);
	m_masterRenderSprite->SetSize(m_width, m_height);
	m_masterRenderSprite->Hide();

	goto success;

error:
	Cleanup();

success:
	W_SAFE_REMOVEREF(compositionPS);
	W_SAFE_REMOVEREF(compositionFX);

	return werr;
}

WError WDeferredRenderer::Resize(unsigned int width, unsigned int height) {
	WError werr = WRenderer::Resize(width, height);
	if (!werr)
		return werr;

	werr = m_GBuffer.Resize(width, height);
	if (!werr)
		return werr;

	werr = m_LightBuffer.Resize(width, height);
	if (!werr)
		return werr;

	m_masterRenderSprite->SetSize(width, height);

	return werr;
}

void WDeferredRenderer::Render(WRenderTarget* rt, unsigned int filter) {
	WError werr;

	//
	// Render to the GBuffer then to the LightBuffer
	//
	if (filter & RENDER_FILTER_OBJECTS) {
		m_GBuffer.Render(rt->GetCamera());
		m_LightBuffer.Render(rt->GetCamera());
	}
	
	//
	// Compose the GBuffer & LightBuffer onto the final render target
	//
	werr = rt->Begin();
	if (!werr)
		return;

	m_masterRenderSprite->Show();
	m_masterRenderSprite->Render(rt);
	m_masterRenderSprite->Hide();

	//
	// Render 2D assets
	//
	if (filter & RENDER_FILTER_SPRITES) {
		m_app->SpriteManager->Render(rt);
	}

	if (filter & RENDER_FILTER_TEXT)
		m_app->TextComponent->Render(rt);

	rt->End(false);
}

void WDeferredRenderer::RenderLight(class WLight* light) {

}

void WDeferredRenderer::Cleanup() {
	if (m_sampler)
		vkDestroySampler(m_device, m_sampler, nullptr);
	m_sampler = VK_NULL_HANDLE;

	W_SAFE_REMOVEREF(m_default_fx);
	W_SAFE_REMOVEREF(m_masterRenderSprite);
	W_SAFE_REMOVEREF(m_compositionMaterial);
	m_GBuffer.Cleanup();
	m_LightBuffer.Cleanup();
}

class WMaterial* WDeferredRenderer::CreateDefaultMaterial() {
	WMaterial* mat = new WMaterial(m_app);
	mat->SetEffect(m_default_fx);
	return mat;
}

VkSampler WDeferredRenderer::GetDefaultSampler() const {
	return m_sampler;
}

WDeferredRenderer::GBufferStage::GBufferStage(WDeferredRenderer* renderer) : m_renderer(renderer) {
	m_renderTarget = nullptr;
	m_depthTarget = nullptr;
	m_colorTarget = nullptr;
	m_normalTarget = nullptr;
}

WDeferredRenderer::GBufferStage::~GBufferStage() {
	Cleanup();
}

void WDeferredRenderer::GBufferStage::Cleanup() {
	W_SAFE_REMOVEREF(m_renderTarget);
	W_SAFE_REMOVEREF(m_depthTarget);
	W_SAFE_REMOVEREF(m_colorTarget);
	W_SAFE_REMOVEREF(m_normalTarget);
}

WError WDeferredRenderer::GBufferStage::Initialize(unsigned int width, unsigned int height) {
	Cleanup();

	m_depthTarget = new WImage(m_renderer->m_app);
	m_colorTarget = new WImage(m_renderer->m_app);
	m_normalTarget = new WImage(m_renderer->m_app);
	m_renderTarget = new WRenderTarget(m_renderer->m_app);
	return Resize(m_renderer->m_width, m_renderer->m_height);
}

WError WDeferredRenderer::GBufferStage::Render(class WCamera* cam) {
	m_renderTarget->SetCamera(cam);
	WError werr = m_renderTarget->Begin();
	if (!werr)
		return werr;

	m_renderer->m_app->ObjectManager->Render(m_renderTarget);

	werr = m_renderTarget->End();
	if (!werr)
		return werr;

	return werr;
}

WError WDeferredRenderer::GBufferStage::Resize(unsigned int width, unsigned int height) {
	WError werr;

	if (m_renderTarget) {
		werr = m_depthTarget->CreateFromPixelsArray(nullptr, width, height, false, 1, VK_FORMAT_D16_UNORM, 2);
		if (!werr)
			return werr;

		werr = m_colorTarget->CreateFromPixelsArray(nullptr, width, height, false, 4, VK_FORMAT_R8G8B8A8_UNORM, 1);
		if (!werr)
			return werr;

		werr = m_normalTarget->CreateFromPixelsArray(nullptr, width, height, false, 4, VK_FORMAT_R8G8B8A8_UNORM, 1);
		if (!werr)
			return werr;

		werr = m_renderTarget->Create(width, height, vector<WImage*>({ m_colorTarget, m_normalTarget }), m_depthTarget);
		if (!werr)
			return werr;

		m_renderTarget->SetClearColor(WColor(0, 0, 0, 0), 0);
		m_renderTarget->SetClearColor(WColor(0, 0, 0, 0), 1);
		m_renderTarget->SetClearColor(WColor(1, 0, 0, 0), 2);
	}

	return werr;
}

WDeferredRenderer::LightBufferStage::LightBufferStage(WDeferredRenderer* renderer) : m_renderer(renderer) {
	m_renderTarget = nullptr;
	m_outputTarget = nullptr;
}

WDeferredRenderer::LightBufferStage::~LightBufferStage() {
	Cleanup();
}

void WDeferredRenderer::LightBufferStage::Cleanup() {
	for (auto iter = m_lightRenderingAssets.begin(); iter != m_lightRenderingAssets.end(); iter++)
		iter->second.Destroy();
	m_lightRenderingAssets.clear();
	W_SAFE_REMOVEREF(m_renderTarget);
	W_SAFE_REMOVEREF(m_outputTarget);
}

WError WDeferredRenderer::LightBufferStage::Initialize(unsigned int width, unsigned int height) {
	Cleanup();

	m_outputTarget = new WImage(m_renderer->m_app);
	m_renderTarget = new WRenderTarget(m_renderer->m_app);

	WError werr = _LoadDirectionalLightsAssets();
	if (!werr)
		return werr;

	werr = _LoadSpotLightsAssets();
	if (!werr)
		return werr;

	werr = _LoadPointLightsAssets();
	if (!werr)
		return werr;

	return Resize(m_renderer->m_width, m_renderer->m_height);
}

WError WDeferredRenderer::LightBufferStage::_LoadPointLightsAssets() {
	LightTypeAssets assets;
	
	m_lightRenderingAssets.insert(std::pair<int, LightTypeAssets>((int)W_LIGHT_POINT, assets));
	return WError(W_SUCCEEDED);
}

WError WDeferredRenderer::LightBufferStage::_LoadSpotLightsAssets() {
	LightTypeAssets assets;

	m_lightRenderingAssets.insert(std::pair<int, LightTypeAssets>((int)W_LIGHT_SPOT, assets));
	return WError(W_SUCCEEDED);
}

WError WDeferredRenderer::LightBufferStage::_LoadDirectionalLightsAssets() {
	LightTypeAssets assets;
	WShader* vertex_shader = m_renderer->m_app->SpriteManager->GetSpriteVertexShader();
	WShader* pixel_shader = new DirectionalLightPS(m_renderer->m_app);
	pixel_shader->Load();
	WEffect* effect = new WEffect(m_renderer->m_app);
	WError werr = effect->BindShader(vertex_shader);
	if (!werr)
		goto error;

	werr = effect->BindShader(pixel_shader);
	if (!werr)
		goto error;

	VkPipelineColorBlendAttachmentState bs;
	bs.colorWriteMask = 0xf;
	bs.blendEnable = VK_FALSE;
	bs.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	effect->SetBlendingState(bs);

	werr = effect->BuildPipeline(m_renderTarget);
	if (!werr)
		goto error;

	assets.material = new WMaterial(m_renderer->m_app);
	werr = assets.material->SetEffect(effect);
	if (!werr)
		goto error;

	werr = assets.material->SetTexture(0, m_renderer->m_GBuffer.m_normalTarget);
	if (!werr)
		goto error;

	werr = assets.material->SetTexture(1, m_renderer->m_GBuffer.m_depthTarget);
	if (!werr)
		goto error;

	assets.fullscreen_sprite = new WSprite(m_renderer->m_app);
	assets.fullscreen_sprite->SetMaterial(assets.material);
	assets.fullscreen_sprite->SetSize(m_renderer->m_width, m_renderer->m_height);
	assets.fullscreen_sprite->Hide();

	m_lightRenderingAssets.insert(std::pair<int, LightTypeAssets>((int)W_LIGHT_DIRECTIONAL, assets));

error:
	W_SAFE_REMOVEREF(pixel_shader);
	W_SAFE_REMOVEREF(effect);

	return werr;
}

WError WDeferredRenderer::LightBufferStage::Render(class WCamera* cam) {
	m_renderTarget->SetCamera(cam);
	WError werr = m_renderTarget->Begin();
	if (!werr)
		return werr;

	for (auto iter = m_lightRenderingAssets.begin(); iter != m_lightRenderingAssets.end(); iter++) {
		if (!iter->second.material)
			continue;
		iter->second.material->SetVariableMatrix("viewProjInv", WMatrixInverse(cam->GetProjectionMatrix()) * WMatrixInverse(cam->GetViewMatrix()));
		iter->second.material->SetVariableVector3("eyePosW", cam->GetPosition());
	}

	unsigned int num_lights = m_renderer->m_app->LightManager->GetEntitiesCount();
	for (unsigned int i = 0; i < num_lights; i++) {
		WLight* light = m_renderer->m_app->LightManager->GetEntityByIndex(i);
		int light_type = (int)light->GetType();
		auto iter = m_lightRenderingAssets.find(light_type);
		if (iter == m_lightRenderingAssets.end())
			return WError(W_ERRORUNK);
		WDeferredRenderer::LightBufferStage::LightTypeAssets assets = iter->second;

		if (!assets.material)
			continue;

		// make sure the light is in the camera's frustum

		// setup the material for this light
		WColor lightColor = light->GetColor();
		assets.material->SetVariableVector3("lightDir", light->GetLVector());
		assets.material->SetVariableVector3("lightColor", WVector3(lightColor.r, lightColor.g, lightColor.b));
		assets.material->SetVariableFloat("lightSpec", lightColor.a); // specular power is store in alpha component
		assets.material->SetVariableFloat("intensity", light->GetIntensity());

		// render the light onto the light map
		assets.material->Bind(m_renderTarget);

		if (assets.fullscreen_sprite) {
			assets.fullscreen_sprite->Show();
			assets.fullscreen_sprite->Render(m_renderTarget);
			assets.fullscreen_sprite->Hide();
		}

		if (assets.geometry)
			assets.geometry->Draw(m_renderTarget);
	}

	werr = m_renderTarget->End();
	if (!werr)
		return werr;

	return werr;
}

WError WDeferredRenderer::LightBufferStage::Resize(unsigned int width, unsigned int height) {
	WError werr;

	if (m_renderTarget) {
		werr = m_outputTarget->CreateFromPixelsArray(nullptr, width, height, false, 4, VK_FORMAT_R8G8B8A8_UNORM, 1);
		if (!werr)
			return werr;

		werr = m_renderTarget->Create(width, height, m_outputTarget);
		if (!werr)
			return werr;
		m_renderTarget->SetClearColor(WColor(0, 0, 0, 0));
		
		for (auto iter = m_lightRenderingAssets.begin(); iter != m_lightRenderingAssets.end(); iter++)
			if (iter->second.fullscreen_sprite)
				iter->second.fullscreen_sprite->SetSize(width, height);
	}

	return werr;
}

