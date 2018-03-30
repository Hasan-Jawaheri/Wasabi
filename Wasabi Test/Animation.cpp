#include "TestSuite.h"

class AnimationDemo : public WGameState {
	WObject* character;
	WGeometry* geometry;
	WImage* texture;
	WSkeleton* animation;

public:
	AnimationDemo(Wasabi* const app) : WGameState(app) {}

	virtual void Load() {
		geometry = new WGeometry(m_app);
		geometry->LoadFromHXM("Media/dante.HXM");

		texture = new WImage(m_app);
		texture->Load("Media/dante.bmp");

		character = new WObject(m_app);
		character->SetGeometry(geometry);
		((WFRMaterial*)character->GetMaterial())->Texture(texture);

		animation = new WSkeleton(m_app);
		animation->LoadFromWA("Media/dante.HXS");

		character->SetAnimation(animation);
		animation->SetPlaySpeed(20.0f);
		animation->Loop();
	}

	virtual void Update(float fDeltaTime) {
	}

	virtual void Cleanup() {
		character->RemoveReference();
		geometry->RemoveReference();
		texture->RemoveReference();
		animation->RemoveReference();
	}
};