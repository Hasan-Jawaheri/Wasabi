/** @file WRigidBody.h
 *  @brief Rigid Body interface to be implemented by physics implementations
 *         such as Havok or Bullet
 *
 *  Rigid bodies are the main way to connect objects to the physics simulation
 *  such that they are fully controlled by the simulation instead of directly
 *  modifying the objects' locations or orientations.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"

 /**
  * @ingroup engineclass
  * This is an interface for implementing a rigid body.
  */
class WRigidBody: public WBase, public WOrientation {
public:
	WRigidBody(class Wasabi* const app, unsigned int ID = 0) : WBase(app, ID) {}
	virtual ~WRigidBody() {}

	/**
	 * Must be implemented to initialize the rigid body.
	 * @return Error code, see WError.h
	 */
	virtual WError Create(float fMass) = 0;

	/**
	 * Called by WBulletPhysics after stepping the physics simulation to
	 * update the rigid bodies.
	 * @param deltaTime  Time elapsed since last frame
	 */
	virtual void Update(float deltaTime) = 0;

	/**
	 * Must be implemented to attach a WOrientation to this rigid body
	 * @param obj     A WOrientation (WObject, WCamera, etc...) to attach
	 * @param objBase An optional (but highly recommended) parameter to
	 *                specify the WBase* pointer that obj is associated
	 *                with. This will cause the rigid body to hold a reference
	 *                to the object until unbound.
	 */
	virtual void BindObject(WOrientation* obj, WBase* objBase = nullptr) = 0;

	virtual void OnStateChange(STATE_CHANGE_TYPE type) = 0;
};

/**
 * @ingroup engineclass
 * Manager class for WRigidBody.
 */
class WRigidBodyManager : public WManager<WRigidBody> {
public:
	WRigidBodyManager(class Wasabi* const app) : WManager<WRigidBody>(app) {}
	~WRigidBodyManager() {}
};
