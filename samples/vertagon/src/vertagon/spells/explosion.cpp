#include "vertagon/spells/explosion.hpp"


Spell_Explosion::Spell_Explosion(Vertagon* app, WVector3 position) : Spell(app) {
    m_name = __func__;

    m_position = position;
    m_particles = nullptr;
    m_light = nullptr;

    m_params.lifetime = 2.0f;
    m_params.peakLightRange = 50.0f;
    m_params.peakIntensity = 12.0f;
}

WError Spell_Explosion::Load() {
    Cleanup();

	m_particles = m_app->ParticlesManager->CreateParticles(W_DEFAULT_PARTICLES_ALPHA);
    if (!m_particles) return WError(W_ERRORUNK);

	((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_colorGradient = {
		std::make_pair(WColor(0xf7e01c00), 0.3f),
		std::make_pair(WColor(0xf7691cff), 0.4f),
		std::make_pair(WColor(0x0f0f0f11), 0.3f),
		std::make_pair(WColor(0xffffff00), 0.0f),
	};
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_particleSpawnVelocity = WVector3(0.0f, 0.4f, 0.0f);
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionSize = 9.4f;
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_deathSize = 0.0f;
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionFrequency = 30.0f;
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionRandomness = WVector3(8.0f, 8.0f, 8.0f);
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionPosition = m_position;

	WImage* texture = m_app->ImageManager->CreateImage("../../media/spark.png");
    if (!texture) return WError(W_ERRORUNK);

	WError status = m_particles->GetMaterials().SetTexture("diffuseTexture", texture);
    texture->RemoveReference();

    m_light = new WPointLight(m_app);
    m_light->SetColor(WColor(1.0f, 0.64f, 0.0f));
    m_light->SetRange(0.1f);
    m_light->SetIntensity(0.0f);
    m_light->SetPosition(m_position);

    m_startTime = m_app->Timer.GetElapsedTime();

    return status;
}

void Spell_Explosion::Update(float fDeltaTime) {
    float percentage = std::min(1.0f, (m_app->Timer.GetElapsedTime() - m_startTime) / m_params.lifetime + 0.05f);
    float sinPercentage = std::sin(percentage * W_PI);
    float expeditedPower = std::sqrt(percentage);
    m_light->SetRange((0.1f + expeditedPower) * m_params.peakLightRange);
    m_light->SetIntensity(sinPercentage * m_params.peakIntensity);
    if (percentage > 0.5f) {
        ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionFrequency = std::max(0.0f, 30.0f - (60.0f * (percentage)));
    }
}

bool Spell_Explosion::IsAlive() {
    return m_particles != nullptr && (m_startTime + m_params.lifetime > m_app->Timer.GetElapsedTime());
}

void Spell_Explosion::Cleanup() {
    W_SAFE_REMOVEREF(m_particles);
    W_SAFE_REMOVEREF(m_light);
}
