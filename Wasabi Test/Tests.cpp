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

class Kofta : public WGameState {
	WObject *o, *o2, *o3;
	WLight* l;
	WRenderTarget* rt;
	WImage* rtImg;

public:
	Kofta(Wasabi* const app) : WGameState(app) {
		rt = nullptr;
	}

	virtual void Load() {
		o = new WObject(m_app);
		o2 = new WObject(m_app, 2);
		o3 = new WObject(m_app);

		WGeometry* g = new WGeometry(m_app);
		if (g->CreateSphere(1, 15, 15)) {
			g->Scale(1.4);

			o2->SetGeometry(g);
			o2->SetName("sphere");
			//o3->SetGeometry(g);

			g->RemoveReference();
		}

		WGeometry* gPlain = new WGeometry(m_app);
		gPlain->CreatePlain(15.0f, 0, 0);
		o->SetGeometry(gPlain);
		o->SetName("plain");
		gPlain->RemoveReference();

		o->SetPosition(0, 0, 0);
		o2->SetPosition(0, 0, -4);
		o3->SetPosition(4, 4, 0);
		o3->SetAngle(0, 0, 80);

		m_app->TextComponent->CreateFont(1, "arial");

		WImage* img = new WImage(m_app);
		((WFRMaterial*)o2->GetMaterial())->Texture(img);
		img->Load("Media/dummy.bmp", true);
		img->RemoveReference();

		l = new WLight(m_app);
		l->SetPosition(-5, 2, 0);
		l->Point(0, 0, 0);
		l->SetType(W_LIGHT_POINT);
		l->SetRange(10);
		l->SetIntensity(4.0f);

		rtImg = new WImage(m_app);
		char* pixels = new char[640 * 480 * 4 * 4];
		rtImg->CreateFromPixelsArray(pixels, 640, 480, false, 4, VK_FORMAT_R32G32B32A32_SFLOAT, 4);
		delete[] pixels;
		rt = new WRenderTarget(m_app);
		rt->SetName("Falla RT");
		rt->Create(640, 480, rtImg, false);
		rt->SetClearColor(WColor(0.2, 0, 0));
		((WFRMaterial*)o->GetMaterial())->Texture(rtImg);
	}

	virtual void Update(float fDeltaTime) {
		int mx = m_app->InputComponent->MouseX();
		int my = m_app->InputComponent->MouseY();

		WVector3 pt;
		if (WObject* o = m_app->ObjectManager->PickObject(mx, my, false, 0, 1, &pt)) {
			m_app->ObjectManager->GetEntity("sphere")->SetPosition(pt);
			WVector2 out;
			int x, y;
			WUtil::Point3DToScreen2D(m_app, pt, &x, &y);
			int g = 0;
		}

		if (rt) {
			rt->Begin();
			m_app->ObjectManager->GetEntity("plain")->Hide();
			m_app->Renderer->Render(rt, RENDER_FILTER_OBJECTS);
			m_app->ObjectManager->GetEntity("plain")->Show();
			rt->End();
		}

		char title[128];
		sprintf_s(title, 128, "Elapsed time: %.2f\nFPS: %.2f", m_app->Timer.GetElapsedTime() / 1000.0f, m_app->FPS);
		int width = m_app->TextComponent->GetTextWidth("Elapsed time: 0.00", 32, 1);
		m_app->TextComponent->RenderText(title, mx - width / 2, my - 45, 32, 1);
	}

	void Cleanup() {
		o->RemoveReference();
		o2->RemoveReference();
		o3->RemoveReference();
		l->RemoveReference();
		rt->RemoveReference();
		rtImg->RemoveReference();
	}
};

class AnimationDemo : public WGameState {
	WObject* character;
	WGeometry* geometry;
	WImage* texture;
	WSkeleton* animation;

public:
	AnimationDemo(Wasabi* const app) : WGameState(app) {}

	virtual void Load() {
		geometry = new WGeometry(m_app);
		geometry->LoadFromHXM("Media/dante.HXM");

		texture = new WImage(m_app);
		texture->Load("Media/dante.bmp");

		character = new WObject(m_app);
		character->SetGeometry(geometry);
		((WFRMaterial*)character->GetMaterial())->Texture(texture);

		animation = new WSkeleton(m_app);
		animation->LoadFromWA("Media/dante.HXS");

		character->SetAnimation(animation);
		animation->SetPlaySpeed(20.0f);
		animation->Loop();
	}

	virtual void Update(float fDeltaTime) {
	}

	virtual void Cleanup() {
		character->RemoveReference();
		geometry->RemoveReference();
		texture->RemoveReference();
		animation->RemoveReference();
	}
};

class InstancingDemo : public WGameState {
	WObject* character;
	WGeometry* geometry;
	WImage* texture;
	vector<WObject*> objectsV;

public:
	InstancingDemo(Wasabi* const app) : WGameState(app) {}

	virtual void Load() {
		geometry = new WGeometry(m_app);
		geometry->LoadFromHXM("Media/dante.HXM");

		texture = new WImage(m_app);
		texture->Load("Media/dante.bmp");

		character = new WObject(m_app);
		character->SetGeometry(geometry);
		((WFRMaterial*)character->GetMaterial())->Texture(texture);

		int instancing = 2;

		if (instancing) {
			int nx = 20, nz = 80;
			float width = 3 * nx, depth = nz;
			((WasabiTester*)m_app)->SetZoom(-depth * 1.2f);
			if (instancing == 2)
				character->InitInstancing(nx * nz);
			for (int x = 0; x < nx; x++) {
				for (int z = 0; z < nz; z++) {
					float px = (((float)x / (float)(nx - 1)) - 0.5f) * width;
					float pz = (((float)z / (float)(nz - 1)) - 0.5f) * depth;
					WInstance* inst = character->CreateInstance();
					if (inst) {
						inst->SetPosition(px, 0, pz);
					}

					if (instancing == 1) {
						WObject* c = new WObject(m_app);
						c->SetGeometry(geometry);
						((WFRMaterial*)c->GetMaterial())->Texture(texture);
						c->SetPosition(px, 0, pz);
						objectsV.push_back(c);
					}
				}
			}
		}
	}

	virtual void Update(float fDeltaTime) {
	}

	virtual void Cleanup() {
		character->RemoveReference();
		geometry->RemoveReference();
		texture->RemoveReference();
		for (int i = 0; i < objectsV.size(); i++)
			objectsV[i]->RemoveReference();
	}
};

class SoundDemo : public WGameState {
	WSound* m_sound;

public:
	SoundDemo(Wasabi* const app) : WGameState(app) {}

	virtual void Load() {
		m_sound = new WSound(m_app);
		m_sound->LoadWAV("Media/test.wav", 0);
		m_sound->Play();
	}

	virtual void Update(float fDeltaTime) {
		if (m_sound->Playing()) {
			m_app->TextComponent->RenderText("Playing...", 5, 5, 16);
		} else {
			m_app->TextComponent->RenderText("Done!", 5, 5, 16);
		}
	}
};


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

	SwitchState(new SoundDemo(this));

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
