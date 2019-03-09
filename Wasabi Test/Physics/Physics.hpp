#pragma once

#include "../TestSuite.hpp"
#include <Renderers/WForwardRenderer.h>

class PhysicsDemo : public WTestState {
public:
	PhysicsDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WRenderer* CreateRenderer() { return new WForwardRenderer(m_app); }
};
