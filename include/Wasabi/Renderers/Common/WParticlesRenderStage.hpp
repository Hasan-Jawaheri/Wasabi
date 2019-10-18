#pragma once

#include "Wasabi/Renderers/WRenderStage.hpp"
#include "Wasabi/Renderers/Common/WRenderFragment.hpp"
#include "Wasabi/Particles/WParticles.hpp"

class WParticlesRenderStage : public WRenderStage {
	WParticlesRenderFragment* m_particlesFragment;

public:
	WParticlesRenderStage(class Wasabi* const app, bool backbuffer = false);
	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint32_t filter);
	virtual void Cleanup();
};

