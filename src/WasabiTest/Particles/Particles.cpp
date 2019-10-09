#include "Particles/Particles.hpp"

ParticlesDemo::ParticlesDemo(Wasabi* const app) : WTestState(app) {
	m_goingLeft = true;
}

void ParticlesDemo::Load() {
	m_particles.push_back(m_app->ParticlesManager->CreateParticles(W_DEFAULT_PARTICLES_ALPHA));
	assert(m_particles[0] != nullptr && m_particles[0]->Valid());
	m_particles[0]->SetName("Fire Particles");
	((WDefaultParticleBehavior*)m_particles[0]->GetBehavior())->m_numTiles = 4;
	((WDefaultParticleBehavior*)m_particles[0]->GetBehavior())->m_numTilesColumns = 2;
	((WDefaultParticleBehavior*)m_particles[0]->GetBehavior())->m_colorGradient = {
		std::make_pair(WColor(0xf7e01c00), 0.3f),
		std::make_pair(WColor(0xf7691cff), 0.4f),
		std::make_pair(WColor(0x0f0f0f11), 0.3f),
		std::make_pair(WColor(0x0f0f0f00), 0.0f),
	};
	m_particles[0]->SetPriority(0);

	WImage* texture = m_app->ImageManager->CreateImage("Media/fire_particles_tiles.png");
	assert(texture != nullptr && texture->Valid());
	texture->SetName("Fire Texture");
	CheckError(m_particles[0]->GetMaterials().SetTexture("diffuseTexture", texture));

	m_particles.push_back(m_app->ParticlesManager->CreateParticles(W_DEFAULT_PARTICLES_ADDITIVE));
	assert(m_particles[1] != nullptr && m_particles[1]->Valid());
	m_particles[1]->SetName("Additive Fire Particles");
	((WDefaultParticleBehavior*)m_particles[1]->GetBehavior())->m_numTiles = 4;
	((WDefaultParticleBehavior*)m_particles[1]->GetBehavior())->m_numTilesColumns = 2;
	((WDefaultParticleBehavior*)m_particles[1]->GetBehavior())->m_colorGradient = {
		std::make_pair(WColor(0xf7e01c00), 0.1f),
		std::make_pair(WColor(0xf9925bff), 0.3f),
		std::make_pair(WColor(0x0f0f0f00), 0.4f),
		std::make_pair(WColor(0x0f0f0f00), 0.2f),
		std::make_pair(WColor(0x0f0f0f00), 0.0f),
	};
	((WDefaultParticleBehavior*)m_particles[1]->GetBehavior())->m_particleLife = ((WDefaultParticleBehavior*)m_particles[0]->GetBehavior())->m_particleLife / 1.5f;
	((WDefaultParticleBehavior*)m_particles[1]->GetBehavior())->m_emissionFrequency = 40.0f;
	m_particles[1]->SetPriority(1);

	CheckError(m_particles[1]->GetMaterials().SetTexture("diffuseTexture", texture));
	W_SAFE_REMOVEREF(texture);

	m_particles.push_back(m_app->ParticlesManager->CreateParticles(W_DEFAULT_PARTICLES_ADDITIVE));
	assert(m_particles[2] != nullptr && m_particles[2]->Valid());
	m_particles[2]->SetName("Fire Glow Particles");
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_colorGradient = {
		std::make_pair(WColor(0xf7e01c00), 0.2f),
		std::make_pair(WColor(0xf9925b66), 0.5f),
		std::make_pair(WColor(0x00000000), 0.3f),
		std::make_pair(WColor(0x00000000), 0.0f),
	};
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_emissionRandomness = WVector3(0.5f, 0.5f, 0.5f);
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_particleLife = ((WDefaultParticleBehavior*)m_particles[0]->GetBehavior())->m_particleLife / 1.5f;
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_emissionSize = ((WDefaultParticleBehavior*)m_particles[0]->GetBehavior())->m_emissionSize * 1.5f;
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_deathSize = ((WDefaultParticleBehavior*)m_particles[0]->GetBehavior())->m_deathSize * 1.5f;
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_emissionPosition = WVector3(0.0f, -0.5f, 0.0f);
	m_particles[2]->SetPriority(1);

	texture = m_app->ImageManager->CreateImage("Media/glow.png");
	assert(texture != nullptr && texture->Valid());
	texture->SetName("Glow Texture");
	CheckError(m_particles[2]->GetMaterials().SetTexture("diffuseTexture", texture));
	W_SAFE_REMOVEREF(texture);


	m_particles.push_back(m_app->ParticlesManager->CreateParticles(W_DEFAULT_PARTICLES_ADDITIVE));
	m_particles[3]->SetName("Fire Sparks Particles");
	((WDefaultParticleBehavior*)m_particles[3]->GetBehavior())->m_colorGradient = {
		std::make_pair(WColor(0xf7e01c00), 0.1f),
		std::make_pair(WColor(0xf9925b66), 0.7f),
		std::make_pair(WColor(0x00000000), 0.2f),
		std::make_pair(WColor(0x00000000), 0.0f),
	};
	((WDefaultParticleBehavior*)m_particles[3]->GetBehavior())->m_emissionFrequency = 140.0f;
	((WDefaultParticleBehavior*)m_particles[3]->GetBehavior())->m_particleLife = 0.5f;
	((WDefaultParticleBehavior*)m_particles[3]->GetBehavior())->m_particleSpawnVelocity = WVector3(2.0f, 0.0f, 0.0f);
	((WDefaultParticleBehavior*)m_particles[3]->GetBehavior())->m_emissionRandomness = WVector3(0.8f, 0.8f, 0.8f);
	((WDefaultParticleBehavior*)m_particles[3]->GetBehavior())->m_moveOutwards = true;
	((WDefaultParticleBehavior*)m_particles[3]->GetBehavior())->m_emissionSize = 0.1f;
	((WDefaultParticleBehavior*)m_particles[3]->GetBehavior())->m_deathSize = 0.05f;
	((WDefaultParticleBehavior*)m_particles[3]->GetBehavior())->m_emissionPosition = WVector3(0.0f, 0.5f, 0.0f);
	m_particles[3]->SetPriority(1);

	texture = m_app->ImageManager->CreateImage("Media/spark.png");
	assert(texture != nullptr && texture->Valid());
	texture->SetName("Sparks Texture");
	CheckError(m_particles[3]->GetMaterials().SetTexture("diffuseTexture", texture));
	W_SAFE_REMOVEREF(texture);

	((WasabiTester*)m_app)->SetCameraPosition(WVector3(0, 3, 0));
}

