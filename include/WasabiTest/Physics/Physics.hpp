#pragma once

#include "TestSuite.hpp"
#include <Wasabi/Physics/Bullet/WBulletPhysics.h>
#include <Wasabi/Renderers/ForwardRenderer/WForwardRenderer.h>

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

	virtual WError SetupRenderer() { return WInitializeForwardRenderer(m_app); }
	virtual WPhysicsComponent* CreatePhysicsComponent();
};
