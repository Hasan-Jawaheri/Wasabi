#pragma once

#include "../TestSuite.hpp"
#include <Physics/Bullet/WBulletPhysics.h>
#include <Renderers/WForwardRenderer.h>
#include <Renderers/WDeferredRenderer.h>

class PhysicsDemo : public WTestState {
	WObject* m_ground;
	WRigidBody* m_groundRB;
	WObject* m_ball;
	WRigidBody* m_ballRB;

public:
	PhysicsDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WRenderer* CreateRenderer() { return new WDeferredRenderer(m_app); }
	virtual WPhysicsComponent* CreatePhysicsComponent();
};
