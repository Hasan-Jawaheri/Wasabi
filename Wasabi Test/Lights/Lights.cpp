#pragma once

#include "Lights.hpp"

LightsDemo::LightsDemo(Wasabi* const app) : WTestState(app) {
	m_plain = nullptr;
}

void LightsDemo::Load() {
	srand(2);

	// Create the plain
	m_plain = m_app->ObjectManager->CreateObject();
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
		WObject* box = m_app->ObjectManager->CreateObject();
		box->SetGeometry(boxGeometry);
		box->SetPosition(x, 1.0f, z);
		m_boxes.push_back(box);
	}
	boxGeometry->RemoveReference();

	// hide default light
	// m_app->LightManager->GetDefaultLight()->Hide();

	int maxLights = min(m_app->engineParams.find("maxLights") != m_app->engineParams.end() ? (int)m_app->engineParams["maxLights"] : INT_MAX, 8);
	WColor colors[] = {
		WColor(1, 0, 0),
		WColor(0, 1, 0),
		WColor(0, 0, 1),
		WColor(1, 1, 0),
		WColor(0, 1, 1),
		WColor(1, 0, 1),
		WColor(1, 1, 1),
	};

	for (int i = 0; i < maxLights / 2; i++) {
		float x = 20.0f * (float)(rand() % 10000) / 10000.0f - 20.0f;
		float z = 30.0f * (float)(rand() % 10000) / 10000.0f - 15.0f;

		WLight* l = new WPointLight(m_app);
		l->SetRange(5.0f);
		l->SetPosition(x, 3.0, z);
		l->SetColor(colors[rand() % (sizeof(colors)/sizeof(WColor))]);
		m_lights.push_back(l);
	}

	for (int i = 0; i < maxLights / 2; i++) {
		float x = 20.0f * (float)(rand() % 10000) / 10000.0f + 20.0f;
		float z = (10.0f + 30.0f * (float)(rand() % 10000) / 10000.0f) * (rand() % 2 == 0 ? 1 : -1);

		WLight* l = new WSpotLight(m_app);
		l->SetIntensity(5.0f);
		l->SetRange(30.0f);
		l->SetPosition(x, 4.0, z);
		l->Point(WVector3(x, 0.0, z) * 0.5f);
		l->SetColor(colors[rand() % (sizeof(colors) / sizeof(WColor))]);
		m_lights.push_back(l);
	}
}

void LightsDemo::Update(float fDeltaTime) {
	for (auto it = m_boxes.begin(); it != m_boxes.end(); it++) {
		WObject* box = *it;
		//box->Yaw(10.0f * fDeltaTime);
	}
}

void LightsDemo::Cleanup() {
	W_SAFE_REMOVEREF(m_plain);

	for (auto it = m_boxes.begin(); it != m_boxes.end(); it++)
		(*it)->RemoveReference();

	for (auto it = m_lights.begin(); it != m_lights.end(); it++)
		(*it)->RemoveReference();
}