/** @file WOrientation.h
 *  @brief An orientation management class
 *
 *  A WOrientation can be inherited by any class to have attributes that
 *  represent a body in 3D space.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "Core/WMath.h"
#include <type_traits>

/** Type of an orientation change happening to a WOrientation */
enum STATE_CHANGE_TYPE { CHANGE_MOTION = 1, CHANGE_ROTATION = 2 };

inline STATE_CHANGE_TYPE operator | (STATE_CHANGE_TYPE lhs, STATE_CHANGE_TYPE rhs) {
	using T = std::underlying_type_t <STATE_CHANGE_TYPE>;
	return static_cast<STATE_CHANGE_TYPE>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline STATE_CHANGE_TYPE operator & (STATE_CHANGE_TYPE lhs, STATE_CHANGE_TYPE rhs) {
	using T = std::underlying_type_t <STATE_CHANGE_TYPE>;
	return static_cast<STATE_CHANGE_TYPE>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline STATE_CHANGE_TYPE& operator |= (STATE_CHANGE_TYPE& lhs, STATE_CHANGE_TYPE rhs) {
	lhs = lhs | rhs;
	return lhs;
}

inline STATE_CHANGE_TYPE& operator &= (STATE_CHANGE_TYPE& lhs, STATE_CHANGE_TYPE rhs) {
	lhs = lhs & rhs;
	return lhs;
}

/**
 * @ingroup engineclass
 *
 * A WOrientation represents a hypothetical object in 3D space. To represent
 * position, the class uses a 3D vector. To represent rotation, the class uses
 * a set of 3 vectors:
 * * U (up) vector: Points where the object's "up" direction is
 * * L (look) vector: Points to where the object is "looking"
 * * R (right) vector: Points to the right of the object
 * The 3 vectors are always perpendicular to one another. Together, they can
 * accurately represent an object's orientation without conflicts.
 */
class WOrientation {
public:
	WOrientation();
	~WOrientation();

	/**
	 * Sets the position of this entity. This will result in calling
	 * OnStateChange(CHANGE_MOTION).
	 * @param x New x position
	 * @param y New y position
	 * @param z New z position
	 */
	void SetPosition(float x, float y, float z);

	/**
	 * Sets the position of this entity.
	 * @param pos New position
	 */
	void SetPosition(const WVector3 pos);

	/**
	 * Sets the entity's orientation to point towards a 3D point. This will
	 * result in calling OnStateChange(CHANGE_ROTATION).
	 * @param x x coordinate of the point to look at
	 * @param y y coordinate of the point to look at
	 * @param z z coordinate of the point to look at
	 */
	void Point(float x, float y, float z);

	/**
	 * Sets the entity's orientation to point towards a 3D point. This will
	 * result in calling OnStateChange(CHANGE_ROTATION).
	 * @param target coordinate of the point to look at
	 */
	void Point(WVector3 target);

	/**
	 * Sets the rotation of the entity to match that of a quaternion. This will
	 * result in calling OnStateChange(CHANGE_ROTATION).
	 * @param quat Quaternion to match
	 */
	void SetAngle(WQuaternion quat);

	/**
	 * Sets the orientation of this entity to match that of another. This will
	 * result in calling OnStateChange(CHANGE_ROTATION).
	 * @param device The orientation to match
	 */
	void SetToRotation(const WOrientation* const device);

	/**
	 * Explicitly sets the up, look and right vectors. This will result in
	 * calling OnStateChange(CHANGE_ROTATION).
	 * @param up    New up vector
	 * @param look  New look vector
	 * @param right New right vector
	 */
	void SetULRVectors(WVector3 up, WVector3 look, WVector3 right);

	/**
	 * Sets the orientation & position to match that of a given transformation
	 * matrix.
	 * @param mtx  Transformation matrix
	 */
	void SetToTransformation(WMatrix mtx);

	/**
	 * Performs a rotation around the up axis of this entity. This will
	 * result in OnOrientationChange(CHANGE_ROTATION) to be called.
	 * @param angle Angle of rotation, in degrees
	 */
	void Yaw(float angle);

	/**
	 * Performs a rotation around the look axis of this entity. This will
	 * result in OnOrientationChange(CHANGE_ROTATION) to be called.
	 * @param angle Angle of rotation, in degrees
	 */
	void Roll(float angle);
	
	/**
	 * Performs a rotation around the right axis of this entity. This will
	 * result in OnOrientationChange(CHANGE_ROTATION) to be called.
	 * @param angle Angle of rotation, in degrees
	 */
	void Pitch(float angle);

	/**
	 * Moves the entity forward. This will result in
	 * OnOrientationChange(CHANGE_MOTION) to be called.
	 * @param units Units to move by
	 */
	void Move(float units);

	/**
	 * Moves the entity to the right. This will result in
	 * OnOrientationChange(CHANGE_MOTION) to be called.
	 * @param units Units to move by
	 */
	void Strafe(float units);

	/**
	 * Moves the entity up. This will result in
	 * OnOrientationChange(CHANGE_MOTION) to be called.
	 * @param units Units to move by
	 */
	void Fly(float units);

	/**
	 * Retrieves the X position of the entity.
	 * @return The X position of the entity
	 */
	float GetPositionX() const;

	/**
	 * Retrieves the Y position of the entity.
	 * @return The Y position of the entity
	 */
	float GetPositionY() const;

	/**
	 * Retrieves the Z position of the entity.
	 * @return The Z position of the entity
	 */
	float GetPositionZ() const;

	/**
	 * Retrieves the position of the entity.
	 * @return Position of the entity in 3D space
	 */
	WVector3 GetPosition() const;

	/**
	 * Gets the angle between the look vector (projected onto the yz plane) and
	 * the world z axis (0, 0, 1).
	 * @return X angle of the entity
	 */
	float GetAngleX() const;

	/**
	 * Gets the angle between the look vector (projected onto the xz plane) and
	 * the world z axis (0, 0, 1).
	 * @return Y angle of the entity
	 */
	float GetAngleY() const;

	/**
	 * Gets the angle between the up vector (projected onto the xy plane) and
	 * the world y axis (0, 1, 0).
	 * @return Z angle of the entity
	 */
	float GetAngleZ() const;

	/**
	 * @return A quaternion representing the rotation of the entity
	 */
	WQuaternion GetRotation() const;

	/**
	 * Retrieves the up vector of the entity.
	 * @return The up vector of the entity
	 */
	WVector3 GetUVector() const;

	/**
	 * Retrieves the look vector of the entity.
	 * @return The look vector of the entity
	 */
	WVector3 GetLVector() const;

	/**
	 * Retrieves the right vector of the entity.
	 * @return The right vector of the entity
	 */
	WVector3 GetRVector() const;

	/**
	 * Binds this entity to the matrix provided. A WOrientation that is bound
	 * to a matrix may choose to alter the final matrix it produces (multiply
	 * it by the binding matrix) to achieve the effect of being "bound" or
	 * "stuck" to something else as it moves.
	 * @param mtx Matrix to bind to
	 */
	virtual void SetBindingMatrix(WMatrix mtx);

	/**
	 * Removes or disables the current binding
	 */
	void RemoveBinding();

	/**
	 * Retrieves the binding matrix. This can be used to multiply a world matrix
	 * corresponding to this WOrientation to produce the effect of being "bound"
	 * or "stuck" to something else as it moves.
	 * @return The binding matrix
	 */
	WMatrix GetBindingMatrix() const;

	/**
	 * Checks whether or not the entity should be bound to a matrix or not.
	 * @return true if the entity is bound, false otherwise
	 */
	bool IsBound() const;

	/**
	 * A callback called by this class when the entity changes its position or
	 * orientation.
	 * @param type Orientation change type
	 */
	virtual void OnStateChange(STATE_CHANGE_TYPE type);

	/**
	 * This function recomputes the transformation matrix for this orientation.
	 * Most objects inheriting from WOrientation have their own GetWorldMatrix
	 * functions that are more efficient to use (since they cache the matrix).
	 * @return returns a newly computed matrix corresponding to this orientation
	 */
	WMatrix ComputeTransformation() const;

	/**
	 * This function recomputes the transformation matrix for this orientation.
	 * Most objects inheriting from WOrientation have their own GetWorldMatrix
	 * functions that are more efficient to use (since they cache the matrix).
	 * @return returns a newly computed matrix corresponding to this orientation
	 */
	WMatrix ComputeInverseTransformation() const;

private:
	/** Position of this entity */
	WVector3 m_pos;
	/** The right vector of this entity */
	WVector3 m_right;
	/** The up vector of this entity */
	WVector3 m_up;
	/** The look vector of this entity */
	WVector3 m_look;
	/** true of the entity is bound, false otherwise */
	bool m_bBind;
	/** Binding matrix, should be valid is m_bBind is true */
	WMatrix m_bindMtx;
};
