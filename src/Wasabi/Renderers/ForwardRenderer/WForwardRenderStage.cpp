#include "Renderers/ForwardRenderer/WForwardRenderStage.h"
#include "Core/WCore.h"
#include "Renderers/WRenderer.h"
#include "Lights/WLight.h"
#include "Images/WRenderTarget.h"
#include "Objects/WObject.h"
#include "Materials/WMaterial.h"
#include "Cameras/WCamera.h"

WForwardRenderStageObjectVS::WForwardRenderStageObjectVS(Wasabi* const app) : WShader(app) {}

void WForwardRenderStageObjectVS::Load(bool bSaveData) {
	int maxLights = m_app->engineParams.find("maxLights") != m_app->engineParams.end() ? (int)m_app->engineParams["maxLights"] : 0;
	m_desc = GetDesc(maxLights);
	vector<byte> code = {
		#include "Shaders/forward.vert.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
}

W_SHADER_DESC WForwardRenderStageObjectVS::GetDesc(int maxLights) {
	W_SHADER_DESC desc;
	desc.type = W_VERTEX_SHADER;
	desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_UBO, 0, 0, "uboPerObject", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "worldMatrix"), // world
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "animationTextureWidth"), // width of the animation texture
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "instanceTextureWidth"), // width of the instance texture
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isAnimated"), // whether or not animation is enabled
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isInstanced"), // whether or not instancing is enabled
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4, "color"), // object color
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isTextured"), // whether or not to use diffuse texture
		}),
		W_BOUND_RESOURCE(W_TYPE_UBO, 1, 1, "uboPerFrame", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "viewMatrix"), // view
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projectionMatrix"), // projection
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "camPosW"),
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "numLights"),
			W_SHADER_VARIABLE_INFO(W_TYPE_STRUCT, maxLights, sizeof(LightStruct), 16, "lights"),
		}),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, 0, "animationTexture"),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 3, 0, "instancingTexture"),
	};
	desc.input_layouts = { W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 1), // texture index
	}), W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 4), // bone indices
		W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4), // bone weights
	}) };
	return desc;
}

WForwardRenderStageObjectPS::WForwardRenderStageObjectPS(Wasabi* const app) : WShader(app) {}

void WForwardRenderStageObjectPS::Load(bool bSaveData) {
	int maxLights = (int)m_app->engineParams["maxLights"];
	m_desc = GetDesc(maxLights);
	vector<byte> code = {
		#include "Shaders/forward.frag.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
}

W_SHADER_DESC WForwardRenderStageObjectPS::GetDesc(int maxLights) {
	W_SHADER_DESC desc;
	desc.type = W_FRAGMENT_SHADER;
	desc.bound_resources = {
		WForwardRenderStageObjectVS::GetDesc(maxLights).bound_resources[0],
		WForwardRenderStageObjectVS::GetDesc(maxLights).bound_resources[1],
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 4, 0, "diffuseTexture", {}, 8),
	};
	return desc;
}

WForwardRenderStageTerrainVS::WForwardRenderStageTerrainVS(Wasabi* const app) : WShader(app) {}

void WForwardRenderStageTerrainVS::Load(bool bSaveData) {
	int maxLights = m_app->engineParams.find("maxLights") != m_app->engineParams.end() ? (int)m_app->engineParams["maxLights"] : 0;
	m_desc = GetDesc(maxLights);
	vector<byte> code = {
		#include "Shaders/terrain.vert.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
}

W_SHADER_DESC WForwardRenderStageTerrainVS::GetDesc(int maxLights) {
	W_SHADER_DESC desc;
	desc.type = W_VERTEX_SHADER;
	desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_UBO, 0, 0, "uboPerTerrain", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "worldMatrix"), // world
		}),
		W_BOUND_RESOURCE(W_TYPE_UBO, 1, 1, "uboPerFrame", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "viewMatrix"), // view
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projectionMatrix"), // projection
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "camPosW"),
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
	int maxLights = (int)m_app->engineParams["maxLights"];
	m_desc = GetDesc(maxLights);
	vector<byte> code = {
		#include "Shaders/terrain.frag.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
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

	if (m_app->engineParams.find("maxLights") == m_app->engineParams.end())
		m_app->engineParams.insert(std::pair<std::string, void*>("maxLights", (void*)16));
	m_lights.resize((int)m_app->engineParams["maxLights"]);

	m_objectsFragment = nullptr;
	m_terrainsFragment = nullptr;
	m_perFrameObjectsMaterial = nullptr;
	m_perFrameTerrainsMaterial = nullptr;
}

