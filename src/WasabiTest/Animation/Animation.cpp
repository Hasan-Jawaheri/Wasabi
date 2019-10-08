#include "Animation/Animation.hpp"

AnimationDemo::AnimationDemo(Wasabi* const app) : WTestState(app) {
	character = nullptr;
}

void AnimationDemo::Load() {
	WGeometry* geometry = new WGeometry(m_app);
	CheckError(geometry->LoadFromHXM("Media/dante.HXM"));

	WImage* texture = m_app->ImageManager->CreateImage("Media/dante.bmp");
	assert(texture != nullptr);

	character = m_app->ObjectManager->CreateObject();
	assert(character!= nullptr);
	CheckError(character->SetGeometry(geometry));
	//character->GetMaterial()->SetVariable<WColor>("color", WColor(1, 0, 0));
	CheckError(character->GetMaterials().SetTexture("diffuseTexture", texture));

	((WasabiTester*)m_app)->SetCameraPosition(WVector3(0, geometry->GetMaxPoint().y / 2, 0));

	// don't need these anymore, character has the reference to them
	geometry->RemoveReference();
	texture->RemoveReference();

	WSkeleton* animation;
	WFile file(m_app);
	CheckError(file.Open("Media/dante.WSBI"));
	assert(file.GetAssetsCount() > 0);
	CheckError(file.LoadAsset<WSkeleton>("dante-animation", &animation, WSkeleton::LoadArgs()));
	file.Close();

	CheckError(character->SetAnimation(animation));
	animation->SetPlaySpeed(20.0f);
	animation->Loop();

	// don't need this anymore
	animation->RemoveReference();
}

void AnimationDemo::Update(float fDeltaTime) {
	UNREFERENCED_PARAMETER(fDeltaTime);
}

void AnimationDemo::Cleanup() {
	W_SAFE_REMOVEREF(character);
}
