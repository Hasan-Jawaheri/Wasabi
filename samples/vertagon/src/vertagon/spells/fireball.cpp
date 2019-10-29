#include "vertagon/spells/fireball.hpp"
#include "vertagon/spells/explosion.hpp"


Spell_Fireball::Spell_Fireball(Vertagon* app) : Spell(app) {
    m_name = __func__;

    m_particles = nullptr;
    m_light = nullptr;
    m_shotTime = -1.0f;
    m_exploded = false;

    m_params.lifetime = 3.0f;
    m_params.growthPeriod = 0.5f;
    m_params.emissionPeriod = 2.0f;
    m_params.idleTrailSpeed = 0.4f;
    m_params.speed = 100.0f;
}

WError Spell_Fireball::Load() {
    Cleanup();

	m_particles = m_app->ParticlesManager->CreateParticles(W_DEFAULT_PARTICLES_ALPHA);
    if (!m_particles) return WError(W_ERRORUNK);

	((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_numTiles = 4;
	((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_numTilesColumns = 2;
	((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_colorGradient = {
		std::make_pair(WColor(0xf7e01c00), 0.3f),
		std::make_pair(WColor(0xf7691cff), 0.4f),
		std::make_pair(WColor(0x0f0f0f11), 0.3f),
		std::make_pair(WColor(0x0f0f0f00), 0.0f),
	};
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_particleSpawnVelocity = WVector3(0.0f, 0.4f, 0.0f);
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionSize = 0.2f;
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_deathSize = 0.5f;
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionFrequency = 30.0f;
    ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionRandomness = WVector3(0.2f, 0.2f, 0.2f);

	WImage* texture = m_app->ImageManager->CreateImage("../../media/fire_particles_tiles.png");
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
    WVector3 E = ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionPosition; // emission position
    if (m_shotTime > 0.0f) {
        float elapsedTime = m_app->Timer.GetElapsedTime() - m_shotTime;

        if (!m_exploded) {
            /**
             * Follow the cursor and move forward
             */
            WVector3 O = m_targetOrigin;
            WVector3 EtoO = O - E;
            WVector3 D = m_targetDirection;
            float targetDistance = abs(WVec3Dot(WVec3Normalize(EtoO), D)) * WVec3Length(EtoO);
            WVector3 targetPoint = O + D * (targetDistance + m_params.speed * 2.0f);
            WVector3 direction = WVec3Normalize(targetPoint - E);
            E = ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionPosition + direction * m_params.speed * fDeltaTime;
            ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionPosition = E;

            /**
             * Check for collision and explode
             */
            if (elapsedTime < m_params.emissionPeriod && m_app->PhysicsComponent->RayCast(E, E + direction * 1.0f)) {
                // explosion!
                ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionFrequency = 0.0f; // stop emission
                m_app->m_spellSystem->CastSpell(std::make_shared<Spell_Explosion>(m_app, E));
                m_exploded = true;
            }
        }

        /**
         * Animate the light (and stop emitting when nearly done)
         */
        if (elapsedTime < m_params.growthPeriod) {
            float percentage = elapsedTime / m_params.growthPeriod;
            m_light->SetIntensity(0.5f + 2.0f * percentage);
            m_light->SetRange(10.0f + 20.0f * percentage);
        } else {
            float percentage = (elapsedTime - m_params.growthPeriod) / (m_params.lifetime - m_params.growthPeriod);
            m_light->SetIntensity(2.5f - percentage * 2.5f);
        }
        if (elapsedTime > m_params.emissionPeriod)
            ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionFrequency = 0.0f;
    }

    // match the particles position
    m_light->Show();
    m_light->SetPosition(E);
}

bool Spell_Fireball::IsAlive() {
    return m_particles != nullptr && (m_shotTime < 0.0f || m_shotTime + m_params.lifetime > m_app->Timer.GetElapsedTime());
}

void Spell_Fireball::Cleanup() {
    W_SAFE_REMOVEREF(m_particles);
    W_SAFE_REMOVEREF(m_light);
}

void Spell_Fireball::SetIdlePosition(WVector3 position, WVector3 up) {
    if (IsAlive()) {
        ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_emissionPosition = position;
        ((WDefaultParticleBehavior*)m_particles->GetBehavior())->m_particleSpawnVelocity = up * m_params.idleTrailSpeed;
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
