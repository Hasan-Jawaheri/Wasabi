#include "WBulletRigidBody.h"
#include "WBulletPhysics.h"
#include "../WRigidBody.h"
#include "../../Core/WCore.h"

WBulletRigidBodyManager::WBulletRigidBodyManager(class Wasabi* const app)
	: WRigidBodyManager(app) {
}

WBulletRigidBodyManager::~WBulletRigidBodyManager() {
}

std::string WBulletRigidBodyManager::GetTypeName() const {
	return "RigidBodyManager";
}

WError WBulletRigidBodyManager::Load() {
	return WError(W_SUCCEEDED);
}

void WBulletRigidBodyManager::Update(float deltaTime) {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < m_entities[i].size(); j++) {
			((WBulletRigidBody*)m_entities[i][j])->Update(deltaTime);
		}
	}
}

WBulletRigidBody::WBulletRigidBody(Wasabi* const app, unsigned int ID) : WRigidBody(app, ID) {
	m_isUpdating = false;
	m_boundObject = nullptr;
	m_boundObjectBase = nullptr;

	m_collisionShape = nullptr;
	m_rigidBody = nullptr;

	app->PhysicsComponent->RigidBodyManager->AddEntity(this);
}

WBulletRigidBody::~WBulletRigidBody() {
	BindObject(nullptr);
	_DestroyResources();

	m_app->PhysicsComponent->RigidBodyManager->RemoveEntity(this);
}

std::string WBulletRigidBody::GetTypeName() const {
	return "RigidBody";
}

void WBulletRigidBody::_DestroyResources() {
	WBulletPhysics* physics = (WBulletPhysics*)m_app->PhysicsComponent;

	if (m_rigidBody) {
		if (m_rigidBody->getMotionState())
			delete m_rigidBody->getMotionState();
		physics->m_dynamicsWorld->removeCollisionObject(m_rigidBody);
	}

	W_SAFE_DELETE(m_rigidBody);
	W_SAFE_DELETE(m_collisionShape);
}

bool WBulletRigidBody::Valid() const {
	return m_rigidBody;
}

WError WBulletRigidBody::Create(float fMass) {
	_DestroyResources();

	WBulletPhysics* physics = (WBulletPhysics*)m_app->PhysicsComponent;

	m_collisionShape = new btBoxShape(btVector3(btScalar(1), btScalar(1), btScalar(1)));

	btTransform groundTransform;
	groundTransform.setIdentity();

	btScalar mass(fMass);

	//rigidbody is dynamic if and only if mass is non zero, otherwise static
	bool isDynamic = mass > 0.0001f;

	btVector3 localInertia(0, 0, 0);
	if (isDynamic)
		m_collisionShape->calculateLocalInertia(mass, localInertia);

	//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, m_collisionShape, localInertia);
	m_rigidBody = new btRigidBody(rbInfo);

	//add the body to the dynamics world
	physics->m_dynamicsWorld->addRigidBody(m_rigidBody);

	return WError(W_SUCCEEDED);
}

void WBulletRigidBody::Update(float deltaTime) {
	m_isUpdating = true;
	if (m_boundObject && m_rigidBody) {
		btQuaternion rotation = m_rigidBody->getOrientation();
		btTransform transformation = m_rigidBody->getWorldTransform();
		btVector3 position = transformation.getOrigin();
		WVector3 wPos = WVector3(position.x(), position.y(), position.z());
		WQuaternion wRot = WQuaternion(rotation.x(), rotation.y(), rotation.z(), rotation.w());
		SetPosition(wPos);
		SetAngle(wRot);

		m_boundObject->SetPosition(wPos);
		m_boundObject->SetAngle(wRot);
	}
	m_isUpdating = false;
}

void WBulletRigidBody::BindObject(WOrientation* obj, WBase* objBase) {
	if (m_boundObjectBase)
		m_boundObjectBase->RemoveReference();
	
	m_boundObject = obj;
	m_boundObjectBase = objBase;

	if (m_boundObjectBase)
		m_boundObjectBase->AddReference();
}

WMatrix WBulletRigidBody::GetWorldMatrix() {
	WMatrix worldM;
	WVector3 _up = GetUVector();
	WVector3 _look = GetLVector();
	WVector3 _right = GetRVector();
	WVector3 _pos = GetPosition();

	//
	//the world matrix is the view matrix's inverse
	//so we build a normal view matrix and invert it
	//

	//build world matrix
	float x = -WVec3Dot(_right, _pos);
	float y = -WVec3Dot(_up, _pos);
	float z = -WVec3Dot(_look, _pos);
	(worldM)(0, 0) = _right.x; (worldM)(0, 1) = _up.x; (worldM)(0, 2) = _look.x; (worldM)(0, 3) = 0.0f;
	(worldM)(1, 0) = _right.y; (worldM)(1, 1) = _up.y; (worldM)(1, 2) = _look.y; (worldM)(1, 3) = 0.0f;
	(worldM)(2, 0) = _right.z; (worldM)(2, 1) = _up.z; (worldM)(2, 2) = _look.z; (worldM)(2, 3) = 0.0f;
	(worldM)(3, 0) = x;        (worldM)(3, 1) = y;     (worldM)(3, 2) = z;       (worldM)(3, 3) = 1.0f;
	return WMatrixInverse(worldM);
}

void WBulletRigidBody::OnStateChange(STATE_CHANGE_TYPE type) {
	if (!m_isUpdating) {
		WMatrix worldM = GetWorldMatrix();
		btScalar scalarMtx[16];
		for (int i = 0; i < 16; i++)
			scalarMtx[i] = worldM.mat[i];
		btTransform mtx;
		mtx.setFromOpenGLMatrix(scalarMtx);
		m_rigidBody->setWorldTransform(mtx);
		m_rigidBody->getMotionState()->setWorldTransform(mtx);
	}
}

