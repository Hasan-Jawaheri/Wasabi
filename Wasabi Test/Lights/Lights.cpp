#pragma once

#include "Lights.hpp"

LightsDemo::LightsDemo(Wasabi* const app) : WGameState(app) {
}

void LightsDemo::Load() {
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

	// remove default light
	m_app->LightManager->GetDefaultLight()->Hide();

	int maxLights = (int)m_app->engineParams.find("maxLights")->second;

	for (int i = 0; i < maxLights / 2; i++) {
		float x = 20.0f * (float)(rand() % 10000) / 10000.0f - 20.0f;
		float z = 30.0f * (float)(rand() % 10000) / 10000.0f - 15.0f;

		WLight* l = new WLight(m_app);
		l->SetType(W_LIGHT_POINT);
		l->SetRange(5.0f);
		l->SetPosition(x, 3.0, z);
		m_lights.push_back(l);
	}

	for (int i = 0; i < maxLights / 2; i++) {
		float x = 20.0f * (float)(rand() % 10000) / 10000.0f + 20.0f;
		float z = (10.0f + 30.0f * (float)(rand() % 10000) / 10000.0f) * (rand() % 2 == 0 ? 1 : -1);

		WLight* l = new WLight(m_app);
		l->SetType(W_LIGHT_SPOT);
		l->SetIntensity(5.0f);
		l->SetRange(30.0f);
		l->SetPosition(x, 4.0, z);
		l->Point(WVector3(x, 0.0, z) * 0.5f);
		m_lights.push_back(l);
	}
}

void LightsDemo::Update(float fDeltaTime) {
}

void LightsDemo::Cleanup() {
	m_plain->RemoveReference();

	for (auto it = m_boxes.begin(); it != m_boxes.end(); it++)
		(*it)->RemoveReference();

	for (auto it = m_lights.begin(); it != m_lights.end(); it++)
		(*it)->RemoveReference();
}