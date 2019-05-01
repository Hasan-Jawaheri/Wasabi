#include "WLightBufferRenderStage.h"
#include "../WRenderer.h"
#include "../../Core/WCore.h"
#include "../../Images/WRenderTarget.h"
#include "../../Materials/WEffect.h"
#include "../../Materials/WMaterial.h"
#include "../../Lights/WLight.h"
#include "../../Geometries/WGeometry.h"
#include "../../Sprites/WSprite.h"
#include "../../Cameras/WCamera.h"
#include "../../WindowAndInput/WWindowAndInputComponent.h"

class OnlyPositionGeometry : public WGeometry {
	const W_VERTEX_DESCRIPTION m_desc = W_VERTEX_DESCRIPTION({ W_ATTRIBUTE_POSITION });

public:
	OnlyPositionGeometry(Wasabi* const app, unsigned int ID = 0) : WGeometry(app, ID) {}

	virtual unsigned int GetVertexBufferCount() const {
		return 1;
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(unsigned int layout_index = 0) const {
		return m_desc;
	}
	virtual size_t GetVertexDescriptionSize(unsigned int layout_index = 0) const {
		return m_desc.GetSize();
	}

	virtual WError CreateFromDefaultVerticesData(vector<WDefaultVertex>& default_vertices, vector<uint>& indices, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_CPU_READABLE) {
		vector<WVector3> custom_verts(default_vertices.size());
		for (unsigned int i = 0; i < default_vertices.size(); i++)
			custom_verts[i] = default_vertices[i].pos;
		return CreateFromData(custom_verts.data(), custom_verts.size(), indices.data(), indices.size(), flags);
	}
};

class SpotLightVS : public WShader {
public:
	SpotLightVS(class Wasabi* const app) : WShader(app) {}

	static vector<W_BOUND_RESOURCE> GetBoundResources() {
		return {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, 0, "uboPerLight", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "wvp"), // world * view * projection
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "lightDir"), // light direction (L vector)
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "lightSpec"), // light specular
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "lightColor"), // light color
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "intensity"), // light intensity
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "position"), // light color
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "range"), // light range
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "minCosAngle"), // used for spot light (angle of beam precomputed)
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "spotRadius"), // radius of the circle at the end of the cone
			}),
		};
	}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = GetBoundResources();
		m_desc.input_layouts = {W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
		})};
		vector<byte> code = {
			#include "Shaders/spotlight.vert.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
	}
};

class SpotLightPS : public WShader {
public:
	SpotLightPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			SpotLightVS::GetBoundResources()[0],
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 1, 1, "normalTexture"),
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, 1, "depthTexture"),
			W_BOUND_RESOURCE(W_TYPE_UBO, 3, 1, "uboPerFrame", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projInv"), // inverse of projection
			}),
		};
		vector<byte> code = {
			#include "Shaders/spotlight.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
	}
};

class PointLightVS : public WShader {
public:
	PointLightVS(class Wasabi* const app) : WShader(app) {}

	static vector<W_BOUND_RESOURCE> GetBoundResources() {
		return {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, 0, "uboPerLight", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "wvp"), // world * view * projection
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "lightDir"), // light direction (L vector)
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "lightSpec"), // light specular
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "lightColor"), // light color
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "intensity"), // light intensity
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "position"), // light position
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "range"), // light range
			}),
		};
	}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = GetBoundResources();
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
		}) };
		vector<byte> code = {
			#include "Shaders/pointlight.vert.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
	}
};

class PointLightPS : public WShader {
public:
	PointLightPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			PointLightVS::GetBoundResources()[0],
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 1, 1, "normalTexture"),
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, 1, "depthTexture"),
			W_BOUND_RESOURCE(W_TYPE_UBO, 3, 1, "uboPerFrame", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projInv"), // inverse of projection
			}),
		};
		vector<byte> code = {
			#include "Shaders/pointlight.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
	}
};

class DirectionalLightPS : public WShader {
public:
	DirectionalLightPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, 0, "uboPerLight", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "wvp"), // world * view * projection
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "lightDir"), // light direction (L vector)
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "lightSpec"), // light specular
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "lightColor"), // light color
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "intensity"), // light intensity
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "position"), // light position
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "range"), // light range
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "minCosAngle"), // used for spot light (angle of beam precomputed)
			}),
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 1, 1, "normalTexture"),
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, 1, "depthTexture"),
			W_BOUND_RESOURCE(W_TYPE_UBO, 3, 1, "uboPerFrame", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projInv"), // inverse of projection
			}),
		};
		vector<byte> code = {
			#include "Shaders/dirlight.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
	}
};

WLightBufferRenderStage::WLightBufferRenderStage(Wasabi* const app) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = RENDER_STAGE_TARGET_BUFFER;
	m_stageDescription.colorOutputs = std::vector<WRenderStage::OUTPUT_IMAGE>({
		WRenderStage::OUTPUT_IMAGE("LightBuffer", VK_FORMAT_R8G8B8A8_UNORM, WColor(0.0f, 0.0f, 0.0f, 0.0f)),
	});
}

