#include "RenderTargetTexture/RenderTargetTexture.hpp"
#include <Wasabi/Renderers/ForwardRenderer/WForwardRenderStage.hpp>

RenderTargetTextureDemo::RenderTargetTextureDemo(Wasabi* const app) : WTestState(app) {
	rt = nullptr;
}

void RenderTargetTextureDemo::Load() {
	o = m_app->ObjectManager->CreateObject(1);
	o2 = m_app->ObjectManager->CreateObject(2);
	o3 = m_app->ObjectManager->CreateObject(3);

	WGeometry* gSphere = new WGeometry(m_app);
	CheckError(gSphere->CreateSphere(1, 15, 15));
	gSphere->Scale(1.4f);

	WGeometry* gPlain = new WGeometry(m_app);
	CheckError(gPlain->CreatePlain(15.0f, 0, 0));

	o->SetGeometry(gPlain);
	o->SetName("plain");
	o2->SetGeometry(gSphere);
	o2->SetName("sphere1");
	o3->SetGeometry(gSphere);
	o3->SetName("sphere2");
	
	gSphere->RemoveReference();
	gPlain->RemoveReference();

	o->SetPosition(0, 0, 0);
	o2->SetPosition(0, 0, -4);
	o3->SetPosition(4, 4, 0);
	o3->Roll(80);

	WImage* img = new WImage(m_app);
	o2->GetMaterials().SetTexture("diffuseTexture", img);
	CheckError(img->Load("media/seamless_snow.jpg"));
	img->RemoveReference();

	l = new WPointLight(m_app);
	l->SetPosition(-5, 2, 0);
	l->Point(0, 0, 0);
	l->SetRange(10);
	l->SetIntensity(4.0f);

	rtImg = new WImage(m_app);
	depthImg = new WImage(m_app);
	rtImg->CreateFromPixelsArray(nullptr, 640, 480, m_app->Renderer->GetRenderTarget()->GetTargetFormat(0), W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_RENDER_TARGET_ATTACHMENT);
	depthImg->CreateFromPixelsArray(nullptr, 640, 480, m_app->Renderer->GetRenderTarget()->GetDepthTargetFormat(), W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_RENDER_TARGET_ATTACHMENT);
	rt = m_app->RenderTargetManager->CreateImmediateRenderTarget();
	rt->SetName("Falla RT");
	rt->Create(640, 480, rtImg, depthImg);
	rt->SetClearColor(WColor(0.6f, 0, 0));
	o->GetMaterials().SetTexture("diffuseTexture", rtImg);
}

void RenderTargetTextureDemo::Update(float fDeltaTime) {
	UNREFERENCED_PARAMETER(fDeltaTime);

	double mx = m_app->WindowAndInputComponent->MouseX();
	double my = m_app->WindowAndInputComponent->MouseY();

	WVector3 pt;
	WObject* picked = m_app->ObjectManager->PickObject(mx, my, false, 0, 1, &pt);
	if (picked) {
		m_app->ObjectManager->GetEntity("sphere1")->SetPosition(pt);
		WVector2 out;
		double x, y;
		WUtil::Point3DToScreen2D(m_app, pt, &x, &y);
	}

	if (rt) {
		o->Hide();
		rt->Begin();
		m_app->Renderer->GetRenderStage("WForwardRenderStage")->Render(m_app->Renderer, rt, RENDER_FILTER_OBJECTS);
		rt->End();
		o->Show();
	}

	char title[128];
	sprintf_s(title, 128, "Elapsed time: %.2f\nFPS: %.2f\nmx=%.2f, my=%.2f", m_app->Timer.GetElapsedTime() / 1000.0f, m_app->FPS, mx, my);
	float width = m_app->TextComponent->GetTextWidth("Elapsed time: 0.00", 32, 1);
	m_app->TextComponent->RenderText(title, (float)mx - width / 2.0f, (float)my - 45.0f, 32, 1);
}

void RenderTargetTextureDemo::Cleanup() {
	o->RemoveReference();
	o2->RemoveReference();
	o3->RemoveReference();
	l->RemoveReference();
	rt->RemoveReference();
	rtImg->RemoveReference();
	depthImg->RemoveReference();
}
