#pragma once

#include "../WRenderStage.h"

#include <map>

class WSpritesRenderStage : public WRenderStage {
	/** Default sprite effect */
	class WEffect* m_defaultSpriteFX;
	/** A counter to generate unique material names */
	uint m_currentMatId;

	struct SpriteKey {
		uint priority;
		class WEffect* fx;
		class WSprite* sprite;

		SpriteKey(class WSprite* sprite);
		const bool operator< (const SpriteKey& that) const;
	};
	/** Sorted container for all the sprites to be rendered by this stage */
	std::map<SpriteKey, class WSprite*> m_allSprites;

	void OnSpriteChange(class WSprite* s, bool added);

public:
	WSpritesRenderStage(class Wasabi* const app, bool backbuffer = false);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);
};