WError WLightBufferRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	m_blendState = {};
	m_blendState.blendEnable = VK_TRUE;
	m_blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_blendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.colorBlendOp = VK_BLEND_OP_ADD;
	m_blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.alphaBlendOp = VK_BLEND_OP_ADD;

	m_rasterizationState = {};
	m_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	m_rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
	m_rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	m_rasterizationState.depthClampEnable = VK_FALSE;
	m_rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	m_rasterizationState.depthBiasEnable = VK_FALSE;
	m_rasterizationState.lineWidth = 1.0f;

	WError werr = LoadDirectionalLightsAssets();
	if (!werr)
		return werr;

	werr = LoadSpotLightsAssets();
	if (!werr)
		return werr;

	werr = LoadPointLightsAssets();
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
		WCamera* cam = rt->GetCamera();

		for (auto it = m_lightRenderingAssets.begin(); it != m_lightRenderingAssets.end(); it++) {
			LightTypeAssets lightTypeAssets = it->second;
			if (lightTypeAssets.material_map.size()) {
				lightTypeAssets.effect->Bind(rt);
				lightTypeAssets.perFrameMaterial->SetVariableMatrix("projInv", WMatrixInverse(cam->GetProjectionMatrix()));
				lightTypeAssets.perFrameMaterial->Bind(rt);

				for (auto materialIt = lightTypeAssets.material_map.begin(); materialIt != lightTypeAssets.material_map.end(); materialIt++) {
					WLight* light = materialIt->first;
					WMaterial* material = materialIt->second;
					int lightType = (int)light->GetType();

					if (light->Hidden())
						continue;
					if (lightType == W_LIGHT_SPOT) {
						if (!cam->CheckSphereInFrustum(light->GetPosition() + (light->GetLVector() * (light->GetRange() / 2.0f)), light->GetRange() / 2.0f))
							continue;
					} else if (lightType == W_LIGHT_POINT) {
						if (!cam->CheckSphereInFrustum(light->GetPosition(), light->GetRange()))
							continue;
					}

					WColor lightColor = light->GetColor();
					material->SetVariableMatrix("wvp", light->GetWorldMatrix() * cam->GetViewMatrix() * cam->GetProjectionMatrix());
					material->SetVariableVector3("lightDir", WVec3TransformNormal(light->GetLVector(), cam->GetViewMatrix()));
					material->SetVariableVector3("position", WVec3TransformCoord(light->GetPosition(), cam->GetViewMatrix()));
					material->SetVariableVector3("lightColor", WVector3(lightColor.r, lightColor.g, lightColor.b));
					material->SetVariableFloat("lightSpec", lightColor.a); // specular power is stored in alpha component
					material->SetVariableFloat("intensity", light->GetIntensity());
					material->SetVariableFloat("range", light->GetRange());
					material->SetVariableFloat("minCosAngle", light->GetMinCosAngle());
					float emittingHalfAngle = acosf(light->GetMinCosAngle());
					float spotRadius = tanf(emittingHalfAngle) * light->GetRange();
					material->SetVariableFloat("spotRadius", spotRadius);
					material->Bind(rt);

					if (lightTypeAssets.fullscreen_sprite)
						lightTypeAssets.fullscreen_sprite->Render(rt);
					if (lightTypeAssets.geometry)
						lightTypeAssets.geometry->Draw(rt);
				}
			}
		}
	}

	return WError(W_SUCCEEDED);
}

void WLightBufferRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	m_app->LightManager->RemoveChangeCallback(m_stageDescription.name);
	for (auto iter = m_lightRenderingAssets.begin(); iter != m_lightRenderingAssets.end(); iter++)
		iter->second.Destroy();
	m_lightRenderingAssets.clear();
}

WError WLightBufferRenderStage::Resize(uint width, uint height) {
	for (auto iter = m_lightRenderingAssets.begin(); iter != m_lightRenderingAssets.end(); iter++)
		if (iter->second.fullscreen_sprite)
			iter->second.fullscreen_sprite->SetSize(WVector2(width, height));
	return WRenderStage::Resize(width, height);
}

void WLightBufferRenderStage::OnLightsChange(WLight* light, bool is_added) {
	auto iter = m_lightRenderingAssets.find(light->GetType());
	if (iter == m_lightRenderingAssets.end())
		return;
	LightTypeAssets assets = iter->second;

	if (is_added) {
		WMaterial* material = assets.effect->CreateMaterial(0);
		iter->second.material_map.insert(std::pair<class WLight*, class WMaterial*>(light, material));
	} else {
		auto it = assets.material_map.find(light);
		if (it != assets.material_map.end()) {
			W_SAFE_REMOVEREF(it->second);
			iter->second.material_map.erase(light);
		}
	}
}

