#pragma once

#include "Wasabi/Core/WCore.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Images/WRenderTarget.h"
#include "Wasabi/Materials/WMaterial.h"
#include "Wasabi/Materials/WEffect.h"
#include "Wasabi/Materials/WMaterialsStore.h"
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
	std::vector<std::pair<SortingKeyT, class WEffect*>> m_reindexEntities;
	W_EFFECT_RENDER_FLAGS m_requiredRenderFlags;

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
		m_requiredRenderFlags = EFFECT_RENDER_FLAG_NONE;
		m_name = fragmentName;
		m_manager = manager;
		m_renderEffect = fx;
		m_currentMatId = 0;

		if (m_manager) {
			uint32_t numEntities = m_manager->GetEntitiesCount();
			for (uint32_t i = 0; i < numEntities; i++)
				OnEntityChange(m_manager->GetEntityByIndex(i), true);
			m_manager->RegisterChangeCallback(m_name, [this](EntityT* o, bool a) {this->OnEntityChange(o, a); });
		}
	}
	virtual ~WRenderFragment() {
		if (m_manager) {
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
	}

	class WEffect* GetEffect() const {
		return m_renderEffect;
	}

	W_EFFECT_RENDER_FLAGS GetRequiredRenderFlags() const {
		return m_requiredRenderFlags;
	}

	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt) {
		UNREFERENCED_PARAMETER(renderer);

		WEffect* boundFX = nullptr;
		for (auto it = m_allEntities.begin(); it != m_allEntities.end(); it++) {
			EntityT* entity = it->second;
			if (ShouldRenderEntity(entity)) {
				WEffect* effect = m_renderEffect;
				WMaterial* material = entity->GetMaterial(effect);
				if (!material) {
					// see if a custom effect can be used
					for (auto mat : entity->GetMaterials().m_materials) {
						if (mat.first->GetEffect()->GetRenderFlags() & m_requiredRenderFlags) {
							material = mat.first;
							effect = material->GetEffect();
							break;
						}
					}
				}
				if (material && entity->WillRender(rt)) {
					if (boundFX != effect) {
						effect->Bind(rt);
						boundFX = effect;
					}
					if (KeyChanged(entity, effect, it->first))
						m_reindexEntities.push_back(std::make_pair(it->first, effect));

					RenderEntity(entity, rt, material);
				}
			}
		}
		for (auto it : m_reindexEntities) {
			m_allEntities.erase(it.first);
			m_allEntities.insert(std::make_pair(SortingKeyT(it.first.GetEntity(), it.second), it.first.GetEntity()));
		}
		m_reindexEntities.clear();

		return WError(W_SUCCEEDED);
	}

	virtual void RenderEntity(EntityT* entity, class WRenderTarget* rt, class WMaterial* material) = 0;

	virtual bool ShouldRenderEntity(EntityT*) { return true; };

	virtual bool KeyChanged(EntityT* entity, class WEffect* effect, SortingKeyT key) = 0;

	virtual void OnEntityAdded(EntityT* entity) {
		bool entityHasUsableNonDefaultMaterial = false;
		for (auto mat : entity->GetMaterials().m_materials)
			if (mat.first->GetEffect()->GetRenderFlags() & m_requiredRenderFlags && !mat.first->GetAppPtr()->FileManager->IsDefaultAsset(mat.first->GetEffect()->GetName()))
				entityHasUsableNonDefaultMaterial = true;

		if (!entityHasUsableNonDefaultMaterial) {
			entity->AddEffect(m_renderEffect, 0);
			entity->GetMaterial(m_renderEffect)->SetName(GenerateMaterialName());
		}
	}
};


struct WObjectSortingKey {
	class WEffect* fx;
	class WObject* obj;

	WObjectSortingKey(class WObject* object, class WEffect* effect = nullptr) {
		obj = object;
		fx = effect;
	}

	const bool operator< (const WObjectSortingKey& that) const {
		return (void*)fx < (void*)that.fx ? true : fx == that.fx ? (obj < that.obj) : false;
	}

	class WObject* GetEntity() { return obj; }
};

class WObjectsRenderFragment : public WRenderFragment<WObject, WObjectSortingKey> {
	bool m_animated;

public:
	WObjectsRenderFragment(std::string fragmentName, bool animated, WEffect* fx, class Wasabi* wasabi, W_EFFECT_RENDER_FLAGS renderFlags) : WRenderFragment(fragmentName, fx, wasabi->ObjectManager) {
		m_animated = animated;
		m_requiredRenderFlags = renderFlags;
		fx->SetRenderFlags(m_requiredRenderFlags);
	}

	virtual void RenderEntity(WObject* object, class WRenderTarget* rt, class WMaterial* material) {
		object->Render(rt, material);
	}

	virtual bool KeyChanged(WObject* obj, class WEffect* effect, WObjectSortingKey key) {
		UNREFERENCED_PARAMETER(obj);
		return key.fx != effect;
	}

	virtual bool ShouldRenderEntity(WObject* object) {
		return (object->GetAnimation() != nullptr) == m_animated;
	};
};

struct WTerrainSortingKey {
	class WTerrain* terrain;

	WTerrainSortingKey(class WTerrain* t, class WEffect* effect = nullptr) {
		UNREFERENCED_PARAMETER(effect);
		terrain = t;
	}

	const bool operator< (const WTerrainSortingKey& that) const {
		return (terrain < that.terrain);
	}

	class WTerrain* GetEntity() { return terrain; }
};

class WTerrainRenderFragment : public WRenderFragment<WTerrain, WTerrainSortingKey> {
public:
	WTerrainRenderFragment(std::string fragmentName, WEffect* fx, class Wasabi* wasabi, W_EFFECT_RENDER_FLAGS renderFlags) : WRenderFragment(fragmentName, fx, wasabi->TerrainManager) {
		m_requiredRenderFlags = renderFlags;
		fx->SetRenderFlags(m_requiredRenderFlags);
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
};


struct WSpriteSortingKey {
	class WEffect* fx;
	class WSprite* sprite;
	uint32_t priority;

	WSpriteSortingKey(class WSprite* spr, class WEffect* effect = nullptr) {
		sprite = spr;
		priority = spr->GetPriority();
		fx = effect;
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
		m_requiredRenderFlags = EFFECT_RENDER_FLAG_RENDER_FORWARD;
		fx->SetRenderFlags(m_requiredRenderFlags);
	}

	virtual void RenderEntity(WSprite* sprite, class WRenderTarget* rt, class WMaterial* material) {
		material->Bind(rt);
		sprite->Render(rt);
	}

	virtual bool KeyChanged(WSprite* sprite, class WEffect* effect, WSpriteSortingKey key) {
		return sprite->GetPriority() != key.priority || key.fx != effect;
	}
};


struct WParticlesSortingKey {
	class WEffect* fx;
	class WParticles* particles;
	uint32_t priority;

	WParticlesSortingKey(class WParticles* par, class WEffect* effect = nullptr) {
		particles = par;
		priority = par->GetPriority();
		fx = effect;
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
		m_requiredRenderFlags = EFFECT_RENDER_FLAG_RENDER_FORWARD;
		m_particleEffects = effects;
		for (auto it : effects)
			it.second->SetRenderFlags(m_requiredRenderFlags);
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
		return particles->GetPriority() != key.priority || key.fx != effect;
	}

	virtual void OnEntityAdded(WParticles* particles) {
		auto it = m_particleEffects.find(particles->GetEffectType());
		if (it != m_particleEffects.end()) {
			m_renderEffect = it->second;
			WRenderFragment::OnEntityAdded(particles);
		}
	}
};
