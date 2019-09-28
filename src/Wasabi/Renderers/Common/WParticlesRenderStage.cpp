#include "Wasabi/Renderers/Common/WParticlesRenderStage.h"
#include "Wasabi/Core/WCore.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Images/WRenderTarget.h"
#include "Wasabi/Particles/WParticles.h"
#include "Wasabi/Materials/WEffect.h"
#include "Wasabi/Materials/WMaterial.h"

WParticlesRenderStage::WParticlesRenderStage(Wasabi* const app, bool backbuffer) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = backbuffer ? RENDER_STAGE_TARGET_BACK_BUFFER : RENDER_STAGE_TARGET_PREVIOUS;
	m_stageDescription.flags = RENDER_STAGE_FLAG_PARTICLES_RENDER_STAGE;

	m_particlesFragment = nullptr;;
}

WError WParticlesRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	int i = 0;
	unordered_map<W_DEFAULT_PARTICLE_EFFECT_TYPE, class WEffect*> particleEffects;
	vector<W_DEFAULT_PARTICLE_EFFECT_TYPE> types({ W_DEFAULT_PARTICLES_ALPHA, W_DEFAULT_PARTICLES_ADDITIVE, W_DEFAULT_PARTICLES_SUBTRACTIVE });
	for (auto it = types.begin(); it != types.end(); it++) {
		WEffect* fx = m_app->ParticlesManager->CreateParticlesEffect(*it);
		if (!fx)
			return WError(W_OUTOFMEMORY);
		fx->SetName("DefaultParticlesFX" + std::to_string(i++));
		m_app->FileManager->AddDefaultAsset(fx->GetName(), fx);

		particleEffects.insert(std::make_pair(*it, fx));
	}

	m_particlesFragment = new WParticlesRenderFragment(m_stageDescription.name, particleEffects, m_app);

	return WError(W_SUCCEEDED);
}

void WParticlesRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_DELETE(m_particlesFragment);
}

WError WParticlesRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint32_t filter) {
	if (filter & RENDER_FILTER_PARTICLES) {
		m_particlesFragment->Render(renderer, rt);
	}

	return WError(W_SUCCEEDED);
}
