#pragma once

#include "../TestSuite.hpp"
#include <Renderers/ForwardRenderer/WForwardRenderer.h>

class InstancingDemo : public WTestState {
	WObject* character;
	WGeometry* geometry;
	WImage* texture;
	vector<WObject*> objectsV;

public:
	InstancingDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WRenderer* CreateRenderer() { return new WForwardRenderer(m_app); }
};