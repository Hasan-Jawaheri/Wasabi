#pragma once

#include "Wasabi/Renderers/WRenderStage.h"

class WTextsRenderStage : public WRenderStage {
public:
	WTextsRenderStage(class Wasabi* const app, bool backbuffer = false);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
};
