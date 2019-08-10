#pragma once

#include "Wasabi/Physics/WPhysicsComponent.h"

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#ifdef _DEBUG
// #pragma comment(lib, "Bullet3Collision_debug.lib")
// #pragma comment(lib, "Bullet3Common_debug.lib")
// #pragma comment(lib, "Bullet3Dynamics_debug.lib")
// #pragma comment(lib, "Bullet3Geometry_debug.lib")
#pragma comment(lib, "BulletSoftBody_debug.lib")
#pragma comment(lib, "BulletCollision_debug.lib")
#pragma comment(lib, "BulletDynamics_debug.lib")
#pragma comment(lib, "LinearMath_debug.lib")
#else
// #pragma comment(lib, "Bullet3Collision.lib")
// #pragma comment(lib, "Bullet3Common.lib")
// #pragma comment(lib, "Bullet3Dynamics.lib")
// #pragma comment(lib, "Bullet3Geometry.lib")
#pragma comment(lib, "BulletSoftBody.lib")
#pragma comment(lib, "BulletCollision.lib")
#pragma comment(lib, "BulletDynamics.lib")
#pragma comment(lib, "LinearMath.lib")
#endif

/*
 * Creating an instance of this class creates the following engine parameters :
 ** "maxBulletDebugLines" : Maximum number of line segments the debugger can draw
 *                          (Default is(void*)(500000))
 */
class WBulletPhysics : public WPhysicsComponent {
	friend class WBulletRigidBodyManager;
	friend class WBulletRigidBody;

public:
	WBulletPhysics(class Wasabi* app);
	~WBulletPhysics();

	virtual WError Initialize(bool debug = false);
	virtual void Cleanup();
	virtual void Start();
	virtual void Stop();
	virtual void Step(float deltaTime);
	virtual bool Stepping() const;
	virtual class WRigidBody* CreateRigidBody(unsigned int ID = 0) const;
	virtual bool RayCast(WVector3 from, WVector3 to, W_RAYCAST_OUTPUT* out = nullptr);

	virtual void SetSpeed(float fSpeed);
	virtual void SetGravity(float x, float y, float z);
	virtual void SetGravity(WVector3 gravity);

private:
	/** Whether or not we are inside Step() */
	bool m_isStepping;
	/** Whether or not the simulation is running */
	bool m_isRunning;
	/** Speed of the simulation (multiplies time delta in the loop) */
	float m_speed;
	/** debugger (if enabled) */
	class BulletDebugger* m_debugger;
	/** Bullet collision configuration */
	btDefaultCollisionConfiguration* m_collisionConfiguration;
	/** Bullet dispatcher */
	btCollisionDispatcher* m_dispatcher;
	/** Bullet stuff */
	btBroadphaseInterface* m_overlappingPairCache;
	/** Bullet solver */
	btSequentialImpulseConstraintSolver* m_solver;
	/** Bullet world */
	btDiscreteDynamicsWorld* m_dynamicsWorld;
};

/** Converts a WVector3 to a btVector3 */
btVector3 WBTConvertVec3(WVector3 vec);
/** Converts a btVector3 to a WVector3 */
WVector3 BTWConvertVec3(btVector3 vec);
/** Converts a WMatrix to a btTransform */
btTransform WBTConverMatrix(WMatrix mtx);
/** Converts a btTransform to a WMatrix */
WMatrix BTWConverMatrix(btTransform mtx);
/** Converts a WVector4 to a btVector4 */
btVector4 WBTConvertVec4(WVector4 vec);
/** Converts a btVector4 to a WVector4 */
WVector4 BTWConvertVec4(btVector4 vec);
/** Converts a WQuaternion to a btQuaternion */
btQuaternion WBTConvertQuaternion(WQuaternion quat);
/** Converts a btQuaternion to a WQuaternion */
WQuaternion BTWConvertQuaternion(btQuaternion quat);
