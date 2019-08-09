#pragma once

#include "TestSuite.hpp"
#include <Physics/Bullet/WBulletPhysics.h>
#include <Renderers/ForwardRenderer/WForwardRenderer.h>

class FilesDemo : public WTestState {
	WMaterial* m_material;
	WObject* m_object;
	WCamera* m_cam;

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
