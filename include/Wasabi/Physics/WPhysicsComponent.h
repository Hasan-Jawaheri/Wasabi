/** @file WPhysicsComponent.h
 *  @brief Physics component implementation spec
 *
 *  A physics component can be implemented by a library and used by an
 *  application to have physics enabled in it. A physics component needs to
 *  define a small set of functions so they could be incorporated into the
 *  engine. This is done by deriving the WPhysicsComponent class.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "Wasabi/Core/WCore.h"
#include "Wasabi/Physics/WRigidBody.h"

/** Output of a ray cast operation */
typedef struct W_RAYCAST_OUTPUT {
	/** Distance from the ray to the hit target */
	float fDist;
	/** The surface normal at which the hit occurred */
	WVector3 normal;
} W_RAYCAST_OUTPUT;

/**
 * @ingroup engineclass
 * A physics component is a base class that can be derived to supply physics
 * implementation for an application. A physics component provides an interface
 * to manage the basic operations of a physics engine.
 */
class WPhysicsComponent {
public:
	WPhysicsComponent(class Wasabi* app) : m_app(app), RigidBodyManager(nullptr){}

	WRigidBodyManager* RigidBodyManager;

	/**
	 * Initializes the physics engine.
	 * @param debug  If set to true, the physics will run in debug mode
	 * @return Error code, see WError.h
	 */
	virtual WError Initialize(bool debug = false) = 0;

	/**
	 * Frees all resources allocated by the physics engine.
	 */
	virtual void Cleanup() = 0;

	/**
	 * Start the physics simulation.
	 */
	virtual void Start() = 0;

	/**
	 * Stop (pause) the physics simulation.
	 */
	virtual void Stop() = 0;

	/**
	 * Step the physics simulation a certain time forward.
	 * @param deltaTime Time to step the simulation
	 */
	virtual void Step(float deltaTime) = 0;

	/**
	 * This function must return true if called during Step(), and false
	 * otherwise
	 * @return true if the physics simulation is stepping, false otherwise
	 */
	virtual bool Stepping() const = 0;

	/**
	 * Creates a new rigid body instance. This is equivalent to calling
	 * new WRigidBody(app, ID), so care must be taken to free the resource
	 * when done with it.
	 * @param ID  ID for the created rigid body
	 * @return    A newly allocated rigid body instance
	 */
	virtual WRigidBody* CreateRigidBody(uint32_t ID = 0) const = 0;

	/**
	 * Performs a ray cast operation on the physics objects. A ray cast operation
	 * checks if a ray segment (given to the function) intersects any geometry,
	 * as far as the physics engine is concerned.
	 * @param  from Starting point of the ray
	 * @param  to   End point of the ray
	 * @param  out  A pointer to a structure to be filled with extra output
	 *              information
	 * @return      true if the ray intersects some geometry, false otherwise
	 */
	virtual bool RayCast(WVector3 from, WVector3 to,
		W_RAYCAST_OUTPUT* out = nullptr) = 0;

	/**
	 * Sets the simulation speed multiplier
	 * @param fSpeed  Speed multiplier, must be > 0 (1 is normal speed)
	 */
	virtual void SetSpeed(float fSpeed) = 0;

	/**
	 * Sets the physics gravity.
	 * @param x  Gravity on the x-axis
	 * @param y  Gravity on the y-axis
	 * @param z  Gravity on the z-axis
	 */
	virtual void SetGravity(float x, float y, float z) = 0;

	/**
	 * Sets the physics gravity.
	 * @param gravity  Physics gravity vector
	 */
	virtual void SetGravity(WVector3 gravity) = 0;

protected:
	/** A pointer to the Wasabi application */
	class Wasabi* m_app;
};

