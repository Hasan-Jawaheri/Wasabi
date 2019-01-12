#pragma once

#include "Particles.hpp"

ParticlesDemo::ParticlesDemo(Wasabi* const app) : WTestState(app) {
	m_particles = nullptr;
}

void ParticlesDemo::Load() {
	m_particles = new WParticles(m_app);
	m_particles->SetName("MainParticles");
	m_particles->Create();

	WImage* texture = new WImage(m_app);
	texture->SetName("Particle Texture");
	texture->Load("Media/particle1.png");
	m_particles->SetTexture(texture);
	W_SAFE_REMOVEREF(texture);
}

void ParticlesDemo::Update(float fDeltaTime) {
}

void ParticlesDemo::Cleanup() {
	W_SAFE_REMOVEREF(m_particles);
}