void ParticlesDemo::Update(float fDeltaTime) {
	if (m_particles.size() > 0 && m_app->WindowAndInputComponent->KeyDown(' ')) {
		WDefaultParticleBehavior* behavior = (WDefaultParticleBehavior*)m_particles[0]->GetBehavior();
		if (m_goingLeft) {
			if (behavior->m_emissionPosition.x > -5.0f)
				behavior->m_emissionPosition.x -= 2.5f * fDeltaTime;
			else
				m_goingLeft = false;
		} else {
			if (behavior->m_emissionPosition.x < 5.0f)
				behavior->m_emissionPosition.x += 2.5f * fDeltaTime;
			else
				m_goingLeft = true;
		}
		for (uint32_t i = 1; i < m_particles.size(); i++)
			((WDefaultParticleBehavior*)m_particles[i]->GetBehavior())->m_emissionPosition.x = behavior->m_emissionPosition.x;
	}
	if (m_particles.size() > 0 && m_app->WindowAndInputComponent->KeyDown('R')) {
		for (uint32_t i = 0; i < m_particles.size(); i++)
			m_particles[i]->Roll(10.0f * fDeltaTime);
	}
}

void ParticlesDemo::Cleanup() {
	for (auto it = m_particles.begin(); it != m_particles.end(); it++)
		W_SAFE_REMOVEREF(*it);
}
