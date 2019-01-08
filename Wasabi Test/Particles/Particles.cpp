#pragma once

#include "Particles.hpp"

ParticlesDemo::ParticlesDemo(Wasabi* const app) : WTestState(app) {
	m_particles = nullptr;
}

void ParticlesDemo::Load() {
	m_particles = new WParticles(m_app);
	m_particles->SetName("MainParticles");
	m_particles->Create();
}

void ParticlesDemo::Update(float fDeltaTime) {
	char title[128];
	sprintf_s(title, 128, "Elapsed time: %.2f\nFPS: %.2f", m_app->Timer.GetElapsedTime() / 1000.0f, m_app->FPS);
	int width = m_app->TextComponent->GetTextWidth("Elapsed time: 0.00", 32, 1);
	m_app->TextComponent->RenderText(title, 5, 5, 32, 1);
}

void ParticlesDemo::Cleanup() {
	W_SAFE_REMOVEREF(m_particles);
}
