#pragma once

#include "TestSuite.hpp"

class LightsDemo : public WGameState {
	WObject* m_plain;
	vector<WObject*> m_boxes;

public:
	LightsDemo(Wasabi* const app) : WGameState(app) {}

	virtual void Load() {
		// Create the plain
		m_plain = new WObject(m_app);
		WGeometry* plainGeometry = new WGeometry(m_app);
		plainGeometry->CreatePlain(50.0f, 0, 0);
		m_plain->SetGeometry(plainGeometry);
		plainGeometry->RemoveReference();

		// Create the boxes
		WGeometry* boxGeometry = new WGeometry(m_app);
		boxGeometry->CreateCube(2.0f);
		for (int i = 0; i < 40; i++) {
			float x = 30.0f * (float)(rand() % 10000) / 10000.0f - 15.0f;
			float z = 30.0f * (float)(rand() % 10000) / 10000.0f - 15.0f;
			WObject* box = new WObject(m_app);
			box->SetGeometry(boxGeometry);
			box->SetPosition(x, 1.0f, z);
			m_boxes.push_back(box);
		}
		boxGeometry->RemoveReference();

		m_app->LightManager->GetDefaultLight()->Hide();
	}

	virtual void Update(float fDeltaTime) {
	}

	virtual void Cleanup() {
		m_plain->RemoveReference();
		for (auto it = m_boxes.begin(); it != m_boxes.end(); it++)
			(*it)->RemoveReference();
	}
};