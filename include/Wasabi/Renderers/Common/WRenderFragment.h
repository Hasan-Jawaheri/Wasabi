#pragma once

#include "Wasabi/Core/WCore.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Images/WRenderTarget.h"
#include "Wasabi/Materials/WMaterial.h"
#include "Wasabi/Materials/WEffect.h"
#include "Wasabi/Cameras/WCamera.h"

#include "Wasabi/Objects/WObject.h"
#include "Wasabi/Terrains/WTerrain.h"
#include "Wasabi/Sprites/WSprite.h"
#include "Wasabi/Particles/WParticles.h"

#include <map>

/*
 * A render fragment is a part of a render stage that renders
 */
template<typename EntityT, typename SortingKeyT>
class WRenderFragment {
protected:
	std::string m_name;
	class WManager<EntityT>* m_manager;
	class WEffect* m_renderEffect;
	uint32_t m_currentMatId;
	std::vector<SortingKeyT> m_reindexEntities;

	/** Sorted container for all the entities to be rendered by this fragment */
	std::map<SortingKeyT, EntityT*> m_allEntities;

	void OnEntityChange(EntityT* entity, bool added) {
		if (added) {
			OnEntityAdded(entity);
			SortingKeyT key(entity);
			m_allEntities.insert(std::make_pair(key, entity));
		} else {
			auto iter = m_allEntities.find(SortingKeyT(entity));
			if (iter != m_allEntities.end())
				m_allEntities.erase(iter);
			else {
				// the entity seems to have changed and then removed before we cloud reindex it in the render loop
				// need to find it manually now...
				for (auto it = m_allEntities.begin(); it != m_allEntities.end(); it++) {
					if (it->second == entity) {
						m_allEntities.erase(it);
						break;
					}
				}
			}
		}
	}

	std::string GenerateMaterialName() {
		return "Material-" + this->m_name + std::to_string(this->m_currentMatId++);
	}

public:
	WRenderFragment(std::string fragmentName, WEffect* fx, class WManager<EntityT>* manager) {
		m_name = fragmentName;
		m_manager = manager;
		m_renderEffect = fx;
		m_currentMatId = 0;

		uint32_t numEntities = m_manager->GetEntitiesCount();
		for (uint32_t i = 0; i < numEntities; i++)
			OnEntityChange(m_manager->GetEntityByIndex(i), true);
		m_manager->RegisterChangeCallback(m_name, [this](EntityT* o, bool a) {this->OnEntityChange(o, a); });
	}
	virtual ~WRenderFragment() {
		if (m_renderEffect) {
			uint32_t numEntities = m_manager->GetEntitiesCount();
			for (uint32_t i = 0; i < numEntities; i++) {
				EntityT* entity = m_manager->GetEntityByIndex(i);
				entity->RemoveEffect(m_renderEffect);
			}
			W_SAFE_REMOVEREF(m_renderEffect);
		}
		m_manager->RemoveChangeCallback(m_name);
	}

	class WEffect* GetEffect() {
		return m_renderEffect;
	}

	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt) {
		UNREFERENCED_PARAMETER(renderer);

		WEffect* boundFX = nullptr;
		for (auto it = m_allEntities.begin(); it != m_allEntities.end(); it++) {
			EntityT* entity = it->second;
			if (ShouldRenderEntity(entity)) {
				WEffect* effect = m_renderEffect;
				WMaterial* material = entity->GetMaterial(effect);
				if (material && entity->WillRender(rt)) {
					if (boundFX != effect) {
						effect->Bind(rt);
						boundFX = effect;
					}
					if (KeyChanged(entity, effect, it->first))
						m_reindexEntities.push_back(it->first);

					RenderEntity(entity, rt, material);
				}
			}
		}
		for (auto it = m_reindexEntities.begin(); it != m_reindexEntities.end(); it++) {
			m_allEntities.erase(*it);
			m_allEntities.insert(std::make_pair(SortingKeyT(it->GetEntity()), it->GetEntity()));
		}
		m_reindexEntities.clear();

		return WError(W_SUCCEEDED);
	}

	virtual void RenderEntity(EntityT* entity, class WRenderTarget* rt, class WMaterial* material) = 0;

	virtual bool ShouldRenderEntity(EntityT*) { return true; };

	virtual bool KeyChanged(EntityT* entity, class WEffect* effect, SortingKeyT key) = 0;

	virtual void OnEntityAdded(EntityT* entity) {
		entity->AddEffect(m_renderEffect, 0);
		entity->GetMaterial(m_renderEffect)->SetName(GenerateMaterialName());
	}
};


struct WObjectSortingKey {
	class WObject* obj;

	WObjectSortingKey(class WObject* object) {
		obj = object;
	}

	const bool operator< (const WObjectSortingKey& that) const {
		return (obj < that.obj);
	}

	class WObject* GetEntity() { return obj; }
};

class WObjectsRenderFragment : public WRenderFragment<WObject, WObjectSortingKey> {
	bool m_animated;

public:
	WObjectsRenderFragment(std::string fragmentName, bool animated, WEffect* fx, class Wasabi* wasabi) : WRenderFragment(fragmentName, fx, wasabi->ObjectManager) {
		m_animated = animated;
	}

	virtual void RenderEntity(WObject* object, class WRenderTarget* rt, class WMaterial* material) {
		object->Render(rt, material);
	}

	virtual bool KeyChanged(WObject* obj, class WEffect* effect, WObjectSortingKey key) {
		UNREFERENCED_PARAMETER(obj);
		UNREFERENCED_PARAMETER(effect);
		UNREFERENCED_PARAMETER(key);
		return false;
	}

