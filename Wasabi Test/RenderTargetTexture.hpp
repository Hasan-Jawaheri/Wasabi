#pragma once

#include "TestSuite.hpp"

class RenderTargetTextureDemo : public WGameState {
	WObject *o, *o2, *o3;
	WLight* l;
	WRenderTarget* rt;
	WImage* rtImg;

public:
	RenderTargetTextureDemo(Wasabi* const app) : WGameState(app) {
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