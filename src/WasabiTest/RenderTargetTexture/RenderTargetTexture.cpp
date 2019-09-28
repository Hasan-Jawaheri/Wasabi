#include "RenderTargetTexture/RenderTargetTexture.hpp"
#include <Wasabi/Renderers/ForwardRenderer/WForwardRenderStage.h>

RenderTargetTextureDemo::RenderTargetTextureDemo(Wasabi* const app) : WTestState(app) {
	rt = nullptr;
}

void RenderTargetTextureDemo::Load() {
	o = m_app->ObjectManager->CreateObject(1);
	o2 = m_app->ObjectManager->CreateObject(2);
	o3 = m_app->ObjectManager->CreateObject(3);

	WGeometry* g = new WGeometry(m_app);
	if (g->CreateSphere(1, 15, 15)) {
		g->Scale(1.4f);

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
	o3->Roll(80);

	m_app->TextComponent->CreateTextFont(2, "arial");

	WImage* img = new WImage(m_app);
	o2->GetMaterial()->SetTexture("diffuseTexture", img);
	img->Load("Media/dummy.bmp");
	img->RemoveReference();

	l = new WPointLight(m_app);
	l->SetPosition(-5, 2, 0);
	l->Point(0, 0, 0);
	l->SetRange(10);
	l->SetIntensity(4.0f);

	rtImg = new WImage(m_app);
	rtImg->CreateFromPixelsArray(nullptr, 640, 480, VK_FORMAT_R32G32B32A32_SFLOAT, W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_RENDER_TARGET_ATTACHMENT);
	rt = m_app->RenderTargetManager->CreateImmediateRenderTarget();
	rt->SetName("Falla RT");
	rt->Create(640, 480, rtImg);
	rt->SetClearColor(WColor(0.6f, 0, 0));
	o->GetMaterial()->SetTexture("diffuseTexture", rtImg);
}

void RenderTargetTextureDemo::Update(float fDeltaTime) {
	UNREFERENCED_PARAMETER(fDeltaTime);

	int mx = m_app->WindowAndInputComponent->MouseX();
	int my = m_app->WindowAndInputComponent->MouseY();

	WVector3 pt;
	WObject* picked = m_app->ObjectManager->PickObject(mx, my, false, 0, 1, &pt);
	if (picked) {
		m_app->ObjectManager->GetEntity("sphere")->SetPosition(pt);
		WVector2 out;
		int x, y;
		WUtil::Point3DToScreen2D(m_app, pt, &x, &y);
	}

	if (rt) {
		m_app->ObjectManager->GetEntity("plain")->Hide();
		rt->Begin();
		m_app->Renderer->GetRenderStage("WForwardRenderStage")->Render(m_app->Renderer, rt, RENDER_FILTER_OBJECTS);
		rt->End();
		m_app->ObjectManager->GetEntity("plain")->Show();
	}

	char title[128];
	sprintf_s(title, 128, "Elapsed time: %.2f\nFPS: %.2f", m_app->Timer.GetElapsedTime() / 1000.0f, m_app->FPS);
	float width = m_app->TextComponent->GetTextWidth("Elapsed time: 0.00", 32, 2);
	m_app->TextComponent->RenderText(title, mx - width / 2.0f, my - 45.0f, 32, 2);
}

void RenderTargetTextureDemo::Cleanup() {
	o->RemoveReference();
	o2->RemoveReference();
	o3->RemoveReference();
	l->RemoveReference();
	rt->RemoveReference();
	rtImg->RemoveReference();
}