WError WForwardRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	WForwardRenderStageObjectVS* vs = new WForwardRenderStageObjectVS(m_app);
	vs->SetName("DefaultForwardVS");
	m_app->FileManager->AddDefaultAsset(vs->GetName(), vs);
	vs->Load();

	WForwardRenderStageObjectPS* ps = new WForwardRenderStageObjectPS(m_app);
	ps->SetName("DefaultForwardPS");
	m_app->FileManager->AddDefaultAsset(ps->GetName(), ps);
	ps->Load();

	WEffect* fx = new WEffect(m_app);
	fx->SetName("DefaultForwardEffect");
	m_app->FileManager->AddDefaultAsset(fx->GetName(), fx);
	err = fx->BindShader(vs);
	if (err) {
		err = fx->BindShader(ps);
		if (err) {
			err = fx->BuildPipeline(m_renderTarget);
		}
	}
	W_SAFE_REMOVEREF(vs);
	W_SAFE_REMOVEREF(ps);
	if (!err)
		return err;

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
	if (!err)
		return err;

	m_objectsFragment = new WObjectsRenderFragment(m_stageDescription.name, fx, m_app, true);

	m_terrainsFragment = new WTerrainRenderFragment(m_stageDescription.name, terrainFX, m_app);

	m_perFrameObjectsMaterial = m_objectsFragment->GetEffect()->CreateMaterial(1);
	if (!m_perFrameObjectsMaterial) {
		err = WError(W_ERRORUNK);
	} else {
		m_perFrameObjectsMaterial->SetName("PerFrameForwardMaterial");
		m_app->FileManager->AddDefaultAsset(m_perFrameObjectsMaterial->GetName(), m_perFrameObjectsMaterial);
	}

	m_perFrameTerrainsMaterial = m_terrainsFragment->GetEffect()->CreateMaterial(1);
	if (!m_perFrameTerrainsMaterial) {
		err = WError(W_ERRORUNK);
	} else {
		m_perFrameTerrainsMaterial->SetName("PerFrameForwardTerrainMaterial");
		m_app->FileManager->AddDefaultAsset(m_perFrameTerrainsMaterial->GetName(), m_perFrameTerrainsMaterial);
	}

	return err;
}

void WForwardRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_perFrameObjectsMaterial);
	W_SAFE_REMOVEREF(m_perFrameTerrainsMaterial);
	W_SAFE_DELETE(m_objectsFragment);
	W_SAFE_DELETE(m_terrainsFragment);
}

WError WForwardRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	WCamera* cam = rt->GetCamera();

	int numLights = 0;
	for (int i = 0; numLights < m_lights.size(); i++) {
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
		m_perFrameTerrainsMaterial->SetVariableMatrix("viewMatrix", cam->GetViewMatrix());
		m_perFrameTerrainsMaterial->SetVariableMatrix("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameTerrainsMaterial->SetVariableVector3("camPosW", cam->GetPosition());
		m_perFrameTerrainsMaterial->SetVariableInt("numLights", numLights);
		m_perFrameTerrainsMaterial->SetVariableData("lights", m_lights.data(), sizeof(LightStruct) * numLights);
		m_perFrameTerrainsMaterial->Bind(rt);

		m_terrainsFragment->Render(renderer, rt);
	}

	if (filter & RENDER_FILTER_OBJECTS) {
		// create the per-frame UBO data
		m_perFrameObjectsMaterial->SetVariableMatrix("viewMatrix", cam->GetViewMatrix());
		m_perFrameObjectsMaterial->SetVariableMatrix("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameObjectsMaterial->SetVariableVector3("camPosW", cam->GetPosition());
		m_perFrameObjectsMaterial->SetVariableInt("numLights", numLights);
		m_perFrameObjectsMaterial->SetVariableData("lights", m_lights.data(), sizeof(LightStruct) * numLights);
		m_perFrameObjectsMaterial->Bind(rt);

		m_objectsFragment->Render(renderer, rt);
	}

	return WError(W_SUCCEEDED);
}

WError WForwardRenderStage::Resize(uint width, uint height) {
	return WRenderStage::Resize(width, height);
}
