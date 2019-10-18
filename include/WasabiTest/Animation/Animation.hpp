#pragma once

#include "TestSuite.hpp"
#include <Wasabi/Renderers/ForwardRenderer/WForwardRenderer.hpp>

class AnimationDemo : public WTestState {
	WObject* character;

public:
	AnimationDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WError SetupRenderer() { return WInitializeForwardRenderer(m_app); }
};
