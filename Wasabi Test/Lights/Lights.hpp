#pragma once

#include "../TestSuite.hpp"
#include <Renderers/DeferredRenderer/WDeferredRenderer.h>

class LightsDemo : public WTestState {
	WObject* m_plain;
	vector<WObject*> m_boxes;
	vector<WLight*> m_lights;

public:
	LightsDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WError SetupRenderer() { return WInitializeDeferredRenderer(m_app); }
};
