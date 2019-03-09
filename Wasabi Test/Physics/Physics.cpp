#include "Physics.hpp"

PhysicsDemo::PhysicsDemo(Wasabi* const app) : WTestState(app) {
}

void PhysicsDemo::Load() {
	m_app->PhysicsComponent->Start();
}

void PhysicsDemo::Update(float fDeltaTime) {
}

void PhysicsDemo::Cleanup() {
}