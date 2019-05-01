#include "WForwardLightRenderStage.h"
#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Lights/WLight.h"
#include "../../Images/WRenderTarget.h"
#include "../../Objects/WObject.h"
#include "../../Materials/WMaterial.h"
#include "../../Cameras/WCamera.h"

WForwardLightRenderStageObjectVS::WForwardLightRenderStageObjectVS(Wasabi* const app) : WShader(app) {}

void WForwardLightRenderStageObjectVS::Load(bool bSaveData) {
	int maxLights = (int)m_app->engineParams["maxLights"];
	m_desc = GetDesc(maxLights);
	vector<byte> code = {
		#include "Shaders/forward.vert.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
}

W_SHADER_DESC WForwardLightRenderStageObjectVS::GetDesc(int maxLights) {
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

WForwardLightRenderStageObjectPS::WForwardLightRenderStageObjectPS(Wasabi* const app) : WShader(app) {}

void WForwardLightRenderStageObjectPS::Load(bool bSaveData) {
	int maxLights = (int)m_app->engineParams["maxLights"];
	m_desc = GetDesc(maxLights);
	vector<byte> code = {
		#include "Shaders/forward.frag.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
}

W_SHADER_DESC WForwardLightRenderStageObjectPS::GetDesc(int maxLights) {
	W_SHADER_DESC desc;
	desc.type = W_FRAGMENT_SHADER;
	desc.bound_resources = {
		WForwardLightRenderStageObjectVS::GetDesc(maxLights).bound_resources[0],
		WForwardLightRenderStageObjectVS::GetDesc(maxLights).bound_resources[1],
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 4, 0, "diffuseTexture", {}, 8),
	};
	return desc;
}

WForwardLightRenderStage::WForwardLightRenderStage(Wasabi* const app) : WForwardRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = RENDER_STAGE_TARGET_BACK_BUFFER;
	m_stageDescription.flags = RENDER_STAGE_FLAG_PICKING_RENDER_STAGE;

	if (m_app->engineParams.find("maxLights") == m_app->engineParams.end())
		m_app->engineParams.insert(std::pair<std::string, void*>("maxLights", (void*)16));
	m_lights.resize((int)m_app->engineParams["maxLights"]);

	m_perFrameMaterial = nullptr;
}

class WEffect* WForwardLightRenderStage::LoadRenderEffect() {
	WForwardLightRenderStageObjectVS* vs = new WForwardLightRenderStageObjectVS(m_app);
	vs->SetName("DefaultForwardVS");
	m_app->FileManager->AddDefaultAsset(vs->GetName(), vs);
	vs->Load();

	WForwardLightRenderStageObjectPS* ps = new WForwardLightRenderStageObjectPS(m_app);
	ps->SetName("DefaultForwardPS");
	m_app->FileManager->AddDefaultAsset(ps->GetName(), ps);
	ps->Load();

	WEffect* fx = new WEffect(m_app);
	fx->SetName("DefaultForwardEffect");
	m_app->FileManager->AddDefaultAsset(fx->GetName(), fx);
	WError err = fx->BindShader(vs);
	if (err) {
		err = fx->BindShader(ps);
		if (err) {
			err = fx->BuildPipeline(m_renderTarget);
		}
	}
	W_SAFE_REMOVEREF(vs);
	W_SAFE_REMOVEREF(ps);
	if (!err)
		return nullptr;

	return fx;
}

WError WForwardLightRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WError err = WForwardRenderStage::Initialize(previousStages, width, height);
	if (err) {
		m_perFrameMaterial = m_renderEffect->CreateMaterial(1);
		if (!m_perFrameMaterial) {
			err = WError(W_ERRORUNK);
		} else {
			m_perFrameMaterial->SetName("PerFrameForwardMaterial");
			m_app->FileManager->AddDefaultAsset(m_perFrameMaterial->GetName(), m_perFrameMaterial);
		}
	}

	return err;
}

void WForwardLightRenderStage::Cleanup() {
	WForwardRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_perFrameMaterial);
}

WError WForwardLightRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_OBJECTS) {
		WCamera* cam = rt->GetCamera();

		// create the per-frame UBO data
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
		m_perFrameMaterial->SetVariableMatrix("viewMatrix", cam->GetViewMatrix());
		m_perFrameMaterial->SetVariableMatrix("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameMaterial->SetVariableVector3("camPosW", cam->GetPosition());
		m_perFrameMaterial->SetVariableInt("numLights", numLights);
		m_perFrameMaterial->SetVariableData("lights", m_lights.data(), sizeof(LightStruct) * numLights);
		m_perFrameMaterial->Bind(rt);
	}

	return WForwardRenderStage::Render(renderer, rt, filter);
}

WError WForwardLightRenderStage::Resize(uint width, uint height) {
	return WForwardRenderStage::Resize(width, height);
}
