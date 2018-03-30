#include "TestSuite.h"

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