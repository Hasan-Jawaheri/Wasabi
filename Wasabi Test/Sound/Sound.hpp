#pragma once

#include "../TestSuite.hpp"

class SoundDemo : public WTestState {
	WSound* m_sound;

public:
	SoundDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
};