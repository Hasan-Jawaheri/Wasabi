#include "Wasabi/Renderers/ForwardRenderer/WForwardRenderStage.hpp"
#include "Wasabi/Core/WCore.hpp"
#include "Wasabi/Renderers/WRenderer.hpp"
#include "Wasabi/Lights/WLight.hpp"
#include "Wasabi/Images/WRenderTarget.hpp"
#include "Wasabi/Objects/WObject.hpp"
#include "Wasabi/Materials/WMaterial.hpp"
#include "Wasabi/Cameras/WCamera.hpp"

WForwardRenderStageObjectVS::WForwardRenderStageObjectVS(Wasabi* const app) : WShader(app) {}

void WForwardRenderStageObjectVS::Load(bool bSaveData) {
	int maxLights = m_app->GetEngineParam<int>("maxLights", 0);
	m_desc = GetDesc(maxLights);
	vector<uint8_t> code {
		#include "Shaders/forward.vert.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
}

W_SHADER_DESC WForwardRenderStageObjectVS::GetDesc(int maxLights) {
	W_SHADER_DESC desc;
	desc.type = W_VERTEX_SHADER;
	desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_UBO, 0, 0, "uboPerObject", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "worldMatrix"), // world
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4, "color"), // object color
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "specularPower"), // specular power (dot raised to this power)
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "specularIntensity"), // specular intensity (specular term is multiplied by this)
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isInstanced"), // whether or not instancing is enabled
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isTextured"), // whether or not to use diffuse texture
		}),
		W_BOUND_RESOURCE(W_TYPE_UBO, 1, 1, "uboPerFrame", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "viewMatrix"), // view
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projectionMatrix"), // projection
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "camDirW"),
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "numLights"),
			W_SHADER_VARIABLE_INFO(W_TYPE_STRUCT, maxLights, sizeof(LightStruct), 16, "lights"),
		}),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 3, 0, "instancingTexture"),
	};
	desc.input_layouts = { W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 1), // texture index
	}) };
	return desc;
}

WForwardRenderStageAnimatedObjectVS::WForwardRenderStageAnimatedObjectVS(Wasabi* const app) : WShader(app) {}

void WForwardRenderStageAnimatedObjectVS::Load(bool bSaveData) {
	int maxLights = m_app->GetEngineParam<int>("maxLights", 0);
	m_desc = GetDesc(maxLights);
	vector<uint8_t> code{
		#include "Shaders/forward-animated.vert.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
}

W_SHADER_DESC WForwardRenderStageAnimatedObjectVS::GetDesc(int maxLights) {
	W_SHADER_DESC desc;
	desc.type = W_VERTEX_SHADER;
	desc.bound_resources = {
		WForwardRenderStageObjectVS::GetDesc(maxLights).bound_resources[0],
		WForwardRenderStageObjectVS::GetDesc(maxLights).bound_resources[1],
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, 0, "animationTexture"),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 3, 0, "instancingTexture"),
	};
	desc.input_layouts = {
		WForwardRenderStageObjectVS::GetDesc(maxLights).input_layouts[0], W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 4), // bone indices
		W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4), // bone weights
	}) };
	return desc;
}

WForwardRenderStageObjectPS::WForwardRenderStageObjectPS(Wasabi* const app) : WShader(app) {}

void WForwardRenderStageObjectPS::Load(bool bSaveData) {
	int maxLights = m_app->GetEngineParam<int>("maxLights", 0);
	m_desc = GetDesc(maxLights);
	vector<uint8_t> code {
		#include "Shaders/forward.frag.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
}

W_SHADER_DESC WForwardRenderStageObjectPS::GetDesc(int maxLights) {
	W_SHADER_DESC desc;
	desc.type = W_FRAGMENT_SHADER;
	desc.bound_resources = {
		WForwardRenderStageObjectVS::GetDesc(maxLights).bound_resources[0],
		WForwardRenderStageObjectVS::GetDesc(maxLights).bound_resources[1],
		W_BOUND_RESOURCE(W_TYPE_UBO, 5, 1, "uboParams", {
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4, "ambient"), // Ambient lighting
		}),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 4, 0, "diffuseTexture", {}, 8),
	};
	return desc;
}

