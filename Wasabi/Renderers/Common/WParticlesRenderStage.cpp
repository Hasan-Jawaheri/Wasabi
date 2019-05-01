#include "WParticlesRenderStage.h"
#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Images/WRenderTarget.h"
#include "../../Particles/WParticles.h"
#include "../../Materials/WEffect.h"
#include "../../Materials/WMaterial.h"

WParticlesRenderStage::ParticlesKey::ParticlesKey(WParticles* par) {
	particles = par;
	priority = par->GetPriority();
	fx = par->GetDefaultEffect();
}

const bool WParticlesRenderStage::ParticlesKey::operator< (const ParticlesKey& that) const {
	if (priority != that.priority)
		return priority < that.priority;

	return (void*)fx < (void*)that.fx ? true : fx == that.fx ? (particles < that.particles) : false;
}

WParticlesRenderStage::WParticlesRenderStage(Wasabi* const app, bool backbuffer) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = backbuffer ? RENDER_STAGE_TARGET_BACK_BUFFER : RENDER_STAGE_TARGET_PREVIOUS;
	m_stageDescription.flags = RENDER_STAGE_FLAG_PARTICLES_RENDER_STAGE;
}

WError WParticlesRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	int i = 0;
	vector<W_DEFAULT_PARTICLE_EFFECT_TYPE> types({ W_DEFAULT_PARTICLES_ALPHA, W_DEFAULT_PARTICLES_ADDITIVE, W_DEFAULT_PARTICLES_SUBTRACTIVE });
	for (auto it = types.begin(); it != types.end(); it++) {
		WEffect* fx = m_app->ParticlesManager->CreateParticlesEffect(*it);
		if (!fx)
			return WError(W_OUTOFMEMORY);
		fx->SetName("DefaultParticlesFX" + std::to_string(i++));
		m_app->FileManager->AddDefaultAsset(fx->GetName(), fx);

		m_particleEffects.insert(std::make_pair(*it, fx));
	}

	for (unsigned int i = 0; i < m_app->ParticlesManager->GetEntitiesCount(); i++)
		OnParticlesChange(m_app->ParticlesManager->GetEntityByIndex(i), true);
	m_app->ParticlesManager->RegisterChangeCallback(m_stageDescription.name, [this](WParticles* p, bool a) { this->OnParticlesChange(p, a); });

	return WError(W_SUCCEEDED);
}

void WParticlesRenderStage::OnParticlesChange(WParticles* p, bool added) {
	if (added) {
		auto it = this->m_particleEffects.find(p->GetType());
		if (it != this->m_particleEffects.end()) {
			p->AddEffect(it->second);
			p->GetMaterial(it->second)->SetName("ParticlesDefaultMaterial" + std::to_string(this->m_currentMatId));
		}
		ParticlesKey key(p);
		this->m_allParticles.insert(std::make_pair(key, p));
	} else {
		auto iter = this->m_allParticles.find(ParticlesKey(p));
		if (iter != this->m_allParticles.end())
			this->m_allParticles.erase(iter);
		else {
			// the sprite seems to have changed and then removed before we cloud reindex it in the render loop
			// need to find it manually now...
			for (auto it = this->m_allParticles.begin(); it != this->m_allParticles.end(); it++) {
				if (it->second == p) {
					this->m_allParticles.erase(it);
					break;
				}
			}
		}
	}
}

void WParticlesRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	for (auto it = m_particleEffects.begin(); it != m_particleEffects.end(); it++)
		W_SAFE_REMOVEREF(it->second);
	m_app->ParticlesManager->RemoveChangeCallback(m_stageDescription.name);
}

WError WParticlesRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_PARTICLES) {
		WEffect* boundFX = nullptr;
		std::vector<ParticlesKey> reindexParticles;
		for (auto it = m_allParticles.begin(); it != m_allParticles.end(); it++) {
			WParticles* particles = it->second;
			WEffect* effect = particles->GetDefaultEffect();
			WMaterial* material = particles->GetMaterial(effect);
			if (material && particles->WillRender(rt)) {
				uint priority = particles->GetPriority();
				if (boundFX != effect) {
					effect->Bind(rt);
					boundFX = effect;
				}
				if (priority != it->first.priority || effect != it->first.fx)
					reindexParticles.push_back(it->first);

				particles->Render(rt, material);
			}
		}
		for (auto it = reindexParticles.begin(); it != reindexParticles.end(); it++) {
			m_allParticles.erase(*it);
			m_allParticles.insert(std::make_pair(ParticlesKey(it->particles), it->particles));
		}
	}

	return WError(W_SUCCEEDED);
}
