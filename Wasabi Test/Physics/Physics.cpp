#include "Physics.hpp"

PhysicsDemo::PhysicsDemo(Wasabi* const app) : WTestState(app) {
}

void PhysicsDemo::Load() {
	m_app->PhysicsComponent->Start();

	WGeometry* ground = new WGeometry(m_app);
	ground->CreatePlain(10.0f, 0, 0);
	m_ground = new WObject(m_app);
	m_ground->SetGeometry(ground);
	ground->RemoveReference();
	m_groundRB = m_app->PhysicsComponent->CreateRigidBody();
	m_groundRB->BindObject(m_ground, m_ground);
	m_groundRB->Create(W_RIGID_BODY_CREATE_INFO::ForObject(m_ground, 0.0f));

	WGeometry* ball = new WGeometry(m_app);
	ball->CreateSphere(1.0f);
	m_ball = new WObject(m_app);
	m_ball->SetPosition(4, 5, 0);
	m_ball->SetGeometry(ball);
	ball->RemoveReference();
	m_ballRB = m_app->PhysicsComponent->CreateRigidBody();
	m_ballRB->BindObject(m_ball, m_ball);
	m_ballRB->Create(W_RIGID_BODY_CREATE_INFO::ForSphere(1, 0.1f, m_ball));
}

void PhysicsDemo::Update(float fDeltaTime) {
	static bool isDown = false;
	if (!m_app->WindowAndInputComponent->KeyDown(' '))
		isDown = true;
	else if (isDown) {
		isDown = false;
		m_ballRB->ApplyImpulse(WVector3(0, 1, 0));
	}
}

void PhysicsDemo::Cleanup() {
	W_SAFE_REMOVEREF(m_ball);
	W_SAFE_REMOVEREF(m_ground);
	W_SAFE_REMOVEREF(m_ballRB);
	W_SAFE_REMOVEREF(m_groundRB);
}