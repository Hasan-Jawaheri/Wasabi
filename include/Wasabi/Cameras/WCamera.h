/** @file WCamera.h
 *  @brief Cameras implementation
 *
 *  A camera in Wasabi is a combination of a view matrix and a projection. A
 *  view matrix is one that shifts objects from their world-space coordinates
 *  to "view-space" coordinates, which means objects would be shifted such that
 *  they remain in the same "view" to the camera but the camera is at the
 *  origin. After applying the view matrix transformation, a projection matrix
 *  can be applied to transform vertices from view-space to screen-space. A
 *  projection matrix controls how far and close objects are seen by the
 *  camera. For example, using a perspective-projection matrix will result in
 *  "realistic" projection such that farther objects appear smaller on screen.
 *  On the other hand, orthogonal project transforms objects disregarding
 *  their distance to the camera.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */
#pragma once

#include "Core/WCore.h"

/** Type of the projection method used by the camera */
enum W_PROJECTIONTYPE {
	PROJECTION_PERSPECTIVE = 0,
	PROJECTION_ORTHOGONAL = 1,
};

/**
 * @ingroup engineclass
 * This implements a camera in Wasabi.
 */
class WCamera : public WBase, public WOrientation {
protected:
	virtual ~WCamera();

public:
	/**
	 * Returns "Camera" string.
	 * @return Returns "Camera" string
	 */
	virtual std::string GetTypeName() const;
	static std::string _GetTypeName();

	WCamera(Wasabi* const app, unsigned int ID = 0);

	/**
	 * This is a callback (inherited from WOrientation) to inform this object of
	 * a change in orientation.
	 * @param type Orientation change type (rotation or motion)
	 */
	void OnStateChange(STATE_CHANGE_TYPE type);

	/**
	 * Update the matrices to match the latest changes. This is automatically
	 * called during rendering of a camera.
	 */
	void UpdateInternals();

	/**
	 * Enables auto aspect ratio calculation when the window/viewport size
	 * changes. Auto aspect ratio is calculated as
	 * viewport_width / viewport_height.
	 */
	void EnableAutoAspect();

	/**
	 * Enables auto aspect ratio calculation when the window/viewport size
	 * changes. Auto aspect ratio is calculated as
	 * viewport_width / viewport_height.
	 */
	void DisableAutoAspect();

	/**
	 * Sets the range in which the camera can see. The camera cannot render
	 * closer to it than min and farther to it than max.
	 * @param min minimum draw distance
	 * @param max maximum draw distance
	 */
	void SetRange(float min, float max);

	/**
	 * Set the FOV (field of view) for the camera.
	 * @param fValue New field of view, in degrees
	 */
	void SetFOV(float fValue);

	/**
	 * Sets the aspect ratio. If auto aspect ratio is enabled, this will be
	 * overridden on the next UpdateInternals() or Render().
	 * @param aspect New aspect ratio
	 */
	void SetAspect(float aspect);

	/**
	 * Sets the projection type for this camera.
	 * @param type Type of projection
	 */
	void SetProjectionType(W_PROJECTIONTYPE type);

	/**
	 * Prepares the camera such that its' matrices can be used to perform
	 * transformations such as object rendering.
	 * @param width  Width of the rendering viewport
	 * @param height Height of the rendering viewport
	 */
	void Render(unsigned int width, unsigned int height);

	/**
	 * Retrieves the current view matrix
	 * @return Current view matrix
	 */
	WMatrix GetViewMatrix() const;

	/**
	 * Retrieves the current projection matrix
	 * @return Current projection matrix
	 */
	/**
	 * Retrieves the current projection matrix
	 * @param  bForceOrtho If set to true, the returned projection matrix will
	 *                     be forcefully set to use PROJECTION_ORTHOGONAL for
	 *                     this call
	 * @return Current projection matrix
	 */
	WMatrix GetProjectionMatrix(bool bForceOrtho = false) const;

	/**
	 * Retrieves the minimum draw distance.
	 * @return Minimum draw distance
	 */
	float GetMinRange() const;

	/**
	 * Retrieves the maximum draw distance.
	 * @return Maximum draw distance
	 */
	float GetMaxRange() const;

	/**
	 * Retrieves the FOV (field of view).
	 * @return FOV (field of view) in degrees
	 */
	float GetFOV() const;

	/**
	 * Retrieves the aspect ratio.
	 * @return Aspect ratio
	 */
	float GetAspect() const;

