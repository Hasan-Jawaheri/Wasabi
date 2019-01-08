#pragma once

#include "../TestSuite.hpp"
#include <Renderers/WForwardRenderer.h>

class ParticlesDemo : public WTestState {
	WParticles* m_particles;

public:
	ParticlesDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WRenderer* CreateRenderer() { return new WForwardRenderer(m_app); }
};
