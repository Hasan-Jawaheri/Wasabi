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
}

WError WForwardRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	WForwardRenderStageObjectVS* vs = new WForwardRenderStageObjectVS(m_app);
	vs->SetName("DefaultForwardVS");
	m_app->FileManager->AddDefaultAsset(vs->GetName(), vs);
	vs->Load();

	WForwardRenderStageAnimatedObjectVS* vsa = new WForwardRenderStageAnimatedObjectVS(m_app);
	vsa->SetName("DefaultForwardAnimatedVS");
	m_app->FileManager->AddDefaultAsset(vsa->GetName(), vsa);
	vsa->Load();

	WForwardRenderStageObjectPS* ps = new WForwardRenderStageObjectPS(m_app);
	ps->SetName("DefaultForwardPS");
	m_app->FileManager->AddDefaultAsset(ps->GetName(), ps);
	ps->Load();

	WEffect* fx = new WEffect(m_app);
	fx->SetName("DefaultForwardEffect");
	m_app->FileManager->AddDefaultAsset(fx->GetName(), fx);

	WEffect* fxa = new WEffect(m_app);
	fxa->SetName("DefaultForwardAnimatedEffect");
	m_app->FileManager->AddDefaultAsset(fxa->GetName(), fxa);

	err = fx->BindShader(vs);
	if (err) {
		err = fx->BindShader(ps);
		if (err) {
			err = fx->BuildPipeline(m_renderTarget);
			if (err) {
				err = fxa->BindShader(vsa);
				if (err) {
					err = fxa->BindShader(ps);
					if (err) {
						err = fxa->BuildPipeline(m_renderTarget);
					}
				}
			}
		}
	}
	W_SAFE_REMOVEREF(vs);
	W_SAFE_REMOVEREF(vsa);
	W_SAFE_REMOVEREF(ps);
	if (!err) {
		W_SAFE_REMOVEREF(fx);
		W_SAFE_REMOVEREF(fxa);
		return err;
	}

	WForwardRenderStageTerrainVS* terrainVS = new WForwardRenderStageTerrainVS(m_app);
	terrainVS->SetName("DefaultForwardTerrainVS");
	m_app->FileManager->AddDefaultAsset(terrainVS->GetName(), terrainVS);
	terrainVS->Load();

	WForwardRenderStageTerrainPS* terrainPS = new WForwardRenderStageTerrainPS(m_app);
	terrainPS->SetName("DefaultForwardTerrainPS");
	m_app->FileManager->AddDefaultAsset(terrainPS->GetName(), terrainPS);
	terrainPS->Load();

	WEffect* terrainFX = new WEffect(m_app);
	terrainFX->SetName("DefaultForwardTerrainEffect");
	m_app->FileManager->AddDefaultAsset(terrainFX->GetName(), terrainFX);
	err = terrainFX->BindShader(terrainVS);
	if (err) {
		err = terrainFX->BindShader(terrainPS);
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
			err = terrainFX->BuildPipeline(m_renderTarget);
		}
	}
	W_SAFE_REMOVEREF(terrainVS);
	W_SAFE_REMOVEREF(terrainPS);
	if (!err) {
		W_SAFE_REMOVEREF(terrainFX);
		return err;
	}

	m_objectsFragment = new WObjectsRenderFragment(m_stageDescription.name, false, fx, m_app, EFFECT_RENDER_FLAG_RENDER_FORWARD, m_addDefaultEffects);
	m_animatedObjectsFragment = new WObjectsRenderFragment(m_stageDescription.name + "-animated", true, fxa, m_app, EFFECT_RENDER_FLAG_RENDER_FORWARD, m_addDefaultEffects);

	m_terrainsFragment = new WTerrainRenderFragment(m_stageDescription.name, terrainFX, m_app, EFFECT_RENDER_FLAG_RENDER_FORWARD);

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
