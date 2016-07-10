#include "Wasabi.h"
#include "WForwardRenderer.h"

//#include "WHavokPhysics.h"
class Kofta : public Wasabi {
	WSprite* spr;

protected:
	void SetupComponents() {
		Wasabi::SetupComponents();
		//PhysicsComponent = new WHavokPhysics(this);
	}

public:
	WError Setup() {
		this->maxFPS = 60;
		WError err = StartEngine(640, 480);
		if (!err) {
			MessageBoxA(nullptr, "Ooops!", "Wasabi", MB_OK | MB_ICONERROR);
			return err;
		}

		WObject* o = new WObject(this);
		WObject* o2 = new WObject(this);
		WObject* o3 = new WObject(this);

		WGeometry* g = new WGeometry(this);
		if (g->CreateCone(1, 2, 4, 20)) {
			g->Scale(1.4);

			o->SetGeometry(g);
			o2->SetGeometry(g);
			o3->SetGeometry(g);

			g->RemoveReference();
		}

		o->SetPosition(-5, 5, 0);
		o2->SetPosition(0, 0, -4);
		o3->SetPosition(5, 5, 0);
		o3->SetAngle(0, 0, 20);

		TextComponent->CreateFont(1, "FORTE");

		WImage* img = new WImage(this);
		((WFRMaterial*)o2->GetMaterial())->Texture(img);
		img->Load("textures/dummy.bmp", true);
		float* pixels;
		int cs = img->GetComponentSize();
		int nc = img->GetNumComponents();
		int ps = img->GetPixelSize();
		img->MapPixels((void**)&pixels);
		for (int y = 0; y < img->GetHeight(); y++) {
			for (int x = 0; x < img->GetWidth(); x++) {
				pixels[(y*img->GetWidth() + x) * nc + 0] *= 0.1f;
				pixels[(y*img->GetWidth() + x) * nc + 1] *= 0.5f;
				pixels[(y*img->GetWidth() + x) * nc + 2] *= 0.3f;
			}
		}
		img->UnmapPixels();
		img->RemoveReference();

		spr = new WSprite(this);
		spr->SetImage(img);
		spr->SetRotationCenter(WVector2(128, 128));
		//spr->SetAlpha(0.6f);

		CameraManager->GetDefaultCamera()->Move(-10);

		return err;
	}
	bool Loop(float fDeltaTime) {
		char title[128];
		sprintf_s(title, 128, "Elapsed time: %.2f\nFPS: %.2f", Timer.GetElapsedTime() / 1000.0f, FPS);
		int mx = InputComponent->MouseX();
		int my = InputComponent->MouseY();
		int width = TextComponent->GetTextWidth("Elapsed time: 0.00", 32, 1);
		TextComponent->RenderText(title, mx - width / 2, my - 45, 32, 1);

		spr->Rotate(40.0f * fDeltaTime);
		spr->Move(100.0f * fDeltaTime);

		return true;
	}
	void Cleanup() {

	}
};

Wasabi* WInitialize() {
	return new Kofta();
}
