#pragma once

#include <Wasabi/Wasabi.h>
#include <Wasabi/Renderers/ForwardRenderer/WForwardRenderer.h>

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

	void CheckError(WError err);
};

class WTestState : public WGameState {
public:
	WTestState(Wasabi* const app) : WGameState(app) {}

	void CheckError(WError err) {
		((WasabiTester*)m_app)->CheckError(err);
	}

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

