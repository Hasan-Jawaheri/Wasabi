#pragma once

#include "../TestSuite.hpp"
#include <Renderers/WForwardRenderer.h>

class LightsDemo : public WTestState {
	WObject* m_plain;
	vector<WObject*> m_boxes;
	vector<WLight*> m_lights;

public:
	LightsDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WRenderer* CreateRenderer() { return new WForwardRenderer(m_app); }
};
