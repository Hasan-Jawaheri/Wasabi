#include "Wasabi.h"
#include "WForwardRenderer.h"

void ApplyMousePivot(Wasabi* app, WCamera* cam, float& fYaw, float& fPitch, float& fDist,
					 float x, float y, float z) {
	static bool bMouseHidden = false;
	static int lx, ly;
	if (app->InputComponent->MouseClick(MOUSE_LEFT)) {
		if (!bMouseHidden) {
			ShowCursor(FALSE);
			bMouseHidden = true;

			lx = app->InputComponent->MouseX(MOUSEPOS_DESKTOP, 0);
			ly = app->InputComponent->MouseY(MOUSEPOS_DESKTOP, 0);

			POINT pt = { 640 / 2, 480 / 2 };
			ClientToScreen(((WWC_Win32*)app->WindowComponent)->GetWindow(), &pt);
			SetCursorPos(pt.x, pt.y);
		}

		int mx = app->InputComponent->MouseX(MOUSEPOS_VIEWPORT, 0);
		int my = app->InputComponent->MouseY(MOUSEPOS_VIEWPORT, 0);

		int dx = mx - 640 / 2;
		int dy = my - 480 / 2;

		if (abs(dx) < 2)
			dx = 0;
		if (abs(dy) < 2)
			dy = 0;

		fYaw += (float)dx / 2.0f;
		fPitch += (float)dy / 2.0f;

		if (dx || dy)
			app->InputComponent->SetMousePosition(640 / 2, 480 / 2);
	} else {
		if (bMouseHidden) {
			ShowCursor(TRUE);
			bMouseHidden = false;

			SetCursorPos(lx, ly);
		}
	}

	float fMouseZ = app->InputComponent->MouseZ();
	fDist += (fMouseZ / 120.0f) * (abs(fDist)/10.0f);
	app->InputComponent->SetMouseZ(0);
	fDist = min(-1, fDist);

	cam->SetPosition(x, y, z);
	cam->SetAngle(0, 0, 0);
	cam->Yaw(fYaw);
	cam->Pitch(fPitch);
	cam->Move(fDist);
}

//#include "WHavokPhysics.h"
class Kofta : public Wasabi {
	float fYaw, fPitch, fDist;

protected:
	void SetupComponents() {
		Wasabi::SetupComponents();
		//PhysicsComponent = new WHavokPhysics(this);
		fYaw = 0;
		fPitch = 30;
		fDist = -15;
	}

public:
	WError Setup() {
		this->maxFPS = 0;
		WError err = StartEngine(640, 480);
		if (!err) {
			MessageBoxA(nullptr, "Ooops!", "Wasabi", MB_OK | MB_ICONERROR);
			return err;
		}

		WObject* o = new WObject(this);
		WObject* o2 = new WObject(this);
		WObject* o3 = new WObject(this);

		WGeometry* g = new WGeometry(this);
		if (g->CreateSphere(1, 15, 15)) {
			g->LoadFromHXM("models/F16.HXM");
			g->Scale(0.2);

			o2->SetGeometry(g);
			o3->SetGeometry(g);

			g->RemoveReference();
		}

		WGeometry* gPlain = new WGeometry(this);
		gPlain->CreatePlain(15.0f, 0, 0);
		o->SetGeometry(gPlain);
		((WFRMaterial*)o->GetMaterial())->SetColor(WColor(0.2, 0.1, 0.15));

		o->SetPosition(0, 0, 0);
		o2->SetPosition(0, 0, -4);
		o3->SetPosition(4, 4, 0);
		o3->SetAngle(0, 0, 80);

		TextComponent->CreateFont(1, "FORTE");

		WImage* img = new WImage(this);
		((WFRMaterial*)o2->GetMaterial())->Texture(img);
		img->Load("textures/dummy.bmp", true);
		img->RemoveReference();

		CameraManager->GetDefaultCamera()->SetPosition(0, 15, -15);
		CameraManager->GetDefaultCamera()->Point(0, 0, 0);

		WLight* sky = LightManager->GetDefaultLight();
		sky->Hide();

		WLight* l = new WLight(this);
		l->SetPosition(-5, 2, 0);
		l->Point(0, 0, 0);
		l->SetType(W_LIGHT_POINT);
		l->SetRange(10);
		l->SetIntensity(4.0f);

		return err;
	}
	bool Loop(float fDeltaTime) {
		char title[128];
		sprintf_s(title, 128, "Elapsed time: %.2f\nFPS: %.2f", Timer.GetElapsedTime() / 1000.0f, FPS);
		int mx = InputComponent->MouseX();
		int my = InputComponent->MouseY();
		int width = TextComponent->GetTextWidth("Elapsed time: 0.00", 32, 1);
		TextComponent->RenderText(title, mx - width / 2, my - 45, 32, 1);

		ApplyMousePivot(this, CameraManager->GetDefaultCamera(), fYaw, fPitch, fDist, 0, 0, 0);

		return true;
	}
	void Cleanup() {

	}
};

Wasabi* WInitialize() {
	return new Kofta();
}
