#pragma once

#include <Wasabi.h>
#include <Renderers/WDeferredRenderer.h>

class WasabiTester : public Wasabi {
	class WTestState* m_state;

	float fYaw, fPitch, fDist;
	WVector3 vPos;

	void ApplyMousePivot();

public:
	WasabiTester();
	virtual WError Setup();
	virtual bool Loop(float fDeltaTime);
	virtual void Cleanup();

	void SetCameraPosition(WVector3 pos);
	void SetZoom(float d);

	virtual WRenderer* CreateRenderer();
	virtual WPhysicsComponent* CreatePhysicsComponent();
};

class WTestState : public WGameState {
public:
	WTestState(Wasabi* const app) : WGameState(app) {}

	virtual WRenderer* CreateRenderer() {
		return new WDeferredRenderer(m_app);
	}
	virtual WPhysicsComponent* CreatePhysicsComponent() {
		return nullptr;
	}
};

