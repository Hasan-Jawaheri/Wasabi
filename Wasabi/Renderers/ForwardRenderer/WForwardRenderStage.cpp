#include "WForwardRenderStage.h"
#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Lights/WLight.h"
#include "../../Images/WRenderTarget.h"
#include "../../Objects/WObject.h"
#include "../../Materials/WMaterial.h"
#include "../../Cameras/WCamera.h"

static void _StringReplaceAll(std::string& source, const std::string& from, const std::string& to) {
	std::string newString;
	newString.reserve(source.length());  // avoids a few memory allocations

	std::string::size_type lastPos = 0;
	std::string::size_type findPos;

	while (std::string::npos != (findPos = source.find(from, lastPos))) {
		newString.append(source, lastPos, findPos - lastPos);
		newString += to;
		lastPos = findPos + from.length();
	}

	// Care for the rest after last occurrence
	newString += source.substr(lastPos);

	source.swap(newString);
}

WForwardRenderStageObjectVS::WForwardRenderStageObjectVS(Wasabi* const app) : WShader(app) {
}

void WForwardRenderStageObjectVS::Load(bool bSaveData) {
	m_desc.type = W_VERTEX_SHADER;
	m_desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_UBO, 0, 0, "uboPerObject", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projectionMatrix"), // projection
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "worldMatrix"), // world
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "viewMatrix"), // view
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "animationTextureWidth"), // width of the animation texture
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "instanceTextureWidth"), // width of the instance texture
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isAnimated"), // whether or not animation is enabled
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isInstanced"), // whether or not instancing is enabled
		}),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 1, 0, "animationTexture"),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, 0, "instancingTexture"),
	};
	m_desc.input_layouts = {W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
	}), W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 4), // bone indices
		W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4), // bone weights
	})};
	LoadCodeGLSL(
		#include "Shaders/vertex_shader.glsl"
	, bSaveData);
}

WForwardRenderStageObjectPS::WForwardRenderStageObjectPS(Wasabi* const app) : WShader(app) {
}

void WForwardRenderStageObjectPS::Load(bool bSaveData) {
	int maxLights = (int)m_app->engineParams["maxLights"];
	m_desc.type = W_FRAGMENT_SHADER;
	m_desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 3, 0, "diffuseTexture"),
		W_BOUND_RESOURCE(W_TYPE_UBO, 4, 0, "uboPerObjectPS", {
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4, "color"),
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isTextured"),
		}),
		W_BOUND_RESOURCE(W_TYPE_UBO, 5, 1, "uboPerFrame", {
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "numLights"),
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "camPosW"),
			W_SHADER_VARIABLE_INFO(W_TYPE_STRUCT, maxLights, sizeof(LightStruct), 16, "lights"),
		}),
	};
	std::string code =
		#include "Shaders/pixel_shader.glsl"
	;
	_StringReplaceAll(code, "~~~~maxLights~~~~", std::to_string(maxLights));
	LoadCodeGLSL(code, bSaveData);
}

WForwardRenderStage::WForwardRenderStage(Wasabi* const app) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = RENDER_STAGE_TARGET_BACK_BUFFER;

	m_lights = new WForwardRenderStageObjectPS::LightStruct[(int)m_app->engineParams["maxLights"]];

	m_defaultFX = nullptr;
	m_perFrameMaterial = nullptr;
}

WError WForwardRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WRenderStage::Initialize(previousStages, width, height);

	WForwardRenderStageObjectVS* vs = new WForwardRenderStageObjectVS(m_app);
	vs->Load();

	WForwardRenderStageObjectPS* ps = new WForwardRenderStageObjectPS(m_app);
	ps->Load();

	m_defaultFX = new WEffect(m_app);
	WError err = m_defaultFX->BindShader(vs);
	if (err) {
		err = m_defaultFX->BindShader(ps);
		if (err) {
			err = m_defaultFX->BuildPipeline(m_app->Renderer->GetRenderTarget(m_stageDescription.name));
		}
	}
	W_SAFE_REMOVEREF(vs);
	W_SAFE_REMOVEREF(ps);
	if (!err)
		return WError(W_ERRORUNK);

	m_perFrameMaterial = new WMaterial(m_app);
	err = m_perFrameMaterial->CreateForEffect(m_defaultFX, 1);
	if (!err)
		return WError(W_ERRORUNK);

	m_objectMaterials.Initialize(m_stageDescription.name, m_app->ObjectManager, [this](WObject*) { return this->CreateDefaultObjectMaterial(); });

	return WError(W_SUCCEEDED);
}

WError WForwardRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_OBJECTS) {
		WCamera* cam = rt->GetCamera();

		// create the lights UBO data
		int maxLights = (int)m_app->engineParams["maxLights"];
		int numLights = 0;
		for (int i = 0; numLights < maxLights; i++) {
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
		m_perFrameMaterial->SetVariableVector3("camPosW", cam->GetPosition());
		m_perFrameMaterial->SetVariableInt("numLights", numLights);
		m_perFrameMaterial->SetVariableData("lights", m_lights, sizeof(WForwardRenderStageObjectPS::LightStruct) * numLights);

		m_perFrameMaterial->Bind(rt);

		uint numObjects = m_app->ObjectManager->GetEntitiesCount();

		// render non-animated objects
		m_defaultFX->Bind(rt, 1);
		for (uint i = 0; i < numObjects; i++) {
			WObject* object = m_app->ObjectManager->GetEntityByIndex(i);
			if (!object->GetAnimation()) {
				WMaterial* material = m_objectMaterials.GetResourceOf(object);
				if (material) {
					material->Bind(rt);
					object->Render(rt);
				}
			}
		}

		// render animated objects
		m_defaultFX->Bind(rt, 2);
		for (uint i = 0; i < numObjects; i++) {
			WObject* object = m_app->ObjectManager->GetEntityByIndex(i);
			if (object->GetAnimation()) {
				WMaterial* material = m_objectMaterials.GetResourceOf(object);
				if (material) {
					material->Bind(rt);
					object->Render(rt);
				}
			}
		}
	}

	return WError(W_SUCCEEDED);
}

void WForwardRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_defaultFX);
	W_SAFE_REMOVEREF(m_perFrameMaterial);
	m_objectMaterials.Cleanup();
}

WError WForwardRenderStage::Resize(uint width, uint height) {
	return WRenderStage::Resize(width, height);
}


WMaterial* WForwardRenderStage::CreateDefaultObjectMaterial() const {
	WMaterial* mat = new WMaterial(m_app);
	if (!mat->CreateForEffect(m_defaultFX, 0))
		W_SAFE_REMOVEREF(mat);
	return mat;
}
