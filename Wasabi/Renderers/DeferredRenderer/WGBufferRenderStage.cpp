#include "WGBufferRenderStage.h"
#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Images/WRenderTarget.h"
#include "../../Objects/WObject.h"
#include "../../Materials/WMaterial.h"
#include "../../Cameras/WCamera.h"

WGBufferVS::WGBufferVS(Wasabi* const app) : WShader(app) {}

void WGBufferVS::Load(bool bSaveData) {
	m_desc = WGBufferVS::GetDesc();
	LoadCodeGLSL(
		#include "Shaders/vertex_shader.glsl"
	, bSaveData);
}

W_SHADER_DESC WGBufferVS::GetDesc() {
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
		}),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, 0, "animationTexture"),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 3, 0, "instancingTexture"),
	};
	desc.input_layouts = { W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
	}), W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 4), // bone index
		W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4), // bone weight
	}) };
	return desc;
}

WGBufferPS::WGBufferPS(Wasabi* const app) : WShader(app) {}

void WGBufferPS::Load(bool bSaveData) {
	m_desc = WGBufferPS::GetDesc();
	LoadCodeGLSL(
		#include "Shaders/pixel_shader.glsl"
	, bSaveData);
}

W_SHADER_DESC WGBufferPS::GetDesc() {
	W_SHADER_DESC desc;
	desc.type = W_FRAGMENT_SHADER;
	desc.bound_resources = {
		WGBufferVS::GetDesc().bound_resources[0],
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 4, 0, "diffuseTexture"),
	};
	return desc;
}

WGBufferRenderStage::ObjectKey::ObjectKey(WObject* object) {
	obj = object;
	fx = obj->GetDefaultEffect();
	animated = obj->GetAnimation() != nullptr;
}

const bool WGBufferRenderStage::ObjectKey::operator< (const ObjectKey& that) const {
	if (animated != that.animated)
		return !animated;
	return (void*)fx < (void*)that.fx ? true : fx == that.fx ? (obj < that.obj) : false;
}

WGBufferRenderStage::WGBufferRenderStage(Wasabi* const app) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = RENDER_STAGE_TARGET_BUFFER;
	m_stageDescription.depthOutput = WRenderStage::OUTPUT_IMAGE("GBufferDepth", VK_FORMAT_D16_UNORM, WColor(1.0f, 0.0f, 0.0f, 0.0f));
	m_stageDescription.colorOutputs = std::vector<WRenderStage::OUTPUT_IMAGE>({
		WRenderStage::OUTPUT_IMAGE("GBufferDiffuse", VK_FORMAT_R8G8B8A8_UNORM, WColor(0.0f, 0.0f, 0.0f, 0.0f)),
		WRenderStage::OUTPUT_IMAGE("GBufferViewSpaceNormal", VK_FORMAT_R8G8B8A8_UNORM, WColor(0.0f, 0.0f, 0.0f, 0.0f)),
	});
	m_stageDescription.flags = RENDER_STAGE_FLAG_PICKING_RENDER_STAGE;

	m_GBufferFX = nullptr;
	m_perFrameMaterial = nullptr;
}

WError WGBufferRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	WGBufferVS* vs = new WGBufferVS(m_app);
	vs->Load();

	WGBufferPS* ps = new WGBufferPS(m_app);
	ps->Load();

	m_GBufferFX = new WEffect(m_app);
	err = m_GBufferFX->BindShader(vs);
	if (err) {
		err = m_GBufferFX->BindShader(ps);
		if (err) {
			err = m_GBufferFX->BuildPipeline(m_renderTarget);
		}
	}
	W_SAFE_REMOVEREF(ps);
	W_SAFE_REMOVEREF(vs);
	if (!err)
		return err;

	m_perFrameMaterial = m_GBufferFX->CreateMaterial(1);
	if (!m_perFrameMaterial)
		return WError(W_ERRORUNK);

	for (unsigned int i = 0; i < m_app->ObjectManager->GetEntitiesCount(); i++)
		OnObjectChange(m_app->ObjectManager->GetEntityByIndex(i), true);
	m_app->ObjectManager->RegisterChangeCallback(m_stageDescription.name, [this](WObject* o, bool a) {this->OnObjectChange(o, a); });

	return WError(W_SUCCEEDED);
}

void WGBufferRenderStage::OnObjectChange(class WObject* object, bool added) {
	if (added) {
		if (!object->GetDefaultEffect())
			object->AddEffect(this->m_GBufferFX, 0);
		ObjectKey key(object);
		this->m_allObjects.insert(std::make_pair(key, object));
	} else {
		auto iter = this->m_allObjects.find(ObjectKey(object));
		if (iter != this->m_allObjects.end())
			this->m_allObjects.erase(iter);
		else {
			// the sprite seems to have changed and then removed before we cloud reindex it in the render loop
			// need to find it manually now...
			for (auto it = this->m_allObjects.begin(); it != this->m_allObjects.end(); it++) {
				if (it->second == object) {
					this->m_allObjects.erase(it);
					break;
				}
			}
		}
	}
}

void WGBufferRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_GBufferFX);
	W_SAFE_REMOVEREF(m_perFrameMaterial);
	m_app->ObjectManager->RemoveChangeCallback(m_stageDescription.name);
}

WError WGBufferRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_OBJECTS) {
		WCamera* cam = rt->GetCamera();

		m_perFrameMaterial->SetVariableMatrix("viewMatrix", cam->GetViewMatrix());
		m_perFrameMaterial->SetVariableMatrix("projectionMatrix", cam->GetProjectionMatrix());
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

WError WGBufferRenderStage::Resize(uint width, uint height) {
	return WRenderStage::Resize(width, height);
}
