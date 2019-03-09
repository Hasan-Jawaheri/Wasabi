#include "WBulletPhysics.h"

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

struct WBulletPhysicsData {
	bool is_stepping;
	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btBroadphaseInterface* overlappingPairCache;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* dynamicsWorld;
	btAlignedObjectArray<btCollisionShape*> collisionShapes;
};

WBulletPhysics::WBulletPhysics(Wasabi* app) : WPhysicsComponent(app) {
	m_data = new WBulletPhysicsData;
}

WBulletPhysics::~WBulletPhysics() {
	W_SAFE_DELETE(m_data);
}

WError WBulletPhysics::Initialize() {
	///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
	m_data->collisionConfiguration = new btDefaultCollisionConfiguration();

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	m_data->dispatcher = new btCollisionDispatcher(m_data->collisionConfiguration);

	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	m_data->overlappingPairCache = new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	m_data->solver = new btSequentialImpulseConstraintSolver;

	m_data->dynamicsWorld = new btDiscreteDynamicsWorld(m_data->dispatcher, m_data->overlappingPairCache, m_data->solver, m_data->collisionConfiguration);

	m_data->dynamicsWorld->setGravity(btVector3(0, -10, 0));

	///-----initialization_end-----

	//keep track of the shapes, we release memory at exit.
	//make sure to re-use collision shapes among rigid bodies whenever possible!
	//btAlignedObjectArray<btCollisionShape*> collisionShapes;

	///create a few basic rigid bodies

	//the ground is a cube of side 100 at position y = -56.
	//the sphere will hit it at y = -6, with center at -5
	{
		btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));

		m_data->collisionShapes.push_back(groundShape);

		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, -56, 0));

		btScalar mass(0.);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);

		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		//add the body to the dynamics world
		m_data->dynamicsWorld->addRigidBody(body);
	}

	{
		//create a dynamic rigidbody

		btCollisionShape* colShape = new btBoxShape(btVector3(1,1,1));
		//btCollisionShape* colShape = new btSphereShape(btScalar(1.));
		m_data->collisionShapes.push_back(colShape);

		/// Create Dynamic Objects
		btTransform startTransform;
		startTransform.setIdentity();

		btScalar mass(1.f);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);

		startTransform.setOrigin(btVector3(2, 10, 0));

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		m_data->dynamicsWorld->addRigidBody(body);
	}

	return WError(W_SUCCEEDED);
}

void WBulletPhysics::Cleanup() {
	//remove the rigidbodies from the dynamics world and delete them
	for (int i = m_data->dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
		btCollisionObject* obj = m_data->dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState()) {
			delete body->getMotionState();
		}
		m_data->dynamicsWorld->removeCollisionObject(obj);
		delete obj;
	}

	//delete collision shapes
	for (int j = 0; j < m_data->collisionShapes.size(); j++) {
		btCollisionShape* shape = m_data->collisionShapes[j];
		m_data->collisionShapes[j] = 0;
		delete shape;
	}

	//delete dynamics world
	delete m_data->dynamicsWorld;

	//delete solver
	delete m_data->solver;

	//delete broadphase
	delete m_data->overlappingPairCache;

	//delete dispatcher
	delete m_data->dispatcher;

	delete m_data->collisionConfiguration;

	//next line is optional: it will be cleared by the destructor when the array goes out of scope
	m_data->collisionShapes.clear();
}

void WBulletPhysics::Start() {

}

void WBulletPhysics::Stop() {

}

void WBulletPhysics::Step(float deltaTime) {
	m_data->is_stepping = true;

	m_data->dynamicsWorld->stepSimulation(1.f / 60.f, 10);

	//print positions of all objects
	for (int j = m_data->dynamicsWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
		btCollisionObject* obj = m_data->dynamicsWorld->getCollisionObjectArray()[j];
		btRigidBody* body = btRigidBody::upcast(obj);
		btTransform trans;
		if (body && body->getMotionState()) {
			body->getMotionState()->getWorldTransform(trans);
		} else {
			trans = obj->getWorldTransform();
		}
		printf("world pos object %d = %f,%f,%f\n", j, float(trans.getOrigin().getX()), float(trans.getOrigin().getY()), float(trans.getOrigin().getZ()));
	}

	m_data->is_stepping = false;
}

bool WBulletPhysics::Stepping() const {
	return m_data->is_stepping;
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

}

void WBulletPhysics::SetGravity(WVector3 gravity) {

}


