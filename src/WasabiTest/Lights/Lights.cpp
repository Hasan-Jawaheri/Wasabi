#include "Lights/Lights.hpp"
#include <Wasabi/Renderers/ForwardRenderer/WForwardRenderer.h>
#include <Wasabi/Renderers/DeferredRenderer/WDeferredRenderer.h>


LightsDemo::LightsDemo(Wasabi* const app) : WTestState(app) {
	m_isDeferred = true;
	m_plain = nullptr;
}

WError LightsDemo::SetupRenderer() {
	if (m_isDeferred)
		return WInitializeDeferredRenderer(m_app);
	return WInitializeForwardRenderer(m_app);
}

void LightsDemo::Load() {
	srand(3);

	// Create the plain
	m_plain = m_app->ObjectManager->CreateObject();
	WGeometry* plainGeometry = new WGeometry(m_app);
	plainGeometry->CreatePlain(50.0f, 0, 0);
	m_plain->SetGeometry(plainGeometry);
	plainGeometry->RemoveReference();
	m_plain->GetMaterial()->SetVariableColor("color", WColor(0.4, 0.4, 0.4));
	m_plain->GetMaterial()->SetVariableInt("isTextured", 0);

	// Create the boxes
	WGeometry* boxGeometry = new WGeometry(m_app);
	boxGeometry->CreateCube(2.0f);
	for (int i = 0; i < 40; i++) {
		float x = 20.0f * (float)(rand() % 10000) / 10000.0f - 10.0f;
		float y =  2.0f * (float)(rand() % 10000) / 10000.0f -  0.0f;
		float z = 20.0f * (float)(rand() % 10000) / 10000.0f - 10.0f;
		WObject* box = m_app->ObjectManager->CreateObject();
		box->SetGeometry(boxGeometry);
		box->SetPosition(x, y, z);
		m_boxes.push_back(box);
		box->GetMaterial()->SetVariableColor("color", WColor(0.7, 0.7, 0.7));
		box->GetMaterial()->SetVariableInt("isTextured", 0);
	}
	boxGeometry->RemoveReference();

	// hide default light
	m_app->LightManager->GetDefaultLight()->Hide();

	int maxLights = std::min(m_app->GetEngineParam<int>("maxLights", INT_MAX), 8);
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
		float x = 20.0f * (float)(rand() % 10000) / 10000.0f - 10.0f;
		float z = 20.0f * (float)(rand() % 10000) / 10000.0f - 10.0f;

		WLight* l = new WPointLight(m_app);
		l->SetRange(5.0f);
		l->SetPosition(x, 3.0, z);
		l->SetColor(colors[rand() % (sizeof(colors)/sizeof(WColor))]);
		m_lights.push_back(l);
	}

	for (int i = 0; i < maxLights / 2; i++) {
		float x = 20.0f * (float)(rand() % 10000) / 10000.0f + 10.0f;
		float z = (10.0f + 15.0f * (float)(rand() % 10000) / 10000.0f) * (rand() % 2 == 0 ? 1 : -1);

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
	if (m_app->WindowAndInputComponent->KeyDown('1') && !m_isDeferred) {
		m_isDeferred = true;
		SetupRenderer();
	} else if (m_app->WindowAndInputComponent->KeyDown('2') && m_isDeferred) {
		m_isDeferred = false;
		SetupRenderer();
	}

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