#include "Wasabi/Physics/Bullet/WBulletPhysics.h"
#include "Wasabi/Physics/Bullet/WBulletRigidBody.h"
#include "Wasabi/Physics/Bullet/BulletDebugger.h"

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <thread>

WBulletPhysics::WBulletPhysics(Wasabi* app) : WPhysicsComponent(app) {
	RigidBodyManager = new WBulletRigidBodyManager(app);

	m_debugger = nullptr;

	m_collisionConfiguration = nullptr;
	m_dispatcher = nullptr;
	m_overlappingPairCache = nullptr;
	m_solver = nullptr;
	m_dynamicsWorld = nullptr;

	m_isStepping = false;
	m_isRunning = false;
	m_speed = 1.0f;

	app->SetEngineParam<uint32_t>("maxBulletDebugLines", 500000);
}

WBulletPhysics::~WBulletPhysics() {
	Cleanup();
}

WError WBulletPhysics::Initialize(bool debug) {
	///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
	m_collisionConfiguration = (void*)new btDefaultCollisionConfiguration();

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	m_dispatcher = (void*)new btCollisionDispatcher(((btDefaultCollisionConfiguration*)m_collisionConfiguration));

	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	m_overlappingPairCache = (void*)new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	m_solver = (void*)new btSequentialImpulseConstraintSolver;

	m_dynamicsWorld = (void*)new btDiscreteDynamicsWorld(
		((btCollisionDispatcher*)m_dispatcher),
		((btDbvtBroadphase*)m_overlappingPairCache),
		((btSequentialImpulseConstraintSolver*)m_solver),
		((btDefaultCollisionConfiguration*)m_collisionConfiguration)
	);

	SetGravity(0, -10, 0);

	if (debug) {
		m_debugger = new BulletDebugger(this, m_app->GetEngineParam<uint32_t>("maxBulletDebugLines"), std::string(m_app->GetEngineParam<const char*>("appName")));
		((btDynamicsWorld*)m_dynamicsWorld)->setDebugDrawer(m_debugger);
		m_debugger->m_thread = std::thread(BulletDebugger::Thread, m_debugger);
	}

	return WError(W_SUCCEEDED);
}

void WBulletPhysics::Cleanup() {
	if (m_debugger) {
		m_debugger->__EXIT = true;
		m_debugger->m_thread.join();
		W_SAFE_DELETE(m_debugger);
	}

	W_SAFE_DELETE(RigidBodyManager);

	//delete dynamics world
	delete ((btDynamicsWorld*)m_dynamicsWorld);

	//delete solver
	delete ((btSequentialImpulseConstraintSolver*)m_solver);

	//delete broadphase
	delete ((btDbvtBroadphase*)m_overlappingPairCache);

	//delete dispatcher
	delete ((btCollisionDispatcher*)m_dispatcher);

	delete ((btDefaultCollisionConfiguration*)m_collisionConfiguration);
}

void WBulletPhysics::Start() {
	m_isRunning = true;
}

void WBulletPhysics::Stop() {
	m_isRunning = false;
}

void WBulletPhysics::Step(float deltaTime) {
	if (m_isRunning) {
		m_isStepping = true;

		static float accumulator = 0.0f;
		accumulator += deltaTime;

		while (accumulator > m_speed / 60.0f) {
			((btDynamicsWorld*)m_dynamicsWorld)->stepSimulation(m_speed / 60.0f, 1, m_speed / 60.0f);
			accumulator -= 1.0f / 60.0f;
		}
		// m_dynamicsWorld->stepSimulation(deltaTime* m_speed, 5, m_speed / 60.0f);

		((WBulletRigidBodyManager*)RigidBodyManager)->Update(deltaTime);
		if (m_debugger)
			((btDynamicsWorld*)m_dynamicsWorld)->debugDrawWorld();

		m_isStepping = false;
	}
}

bool WBulletPhysics::Stepping() const {
	return m_isStepping;
}

WRigidBody* WBulletPhysics::CreateRigidBody(uint32_t ID) const {
	return new WBulletRigidBody(m_app, ID);
}

bool WBulletPhysics::RayCast(WVector3 _from, WVector3 _to, W_RAYCAST_OUTPUT* out) {
	btVector3 from = WBTConvertVec3(_from);
	btVector3 to = WBTConvertVec3(_to);
	btCollisionWorld::ClosestRayResultCallback result(from, to);
	((btDynamicsWorld*)m_dynamicsWorld)->rayTest(from, to, result);

	if (result.hasHit()) {
		if (out) {
			out->fDist = WVec3Length(_to - _from) * result.m_closestHitFraction;
			out->normal = BTWConvertVec3(result.m_hitNormalWorld);
		}

		return true;
	}

	return false;
}

void WBulletPhysics::SetSpeed(float fSpeed) {
	if (fSpeed > 0.0f)
		m_speed = fSpeed;
}

void WBulletPhysics::SetGravity(float x, float y, float z) {
	((btDynamicsWorld*)m_dynamicsWorld)->setGravity(btVector3(x, y, z));
}

void WBulletPhysics::SetGravity(WVector3 gravity) {
	((btDynamicsWorld*)m_dynamicsWorld)->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
}


btVector3 WBTConvertVec3(WVector3 vec) {
	return btVector3(vec.x, vec.y, vec.z);
}

WVector3 BTWConvertVec3(btVector3 vec) {
	return WVector3(vec.x(), vec.y(), vec.z());
}

btTransform WBTConverMatrix(WMatrix wmtx) {
	btScalar scalarMtx[16];
	for (int i = 0; i < 16; i++)
		scalarMtx[i] = wmtx.mat[i];
	btTransform mtx;
	mtx.setFromOpenGLMatrix(scalarMtx);
	return mtx;
}

WMatrix BTWConverMatrix(btTransform mtx) {
	btScalar scalarMtx[16];
	mtx.getOpenGLMatrix(scalarMtx);
	WMatrix wmtx;
	for (int i = 0; i < 16; i++)
		wmtx.mat[i] = scalarMtx[i];
	return wmtx;
}

btVector4 WBTConvertVec4(WVector4 vec) {
	return btVector4(vec.x, vec.y, vec.z, vec.w);
}

WVector4 BTWConvertVec4(btVector4 vec) {
	return WVector4(vec.x(), vec.y(), vec.z(), vec.w());
}

btQuaternion WBTConvertQuaternion(WQuaternion quat) {
	return btQuaternion(quat.x, quat.y, quat.z, quat.w);
}

WQuaternion BTWConvertQuaternion(btQuaternion quat) {
	return WQuaternion(quat.x(), quat.y(), quat.z(), quat.w());
}
