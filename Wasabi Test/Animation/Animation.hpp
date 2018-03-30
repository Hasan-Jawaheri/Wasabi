#pragma once

#include "../TestSuite.hpp"

class AnimationDemo : public WGameState {
	WObject* character;

public:
	AnimationDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();
};