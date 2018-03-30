#pragma once

#include <Wasabi.h>
#include <Renderers/WForwardRenderer.h>

class WasabiTester : public Wasabi {
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
};

#include "RenderTargetTexture.cpp"
#include "Animation.cpp"
#include "Instancing.cpp"
#include "Sound.cpp"