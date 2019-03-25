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
	int maxLights = (int)m_app->engineParams["maxLights"];
	m_desc = GetDesc(maxLights);
	std::string code =
		#include "Shaders/vertex_shader.glsl"
	;
	_StringReplaceAll(code, "~~~~maxLights~~~~", std::to_string(maxLights));
	LoadCodeGLSL(code, bSaveData);
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
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4, "color"),
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
	}), W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 4), // bone indices
		W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4), // bone weights
	}) };
	return desc;
}

WForwardRenderStageObjectPS::WForwardRenderStageObjectPS(Wasabi* const app) : WShader(app) {
}

void WForwardRenderStageObjectPS::Load(bool bSaveData) {
	int maxLights = (int)m_app->engineParams["maxLights"];
	m_desc = GetDesc(maxLights);
	std::string code =
		#include "Shaders/pixel_shader.glsl"
	;
	_StringReplaceAll(code, "~~~~maxLights~~~~", std::to_string(maxLights));
	LoadCodeGLSL(code, bSaveData);
}

W_SHADER_DESC WForwardRenderStageObjectPS::GetDesc(int maxLights) {
	W_SHADER_DESC desc;
	desc.type = W_FRAGMENT_SHADER;
	desc.bound_resources = {
		WForwardRenderStageObjectVS::GetDesc(maxLights).bound_resources[0],
		WForwardRenderStageObjectVS::GetDesc(maxLights).bound_resources[1],
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 4, 0, "diffuseTexture"),
	};
	return desc;
}

WForwardRenderStage::ObjectKey::ObjectKey(WObject* object) {
	obj = object;
	fx = obj->GetDefaultEffect();
	animated = obj->GetAnimation() != nullptr;
}

const bool WForwardRenderStage::ObjectKey::operator< (const ObjectKey& that) const {
	if (animated != that.animated)
		return !animated;
	return (void*)fx < (void*)that.fx ? true : fx == that.fx ? (obj < that.obj) : false;
}

WForwardRenderStage::WForwardRenderStage(Wasabi* const app) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = RENDER_STAGE_TARGET_BACK_BUFFER;

	m_lights = new LightStruct[(int)m_app->engineParams["maxLights"]];

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

	m_perFrameMaterial = m_defaultFX->CreateMaterial(1);
	if (!m_perFrameMaterial)
		return WError(W_ERRORUNK);

	m_app->ObjectManager->RegisterChangeCallback(m_stageDescription.name, [this](WObject* o, bool added) {
		if (added) {
			if (!o->GetDefaultEffect())
				o->AddEffect(this->m_defaultFX, 0);
			ObjectKey key(o);
			this->m_allObjects.insert(std::make_pair(key, o));
		} else {
			auto iter = this->m_allObjects.find(ObjectKey(o));
			if (iter != this->m_allObjects.end())
				this->m_allObjects.erase(iter);
			else {
				// the sprite seems to have changed and then removed before we cloud reindex it in the render loop
				// need to find it manually now...
				for (auto it = this->m_allObjects.begin(); it != this->m_allObjects.end(); it++) {
					if (it->second == o) {
						this->m_allObjects.erase(it);
						break;
					}
				}
			}
		}
	});

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
		m_perFrameMaterial->SetVariableMatrix("viewMatrix", cam->GetViewMatrix());
		m_perFrameMaterial->SetVariableMatrix("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameMaterial->SetVariableVector3("camPosW", cam->GetPosition());
		m_perFrameMaterial->SetVariableInt("numLights", numLights);
		m_perFrameMaterial->SetVariableData("lights", m_lights, sizeof(LightStruct) * numLights);

		m_perFrameMaterial->Bind(rt);

		uint numObjects = m_app->ObjectManager->GetEntitiesCount();

		WEffect* boundFX = nullptr;
		bool isBoundFXAnimated = false;
		std::vector<ObjectKey> reindexObjects;
		for (auto it = m_allObjects.begin(); it != m_allObjects.end(); it++) {
			WObject* object = it->second;
			WEffect* effect = object->GetDefaultEffect();
			WMaterial* material = object->GetMaterial(effect);
			if (material && object->WillRender(rt)) {
				bool isAnimated = object->GetAnimation() != nullptr;
				if (boundFX != effect || (isAnimated != isBoundFXAnimated)) {
					effect->Bind(rt, isAnimated ? 2 : 1);
					boundFX = effect;
					isBoundFXAnimated = isAnimated;
				}
				if (effect != it->first.fx || isAnimated != it->first.animated)
					reindexObjects.push_back(it->first);

				object->Render(rt, material);
			}
		}
		for (auto it = reindexObjects.begin(); it != reindexObjects.end(); it++) {
			m_allObjects.erase(*it);
			m_allObjects.insert(std::make_pair(ObjectKey(it->obj), it->obj));
		}
	}

	return WError(W_SUCCEEDED);
}

void WForwardRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_defaultFX);
	W_SAFE_REMOVEREF(m_perFrameMaterial);
	m_app->ObjectManager->RemoveChangeCallback(m_stageDescription.name);
}

WError WForwardRenderStage::Resize(uint width, uint height) {
	return WRenderStage::Resize(width, height);
}
