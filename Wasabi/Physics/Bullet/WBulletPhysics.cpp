#include "WBulletPhysics.h"
#include "WBulletRigidBody.h"

WBulletPhysics::WBulletPhysics(Wasabi* app) : WPhysicsComponent(app) {
	RigidBodyManager = new WBulletRigidBodyManager(app);

	m_collisionConfiguration = nullptr;
	m_dispatcher = nullptr;
	m_overlappingPairCache = nullptr;
	m_solver = nullptr;
	m_dynamicsWorld = nullptr;
}

WBulletPhysics::~WBulletPhysics() {
	Cleanup();
}

WError WBulletPhysics::Initialize() {
	///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
	m_collisionConfiguration = new btDefaultCollisionConfiguration();

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);

	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	m_overlappingPairCache = new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	m_solver = new btSequentialImpulseConstraintSolver;

	m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_overlappingPairCache, m_solver, m_collisionConfiguration);

	SetGravity(0, -10, 0);

	return WError(W_SUCCEEDED);
}

void WBulletPhysics::Cleanup() {
	W_SAFE_DELETE(RigidBodyManager);

	//delete dynamics world
	delete m_dynamicsWorld;

	//delete solver
	delete m_solver;

	//delete broadphase
	delete m_overlappingPairCache;

	//delete dispatcher
	delete m_dispatcher;

	delete m_collisionConfiguration;
}

void WBulletPhysics::Start() {

}

void WBulletPhysics::Stop() {

}

void WBulletPhysics::Step(float deltaTime) {
	m_isStepping = true;

	m_dynamicsWorld->stepSimulation(deltaTime);
	((WBulletRigidBodyManager*)RigidBodyManager)->Update(deltaTime);

	m_isStepping = false;
}

bool WBulletPhysics::Stepping() const {
	return m_isStepping;
}

WRigidBody* WBulletPhysics::CreateRigidBody(unsigned int ID) const {
	return new WBulletRigidBody(m_app, ID);
}

bool WBulletPhysics::RayCast(WVector3 from, WVector3 to) {
	return false;
}

bool WBulletPhysics::RayCast(WVector3 from, WVector3 to, W_RAYCAST_OUTPUT* out) {
	return false;
}

void WBulletPhysics::SetSpeed(float fSpeed) {

}

void WBulletPhysics::SetGravity(float x, float y, float z) {
	m_dynamicsWorld->setGravity(btVector3(x, y, z));
}

void WBulletPhysics::SetGravity(WVector3 gravity) {
	m_dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
}