	/**
	 * Checks if a given point is in the viewing frustum of the camera.
	 * @param  x Point x
	 * @param  y Point y
	 * @param  z Point z
	 * @return   true if (x, y, z) is in the viewing frustum, false otherwise
	 */
	bool CheckPointInFrustum(float x, float y, float z) const;

	/**
	 * Checks if a given point is in the viewing frustum of the camera.
	 * @param  point Point to check
	 * @return   		 true if point is in the viewing frustum, false otherwise
	 */
	bool CheckPointInFrustum(WVector3 point) const;

	/**
	 * Checks if a given cube is in the viewing frustum of the camera.
	 * @param  x      Cube center x
	 * @param  y      Cube center y
	 * @param  z      Cube center z
	 * @param  radius Cube diagonal (distance from the center to a corner)
	 * @return   			true if the cube is in the viewing frustum, false otherwise
	 */
	bool CheckCubeInFrustum(float x, float y, float z, float radius) const;

	/**
	 * Checks if a given cube is in the viewing frustum of the camera.
	 * @param  pos    Cube center
	 * @param  radius Cube diagonal (distance from the center to a corner)
	 * @return   			true if the cube is in the viewing frustum, false otherwise
	 */
	bool CheckCubeInFrustum(WVector3 pos, float radius) const;

	/**
	 * Checks if a given sphere is in the viewing frustum of the camera.
	 * @param  x      Sphere center x
	 * @param  y      Sphere center y
	 * @param  z      Sphere center z
	 * @param  radius Sphere radius
	 * @return        true if the sphere is in the viewing frustum, false
	 *                otherwise
	 */
	bool CheckSphereInFrustum(float x, float y, float z, float radius) const;

	/**
	 * Checks if a given sphere is in the viewing frustum of the camera.
	 * @param  pos    Sphere center
	 * @param  radius Sphere radius
	 * @return        true if the sphere is in the viewing frustum, false
	 *                otherwise
	 */
	bool CheckSphereInFrustum(WVector3 pos, float radius) const;

	/**
	 * Checks if a given box is in the viewing frustum of the camera
	 * @param  x     Box center x
	 * @param  y     Box center y
	 * @param  z     Box center z
	 * @param  xSize Box dimension x from the center to the edge
	 * @param  ySize Box dimension y from the center to the edge
	 * @param  zSize Box dimension z from the center to the edge
	 * @return       true if the box is in the viewing frustum, false otherwise
	 */
	bool CheckBoxInFrustum(float x, float y, float z,
						   float xSize, float ySize, float zSize) const;

	/**
	 * Checks if a given box is in the viewing frustum of the camera
	 * @param  pos  Box center
	 * @param  size Box dimensions from the center to each edge
	 * @return      true if the box is in the viewing frustum, false otherwise
	 */
	bool CheckBoxInFrustum(WVector3 pos, WVector3 size) const;

	/**
	 * Checks if the camera is valid (always true).
	 * @return true
	 */
	virtual bool Valid() const;

private:
	/** true if one of the matrices needs to be updated, false otherwise */
	bool m_bAltered;
	/** Current view matrix */
	WMatrix m_ViewM;
	/** Current projection matrix */
	WMatrix m_ProjM;
	/** Current projection matrix to be used when the user sets bForceOrtho to
	 *  true in GetProjectionMatrix()
	 */
	WMatrix m_orthoMatrix;
	/** Maximum draw distance */
	float m_maxRange;
	/** Minimum draw distance */
	float m_minRange;
	/** FOV in radians */
	float m_fFOV;
	/** Aspect ratio */
	float m_fAspect;
	/** true if auto aspect setting is enabled, false otherwise */
	bool m_bAutoAspect;
	/** Camera projection type */
	W_PROJECTIONTYPE m_projType;
	/** Frustum planes used for determining if objects are within view frustum */
	WPlane m_frustumPlanes[6];
	/** Saved previous viewport width */
	unsigned int m_lastWidth;
	/** Saved previous viewport height */
	unsigned int m_lastHeight;
};

/**
 * @ingroup engineclass
 * Manager class for WCamera.
 */
class WCameraManager : public WManager<WCamera> {
	/**
	 * Returns "Camera" string.
	 * @return Returns "Camera" string
	 */
	virtual std::string GetTypeName() const;

	/** Default camera created automatically */
	WCamera* m_default_camera;

public:
	WCameraManager(Wasabi* const app);
	~WCameraManager();

	/**
	 * Prepare the Camera manager and create the default camera.
	 * @return Error code, see WError.h
	 */
	WError	Load();

	/**
	 * Retrieves a pointer to the default camera.
	 * @return Pointer to the default camera
	 */
	WCamera* GetDefaultCamera() const;
};
