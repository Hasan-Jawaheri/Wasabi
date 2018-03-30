#pragma once

#include "../TestSuite.hpp"
#include <Renderers/WDeferredRenderer.h>

class LightsDemo : public WTestState {
	WObject* m_plain;
	vector<WObject*> m_boxes;
	vector<WLight*> m_lights;

public:
	LightsDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WRenderer* CreateRenderer() { return new WDeferredRenderer(m_app); }
};
