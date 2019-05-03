#pragma once

#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Images/WRenderTarget.h"
#include "../../Materials/WMaterial.h"
#include "../../Materials/WEffect.h"
#include "../../Cameras/WCamera.h"

#include "../../Objects/WObject.h"

#include "../../Terrains/WTerrain.h"

#include "../../Sprites/WSprite.h"

#include "../../Particles/WParticles.h"

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
	uint m_currentMatId;

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

		unsigned int numEntities = m_manager->GetEntitiesCount();
		for (unsigned int i = 0; i < numEntities; i++)
			OnEntityChange(m_manager->GetEntityByIndex(i), true);
		m_manager->RegisterChangeCallback(m_name, [this](EntityT* o, bool a) {this->OnEntityChange(o, a); });
	}
	virtual ~WRenderFragment() {
		W_SAFE_REMOVEREF(m_renderEffect);
		m_manager->RemoveChangeCallback(m_name);
		unsigned int numEntities = m_manager->GetEntitiesCount();
		for (unsigned int i = 0; i < numEntities; i++) {
			EntityT* entity = m_manager->GetEntityByIndex(i);
			entity->RemoveEffect(m_renderEffect);
		}
	}

	class WEffect* GetEffect() {
		return m_renderEffect;
	}

	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt) {
		WCamera* cam = rt->GetCamera();

		uint numEntities = m_manager->GetEntitiesCount();

		WEffect* boundFX = nullptr;
		bool isBoundFXAnimated = false;
		std::vector<SortingKeyT> reindexEntities;
		for (auto it = m_allEntities.begin(); it != m_allEntities.end(); it++) {
			EntityT* entity = it->second;
			WEffect* effect = m_renderEffect;
			WMaterial* material = entity->GetMaterial(effect);
			if (!material)
				material = entity->GetMaterial();
			effect = material->GetEffect(); // in case m_renderEffect is null, then effect is the (default) material's effect
			if (material && entity->WillRender(rt)) {
				bool isAnimated = IsEntityAnimated(entity);
				if (boundFX != effect || (isAnimated != isBoundFXAnimated)) {
					effect->Bind(rt, isAnimated ? 2 : 1);
					boundFX = effect;
					isBoundFXAnimated = isAnimated;
				}
				if (KeyChanged(entity, effect, it->first))
					reindexEntities.push_back(it->first);

				RenderEntity(entity, rt, material);
			}
		}
		for (auto it = reindexEntities.begin(); it != reindexEntities.end(); it++) {
			m_allEntities.erase(*it);
			m_allEntities.insert(std::make_pair(SortingKeyT(it->GetEntity()), it->GetEntity()));
		}

		return WError(W_SUCCEEDED);
	}

	virtual void RenderEntity(EntityT* entity, class WRenderTarget* rt, class WMaterial* material) = 0;

	virtual bool KeyChanged(EntityT* entity, class WEffect* effect, SortingKeyT key) = 0;

	virtual bool IsEntityAnimated(EntityT* entity) = 0;

	virtual void OnEntityAdded(EntityT* entity) {
		entity->AddEffect(m_renderEffect, 0);
		entity->GetMaterial(m_renderEffect)->SetName(GenerateMaterialName());
	}
};


struct WObjectSortingKey {
	class WEffect* fx;
	class WObject* obj;
	bool animated;

	WObjectSortingKey(class WObject* object) {
		obj = object;
		fx = obj->GetDefaultEffect();
		animated = obj->GetAnimation() != nullptr;
	}

	const bool operator< (const WObjectSortingKey& that) const {
		if (animated != that.animated)
			return !animated;
		return (void*)fx < (void*)that.fx ? true : fx == that.fx ? (obj < that.obj) : false;
	}

	class WObject* GetEntity() { return obj; }
};

class WObjectsRenderFragment : public WRenderFragment<WObject, WObjectSortingKey> {
	bool m_setEffectDefault;

public:
	WObjectsRenderFragment(std::string fragmentName, WEffect* fx, class Wasabi* wasabi, bool setEffectDefault = true) : WRenderFragment(fragmentName, fx, wasabi->ObjectManager) {
		m_setEffectDefault = setEffectDefault;
	}

	virtual void RenderEntity(WObject* object, class WRenderTarget* rt, class WMaterial* material) {
		object->Render(rt, material);
	}

	virtual bool KeyChanged(WObject* object, class WEffect* effect, WObjectSortingKey key) {
		return effect != key.fx || IsEntityAnimated(object) != key.animated;
	}

	virtual bool IsEntityAnimated(WObject* object) {
		return object->GetAnimation() != nullptr;
	}

	virtual void OnEntityAdded(WObject* object) {
		object->AddEffect(m_renderEffect, 0, m_setEffectDefault);
		object->GetMaterial(m_renderEffect)->SetName(GenerateMaterialName());
	}
};

struct WTerrainSortingKey {
	class WEffect* fx;
	class WTerrain* terrain;

	WTerrainSortingKey(class WTerrain* t) {
		terrain = t;
		fx = terrain->GetDefaultEffect();
	}

	const bool operator< (const WTerrainSortingKey& that) const {
		return (void*)fx < (void*)that.fx ? true : fx == that.fx ? (terrain < that.terrain) : false;
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
		return effect != key.fx;
	}

	virtual bool IsEntityAnimated(WTerrain* terrain) {
		return false;
	}
};


struct WSpriteSortingKey {
	class WEffect* fx;
	class WSprite* sprite;
	uint priority;

	WSpriteSortingKey(class WSprite* spr) {
		sprite = spr;
		fx = spr->GetDefaultEffect();
		priority = spr->GetPriority();
	}

	const bool operator< (const WSpriteSortingKey& that) const {
		if (priority != that.priority)
			return priority < that.priority;
		return (void*)fx < (void*)that.fx ? true : fx == that.fx ? (sprite < that.sprite) : false;
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
		return effect != key.fx || sprite->GetPriority() != key.priority;
	}

	virtual bool IsEntityAnimated(WSprite* sprite) {
		return false;
	}
};


struct WParticlesSortingKey {
	class WEffect* fx;
	class WParticles* particles;
	uint priority;

	WParticlesSortingKey(class WParticles* par) {
		particles = par;
		priority = par->GetPriority();
		fx = par->GetDefaultEffect();
	}

	const bool operator< (const WParticlesSortingKey& that) const {
		if (priority != that.priority)
			return priority < that.priority;
		return (void*)fx < (void*)that.fx ? true : fx == that.fx ? (particles < that.particles) : false;
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

	virtual void RenderEntity(WParticles* particles, class WRenderTarget* rt, class WMaterial* material) {
		particles->Render(rt, material);
	}

	virtual bool KeyChanged(WParticles* particles, class WEffect* effect, WParticlesSortingKey key) {
		return effect != key.fx || particles->GetPriority() != key.priority;
	}

	virtual bool IsEntityAnimated(WParticles* sprite) {
		return false;
	}

	virtual void OnEntityAdded(WParticles* particles) {
		auto it = m_particleEffects.find(particles->GetType());
		if (it != m_particleEffects.end()) {
			particles->AddEffect(it->second, 0);
			particles->GetMaterial(it->second)->SetName(GenerateMaterialName());
		}
	}
};

