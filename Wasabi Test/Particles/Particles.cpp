#pragma once

#include "Particles.hpp"

ParticlesDemo::ParticlesDemo(Wasabi* const app) : WTestState(app) {
}

void ParticlesDemo::Load() {
	m_particles.push_back(new WParticles(m_app));
	m_particles[0]->SetName("Fire Particles");
	m_particles[0]->Create(m_app->ParticlesManager->GetDefaultEffect(W_DEFAULT_PARTICLES_ALPHA));

	WImage* texture = new WImage(m_app);
	texture->SetName("Fire Texture");
	texture->Load("Media/particle2.png");
	m_particles[0]->SetTexture(texture);
	W_SAFE_REMOVEREF(texture);

	m_particles.push_back(new WParticles(m_app));
	m_particles[1]->SetName("Smoke Particles");
	m_particles[1]->Create(m_app->ParticlesManager->GetDefaultEffect(W_DEFAULT_PARTICLES_SUBTRACTIVE));
	((WDefaultParticleBehavior*)m_particles[1]->GetBehavior())->m_particleLife = 3;
	((WDefaultParticleBehavior*)m_particles[1]->GetBehavior())->m_emissionPosition = WVector3(0, 1, 0);
	((WDefaultParticleBehavior*)m_particles[1]->GetBehavior())->m_emissionFrequency = 5;

	texture = new WImage(m_app);
	texture->SetName("Smoke Texture");
	texture->Load("Media/particle3.png");
	m_particles[1]->SetTexture(texture);
	W_SAFE_REMOVEREF(texture);

	m_particles.push_back(new WParticles(m_app));
	m_particles[2]->SetName("Light Smoke Particles");
	m_particles[2]->Create(m_app->ParticlesManager->GetDefaultEffect(W_DEFAULT_PARTICLES_ALPHA));
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_particleLife = 3;
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_emissionPosition = WVector3(0, 3.5, 0);
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_emissionFrequency = 4;
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_emissionSize = 1;
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_particleSpawnVelocity = WVector3(0, 1.5, 0);
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_emissionRandomness = WVector3(2, 1, 2);

	texture = new WImage(m_app);
	texture->SetName("Smoke Texture");
	texture->Load("Media/particle3.png");
	m_particles[2]->SetTexture(texture);
	W_SAFE_REMOVEREF(texture);
}

void ParticlesDemo::Update(float fDeltaTime) {
}

void ParticlesDemo::Cleanup() {
	for (auto it = m_particles.begin(); it != m_particles.end(); it++)
		W_SAFE_REMOVEREF(*it);
}
