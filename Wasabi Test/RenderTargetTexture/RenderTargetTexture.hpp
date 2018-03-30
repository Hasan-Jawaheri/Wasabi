#pragma once

#include "../TestSuite.hpp"

class RenderTargetTextureDemo : public WGameState {
	WObject *o, *o2, *o3;
	WLight* l;
	WRenderTarget* rt;
	WImage* rtImg;

public:
	RenderTargetTextureDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();
};