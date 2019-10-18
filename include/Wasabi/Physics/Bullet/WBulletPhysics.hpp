#pragma once

#include "Wasabi/Physics/WPhysicsComponent.hpp"

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
	virtual class WRigidBody* CreateRigidBody(uint32_t ID = 0) const;
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
	void* m_collisionConfiguration;
	/** Bullet dispatcher */
	void* m_dispatcher;
	/** Bullet stuff */
	void* m_overlappingPairCache;
	/** Bullet solver */
	void* m_solver;
	/** Bullet world */
	void* m_dynamicsWorld;
};

#ifdef COMPILING_WASABI
#include <btBulletCollisionCommon.h>
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
#endif