	virtual bool ShouldRenderEntity(WObject* object) {
		return (object->GetAnimation() != nullptr) == m_animated;
	};

	virtual void OnEntityAdded(WObject* object) {
		object->AddEffect(m_renderEffect, 0);
		object->GetMaterial(m_renderEffect)->SetName(GenerateMaterialName());
	}
};

struct WTerrainSortingKey {
	class WTerrain* terrain;

	WTerrainSortingKey(class WTerrain* t) {
		terrain = t;
	}

	const bool operator< (const WTerrainSortingKey& that) const {
		return (terrain < that.terrain);
	}

	class WTerrain* GetEntity() { return terrain; }
};

class WTerrainRenderFragment : public WRenderFragment<WTerrain, WTerrainSortingKey> {
public:
	WTerrainRenderFragment(std::string fragmentName, WEffect* fx, class Wasabi* wasabi) : WRenderFragment(fragmentName, fx, wasabi->TerrainManager) {
	}

	virtual void RenderEntity(WTerrain* terrain, class WRenderTarget* rt, class WMaterial* material) {
		terrain->Render(rt, material);
	}

	virtual bool KeyChanged(WTerrain* terrain, class WEffect* effect, WTerrainSortingKey key) {
		UNREFERENCED_PARAMETER(terrain);
		UNREFERENCED_PARAMETER(effect);
		UNREFERENCED_PARAMETER(key);
		return false;
	}

	virtual bool IsEntityAnimated(WTerrain* terrain) {
		UNREFERENCED_PARAMETER(terrain);
		return false;
	}
};


struct WSpriteSortingKey {
	class WSprite* sprite;
	uint32_t priority;

	WSpriteSortingKey(class WSprite* spr) {
		sprite = spr;
		priority = spr->GetPriority();
	}

	const bool operator< (const WSpriteSortingKey& that) const {
		if (priority != that.priority)
			return priority < that.priority;
		return (sprite < that.sprite);
	}

	class WSprite* GetEntity() { return sprite; }
};

class WSpritesRenderFragment : public WRenderFragment<WSprite, WSpriteSortingKey> {
public:
	WSpritesRenderFragment(std::string fragmentName, WEffect* fx, class Wasabi* wasabi) : WRenderFragment(fragmentName, fx, wasabi->SpriteManager) {
	}

	virtual void RenderEntity(WSprite* sprite, class WRenderTarget* rt, class WMaterial* material) {
		material->Bind(rt);
		sprite->Render(rt);
	}

	virtual bool KeyChanged(WSprite* sprite, class WEffect* effect, WSpriteSortingKey key) {
		UNREFERENCED_PARAMETER(effect);
		return sprite->GetPriority() != key.priority;
	}

	virtual bool IsEntityAnimated(WSprite* sprite) {
		UNREFERENCED_PARAMETER(sprite);
		return false;
	}
};


struct WParticlesSortingKey {
	class WParticles* particles;
	uint32_t priority;

	WParticlesSortingKey(class WParticles* par) {
		particles = par;
		priority = par->GetPriority();
	}

	const bool operator< (const WParticlesSortingKey& that) const {
		if (priority != that.priority)
			return priority < that.priority;
		return (particles < that.particles);
	}

	class WParticles* GetEntity() { return particles; }
};

class WParticlesRenderFragment : public WRenderFragment<WParticles, WParticlesSortingKey> {
	/** Default effect used by different particle systems */
	unordered_map<W_DEFAULT_PARTICLE_EFFECT_TYPE, class WEffect*> m_particleEffects;

public:
	WParticlesRenderFragment(std::string fragmentName, unordered_map<W_DEFAULT_PARTICLE_EFFECT_TYPE, class WEffect*> effects, class Wasabi* wasabi) : WRenderFragment(fragmentName, nullptr, wasabi->ParticlesManager) {
		m_particleEffects = effects;
	}

	virtual ~WParticlesRenderFragment() {
		uint32_t numEntities = m_manager->GetEntitiesCount();
		for (uint32_t i = 0; i < numEntities; i++) {
			WParticles* particles= m_manager->GetEntityByIndex(i);
			for (auto renderEffect : m_particleEffects)
				particles->RemoveEffect(renderEffect.second);
		}
		for (auto effect : m_particleEffects)
			effect.second->RemoveReference();
		m_particleEffects.clear();
	}

	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt) {
		WError err = WError(W_SUCCEEDED);
		for (auto renderEffect : m_particleEffects) {
			m_renderEffect = renderEffect.second;
			err = WRenderFragment::Render(renderer, rt);
			if (!err)
				break;
		}
		m_renderEffect = nullptr;
		return err;
	}

	virtual void RenderEntity(WParticles* particles, class WRenderTarget* rt, class WMaterial* material) {
		particles->Render(rt, material);
	}

	virtual bool KeyChanged(WParticles* particles, class WEffect* effect, WParticlesSortingKey key) {
		UNREFERENCED_PARAMETER(effect);
		return particles->GetPriority() != key.priority;
	}

	virtual bool IsEntityAnimated(WParticles* sprite) {
		UNREFERENCED_PARAMETER(sprite);
		return false;
	}

	virtual void OnEntityAdded(WParticles* particles) {
		auto it = m_particleEffects.find(particles->GetEffectType());
		if (it != m_particleEffects.end()) {
			particles->AddEffect(it->second, 0);
			particles->GetMaterial(it->second)->SetName(GenerateMaterialName());
		}
	}
};
