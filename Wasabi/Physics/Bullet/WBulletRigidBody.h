/** @file WBulletRigidBody.h
 *  @brief Rigid Body system for the Bullet physics implementation
 *
 *  Rigid bodies are the main way to connect objects to the physics simulation
 *  such that they are fully controlled by the simulation instead of directly
 *  modifying the objects' locations or orientations.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../WRigidBody.h"
#include "../../Core/WCore.h"

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

 /**
  * @ingroup engineclass
  * This represents a rigid body and is responsible for integrating it with
  * the Bullet physics simulation.
  */
class WBulletRigidBody : public WRigidBody {
	/**
	 * Returns "RigidBody" string.
	 * @return Returns "RigidBody" string
	 */
	virtual std::string GetTypeName() const;

public:
	WBulletRigidBody(class Wasabi* const app, unsigned int ID = 0);
	virtual ~WBulletRigidBody();

	virtual WError Create(float fMass);

	virtual void Update(float deltaTime);

	virtual void BindObject(WOrientation* obj, WBase* objBase = nullptr);

	/**
	 * Retrieves the world matrix for the rigid body.
	 * @return World matrix for the rigid body
	 */
	WMatrix GetWorldMatrix();

	/**
	 * @return Whether or not the rigid body will render properly
	 */
	virtual bool Valid() const;

	virtual void OnStateChange(STATE_CHANGE_TYPE type);

private:
	/** Set to true while inside Update() */
	bool m_isUpdating;

	/** Object bound to this rigid body */
	WOrientation* m_boundObject;
	/** WBase associated with m_boundObject to hold its reference */
	WBase* m_boundObjectBase;

	/** Collision shape used by Bullet */
	btCollisionShape* m_collisionShape;
	/** Bullet's rigid body */
	btRigidBody* m_rigidBody;

	/**
	 * Destroys all resources held by this rigid body
	 */
	void _DestroyResources();
};

/**
 * @ingroup engineclass
 * Manager class for WRigidBody.
 */
class WBulletRigidBodyManager : public WRigidBodyManager {
	friend class WRigidBody;

	/**
	 * Returns "RigidBody" string.
	 * @return Returns "RigidBody" string
	 */
	virtual std::string GetTypeName() const;

public:
	WBulletRigidBodyManager(class Wasabi* const app);
	~WBulletRigidBodyManager();

	/**
	 * Loads the manager.
	 * @return Error code, see WError.h
	 */
	WError Load();

	/**
	 * Called by WBulletPhysics after stepping the physics simulation to
	 * update the rigid bodies.
	 * @param deltaTime  Time elapsed since last frame
	 */
	void Update(float deltaTime);
};
