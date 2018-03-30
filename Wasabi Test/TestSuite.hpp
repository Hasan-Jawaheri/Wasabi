#pragma once

#include <Wasabi.h>
#include <Renderers/WDeferredRenderer.h>

class WasabiTester : public Wasabi {
	WRenderer* m_renderer;

	float fYaw, fPitch, fDist;
	WVector3 vPos;

	void ApplyMousePivot();

public:
	WasabiTester();
	WError Setup();
	bool Loop(float fDeltaTime);
	void Cleanup();
	void SetCameraPosition(WVector3 pos);
	void SetZoom(float d);

	WRenderer* CreateRenderer();
};

class WTestState : public WGameState {
public:
	WTestState(Wasabi* const app) : WGameState(app) {}

	virtual WRenderer* CreateRenderer() {
		return new WDeferredRenderer(m_app);
	}
};

