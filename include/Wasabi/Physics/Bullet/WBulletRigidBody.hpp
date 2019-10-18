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

#include "Wasabi/Physics/WRigidBody.h"
#include "Wasabi/Core/WCore.h"

 /**
  * @ingroup engineclass
  * This represents a rigid body and is responsible for integrating it with
  * the Bullet physics simulation.
  */
class WBulletRigidBody : public WRigidBody {
protected:
	virtual ~WBulletRigidBody();

public:
	/**
	 * Returns "RigidBody" string.
	 * @return Returns "RigidBody" string
	 */
	static std::string _GetTypeName();
	virtual std::string GetTypeName() const override;
	virtual void SetID(uint32_t newID) override;
	virtual void SetName(std::string newName) override;

	WBulletRigidBody(class Wasabi* const app, uint32_t ID = 0);

	virtual WError Create(W_RIGID_BODY_CREATE_INFO createInfo, bool bSaveInfo = false) override;

	virtual void Update(float deltaTime) override;

	virtual void BindObject(WOrientation* obj, WBase* objBase = nullptr) override;

	virtual void SetLinearVelocity(WVector3 vel) override;
	virtual void SetAngularVelocity(WVector3 vel) override;
	virtual void SetLinearDamping(float power) override;
	virtual void SetAngularDamping(float power) override;
	virtual void SetBouncingPower(float bouncing) override;
	virtual void SetMass(float mass) override;
	virtual void SetMassCenter(float x, float y, float z) override;
	virtual void SetFriction(float friction) override;
	virtual void ApplyForce(WVector3 force) override;
	virtual void ApplyForce(WVector3 force, WVector3 relative_pos) override;
	virtual void ApplyImpulse(WVector3 impulse) override;
	virtual void ApplyImpulse(WVector3 impulse, WVector3 relative_pos) override;
	virtual void ApplyTorque(WVector3 torque) override;
	virtual WVector3 getLinearVelocity() const override;
	virtual WVector3 getAngularVelocity() const override;
	virtual WVector3 getTotalForce() const override;
	virtual WVector3 getTotalTorque() const override;

	/**
	 * Retrieves the world matrix for the rigid body.
	 * @return World matrix for the rigid body
	 */
	WMatrix GetWorldMatrix();

	/**
	 * @return Whether or not the rigid body will render properly
	 */
	virtual bool Valid() const override;

	virtual void OnStateChange(STATE_CHANGE_TYPE type) override;

	static std::vector<void*> LoadArgs(bool bSaveInfo = false);
	virtual WError SaveToStream(WFile* file, std::ostream& outputStream) override;
	virtual WError LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args, std::string nameSuffix) override;

private:
	/** Set to true while inside Update() */
	bool m_isUpdating;

	/** Object bound to this rigid body */
	WOrientation* m_boundObject;
	/** WBase associated with m_boundObject to hold its reference */
	WBase* m_boundObjectBase;

	/** Collision shape used by Bullet */
	void* m_collisionShape;
	/** Bullet's rigid body */
	void* m_rigidBody;

	/** If Create is called with bSaveInfo set to true, this will point to
	 *  a an allocated copy of it. the WOrientation* part will always be null
	 *  and the geometry will be kept and will have a reference held (so it
	 *  must eventually be freed)
	 */
	W_RIGID_BODY_CREATE_INFO* m_savedCreateInfo;

	/**
	 * Destroys all resources held by this rigid body
	 */
	void _DestroyResources();
};

/**
 * @ingroup engineclass
 * Bullet's manager class for WRigidBody.
 */
class WBulletRigidBodyManager : public WRigidBodyManager {
	/**
	 * Returns "RigidBody" string.
	 * @return Returns "RigidBody" string
	 */
	virtual std::string GetTypeName() const;

public:
	WBulletRigidBodyManager(class Wasabi* const app);
	~WBulletRigidBodyManager();

	/**
	 * Called by WBulletPhysics after stepping the physics simulation to
	 * update the rigid bodies.
	 * @param deltaTime  Time elapsed since last frame
	 */
	void Update(float deltaTime);
};
