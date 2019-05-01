#include "Physics.hpp"

WPhysicsComponent* PhysicsDemo::CreatePhysicsComponent() {
	WBulletPhysics* physics = new WBulletPhysics(m_app);
	m_app->engineParams["maxBulletDebugLines"] = (void*)10000;
	WError werr = physics->Initialize(true);
	if (!werr)
		W_SAFE_DELETE(physics);
	return physics;
}

PhysicsDemo::PhysicsDemo(Wasabi* const app) : WTestState(app) {
}

void PhysicsDemo::Load() {
	m_app->PhysicsComponent->Start();

	WGeometry* ground = new WGeometry(m_app);
	ground->CreatePlain(100.0f, 2, 2);

	m_ground = m_app->ObjectManager->CreateObject();
	m_ground->SetGeometry(ground);
	ground->RemoveReference();
	m_groundRB = m_app->PhysicsComponent->CreateRigidBody();
	m_groundRB->BindObject(m_ground, m_ground);
	m_groundRB->Create(W_RIGID_BODY_CREATE_INFO::ForComplexObject(m_ground));

	WGeometry* ball = new WGeometry(m_app);
	ball->CreateSphere(1.0f);
	m_ball = m_app->ObjectManager->CreateObject();
	m_ball->SetPosition(ground->GetMaxPoint().x - 20, ground->GetMaxPoint().y + 10, 0);
	m_ball->SetGeometry(ball);
	ball->RemoveReference();
	m_ballRB = m_app->PhysicsComponent->CreateRigidBody();
	m_ballRB->BindObject(m_ball, m_ball);
	m_ballRB->Create(W_RIGID_BODY_CREATE_INFO::ForSphere(1, 20.0f, m_ball));

	m_ballRB->SetLinearDamping(0.8f);
	m_ballRB->SetAngularDamping(0.6f);
	m_ballRB->SetFriction(1.0f);
	m_groundRB->SetFriction(1.0f);
	m_ballRB->SetBouncingPower(0.2f);

	m_app->PhysicsComponent->SetGravity(0, -40, 0);
}

void PhysicsDemo::Update(float fDeltaTime) {
	static bool isDown = false;
	static bool didDash = true;
	static WVector3 jumpDirection;

	WasabiTester* app = (WasabiTester*)m_app;
	WVector3 rbPos = m_ballRB->GetPosition();
	WVector3 cameraPos = app->GetCameraPosition();
	if (WVec3LengthSq(rbPos - cameraPos) > 0.1)
		app->SetCameraPosition(cameraPos + (rbPos - cameraPos) * 10.0f * fDeltaTime);

	WVector3 direction = WVec3TransformNormal(WVector3(0, 0, 1), WRotationMatrixY(W_DEGTORAD(app->GetYawAngle())));
	WVector3 right = WVec3TransformNormal(WVector3(1, 0, 0), WRotationMatrixY(W_DEGTORAD(app->GetYawAngle())));

	bool isGrounded = m_app->PhysicsComponent->RayCast(rbPos + WVector3(0, -0.98, 0), rbPos + WVector3(0, -1.2, 0));

	WVector3 inputDirection(0, 0, 0);
	if (m_app->WindowAndInputComponent->KeyDown('W'))
		inputDirection += direction;
	if (m_app->WindowAndInputComponent->KeyDown('S'))
		inputDirection -= direction;
	if (m_app->WindowAndInputComponent->KeyDown('A'))
		inputDirection -= right;
	if (m_app->WindowAndInputComponent->KeyDown('D'))
		inputDirection += right;

	if (isGrounded)
		m_ballRB->SetLinearDamping(0.8f);
	else
		m_ballRB->SetLinearDamping(0.2f);

	bool isDirection = WVec3LengthSq(inputDirection) > 0.1f;

	if (!m_app->WindowAndInputComponent->KeyDown(' '))
		isDown = true;
	else if (isDown) {
		isDown = false;
		if (isGrounded) {
			m_ballRB->SetLinearDamping(0.2f);
			m_ballRB->ApplyImpulse(WVector3(0, 300, 0));
			jumpDirection = inputDirection;
			didDash = false;
		} else if (!didDash && isDirection) {
			m_ballRB->ApplyImpulse(inputDirection * 300.0f);
			didDash = true;
		}
	}

	if (isDirection && isGrounded) {
		inputDirection = WVec3Normalize(inputDirection);
		m_ballRB->ApplyForce(inputDirection * 500.0f , WVector3(0, 0.3, 0));
	}

	if (m_app->WindowAndInputComponent->KeyDown('R')) {
		m_ballRB->SetPosition(m_ground->GetGeometry()->GetMaxPoint().x - 20, m_ground->GetGeometry()->GetMaxPoint().y + 10, 0);
		m_ballRB->SetLinearVelocity(WVector3(0, 0, 0));
		//m_ballRB->SetAngularVelocity(WVector3(0, 0, 0));
	}
}

void PhysicsDemo::Cleanup() {
	W_SAFE_REMOVEREF(m_ball);
	W_SAFE_REMOVEREF(m_ground);
	W_SAFE_REMOVEREF(m_ballRB);
	W_SAFE_REMOVEREF(m_groundRB);
}