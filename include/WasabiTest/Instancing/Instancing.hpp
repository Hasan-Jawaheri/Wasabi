#pragma once

#include "TestSuite.hpp"
#include <Wasabi/Renderers/ForwardRenderer/WForwardRenderer.hpp>

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

	virtual WError SetupRenderer() { return WInitializeForwardRenderer(m_app); }
};