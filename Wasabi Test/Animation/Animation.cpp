#if 0

#include "Animation.hpp"

AnimationDemo::AnimationDemo(Wasabi* const app) : WTestState(app) {
}

void AnimationDemo::Load() {
	WGeometry* geometry = new WGeometry(m_app);
	geometry->LoadFromHXM("Media/dante.HXM");

	WImage* texture = new WImage(m_app);
	texture->Load("Media/dante.bmp");

	character = new WObject(m_app);
	character->SetGeometry(geometry);
	character->GetMaterial()->SetTexture(0, texture);

	// don't need these anymore, character has the reference to them
	geometry->RemoveReference();
	texture->RemoveReference();

	WSkeleton* animation;
	WFile file(m_app);
	file.Open("Media/dante.WSBI");
	file.LoadAsset<WSkeleton>(2, &animation);
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
	character->RemoveReference();
}

#endif