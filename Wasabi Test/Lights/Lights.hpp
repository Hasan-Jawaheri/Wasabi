#pragma once

#include "../TestSuite.hpp"

class LightsDemo : public WGameState {
	WObject* m_plain;
	vector<WObject*> m_boxes;
	vector<WLight*> m_lights;

public:
	LightsDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();
};
