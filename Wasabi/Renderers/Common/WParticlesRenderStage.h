#pragma once

#include "../WRenderStage.h"
#include "../../Particles/WParticles.h"

class WParticlesRenderStage : public WRenderStage {
	/** Default effect used by different particle systems */
	unordered_map<W_DEFAULT_PARTICLE_EFFECT_TYPE, class WEffect*> m_particleEffects;

	struct ParticlesKey {
		uint priority;
		class WEffect* fx;
		class WParticles* particles;

		ParticlesKey(class WParticles* particles);
		const bool operator< (const ParticlesKey& that) const;
	};
	/** Sorted container for all the particles to be rendered by this stage */
	std::map<ParticlesKey, class WParticles*> m_allParticles;

	void OnParticlesChange(class WParticles* p, bool added);

public:
	WParticlesRenderStage(class Wasabi* const app, bool backbuffer = false);
	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
	virtual void Cleanup();
};

