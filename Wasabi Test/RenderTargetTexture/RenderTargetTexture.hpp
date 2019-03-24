#pragma once

#include "../TestSuite.hpp"
#include <Renderers/ForwardRenderer/WForwardRenderer.h>

class RenderTargetTextureDemo : public WTestState {
	WObject *o, *o2, *o3;
	WLight* l;
	WRenderTarget* rt;
	WImage* rtImg;

public:
	RenderTargetTextureDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WRenderer* CreateRenderer() { return new WForwardRenderer(m_app); }
};