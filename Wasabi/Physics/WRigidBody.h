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

/** Shape of a rigid body in the physics simulation */
enum W_RIGID_BODY_SHAPE {
	/** A convex shape is an outer-shape of a geometry (no inner cavities) */
	RIGID_BODY_SHAPE_CONVEX = 0,
	/** A mesh shape is a complex geometry, can only be used for static RBs */
	RIGID_BODY_SHAPE_MESH = 1,
	/** A cube mesh */
	RIGID_BODY_SHAPE_CUBE = 2,
	/** A sphere mesh */
	RIGID_BODY_SHAPE_SPHERE = 3,
	/** A capsule mesh */
	RIGID_BODY_SHAPE_CAPSULE = 4,
	/** A cylinder mesh */
	RIGID_BODY_SHAPE_CYLINDER = 5,
	/** A cone mesh */
	RIGID_BODY_SHAPE_CONE = 6,
};

/** Properties to create a rigid body with */
struct W_RIGID_BODY_CREATE_INFO {
	/** General properties */
	/** Mass of the rigid body, a mass of 0 makes it static */
	float mass;

	/** Shape properties */
	/** Shape of the rigid body */
	W_RIGID_BODY_SHAPE shape;
	/** 3D dimensions of the shape */
	WVector3 dimensions;
	/** An optional geometry object to use (only when shape is _CONVEX) */
	class WGeometry* geometry;
	/** If shape is mesh, this is whether the geometry uses triangle list or strip*/
	bool isTriangleList;

	/** Initial position/orientation */
	/** Initial position */
	WVector3 initialPosition;
	/** Initial rotation quaternion */
	WQuaternion initialRotation;
	/** An optional WOrientation to copy position/rotation from */
	class WOrientation* orientation;

	static W_RIGID_BODY_CREATE_INFO ForComplexObject(class WObject* object, bool isTriangleList = true);
	static W_RIGID_BODY_CREATE_INFO ForComplexGeometry(class WGeometry* geometry, bool isTriangleList = true, class WOrientation* orientation = nullptr, WVector3 pos = WVector3(), WQuaternion rot = WQuaternion());
	static W_RIGID_BODY_CREATE_INFO ForObject(class WObject* object, float mass);
	static W_RIGID_BODY_CREATE_INFO ForGeometry(class WGeometry* geometry, float mass, class WOrientation* orientation = nullptr, WVector3 pos = WVector3(), WQuaternion rot = WQuaternion());
	static W_RIGID_BODY_CREATE_INFO ForCube(WVector3 dimensions, float mass, class WOrientation* orientation = nullptr, WVector3 pos = WVector3(), WQuaternion rot = WQuaternion());
	static W_RIGID_BODY_CREATE_INFO ForSphere(float radius, float mass, class WOrientation* orientation = nullptr, WVector3 pos = WVector3(), WQuaternion rot = WQuaternion());
	static W_RIGID_BODY_CREATE_INFO ForCapsule(float height, float radius, float mass, class WOrientation* orientation = nullptr, WVector3 pos = WVector3(), WQuaternion rot = WQuaternion());
	static W_RIGID_BODY_CREATE_INFO ForCylinder(float height, float radius, float mass, class WOrientation* orientation = nullptr, WVector3 pos = WVector3(), WQuaternion rot = WQuaternion());
	static W_RIGID_BODY_CREATE_INFO ForCone(float height, float radius, float mass, class WOrientation* orientation = nullptr, WVector3 pos = WVector3(), WQuaternion rot = WQuaternion());
};

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
	 * @param createInfo  Various parameters for the rigid body creation
	 * @return Error code, see WError.h
	 */
	virtual WError Create(W_RIGID_BODY_CREATE_INFO createInfo) = 0;

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

	/** Sets the linear velocity of the rigid body */
	virtual void SetLinearVelocity(WVector3 vel) = 0;
	/** Sets the angular velocity of the rigid body */
	virtual void SetAngularVelocity(WVector3 vel) = 0;
	/** Sets the linear damping of the rigid body */
	virtual void SetLinearDamping(float power) = 0;
	/** Sets the angular damping of the rigid body */
	virtual void SetAngularDamping(float power) = 0;
	/** Sets the bouncing power (restitution) of the rigid body */
	virtual void SetBouncingPower(float bouncing) = 0;
	/** Sets the mass of the rigid body */
	virtual void SetMass(float mass) = 0;
	/** Sets the center of mass of the rigid body */
	virtual void SetMassCenter(float x, float y, float z) = 0;
	/** Sets the friction of the rigid body */
	virtual void SetFriction(float friction) = 0;
	/** Applies force to the center of the rigid body */
	virtual void ApplyForce(WVector3 force) = 0;
	/** Applies force to the rigid body relative to a coordinate
	    (will also apply rotation) */
	virtual void ApplyForce(WVector3 force, WVector3 relative_pos) = 0;
	/** Applies an impulse to the center of the rigid body */
	virtual void ApplyImpulse(WVector3 impulse) = 0;
	/** Applies an impulse to the rigid body relative to a coordinate */
	virtual void ApplyImpulse(WVector3 impulse, WVector3 relative_pos) = 0;
	/** Applies torque to the rigid body */
	virtual void ApplyTorque(WVector3 torque) = 0;
	/** Retrieves the linear velocity of the rigid body */
	virtual WVector3 getLinearVelocity() const = 0;
	/** Retrieves the angular velocity of the rigid body */
	virtual WVector3 getAngularVelocity() const = 0;
	/** Retrieves the total force currently applied on the rigid body */
	virtual WVector3 getTotalForce() const = 0;
	/** Retrieves the total torque currently applied on the rigid body */
	virtual WVector3 getTotalTorque() const = 0;

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
