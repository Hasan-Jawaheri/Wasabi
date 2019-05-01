#pragma once

#include <Wasabi.h>
#include <Renderers/ForwardRenderer/WForwardRenderer.h>

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
	WVector3 GetCameraPosition() const;
	float GetYawAngle() const;

	virtual WError SetupRenderer();
	virtual WPhysicsComponent* CreatePhysicsComponent();
	virtual WSoundComponent* CreateSoundComponent();
};

class WTestState : public WGameState {
public:
	WTestState(Wasabi* const app) : WGameState(app) {}

	virtual WError SetupRenderer() {
		return WInitializeForwardRenderer(m_app);
	}
	virtual WPhysicsComponent* CreatePhysicsComponent() {
		return nullptr;
	}
	virtual WSoundComponent* CreateSoundComponent() {
		return nullptr;
	}
};

