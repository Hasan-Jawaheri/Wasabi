#pragma once

#include "Sound.hpp"

SoundDemo::SoundDemo(Wasabi* const app) : WTestState(app) {}

void SoundDemo::Load() {
	m_sound = new WSound(m_app);
	m_sound->LoadWAV("Media/test.wav", 0);
	m_sound->Play();
}

void SoundDemo::Update(float fDeltaTime) {
	if (m_sound->Playing()) {
		m_app->TextComponent->RenderText("Playing...", 5, 5, 16);
	} else {
		m_app->TextComponent->RenderText("Done!", 5, 5, 16);
	}
}