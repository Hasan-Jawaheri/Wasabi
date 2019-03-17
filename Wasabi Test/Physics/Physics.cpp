#include "Physics.hpp"

PhysicsDemo::PhysicsDemo(Wasabi* const app) : WTestState(app) {
}

void PhysicsDemo::Load() {
	m_app->PhysicsComponent->Start();

	WGeometry* ground = new WGeometry(m_app);
	ground->CreatePlain(100.0f, 2, 2);
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
	m_ballRB->Create(W_RIGID_BODY_CREATE_INFO::ForSphere(1, 20.0f, m_ball));

	m_ballRB->SetLinearDamping(0.8f);
	m_ballRB->SetAngularDamping(0.8f);
	m_ballRB->SetFriction(1.0f);
	m_groundRB->SetFriction(1.0f);

	m_app->PhysicsComponent->SetGravity(0, -40, 0);
}

void PhysicsDemo::Update(float fDeltaTime) {
	WasabiTester* app = (WasabiTester*)m_app;
	WVector3 rbPos = m_ballRB->GetPosition();
	WVector3 cameraPos = app->GetCameraPosition();
	if (WVec3LengthSq(rbPos - cameraPos) > 1)
		app->SetCameraPosition(cameraPos + (rbPos - cameraPos) * 10.0f * fDeltaTime);

	WVector3 direction = WVec3TransformNormal(WVector3(0, 0, 1), WRotationMatrixY(W_DEGTORAD(app->GetYawAngle())));
	WVector3 right = WVec3TransformNormal(WVector3(1, 0, 0), WRotationMatrixY(W_DEGTORAD(app->GetYawAngle())));

	WVector3 inputDirection(0, 0, 0);
	if (m_app->WindowAndInputComponent->KeyDown('W'))
		inputDirection += direction;
	if (m_app->WindowAndInputComponent->KeyDown('S'))
		inputDirection -= direction;
	if (m_app->WindowAndInputComponent->KeyDown('A'))
		inputDirection -= right;
	if (m_app->WindowAndInputComponent->KeyDown('D'))
		inputDirection += right;

	bool isGrounded = m_app->PhysicsComponent->RayCast(rbPos + WVector3(0, -0.98, 0), rbPos + WVector3(0, -1.2, 0));
	bool isDirection = WVec3LengthSq(inputDirection) > 0.1f;

	static bool isDown = false;
	static bool didDash = true;
	if (!m_app->WindowAndInputComponent->KeyDown(' '))
		isDown = true;
	else if (isDown) {
		isDown = false;
		if (isGrounded) {
			m_ballRB->ApplyImpulse(WVector3(0, 500, 0));
			didDash = false;
		} else if (!didDash && isDirection) {
			m_ballRB->ApplyImpulse(inputDirection * 500.0f);
			didDash = true;
		}
	}

	if (isDirection && isGrounded) {
		inputDirection = WVec3Normalize(inputDirection);
		m_ballRB->ApplyForce(inputDirection * 400.0f , WVector3(0, 0.3, 0));
	}

	if (m_app->WindowAndInputComponent->KeyDown('R')) {
		m_ballRB->SetPosition(0, 5, 0);
		m_ballRB->SetLinearVelocity(WVector3(0, -100, 0));
		m_ballRB->SetAngularVelocity(WVector3(0, 0, 0));
	}
}

void PhysicsDemo::Cleanup() {
	W_SAFE_REMOVEREF(m_ball);
	W_SAFE_REMOVEREF(m_ground);
	W_SAFE_REMOVEREF(m_ballRB);
	W_SAFE_REMOVEREF(m_groundRB);
}