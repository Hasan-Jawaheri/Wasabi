#if 0

#include "WLightBufferRenderStage.h"
#include "../WRenderer.h"
#include "../../Core/WCore.h"
#include "../../Images/WRenderTarget.h"
#include "../../Materials/WEffect.h"
#include "../../Materials/WMaterial.h"
#include "../../Lights/WLight.h"

WLightBufferRenderStage::WLightBufferRenderStage(Wasabi* const app) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = RENDER_STAGE_TARGET_BUFFER;
	m_stageDescription.depthOutput = WRenderStage::OUTPUT_IMAGE("GBufferDepth", WColor(1.0f, 0.0f, 0.0f, 0.0f));
	m_stageDescription.colorOutputs = std::vector<WRenderStage::OUTPUT_IMAGE>({
		WRenderStage::OUTPUT_IMAGE("GBufferDiffuse", WColor(0.0f, 0.0f, 0.0f, 0.0f)),
		WRenderStage::OUTPUT_IMAGE("GBufferViewSpaceNormal", WColor(0.0f, 0.0f, 0.0f, 0.0f)),
	});
}

WError WLightBufferRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WRenderStage::Initialize(previousStages, width, height);

	m_blendState.colorWriteMask = 0xff;
	m_blendState.blendEnable = VK_TRUE;
	m_blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_blendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.colorBlendOp = VK_BLEND_OP_ADD;
	m_blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.alphaBlendOp = VK_BLEND_OP_ADD;

	m_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	m_rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
	m_rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	m_rasterizationState.depthClampEnable = VK_FALSE;
	m_rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	m_rasterizationState.depthBiasEnable = VK_FALSE;
	m_rasterizationState.lineWidth = 1.0f;

	WError werr = _LoadDirectionalLightsAssets();
	if (!werr)
		return werr;

	werr = _LoadSpotLightsAssets();
	if (!werr)
		return werr;

	werr = _LoadPointLightsAssets();
	if (!werr)
		return werr;

	// Call OnLightsChange for all current lights and register a callback to catch all future changes
	for (unsigned int i = 0; i < m_app->LightManager->GetEntitiesCount(); i++)
		OnLightsChange(m_app->LightManager->GetEntityByIndex(i), true);
	m_app->LightManager->RegisterChangeCallback(m_stageDescription.name, [this](WLight* l, bool add) { this->OnLightsChange(l, add); });

	return werr;
}

WError WLightBufferRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_OBJECTS) {
	}
}

void WLightBufferRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	m_app->LightManager->RemoveChangeCallback(m_stageDescription.name);
}

WError WLightBufferRenderStage::Resize(uint width, uint height) {
	WRenderStage::Resize(width, height);
}

void WLightBufferRenderStage::OnLightsChange(WLight* light, bool is_added) {
	auto iter = m_lightRenderingAssets.find(light->GetType());
	if (iter == m_lightRenderingAssets.end())
		return;
	LightTypeAssets assets = iter->second;

	if (is_added) {
		WMaterial* material = new WMaterial(m_app);
		WError werr = material->CreateForEffect(assets.effect);
		if (werr == W_SUCCEEDED) {
			material->SetTexture(1, m_GBuffer.m_normalTarget);
			material->SetTexture(2, m_GBuffer.m_depthTarget);
		}
		iter->second.material_map.insert(std::pair<class WLight*, class WMaterial*>(light, material));
	} else {
		auto it = assets.material_map.find(light);
		if (it != assets.material_map.end()) {
			W_SAFE_REMOVEREF(it->second);
			iter->second.material_map.erase(light);
		}
	}
}

WError WLightBufferRenderStage::_LoadPointLightsAssets() {
	LightTypeAssets assets;
	WShader* vertex_shader = new PointLightVS(m_app);
	vertex_shader->Load();
	WShader* pixel_shader = new PointLightPS(m_app);
	pixel_shader->Load();
	assets.effect = new WEffect(m_app);
	WError werr = assets.effect->BindShader(vertex_shader);
	if (!werr)
		goto error;

	werr = assets.effect->BindShader(pixel_shader);
	if (!werr)
		goto error;

	assets.effect->SetBlendingState(m_blendState);
	assets.effect->SetRasterizationState(m_rasterizationState);

	werr = assets.effect->BuildPipeline(m_renderTarget);
	if (!werr)
		goto error;

	assets.geometry = new OnlyPositionGeometry(m_app);
	assets.geometry->CreateSphere(1.0f, 10, 10);

	m_lightRenderingAssets.insert(std::pair<int, LightTypeAssets>((int)W_LIGHT_POINT, assets));

error:
	W_SAFE_REMOVEREF(pixel_shader);
	W_SAFE_REMOVEREF(vertex_shader);

	return werr;
}

WError WLightBufferRenderStage::_LoadSpotLightsAssets() {
	LightTypeAssets assets;
	WShader* vertex_shader = new SpotLightVS(m_app);
	vertex_shader->Load();
	WShader* pixel_shader = new SpotLightPS(m_app);
	pixel_shader->Load();
	assets.effect = new WEffect(m_app);
	WError werr = assets.effect->BindShader(vertex_shader);
	if (!werr)
		goto error;

	werr = assets.effect->BindShader(pixel_shader);
	if (!werr)
		goto error;

	assets.effect->SetBlendingState(m_blendState);
	assets.effect->SetRasterizationState(m_rasterizationState);

	werr = assets.effect->BuildPipeline(m_renderTarget);
	if (!werr)
		goto error;

	assets.geometry = new OnlyPositionGeometry(m_app);
	assets.geometry->CreateCone(1.0f, 1.0f, 0, 16);
	assets.geometry->ApplyTransformation(WTranslationMatrix(0, -0.5, 0) * WRotationMatrixX(W_DEGTORAD(-90)));

	m_lightRenderingAssets.insert(std::pair<int, LightTypeAssets>((int)W_LIGHT_SPOT, assets));

error:
	W_SAFE_REMOVEREF(pixel_shader);
	W_SAFE_REMOVEREF(vertex_shader);

	return werr;
}

WError WLightBufferRenderStage::_LoadDirectionalLightsAssets() {
	LightTypeAssets assets;
	WError werr(W_SUCCEEDED);
	WShader* pixel_shader = new DirectionalLightPS(m_app);
	pixel_shader->Load();
	assets.effect = m_renderer->m_app->SpriteManager->CreateSpriteEffect(pixel_shader, m_blendState);
	if (!assets.effect) {
		werr = WError(W_OUTOFMEMORY);
		goto error;
	}

	assets.fullscreen_sprite = new WSprite(m_renderer->m_app);
	assets.fullscreen_sprite->Load(false);
	assets.fullscreen_sprite->SetSize(m_renderer->m_width, m_renderer->m_height);
	assets.fullscreen_sprite->Hide();

	m_lightRenderingAssets.insert(std::pair<int, LightTypeAssets>((int)W_LIGHT_DIRECTIONAL, assets));

error:
	W_SAFE_REMOVEREF(pixel_shader);

	return werr;
}

#endif