WError WLightBufferRenderStage::LoadPointLightsAssets() {
	LightTypeAssets assets;

	WShader* vertex_shader = new PointLightVS(m_app);
	vertex_shader->Load();
	WShader* pixel_shader = new PointLightPS(m_app);
	pixel_shader->Load();

	assets.effect = new WEffect(m_app);
	VkPipelineDepthStencilStateCreateInfo depthState = {};
	assets.effect->SetDepthStencilState(depthState);
	assets.effect->SetBlendingState(m_blendState);
	assets.effect->SetRasterizationState(m_rasterizationState);

	WError werr = assets.effect->BindShader(vertex_shader);
	if (werr) {
		werr = assets.effect->BindShader(pixel_shader);
		if (werr) {
			werr = assets.effect->BuildPipeline(m_renderTarget);

			assets.perFrameMaterial = assets.effect->CreateMaterial(1);
			assets.perFrameMaterial->SetTexture(1, m_app->Renderer->GetRenderTargetImage("GBufferViewSpaceNormal"));
			assets.perFrameMaterial->SetTexture(2, m_app->Renderer->GetRenderTargetImage("GBufferDepth"));

			assets.geometry = new OnlyPositionGeometry(m_app);
			assets.geometry->CreateSphere(1.0f, 10, 10);

			m_lightRenderingAssets.insert(std::pair<int, LightTypeAssets>((int)W_LIGHT_POINT, assets));
		}
	}

	W_SAFE_REMOVEREF(pixel_shader);
	W_SAFE_REMOVEREF(vertex_shader);

	return werr;
}

WError WLightBufferRenderStage::LoadSpotLightsAssets() {
	LightTypeAssets assets;

	WShader* vertex_shader = new SpotLightVS(m_app);
	vertex_shader->Load();
	WShader* pixel_shader = new SpotLightPS(m_app);
	pixel_shader->Load();

	assets.effect = new WEffect(m_app);
	VkPipelineDepthStencilStateCreateInfo depthState = {};
	assets.effect->SetDepthStencilState(depthState);
	assets.effect->SetBlendingState(m_blendState);
	assets.effect->SetRasterizationState(m_rasterizationState);

	WError werr = assets.effect->BindShader(vertex_shader);
	if (werr) {
		werr = assets.effect->BindShader(pixel_shader);
		if (werr) {
			werr = assets.effect->BuildPipeline(m_renderTarget);

			assets.perFrameMaterial = assets.effect->CreateMaterial(1);
			assets.perFrameMaterial->SetTexture(1, m_app->Renderer->GetRenderTargetImage("GBufferViewSpaceNormal"));
			assets.perFrameMaterial->SetTexture(2, m_app->Renderer->GetRenderTargetImage("GBufferDepth"));

			assets.geometry = new OnlyPositionGeometry(m_app);
			assets.geometry->CreateCone(1.0f, 1.0f, 0, 16);
			assets.geometry->ApplyTransformation(WTranslationMatrix(0, -0.5, 0) * WRotationMatrixX(W_DEGTORAD(-90)));

			m_lightRenderingAssets.insert(std::pair<int, LightTypeAssets>((int)W_LIGHT_SPOT, assets));
		}
	}

	W_SAFE_REMOVEREF(pixel_shader);
	W_SAFE_REMOVEREF(vertex_shader);

	return werr;
}

WError WLightBufferRenderStage::LoadDirectionalLightsAssets() {
	LightTypeAssets assets;

	WShader* pixel_shader = new DirectionalLightPS(m_app);
	pixel_shader->Load();

	assets.effect = m_app->SpriteManager->CreateSpriteEffect(m_renderTarget);
	W_SAFE_REMOVEREF(pixel_shader);
	if (!assets.effect)
		return WError(W_OUTOFMEMORY);

	assets.perFrameMaterial = assets.effect->CreateMaterial(1);
	assets.perFrameMaterial->SetTexture(1, m_app->Renderer->GetRenderTargetImage("GBufferViewSpaceNormal"));
	assets.perFrameMaterial->SetTexture(2, m_app->Renderer->GetRenderTargetImage("GBufferDepth"));

	uint windowWidth = m_app->WindowAndInputComponent->GetWindowWidth();
	uint windowHeight = m_app->WindowAndInputComponent->GetWindowHeight();
	assets.fullscreen_sprite = m_app->SpriteManager->CreateSprite();
	assets.fullscreen_sprite->SetSize(WVector2(windowWidth, windowHeight));
	assets.fullscreen_sprite->Hide();

	m_lightRenderingAssets.insert(std::pair<int, LightTypeAssets>((int)W_LIGHT_DIRECTIONAL, assets));

	return WError(W_SUCCEEDED);
}

void WLightBufferRenderStage::LightTypeAssets::Destroy() {
	for (auto it = material_map.begin(); it != material_map.end(); it++)
		W_SAFE_REMOVEREF(it->second);
	W_SAFE_REMOVEREF(geometry);
	W_SAFE_REMOVEREF(effect);
	W_SAFE_REMOVEREF(fullscreen_sprite);
	material_map.clear();
}
