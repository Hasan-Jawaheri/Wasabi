#pragma once

#include "TestSuite.hpp"
#include <Wasabi/Physics/Bullet/WBulletPhysics.hpp>
#include <Wasabi/Renderers/ForwardRenderer/WForwardRenderer.hpp>

class FilesDemo : public WTestState {
	WMaterialCollection m_materials;
	WObject* m_object;
	WRigidBody* m_rb;
	WCamera* m_cam;
	WSprite* m_sprite;

public:
	FilesDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WError SetupRenderer() { return WInitializeForwardRenderer(m_app); }
	virtual WPhysicsComponent* CreatePhysicsComponent() {
		WBulletPhysics* physics = new WBulletPhysics(m_app);
		WError werr = physics->Initialize();
		if (!werr)
			W_SAFE_DELETE(physics);
		return physics;
	}
};
