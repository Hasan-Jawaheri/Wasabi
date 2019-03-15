#include "Files.hpp"

FilesDemo::FilesDemo(Wasabi* const app) : WTestState(app) {
}

void FilesDemo::Load() {
	WImage* img = new WImage(m_app);
	img->Load("Media/dummy.bmp", true);

	WFile file(m_app);
	file.Open("WFile.WSBI");

	uint assetId;
	file.SaveAsset(img, &assetId);
	W_SAFE_REMOVEREF(img);

	file.LoadAsset<WImage>(assetId, &img);

	file.Close();
	
	WSprite* spr = new WSprite(m_app);
	spr->SetImage(img);
}

void FilesDemo::Update(float fDeltaTime) {
}

void FilesDemo::Cleanup() {
}