#include "WForwardRenderStage.h"
#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Lights/WLight.h"
#include "../../Images/WRenderTarget.h"
#include "../../Objects/WObject.h"
#include "../../Materials/WMaterial.h"
#include "../../Cameras/WCamera.h"

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

WForwardRenderStage::WForwardRenderStage(Wasabi* const app, bool backbuffer) : WRenderStage(app) {
	m_currentMatId = 0;
	m_renderEffect = nullptr;
	m_setEffectDefault = false;
}

WError WForwardRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	m_renderEffect = LoadRenderEffect();
	if (!m_renderEffect)
		return WError(W_ERRORUNK);

	for (unsigned int i = 0; i < m_app->ObjectManager->GetEntitiesCount(); i++)
		OnObjectChange(m_app->ObjectManager->GetEntityByIndex(i), true);
	m_app->ObjectManager->RegisterChangeCallback(m_stageDescription.name, [this](WObject* o, bool a) {this->OnObjectChange(o, a); });

	return WError(W_SUCCEEDED);
}

void WForwardRenderStage::OnObjectChange(class WObject* object, bool added) {
	if (added) {
		object->AddEffect(this->m_renderEffect, 0, this->m_setEffectDefault);
		object->GetMaterial(this->m_renderEffect)->SetName("Material-" + m_stageDescription.name + std::to_string(this->m_currentMatId++));
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

void WForwardRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_renderEffect);
	m_app->ObjectManager->RemoveChangeCallback(m_stageDescription.name);
}

WError WForwardRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_OBJECTS) {
		WCamera* cam = rt->GetCamera();

		uint numObjects = m_app->ObjectManager->GetEntitiesCount();

		WEffect* boundFX = nullptr;
		bool isBoundFXAnimated = false;
		std::vector<ObjectKey> reindexObjects;
		for (auto it = m_allObjects.begin(); it != m_allObjects.end(); it++) {
			WObject* object = it->second;
			WEffect* effect = m_renderEffect;
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

WError WForwardRenderStage::Resize(uint width, uint height) {
	return WRenderStage::Resize(width, height);
}
