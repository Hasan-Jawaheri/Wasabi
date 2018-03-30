#include "TestSuite.hpp"
#include "RenderTargetTexture.hpp"
#include "Animation.hpp"
#include "Instancing.hpp"
#include "Lights.hpp"
#include "Sound.hpp"

/******************************************************************
 *          CHANGE THIS LINE TO CHOOSE THE DEMO TO RUN            *
 ******************************************************************/
#define _DEMO_STATE_CLASSNAME_ LightsDemo
 /******************************************************************
 * OPTIONS:
 * - RenderTargetTextureDemo
 * - AnimationDemo
 * - SoundDemo
 * - InstancingDemo
 * - LightsDemo
 ******************************************************************/

void WasabiTester::ApplyMousePivot() {
	WCamera* cam = CameraManager->GetDefaultCamera();
	static bool bMouseHidden = false;
	static int lx, ly;
	if (InputComponent->MouseClick(MOUSE_LEFT)) {
		if (!bMouseHidden) {
			ShowCursor(FALSE);
			bMouseHidden = true;

			lx = InputComponent->MouseX(MOUSEPOS_DESKTOP, 0);
			ly = InputComponent->MouseY(MOUSEPOS_DESKTOP, 0);

			POINT pt = { 640 / 2, 480 / 2 };
			ClientToScreen(((WWC_Win32*)WindowComponent)->GetWindow(), &pt);
			SetCursorPos(pt.x, pt.y);
		}

		int mx = InputComponent->MouseX(MOUSEPOS_VIEWPORT, 0);
		int my = InputComponent->MouseY(MOUSEPOS_VIEWPORT, 0);

		int dx = mx - 640 / 2;
		int dy = my - 480 / 2;

		if (abs(dx) < 2)
			dx = 0;
		if (abs(dy) < 2)
			dy = 0;

		fYaw += (float)dx / 2.0f;
		fPitch += (float)dy / 2.0f;

		if (dx || dy)
			InputComponent->SetMousePosition(640 / 2, 480 / 2);
	} else {
		if (bMouseHidden) {
			ShowCursor(TRUE);
			bMouseHidden = false;

			SetCursorPos(lx, ly);
		}
	}

	float fMouseZ = InputComponent->MouseZ();
	fDist += (fMouseZ / 120.0f) * (abs(fDist) / 10.0f);
	InputComponent->SetMouseZ(0);
	fDist = min(-1, fDist);

	cam->SetPosition(vPos);
	cam->SetAngle(0, 0, 0);
	cam->Yaw(fYaw);
	cam->Pitch(fPitch);
	cam->Move(fDist);
}

WasabiTester::WasabiTester() : Wasabi() {
	fYaw = 0;
	fPitch = 30;
	fDist = -15;
}

WError WasabiTester::Setup() {
	this->maxFPS = 0;
	WError ret = StartEngine(640, 480);
	if (!ret) {
		MessageBoxA(nullptr, "Ooops!", "Wasabi", MB_OK | MB_ICONERROR);
		return ret;
	}

	LightManager->GetDefaultLight()->Point(0, -1, -1);

	SwitchState(new _DEMO_STATE_CLASSNAME_(this));

	return ret;
}
bool WasabiTester::Loop(float fDeltaTime) {
	ApplyMousePivot();

	char title[128];
	sprintf_s(title, 128, "FPS: %.2f", FPS);
	TextComponent->RenderText(title, 5, 5, 32);

	return true;
}
void WasabiTester::Cleanup() {
	SwitchState(nullptr);
}

void WasabiTester::SetCameraPosition(WVector3 pos) {
	vPos = pos;
}
void WasabiTester::SetZoom(float d) {
	fDist = d;
}

Wasabi* WInitialize() {
	return new WasabiTester();
}