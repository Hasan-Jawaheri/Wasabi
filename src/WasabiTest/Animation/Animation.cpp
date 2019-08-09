#include "Animation/Animation.hpp"

AnimationDemo::AnimationDemo(Wasabi* const app) : WTestState(app) {
	character = nullptr;
}

void AnimationDemo::Load() {
	WGeometry* geometry = new WGeometry(m_app);
	geometry->LoadFromHXM("Media/dante.HXM");

	WImage* texture = m_app->ImageManager->CreateImage("Media/dante.bmp");

	character = m_app->ObjectManager->CreateObject();
	character->SetGeometry(geometry);
	//character->GetMaterial()->SetVariableColor("color", WColor(1, 0, 0));
	character->GetMaterial()->SetTexture("diffuseTexture", texture);

	// don't need these anymore, character has the reference to them
	geometry->RemoveReference();
	texture->RemoveReference();

	WSkeleton* animation;
	WFile file(m_app);
	file.Open("Media/dante.WSBI");
	file.LoadAsset<WSkeleton>(file.GetAssetInfo(0).first, &animation, WSkeleton::LoadArgs());
	file.Close();

	character->SetAnimation(animation);
	animation->SetPlaySpeed(20.0f);
	animation->Loop();

	// don't need this anymore
	animation->RemoveReference();

	((WasabiTester*)m_app)->SetCameraPosition(WVector3(0, geometry->GetMaxPoint().y / 2, 0));
}

void AnimationDemo::Update(float fDeltaTime) {
}

void AnimationDemo::Cleanup() {
	W_SAFE_REMOVEREF(character);
}
