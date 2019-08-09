#pragma once

#include "TestSuite.hpp"
#include <Renderers/ForwardRenderer/WForwardRenderer.h>

class ParticlesDemo : public WTestState {
	vector<WParticles*> m_particles;
	bool m_goingLeft;

public:
	ParticlesDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WError SetupRenderer() { return WInitializeForwardRenderer(m_app); }
};
