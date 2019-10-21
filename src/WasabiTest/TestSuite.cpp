#include "TestSuite.hpp"

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
 * - ParticlesDemo
 * - TerrainDemo
 * - SpritesDemo
 * - PhysicsDemo
 * - FilesDemo
 ******************************************************************/

#include "RenderTargetTexture/RenderTargetTexture.hpp"
#include "Animation/Animation.hpp"
#include "Sound/Sound.hpp"
#include "Instancing/Instancing.hpp"
#include "Lights/Lights.hpp"
#include "Particles/Particles.hpp"
#include "Terrain/Terrain.hpp"
#include "Sprites/Sprites.hpp"
#include "Physics/Physics.hpp"
#include "Files/Files.hpp"

void WasabiTester::ApplyMousePivot() {
	static bool bMouseHidden = false;
	static double lx, ly;
	WCamera* cam = CameraManager->GetDefaultCamera();
	WWindowAndInputComponent* WIC = WindowAndInputComponent;
	if (WIC->MouseClick(MOUSE_LEFT)) {
		if (!bMouseHidden) {
			bMouseHidden = true;
			lx = WIC->MouseX(MOUSEPOS_DESKTOP, 0);
			ly = WIC->MouseY(MOUSEPOS_DESKTOP, 0);
			WIC->SetCursorMotionMode(true);
		}

		double mx = WIC->MouseX(MOUSEPOS_DESKTOP, 0);
		double my = WIC->MouseY(MOUSEPOS_DESKTOP, 0);

		double dx = mx - lx;
		double dy = my - ly;

		fYaw += (float)dx / 2.0f;
		fPitch += (float)dy / 2.0f;

		WIC->SetMousePosition(lx, ly, MOUSEPOS_DESKTOP);
	} else {
		if (bMouseHidden) {
			bMouseHidden = false;
			WIC->SetCursorMotionMode(false);
		}
	}

	float fMouseZ = (float)WIC->MouseZ();
	fDist += (fMouseZ) * (std::abs(fDist) / 10.0f);
	WIC->SetMouseZ(0);
	fDist = std::min(-1.0f, fDist);

	cam->SetPosition(vPos);
	cam->SetAngle(WQuaternion());
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
	WTestState* state = new _DEMO_STATE_CLASSNAME_(this);
	this->maxFPS = 0;
	this->m_state = state;
	CheckError(StartEngine(640, 480));

	std::vector<std::string> fontNames = {
		"Calibri",
		"FreeSans",
		"Arial Unicode",
	};
	for (auto fontName : fontNames) {
		if (TextComponent->CreateTextFont(1, fontName))
			break;
	}
	assert((bool)TextComponent->SetFont(1));

	LightManager->GetDefaultLight()->Point(0, -1, -1);

	SwitchState(state);

	return WError(W_SUCCEEDED);
}

bool WasabiTester::Loop(float fDeltaTime) {
	UNREFERENCED_PARAMETER(fDeltaTime);

	ApplyMousePivot();

	if (!WindowAndInputComponent->KeyDown(W_KEY_F1)) {
		char title[128];
		sprintf_s(title, 128, "FPS: %.2f (Elapsed %.2fs)", FPS, Timer.GetElapsedTime());
		TextComponent->RenderText(title, 5, 5, 32, 1);
	}

	if (WindowAndInputComponent->KeyDown(W_KEY_F8)) {
		WindowAndInputComponent->SetFullScreenState(true);
	}
	if (WindowAndInputComponent->KeyDown(W_KEY_F7)) {
		WindowAndInputComponent->SetFullScreenState(false);
	}

	return true;
}

void WasabiTester::Cleanup() {
	SwitchState(nullptr);
}

WError WasabiTester::SetupRenderer() {
	return m_state->SetupRenderer();
}

WPhysicsComponent* WasabiTester::CreatePhysicsComponent() {
	WPhysicsComponent* physics = m_state->CreatePhysicsComponent();
	if (physics)
		return physics;
	return Wasabi::CreatePhysicsComponent();
}

WSoundComponent* WasabiTester::CreateSoundComponent() {
	return m_state->CreateSoundComponent();
}

void WasabiTester::SetCameraPosition(WVector3 pos) {
	vPos = pos;
}

void WasabiTester::SetZoom(float d) {
	fDist = d;
}

WVector3 WasabiTester::GetCameraPosition() const {
	return vPos;
}

float WasabiTester::GetYawAngle() const {
	return fYaw;
}

void WasabiTester::CheckError(WError err) {
	if (!err) {
		WindowAndInputComponent->ShowErrorMessage(err.AsString(true).c_str());
		assert(err == W_SUCCEEDED);
	}
}

Wasabi* WInitialize() {
	return new WasabiTester();
}
