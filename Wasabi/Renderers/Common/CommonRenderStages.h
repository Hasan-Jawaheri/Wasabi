#pragma once

#include "../WRenderStage.h"

class WParticlesRenderStage : public WRenderStage {
public:
	WParticlesRenderStage(class Wasabi* const app, bool backbuffer = false);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
};

class WTextsRenderStage : public WRenderStage {
public:
	WTextsRenderStage(class Wasabi* const app, bool backbuffer = false);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
};
