#include "Animation/Animation.hpp"

AnimationDemo::AnimationDemo(Wasabi* const app) : WTestState(app) {
	character = nullptr;
}

void AnimationDemo::Load() {
	WSkeleton* animation;
	WGeometry* geometry;
	WFile file(m_app);
	CheckError(file.Open("media/dante.WSBI"));
	assert(file.GetAssetsCount() >= 2);
	CheckError(file.LoadAsset<WSkeleton>("dante-animation", &animation, WSkeleton::LoadArgs()));
	CheckError(file.LoadAsset<WGeometry>("dante-geometry", &geometry, WGeometry::LoadArgs()));
	file.Close();

	WImage* texture = m_app->ImageManager->CreateImage("media/dante.png");
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
