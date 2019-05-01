#pragma once

#include "../TestSuite.hpp"
#include <Renderers/DeferredRenderer/WDeferredRenderer.h>

class ParticlesDemo : public WTestState {
	vector<WParticles*> m_particles;
	bool m_goingLeft;

public:
	ParticlesDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WError SetupRenderer() { return WInitializeDeferredRenderer(m_app); }
};
