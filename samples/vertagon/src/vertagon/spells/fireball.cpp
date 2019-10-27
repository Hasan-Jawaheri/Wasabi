#include "vertagon/spells/fireball.hpp"


Spell_Fireball::Spell_Fireball(Vertagon* app) : Spell(app) {
    m_name = __func__;

    m_particles = nullptr;
    m_light = nullptr;
    m_shotTime = -1.0f;
}

WError Spell_Fireball::Load() {
    Cleanup();

	m_particles = m_app->ParticlesManager->CreateParticles(W_DEFAULT_PARTICLES_ALPHA);
    if (!m_particles) return WError(W_ERRORUNK);

	// ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_numTiles = 4;
	// ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_numTilesColumns = 2;
	((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_colorGradient = {
		std::make_pair(WColor(0xf7e01c00), 0.3f),
		std::make_pair(WColor(0xf7691cff), 0.4f),
		std::make_pair(WColor(0x0f0f0f11), 0.3f),
		std::make_pair(WColor(0x0f0f0f00), 0.0f),
	};
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_particleSpawnVelocity = WVector3(0.0f, 0.2f, 0.0f);
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionSize = 0.4f;
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_deathSize = 0.5f;
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionFrequency = 30.0f;
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionRandomness = WVector3(0.2f, 0.2f, 0.2f);

	WImage* texture = m_app->ImageManager->CreateImage("../../media/spark.png");
    if (!texture) return WError(W_ERRORUNK);

	WError status = m_particles->GetMaterials().SetTexture("diffuseTexture", texture);
    texture->RemoveReference();

    m_light = new WPointLight(m_app);
    m_light->Hide();
    m_light->SetColor(WColor(1.0f, 0.64f, 0.0f));
    m_light->SetRange(10.0f);
    m_light->SetIntensity(0.5f);

    return status;
}

void Spell_Fireball::Update(float fDeltaTime) {
    WVector3 E = ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionPosition;
    WVector3 O = m_targetOrigin;
    WVector3 EtoO = O - E;
    WVector3 D = m_targetDirection;
    float targetDistance = abs(WVec3Dot(WVec3Normalize(EtoO), D)) * WVec3Length(EtoO);
    float speed = 100.0f;
    WVector3 targetPoint = O + D * (targetDistance + speed * 2.0f);
    WVector3 direction = WVec3Normalize(targetPoint - E);
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionPosition += direction * speed * fDeltaTime;

    m_light->Show();
    m_light->SetPosition(((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionPosition);

    if (m_shotTime > 0.0f) {
        float elapsedTime = m_app->Timer.GetElapsedTime() - m_shotTime;
        if (elapsedTime < 0.5f) {
            float percentage = elapsedTime * 2.0f;
            m_light->SetIntensity(0.5f + 2.0f * percentage);
            m_light->SetRange(10.0f + 20.0f * percentage);
        } else {
            float percentage = (elapsedTime-0.5f) / 2.5f;
            m_light->SetIntensity(2.5f - percentage * 2.5f);
            if (elapsedTime > 2.0f)
                ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionFrequency = 0.0f;
        }
    }
}

bool Spell_Fireball::IsAlive() {
    return m_particles != nullptr && (m_shotTime < 0.0f || m_shotTime + 3 > m_app->Timer.GetElapsedTime());
}

void Spell_Fireball::Cleanup() {
    W_SAFE_REMOVEREF(m_particles);
    W_SAFE_REMOVEREF(m_light);
}

void Spell_Fireball::SetIdlePosition(WVector3 position) {
    if (IsAlive()) {
        ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionPosition = position;
    }
}

void Spell_Fireball::SetDirection(WVector3 origin, WVector3 direction) {
    if (IsAlive()) {
        if (m_shotTime < 0.0f) {
            m_shotTime = m_app->Timer.GetElapsedTime();
            ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionRandomness = WVector3(1.0f, 1.0f, 1.0f);
            ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionFrequency = 60.0f;
            ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionSize = 0.5f;
            ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_deathSize = 1.5f;
        }
        m_targetOrigin = origin;
        m_targetDirection = direction;
    }
}