WForwardRenderStageTerrainVS::WForwardRenderStageTerrainVS(Wasabi* const app) : WShader(app) {}

void WForwardRenderStageTerrainVS::Load(bool bSaveData) {
	int maxLights = m_app->GetEngineParam<int>("maxLights", 0);
	m_desc = GetDesc(maxLights);
	vector<uint8_t> code {
		#include "Shaders/terrain.vert.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
}

W_SHADER_DESC WForwardRenderStageTerrainVS::GetDesc(int maxLights) {
	W_SHADER_DESC desc;
	desc.type = W_VERTEX_SHADER;
	desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_UBO, 0, 0, "uboPerTerrain", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "worldMatrix"), // world
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "specularPower"), // specular power (dot raised to this power)
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "specularIntensity"), // specular intensity (specular term is multiplied by this)
		}),
		W_BOUND_RESOURCE(W_TYPE_UBO, 1, 1, "uboPerFrame", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "viewMatrix"), // view
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projectionMatrix"), // projection
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "camDirW"),
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "numLights"),
			W_SHADER_VARIABLE_INFO(W_TYPE_STRUCT, maxLights, sizeof(LightStruct), 16, "lights"),
		}),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, 0, "instancingTexture"),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 3, 0, "heightTexture"),
		W_BOUND_RESOURCE(W_TYPE_PUSH_CONSTANT, 0, "pcPerGeometry", {
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "geometryOffsetInTexture"),
		}),
	};
	desc.input_layouts = { W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 1), // texture index
	}), };
	return desc;
}

WForwardRenderStageTerrainPS::WForwardRenderStageTerrainPS(Wasabi* const app) : WShader(app) {}

void WForwardRenderStageTerrainPS::Load(bool bSaveData) {
	int maxLights = m_app->GetEngineParam<int>("maxLights", 0);
	m_desc = GetDesc(maxLights);
	vector<uint8_t> code {
		#include "Shaders/terrain.frag.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
}

W_SHADER_DESC WForwardRenderStageTerrainPS::GetDesc(int maxLights) {
	W_SHADER_DESC desc;
	desc.type = W_FRAGMENT_SHADER;
	desc.bound_resources = {
		WForwardRenderStageTerrainVS::GetDesc(maxLights).bound_resources[0],
		WForwardRenderStageTerrainVS::GetDesc(maxLights).bound_resources[1],
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 4, 0, "diffuseTexture"),
	};
	return desc;
}

WForwardRenderStage::WForwardRenderStage(Wasabi* const app) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = RENDER_STAGE_TARGET_BACK_BUFFER;
	m_stageDescription.flags = RENDER_STAGE_FLAG_PICKING_RENDER_STAGE;

	if (m_app->GetEngineParam<int>("maxLights", -1) == -1)
		m_app->SetEngineParam<int>("maxLights", 16);
	m_lights.resize(m_app->GetEngineParam<int>("maxLights"));

	m_objectsFragment = nullptr;
	m_animatedObjectsFragment = nullptr;
	m_terrainsFragment = nullptr;
	m_perFrameObjectsMaterial = nullptr;
	m_perFrameAnimatedObjectsMaterial = nullptr;
	m_perFrameTerrainsMaterial = nullptr;
	m_addDefaultEffects = true;

	m_defaultObjectVS = nullptr;
	m_defaultAnimatedObjectVS = nullptr;
	m_defaultObjectPS = nullptr;
	m_defaultObjectFX = nullptr;
	m_defaultAnimatedObjectFX = nullptr;
	m_defaultTerrainVS = nullptr;
	m_defaultTerrainPS = nullptr;
	m_defaultTerrainFX = nullptr;
}

