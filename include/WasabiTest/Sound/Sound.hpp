#pragma once

#include "TestSuite.hpp"
#include <Wasabi/Sounds/OpenAL/WOpenAL.h>

class SoundDemo : public WTestState {
	WSound* m_sound;

public:
	SoundDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);

	virtual WSoundComponent* CreateSoundComponent() {
		WOpenALSoundComponent* oal = new WOpenALSoundComponent(m_app);
		oal->Initialize();
		return oal;
	}
};