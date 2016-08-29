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
	WRenderTarget* rt;
	WImage* rtImg;

protected:
	void SetupComponents() {
		Wasabi::SetupComponents();
		//PhysicsComponent = new WHavokPhysics(this);
		fYaw = 0;
		fPitch = 30;
		fDist = -15;
		rt = nullptr;
	}

public:
	WError Setup() {
		this->maxFPS = 0;
		WError ret = StartEngine(640, 480);
		if (!ret) {
			MessageBoxA(nullptr, "Ooops!", "Wasabi", MB_OK | MB_ICONERROR);
			return ret;
		}

		WObject* o = new WObject(this);
		WObject* o2 = new WObject(this, 2);
		WObject* o3 = new WObject(this);

		WGeometry* g = new WGeometry(this);
		if (g->CreateSphere(1, 15, 15)) {
			g->Scale(1.4);

			o2->SetGeometry(g);
			o2->SetName("sphere");
			//o3->SetGeometry(g);

			g->RemoveReference();
		}

		WGeometry* gPlain = new WGeometry(this);
		gPlain->CreatePlain(15.0f, 0, 0);
		o->SetGeometry(gPlain);
		o->SetName("plain");

		o->SetPosition(0, 0, 0);
		o2->SetPosition(0, 0, -4);
		o3->SetPosition(4, 4, 0);
		o3->SetAngle(0, 0, 80);

		TextComponent->CreateFont(1, "arial");

		WImage* img = new WImage(this);
		((WFRMaterial*)o2->GetMaterial())->Texture(img);
		img->Load("textures/dummy.bmp", true);
		img->RemoveReference();

		CameraManager->GetDefaultCamera()->SetPosition(0, 15, -15);
		CameraManager->GetDefaultCamera()->Point(0, 0, 0);

		WLight* l = new WLight(this);
		l->SetPosition(-5, 2, 0);
		l->Point(0, 0, 0);
		l->SetType(W_LIGHT_POINT);
		l->SetRange(10);
		l->SetIntensity(4.0f);

		rtImg = new WImage(this);
		char* pixels = new char[640 * 480 * 4];
		for (int i = 0; i < 640 * 480; i++) {
			pixels[i * 3 + 0] = 255;
			pixels[i * 3 + 1] = 0;
			pixels[i * 3 + 2] = 0;
			pixels[i * 3 + 3] = 255;
		}
		rtImg->CreateFromPixelsArray(pixels, 640, 480, false, 4, VK_FORMAT_R8G8B8A8_UNORM, 1);
		delete[] pixels;
		rt = new WRenderTarget(this);
		rt->SetName("Falla RT");
		rt->Create(640, 480, rtImg);
		((WFRMaterial*)o->GetMaterial())->Texture(rtImg);

		return ret;
	}
	bool Loop(float fDeltaTime) {
		int mx = InputComponent->MouseX();
		int my = InputComponent->MouseY();

		ApplyMousePivot(this, CameraManager->GetDefaultCamera(), fYaw, fPitch, fDist, 0, 0, 0);

		WVector3 pt;
		if (WObject* o = ObjectManager->PickObject(mx, my, false, 0, 1, &pt)) {
			ObjectManager->GetEntity("sphere")->SetPosition(pt);
			WVector2 out;
			int x, y;
			WUtil::Point3DToScreen2D(this, pt, &x, &y);
			int g = 0;
		}

		if (rt) {
			rt->Begin();
			ObjectManager->GetEntity("plain")->Hide();
			Renderer->Render(rt, RENDER_FILTER_OBJECTS);
			ObjectManager->GetEntity("plain")->Show();
			rt->End();
		}

		char title[128];
		sprintf_s(title, 128, "Elapsed time: %.2f\nFPS: %.2f", Timer.GetElapsedTime() / 1000.0f, FPS);
		int width = TextComponent->GetTextWidth("Elapsed time: 0.00", 32, 1);
		TextComponent->RenderText(title, mx - width / 2, my - 45, 32, 1);

		return true;
	}
	void Cleanup() {

	}
};

class AnimationDemo : public Wasabi {
	WObject* character;
	WGeometry* geometry;
	WImage* texture;
	WSkeleton* animation;

	float fYaw, fPitch, fDist;

public:
	AnimationDemo() {
		fYaw = 0;
		fPitch = 30;
		fDist = -15;
	}

	WError Setup() {
		this->maxFPS = 0;
		WError ret = StartEngine(640, 480);
		if (!ret) {
			MessageBoxA(nullptr, "Ooops!", "Wasabi", MB_OK | MB_ICONERROR);
			return ret;
		}

		LightManager->GetDefaultLight()->Point(0, -1, -1);

		geometry = new WGeometry(this);
		geometry->LoadFromHXM("Media/dante.HXM");

		texture = new WImage(this);
		texture->Load("Media/dante.bmp");

		character = new WObject(this);
		character->SetGeometry(geometry);
		((WFRMaterial*)character->GetMaterial())->Texture(texture);

		animation = new WSkeleton(this);
		animation->LoadFromWS("Media/dante.HXS");

		character->SetAnimation(animation);
		animation->SetPlaySpeed(30.0f);
		animation->Loop();

		return ret;
	}
	bool Loop(float fDeltaTime) {
		ApplyMousePivot(this, CameraManager->GetDefaultCamera(), fYaw, fPitch, fDist, 0, 2.5, 0);
		return true;
	}
	void Cleanup() {

	}
};

Wasabi* WInitialize() {
	return new AnimationDemo();
}