WError WForwardRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	m_defaultObjectVS = new WForwardRenderStageObjectVS(m_app);
	m_defaultObjectVS->SetName("DefaultForwardVS");
	m_app->FileManager->AddDefaultAsset(m_defaultObjectVS->GetName(), m_defaultObjectVS);
	m_defaultObjectVS->Load();

	m_defaultAnimatedObjectVS = new WForwardRenderStageAnimatedObjectVS(m_app);
	m_defaultAnimatedObjectVS->SetName("DefaultForwardAnimatedVS");
	m_app->FileManager->AddDefaultAsset(m_defaultAnimatedObjectVS->GetName(), m_defaultAnimatedObjectVS);
	m_defaultAnimatedObjectVS->Load();

	m_defaultObjectPS = new WForwardRenderStageObjectPS(m_app);
	m_defaultObjectPS->SetName("DefaultForwardPS");
	m_app->FileManager->AddDefaultAsset(m_defaultObjectPS->GetName(), m_defaultObjectPS);
	m_defaultObjectPS->Load();

	m_defaultObjectFX = new WEffect(m_app);
	m_defaultObjectFX->SetName("DefaultForwardEffect");
	m_app->FileManager->AddDefaultAsset(m_defaultObjectFX->GetName(), m_defaultObjectFX);

	m_defaultAnimatedObjectFX = new WEffect(m_app);
	m_defaultAnimatedObjectFX->SetName("DefaultForwardAnimatedEffect");
	m_app->FileManager->AddDefaultAsset(m_defaultAnimatedObjectFX->GetName(), m_defaultAnimatedObjectFX);

	err = m_defaultObjectFX->BindShader(m_defaultObjectVS);
	if (err) {
		err = m_defaultObjectFX->BindShader(m_defaultObjectPS);
		if (err) {
			err = m_defaultObjectFX->BuildPipeline(m_renderTarget);
			if (err) {
				err = m_defaultAnimatedObjectFX->BindShader(m_defaultAnimatedObjectVS);
				if (err) {
					err = m_defaultAnimatedObjectFX->BindShader(m_defaultObjectPS);
					if (err) {
						err = m_defaultAnimatedObjectFX->BuildPipeline(m_renderTarget);
					}
				}
			}
		}
	}
	if (!err)
		return err;

	m_defaultTerrainVS = new WForwardRenderStageTerrainVS(m_app);
	m_defaultTerrainVS->SetName("DefaultForwardTerrainVS");
	m_app->FileManager->AddDefaultAsset(m_defaultTerrainVS->GetName(), m_defaultTerrainVS);
	m_defaultTerrainVS->Load();

	m_defaultTerrainPS = new WForwardRenderStageTerrainPS(m_app);
	m_defaultTerrainPS->SetName("DefaultForwardTerrainPS");
	m_app->FileManager->AddDefaultAsset(m_defaultTerrainPS->GetName(), m_defaultTerrainPS);
	m_defaultTerrainPS->Load();

	m_defaultTerrainFX = new WEffect(m_app);
	m_defaultTerrainFX->SetName("DefaultForwardTerrainEffect");
	m_app->FileManager->AddDefaultAsset(m_defaultTerrainFX->GetName(), m_defaultTerrainFX);
	err = m_defaultTerrainFX->BindShader(m_defaultTerrainVS);
	if (err) {
		err = m_defaultTerrainFX->BindShader(m_defaultTerrainPS);
		if (err) {
			/*VkPipelineRasterizationStateCreateInfo rs = {};
			rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rs.polygonMode = VK_POLYGON_MODE_LINE;
			rs.cullMode = VK_CULL_MODE_BACK_BIT;
			rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rs.depthClampEnable = VK_FALSE;
			rs.rasterizerDiscardEnable = VK_FALSE;
			rs.depthBiasEnable = VK_FALSE;
			rs.lineWidth = 1.0f;
			terrainFX->SetRasterizationState(rs);*/
			err = m_defaultTerrainFX->BuildPipeline(m_renderTarget);
		}
	}
	if (!err)
		return err;

	m_objectsFragment = new WObjectsRenderFragment(m_stageDescription.name, false, m_defaultObjectFX, m_app, EFFECT_RENDER_FLAG_RENDER_FORWARD, m_addDefaultEffects);
	m_animatedObjectsFragment = new WObjectsRenderFragment(m_stageDescription.name + "-animated", true, m_defaultAnimatedObjectFX, m_app, EFFECT_RENDER_FLAG_RENDER_FORWARD, m_addDefaultEffects);

	m_terrainsFragment = new WTerrainRenderFragment(m_stageDescription.name, m_defaultTerrainFX, m_app, EFFECT_RENDER_FLAG_RENDER_FORWARD);

	m_perFrameObjectsMaterial = m_objectsFragment->GetEffect()->CreateMaterial(1, true);
	if (!m_perFrameObjectsMaterial) {
		err = WError(W_ERRORUNK);
	} else {
		m_perFrameObjectsMaterial->SetName("PerFrameForwardMaterial");
		m_app->FileManager->AddDefaultAsset(m_perFrameObjectsMaterial->GetName(), m_perFrameObjectsMaterial);
	}

	m_perFrameAnimatedObjectsMaterial = m_animatedObjectsFragment->GetEffect()->CreateMaterial(1, true);
	if (!m_perFrameAnimatedObjectsMaterial) {
		err = WError(W_ERRORUNK);
	} else {
		m_perFrameAnimatedObjectsMaterial->SetName("PerFrameForwardAnimatedMaterial");
		m_app->FileManager->AddDefaultAsset(m_perFrameAnimatedObjectsMaterial->GetName(), m_perFrameAnimatedObjectsMaterial);
	}

	m_perFrameTerrainsMaterial = m_terrainsFragment->GetEffect()->CreateMaterial(1, true);
	if (!m_perFrameTerrainsMaterial) {
		err = WError(W_ERRORUNK);
	} else {
		m_perFrameTerrainsMaterial->SetName("PerFrameForwardTerrainMaterial");
		m_app->FileManager->AddDefaultAsset(m_perFrameTerrainsMaterial->GetName(), m_perFrameTerrainsMaterial);
	}

	SetAmbientLight(WColor(0.3f, 0.3f, 0.3f));

	return err;
}

void WForwardRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_perFrameObjectsMaterial);
	W_SAFE_REMOVEREF(m_perFrameAnimatedObjectsMaterial);
	W_SAFE_REMOVEREF(m_perFrameTerrainsMaterial);
	W_SAFE_DELETE(m_objectsFragment);
	W_SAFE_DELETE(m_animatedObjectsFragment);
	W_SAFE_DELETE(m_terrainsFragment);

	W_SAFE_REMOVEREF(m_defaultObjectVS);
	W_SAFE_REMOVEREF(m_defaultAnimatedObjectVS);
	W_SAFE_REMOVEREF(m_defaultObjectPS);
	W_SAFE_REMOVEREF(m_defaultTerrainVS);
	W_SAFE_REMOVEREF(m_defaultTerrainPS);
}

WError WForwardRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint32_t filter) {
	WCamera* cam = rt->GetCamera();

	int numLights = 0;
	for (uint32_t i = 0; (size_t)numLights < m_lights.size(); i++) {
		WLight* light = m_app->LightManager->GetEntityByIndex(i);
		if (!light)
			break;
		if (!light->Hidden() && light->InCameraView(cam)) {
			WColor c = light->GetColor();
			WVector3 l = light->GetLVector();
			WVector3 p = light->GetPosition();
			m_lights[numLights].color = WVector4(c.r, c.g, c.b, light->GetIntensity());
			m_lights[numLights].dir = WVector4(l.x, l.y, l.z, light->GetRange());
			m_lights[numLights].pos = WVector4(p.x, p.y, p.z, light->GetMinCosAngle());
			m_lights[numLights].type = (int)light->GetType();
			numLights++;
		}
	}

	if (filter & RENDER_FILTER_TERRAIN) {
		// create the per-frame UBO data
		m_perFrameTerrainsMaterial->SetVariable<WMatrix>("viewMatrix", cam->GetViewMatrix());
		m_perFrameTerrainsMaterial->SetVariable<WMatrix>("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameTerrainsMaterial->SetVariable<WVector3>("camDirW", cam->GetLVector());
		m_perFrameTerrainsMaterial->SetVariable<int>("numLights", numLights);
		m_perFrameTerrainsMaterial->SetVariableData("lights", m_lights.data(), sizeof(LightStruct) * numLights);

		m_terrainsFragment->Render(renderer, rt);
	}

	if (filter & RENDER_FILTER_OBJECTS) {
		// create the per-frame UBO data
		m_perFrameObjectsMaterial->SetVariable<WMatrix>("viewMatrix", cam->GetViewMatrix());
		m_perFrameObjectsMaterial->SetVariable<WMatrix>("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameObjectsMaterial->SetVariable<WVector3>("camDirW", cam->GetLVector());
		m_perFrameObjectsMaterial->SetVariable<int>("numLights", numLights);
		m_perFrameObjectsMaterial->SetVariableData("lights", m_lights.data(), sizeof(LightStruct) * numLights);

		m_perFrameAnimatedObjectsMaterial->SetVariable<WMatrix>("viewMatrix", cam->GetViewMatrix());
		m_perFrameAnimatedObjectsMaterial->SetVariable<WMatrix>("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameAnimatedObjectsMaterial->SetVariable<WVector3>("camDirW", cam->GetLVector());
		m_perFrameAnimatedObjectsMaterial->SetVariable<int>("numLights", numLights);
		m_perFrameAnimatedObjectsMaterial->SetVariableData("lights", m_lights.data(), sizeof(LightStruct) * numLights);

		m_objectsFragment->Render(renderer, rt);

		m_animatedObjectsFragment->Render(renderer, rt);
	}

	return WError(W_SUCCEEDED);
}

WError WForwardRenderStage::Resize(uint32_t width, uint32_t height) {
	return WRenderStage::Resize(width, height);
}

void WForwardRenderStage::SetAmbientLight(WColor color) {
	m_perFrameObjectsMaterial->SetVariable<WColor>("ambient", color);
	m_perFrameAnimatedObjectsMaterial->SetVariable<WColor>("ambient", color);
}

WForwardRenderStageObjectVS* WForwardRenderStage::GetDefaultObjectVertexShader() const {
	return m_defaultObjectVS;
}

WForwardRenderStageAnimatedObjectVS* WForwardRenderStage::GetDefaultAnimatedObjectVertexShader() const {
	return m_defaultAnimatedObjectVS;
}

WForwardRenderStageObjectPS* WForwardRenderStage::GetDefaultObjectPixelShader() const {
	return m_defaultObjectPS;
}

WEffect* WForwardRenderStage::GetDefaultObjectEffect() const {
	return m_defaultObjectFX;
}

WEffect* WForwardRenderStage::GetDefaultAnimatedObjectEffect() const {
	return m_defaultAnimatedObjectFX;
}

WForwardRenderStageTerrainVS* WForwardRenderStage::GetDefaultTerrainVertexShader() const {
	return m_defaultTerrainVS;
}

WForwardRenderStageTerrainPS* WForwardRenderStage::GetDefaultTerrainPixelShader() const {
	return m_defaultTerrainPS;
}

WEffect* WForwardRenderStage::GetDefaultTerrainEffect() const {
	return m_defaultTerrainFX;
}
