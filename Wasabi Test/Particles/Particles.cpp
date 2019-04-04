#pragma once

#include "Particles.hpp"

ParticlesDemo::ParticlesDemo(Wasabi* const app) : WTestState(app) {
}

void ParticlesDemo::Load() {
	m_particles.push_back(m_app->ParticlesManager->CreateParticles(W_DEFAULT_PARTICLES_ALPHA));
	m_particles[0]->SetName("Fire Particles");

	WImage* texture = m_app->ImageManager->CreateImage("Media/particle2.png");
	texture->SetName("Fire Texture");
	m_particles[0]->GetMaterial()->SetTexture("diffuseTexture", texture);
	W_SAFE_REMOVEREF(texture);

	m_particles.push_back(m_app->ParticlesManager->CreateParticles(W_DEFAULT_PARTICLES_SUBTRACTIVE));
	m_particles[1]->SetName("Smoke Particles");
	((WDefaultParticleBehavior*)m_particles[1]->GetBehavior())->m_particleLife = 3;
	((WDefaultParticleBehavior*)m_particles[1]->GetBehavior())->m_emissionPosition = WVector3(0, 1, 0);
	((WDefaultParticleBehavior*)m_particles[1]->GetBehavior())->m_emissionFrequency = 5;

	texture = m_app->ImageManager->CreateImage("Media/particle3.png");
	texture->SetName("Smoke Texture");
	m_particles[1]->GetMaterial()->SetTexture("diffuseTexture", texture);
	W_SAFE_REMOVEREF(texture);

	m_particles.push_back(m_app->ParticlesManager->CreateParticles(W_DEFAULT_PARTICLES_ALPHA));
	m_particles[2]->SetName("Light Smoke Particles");
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_particleLife = 3;
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_emissionPosition = WVector3(0, 3.5, 0);
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_emissionFrequency = 4;
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_emissionSize = 1;
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_particleSpawnVelocity = WVector3(0, 1.5, 0);
	((WDefaultParticleBehavior*)m_particles[2]->GetBehavior())->m_emissionRandomness = WVector3(2, 1, 2);

	texture = m_app->ImageManager->CreateImage("Media/particle3.png");
	texture->SetName("Smoke Texture");
	m_particles[2]->GetMaterial()->SetTexture("diffuseTexture", texture);
	W_SAFE_REMOVEREF(texture);
}

void ParticlesDemo::Update(float fDeltaTime) {
}

void ParticlesDemo::Cleanup() {
	for (auto it = m_particles.begin(); it != m_particles.end(); it++)
		W_SAFE_REMOVEREF(*it);
}
