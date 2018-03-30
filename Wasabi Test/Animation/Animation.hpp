#pragma once

#include "../TestSuite.hpp"
#include <Renderers/WForwardRenderer.h>

class AnimationDemo : public WTestState {
	WObject* character;

public:
	AnimationDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WRenderer* CreateRenderer() { return new WForwardRenderer(m_app); }
};
