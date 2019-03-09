#pragma once

#include "../../Wasabi.h"

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

class WBulletPhysics : public WPhysicsComponent {
	friend class WBulletRigidBodyManager;
	friend class WBulletRigidBody;

public:
	WBulletPhysics(class Wasabi* app);
	~WBulletPhysics();

	virtual WError Initialize();
	virtual void Cleanup();
	virtual void Start();
	virtual void Stop();
	virtual void Step(float deltaTime);
	virtual bool Stepping() const;
	virtual WRigidBody* CreateRigidBody(unsigned int ID = 0) const;
	virtual bool RayCast(WVector3 from, WVector3 to);
	virtual bool RayCast(WVector3 from, WVector3 to, W_RAYCAST_OUTPUT* out);

	void SetSpeed(float fSpeed);
	void SetGravity(float x, float y, float z);
	void SetGravity(WVector3 gravity);

private:
	bool m_isStepping;
	btDefaultCollisionConfiguration* m_collisionConfiguration;
	btCollisionDispatcher* m_dispatcher;
	btBroadphaseInterface* m_overlappingPairCache;
	btSequentialImpulseConstraintSolver* m_solver;
	btDiscreteDynamicsWorld* m_dynamicsWorld;
};
