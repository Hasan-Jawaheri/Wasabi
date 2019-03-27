#include "WSpritesRenderStage.h"
#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Images/WRenderTarget.h"
#include "../../Sprites/WSprite.h"
#include "../../Materials/WEffect.h"
#include "../../Materials/WMaterial.h"

WSpritesRenderStage::SpriteKey::SpriteKey(WSprite* spr) {
	sprite = spr;
	fx = spr->GetDefaultEffect();
	priority = spr->GetPriority();
}

const bool WSpritesRenderStage::SpriteKey::operator< (const SpriteKey& that) const {
	if (priority != that.priority)
		return priority < that.priority;

	return (void*)fx < (void*)that.fx ? true : fx == that.fx ? (sprite < that.sprite) : false;
}

WSpritesRenderStage::WSpritesRenderStage(Wasabi* const app, bool backbuffer) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = backbuffer ? RENDER_STAGE_TARGET_BACK_BUFFER : RENDER_STAGE_TARGET_PREVIOUS;

	m_defaultSpriteFX = nullptr;
}

WError WSpritesRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WRenderStage::Initialize(previousStages, width, height);

	m_defaultSpriteFX = m_app->SpriteManager->CreateSpriteEffect();
	if (!m_defaultSpriteFX)
		return WError(W_ERRORUNK);

	m_app->SpriteManager->RegisterChangeCallback(m_stageDescription.name, [this](WSprite* s, bool added) {
		if (added) {
			if (!s->GetDefaultEffect())
				s->AddEffect(this->m_defaultSpriteFX);
			SpriteKey key(s);
			this->m_allSprites.insert(std::make_pair(key, s));
		} else {
			auto iter = this->m_allSprites.find(SpriteKey(s));
			if (iter != this->m_allSprites.end())
				this->m_allSprites.erase(iter);
			else {
				// the sprite seems to have changed and then removed before we cloud reindex it in the render loop
				// need to find it manually now...
				for (auto it = this->m_allSprites.begin(); it != this->m_allSprites.end(); it++) {
					if (it->second == s) {
						this->m_allSprites.erase(it);
						break;
					}
				}
			}
		}
	});

	return WError(W_SUCCEEDED);
}

WError WSpritesRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_SPRITES) {
		WEffect* boundFX = nullptr;
		std::vector<SpriteKey> reindexSprites;
		for (auto it = m_allSprites.begin(); it != m_allSprites.end(); it++) {
			WSprite* sprite = it->second;
			WEffect* effect = sprite->GetDefaultEffect();
			WMaterial* material = sprite->GetMaterial(effect);
			if (material && sprite->WillRender(rt)) {
				uint priority = sprite->GetPriority();
				if (boundFX != effect) {
					effect->Bind(rt);
					boundFX = effect;
				}
				if (priority != it->first.priority || effect != it->first.fx)
					reindexSprites.push_back(it->first);

				material->Bind(rt);
				sprite->Render(rt);
			}
		}
		for (auto it = reindexSprites.begin(); it != reindexSprites.end(); it++) {
			m_allSprites.erase(*it);
			m_allSprites.insert(std::make_pair(SpriteKey(it->sprite), it->sprite));
		}
	}

	return WError(W_SUCCEEDED);
}

void WSpritesRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_defaultSpriteFX);
	m_app->SpriteManager->RemoveChangeCallback(m_stageDescription.name);
}

WError WSpritesRenderStage::Resize(uint width, uint height) {
	return WRenderStage::Resize(width, height);
}

