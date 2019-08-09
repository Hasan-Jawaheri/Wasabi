#pragma once

#include "TestSuite.hpp"

class LightsDemo : public WTestState {
	WObject* m_plain;
	vector<WObject*> m_boxes;
	vector<WLight*> m_lights;

	bool m_wantDeferred, m_isDeferred;

public:
	LightsDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WError SetupRenderer();
};
