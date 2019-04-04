#include "Instancing.hpp"

InstancingDemo::InstancingDemo(Wasabi* const app) : WTestState(app) {
}

void InstancingDemo::Load() {
	geometry = new WGeometry(m_app);
	geometry->LoadFromHXM("Media/dante.HXM");
	//geometry->CreateCube(0.9);

	texture = new WImage(m_app);
	texture->Load("Media/dante.bmp");

	character = m_app->ObjectManager->CreateObject();
	character->SetGeometry(geometry);
	character->GetMaterial()->SetTexture("diffuseTexture", texture);

	int instancing = 2;

	if (instancing) {
		int nx = 20, nz = 80;
		float width = 3.0f * nx, depth = (float)nz;
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
					WObject* c = m_app->ObjectManager->CreateObject();
					c->SetGeometry(geometry);
					c->GetMaterial()->SetTexture("diffuseTexture", texture);
					c->SetPosition(px, 0, pz);
					objectsV.push_back(c);
				}
			}
		}
	}
}

void InstancingDemo::Update(float fDeltaTime) {
}

void InstancingDemo::Cleanup() {
	character->RemoveReference();
	geometry->RemoveReference();
	texture->RemoveReference();
	for (unsigned int i = 0; i < objectsV.size(); i++)
		objectsV[i]->RemoveReference();
}
