#pragma once

#include "TestSuite.hpp"

class SoundDemo : public WGameState {
	WSound* m_sound;

public:
	SoundDemo(Wasabi* const app) : WGameState(app) {}

	virtual void Load() {
		m_sound = new WSound(m_app);
		m_sound->LoadWAV("Media/test.wav", 0);
		m_sound->Play();
	}

	virtual void Update(float fDeltaTime) {
		if (m_sound->Playing()) {
			m_app->TextComponent->RenderText("Playing...", 5, 5, 16);
		} else {
			m_app->TextComponent->RenderText("Done!", 5, 5, 16);
		}
	}
};