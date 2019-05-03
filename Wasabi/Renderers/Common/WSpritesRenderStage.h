#pragma once

#include "../WRenderStage.h"
#include "../Common/WRenderFragment.h"

class WSpritesRenderStage : public WRenderStage {
	WSpritesRenderFragment* m_spritesFragment;

public:
	WSpritesRenderStage(class Wasabi* const app, bool backbuffer = false);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);
};
