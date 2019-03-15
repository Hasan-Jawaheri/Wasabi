#include "Files.hpp"

FilesDemo::FilesDemo(Wasabi* const app) : WTestState(app) {
}

void FilesDemo::Load() {
	WImage* img = new WImage(m_app);
	img->Load("Media/dummy.bmp", true);

	WGeometry* geometry = new WGeometry(m_app);
	geometry->CreateCube(1, true);

	WPointLight* light = new WPointLight(m_app);
	light->SetPosition(2, 2, 2);
	light->SetRange(5);
	light->SetColor(WColor(1, 0, 0));

	WFile file(m_app);
	file.Open("WFile.WSBI");

	uint imgId, geoId;
	file.SaveAsset(img, &imgId);
	file.SaveAsset(geometry, &geoId);
	W_SAFE_REMOVEREF(img);
	W_SAFE_REMOVEREF(geometry);

	file.LoadAsset<WImage>(imgId, &img);
	file.LoadAsset<WGeometry>(geoId, &geometry);

	file.Close();
	
	WSprite* spr = new WSprite(m_app);
	spr->SetImage(img);

	WObject* obj = new WObject(m_app);
	obj->SetGeometry(geometry);
}

void FilesDemo::Update(float fDeltaTime) {
}

void FilesDemo::Cleanup() {
}