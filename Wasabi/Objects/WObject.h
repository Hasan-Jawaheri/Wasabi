/** @file WObject.h
 *  @brief Implementation of rendered objects
 *
 *  An object in Wasabi is the normal way to render geometry. Objects provide
 *  a convenient way to represents objects in an application. Objects contain
 *  information about position, orientation, scale, instancing, animation,
 *  frustum culling and more.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"
#include "../Materials/WMaterialsStore.h"

/**
 * @ingroup engineclass
 * An instance is used by a WObject to provide a way for users to easily access
 * and modify instances of an object (when it enables geometry instancing).
 */
class WInstance : public WOrientation {
	friend class WObject;

public:
	WInstance();
	~WInstance();

	/**
	 * Sets the scale of this instance.
	 * @param x X scale multiplier
	 * @param y Y scale multiplier
	 * @param z Z scale multiplier
	 */
	void Scale(float x, float y, float z);

	/**
	 * Sets the scale of this instance.
	 * @param scale Scale factor components
	 */
	void Scale(WVector3 scale);

	/**
	 * Retrieves the scale factors of this instance.
	 * @return 3D vector containing the scale factors
	 */
	WVector3 GetScale() const;

	/**
	 * Retrieves the world matrix computed so far. A call to UpdateLocals()
	 * should be made prior to this one to ensure getting the most recent matrix.
	 * @return World matrix of this instance
	 */
	WMatrix GetWorldMatrix();

	/**
	 * Updates the locally computed world matrix of this instance. This function
	 * should only be called by WObject, otherwise changes might not be reflected
	 * on the instanced draw of the owner WObject.
	 * @return true if changes have occurred since the last update, false
	 *              otherwise
	 */
	bool UpdateLocals();

	void OnStateChange(STATE_CHANGE_TYPE type);

private:
	/** Scale of this instance */
	WVector3 m_scale;
	/** true if m_worldM needs to change, false otherwise */
	bool m_bAltered;
	/** The world matrix */
	WMatrix m_worldM;
};

/**
 * @ingroup engineclass
 * A WObject provides an easy way to render things in Wasabi. A WObject
 * combines WGeometry and WMaterial to render geometry using the material.
 * Furthermore, WObject provides an interface to manipulate the rendered
 * object's desired position, orientation and scale. A WObject also provides
 * an easy way to apply animations and instancing on rendered geometry.
 */
class WObject : public WOrientation, public WFileAsset, public WMaterialsStore {
	friend class WObjectManager;

protected:
	virtual ~WObject();

public:
	/**
	 * Returns "Object" string.
	 * @return Returns "Object" string
	 */
	virtual std::string GetTypeName() const;
	static std::string _GetTypeName();

	WObject(Wasabi* const app, unsigned int ID = 0);

	/**
	 * Checks whether a call to Render() will cause any rendering (draw call) to
	 * happen.
	 */
	bool WillRender(class WRenderTarget* rt);

	/**
	 * Renders this object. An object will only render to the render target if
	 * its valid (see Valid()) and not hidden (see Hide()). If frustum culling
	 * is enabled (see EnableFrustumCulling()), the object will only render if
	 * it is within the viewing frustum of the render target's camera. Before an
	 * object binds the provided material (WMaterial::Bind()), it will set the
	 * following variables and resources in the material, if they exist:
	 * * "worldMatrix" (WMatrix) will be set to the world matrix of this object.
	 * * "isAnimated" (int) will be set to 1 if animation data is available,
	 * 		0 otherwise.
	 * * "isInstanced" (int) will be set to 1 if instancing data is available,
	 * 		0 otherwise.
	 * * "animationTextureWidth" (int) will be set to the width of the
	 * 		animation texture. This will only be set if gAnimation was set to 1.
	 * * "instanceTextureWidth" (int) will be set to the width of the
	 * 		instancing texture. This will only be set if gInstancing was set to 1.
	 * * texture "animationTexture" will be assigned to the animation texture
	 *      from the attached animation. This will only occur if isAnimated was
	 *      set to 1.
	 * * texture "instancingTexture" will be assigned to the instancing texture
	 * 	    created by this object. This will only occur if isInstanced was set
	 *      to 1.
	 *
	 * If the object's instancing is initiated (see InitInstancing()), and there
	 * is at least one instance created (see CreateInstance()), the object will
	 * be rendered using geometry instancing.
	 * 
	 * @param rt              Render target to render to.
	 * @param material        Material to fill in with object data and bind
	 * @param updateInstances Whether or not to update the instances data
	 */
	void Render(class WRenderTarget* rt, class WMaterial* material, bool updateInstances = true);

	/**
	 * Sets the attached geometry.
	 * @param  geometry Geometry to attach, or nullptr to remove the attachment
	 * @return          Error code, see WError.h
	 */
	WError SetGeometry(class WGeometry* geometry);

	/**
	 * Sets the attached animation.
	 * @param  animation Animation to attach, or nullptr to remove the attachment
	 * @return           Error code, see WError.h
	 */
	WError SetAnimation(class WAnimation* animation);

	/**
	 * Retrieves the attached geometry.
	 * @return Attached geometry, nullptr if none exists
	 */
	class WGeometry* GetGeometry() const;

	/**
	 * Retrieves the attached animation.
	 * @return Attached animation, nullptr if none exists
	 */
	class WAnimation* GetAnimation() const;

	/**
	 * Initiates geometry instancing for this object. When geometry instancing
	 * is initiated, and at least one instance is created (via CreateInstance()),
	 * the object will be rendered with geometry instancing.
	 * @param  maxInstances Maximum number of instanced allowed to be created
	 * @return              Error code, see WError.h
	 */
	WError InitInstancing(unsigned int maxInstances);

	/**
	 * Cleans up all instancing assets, and disables instancing.
	 */
	void DestroyInstancingResources();

	/**
	 * Creates an instance of this object. Instancing has to be initialized prior
	 * to calling this function (see InitInstancing()).
	 * @return The newly created instance, or nullptr if it cannot be created. An
	 *         instance creation failure will occur if instancing is not
	 *         initiated or if the maximum number of instances is reached
	 */
	WInstance* CreateInstance();

	/**
	 * Retrieves an instance of the object at a given index.
	 * @param  index Index of the instance to retrieve
	 * @return       Pointer to the instance at the given index, or nullptr if
	 *               it  cannot be found
	 */
	WInstance* GetInstance(unsigned int index) const;

	/**
	 * Destroys an instance created for this object. The memory of the instance
	 * will be freed and <b>The pointer given to the function cannot be used
	 * after this call</b>.
	 * @param instance Instance to destroy
	 */
	void DeleteInstance(WInstance* instance);

	/**
	 * Destroys an instance created for this object. <b>Any pointer to the
	 * instance destroyed by this call cannot be used as that memory will be
	 * freed</b>.
	 * @param index Index of the instance to destroy
	 */
	void DeleteInstance(unsigned int index);

	/**
	 * Retrieves the number of instances for this object that are currently
	 * created.
	 * @return Number of currently created instances of the object
	 */
	unsigned int GetInstancesCount() const;

	/**
	 * Shows the object, allowing to render.
	 */
	void Show();

	/**
	 * Hides the object, making it unable to render.
	 */
	void Hide();

	/**
	 * Checks if the object is hidden.
	 * @return true if the object is hidden, false otherwise
	 */
	bool Hidden() const;

	/**
	 * Enables frustum culling. Frustum culling causes the object to only be
	 * rendered if part of its geometry is within the viewing frustum of the
	 * camera.
	 */
	void EnableFrustumCulling();

	/**
	 * Disables frustum culling, see EnableFrustumCulling() for more info.
	 */
	void DisableFrustumCulling();

	/**
	 * Checks if the object appears anywhere in the view of the camera
	 * @param  cam Camera to check against
	 * @return     true of the object is in the viewing frustum of cam, false
	 *             otherwise
	 */
	bool InCameraView(class WCamera* cam);

	/**
	 * Sets the scale of this object.
	 * @param scale Scale factor components
	 */
	void Scale(WVector3 scale);

	/**
	 * Retrieves the scale factors of this object.
	 * @return 3D vector containing the scale factors
	 */
	WVector3 GetScale() const;

	/**
	 * Retrieves the world matrix computed so far. A call to UpdateLocals()
	 * should be made prior to this one to ensure getting the most recent matrix.
	 * @return World matrix of this object
	 */
	WMatrix GetWorldMatrix();

	/**
	 * Updates the locally computed world matrix of this object.
	 * @param  offset An offset position to apply to the local matrix
	 * @return        true if changes have occurred since the last update, false
	 *                otherwise
	 */
	bool UpdateLocals();

	virtual void OnStateChange(STATE_CHANGE_TYPE type);

	/**
	 * Checks the validity of this object. An object is valid if it meets all the
	 * following conditions:
	 * * it has a valid geometry and a valid material attached
	 * * both the input layout (at the 0th index) of the material and the vertex
	 *   description of the geometry have the same size.
	 * @return true if the object is valid, false otherwise
	 */
	virtual bool Valid() const;

	/**
	 * Animations can only be saved if they are an instance of WSkeleton
	 */
	static std::vector<void*> LoadArgs();
	virtual WError SaveToStream(WFile* file, std::ostream& outputStream);
	virtual WError LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args);

private:
	/** Attached geometry */
	class WGeometry* m_geometry;
	/** Attached animation */
	class WAnimation* m_animation;
	/** true if the world matrix needs to be updated, false otherwise */
	bool m_bAltered;
	/** true if the object is hidden, false otherwise */
	bool m_hidden;
	/** true if frustum culling is enabled, false otherwise */
	bool m_bFrustumCull;
	/** Local world matrix */
	WMatrix m_WorldM;
	/** Scale of the object */
	WVector3 m_scale;
	/** Instancing texture */
	class WImage* m_instanceTexture;
	/** Maximum number of instances allowed */
	unsigned int m_maxInstances;
	/** true if the instance texture needs an update, false otherwise */
	bool m_instancesDirty;
	/** List of created instances */
	vector<WInstance*> m_instanceV;

	/**
	 * Updates all the instances and the instance buffer.
	 */
	void _UpdateInstanceBuffer();
};

/**
 * @ingroup engineclass
 * Manager class for WObject.
 */
class WObjectManager : public WManager<WObject> {
	friend class WObject;

	/**
	 * Returns "Object" string.
	 * @return Returns "Object" string
	 */
	virtual std::string GetTypeName() const;

public:
	WObjectManager(class Wasabi* const app);

	/**
	 * Loads the manager.
	 * @return Error code, see WError.h
	 */
	WError Load();

	/**
	 * Allocates and initializes a new object.
	 * @param ID  Id of the new object
	 * @return    New object
	 */
	WObject* CreateObject(unsigned int ID = 0) const;

	/**
	 * Checks if an object is in the view in the default renderer's camera and
	 * and part of that object is at the given (x,y) coordinates on the screen.
	 * If multiple objects are under the (x,y) coordinate, the one with the
	 * closest hit will be returned.
	 * @param  x           Screen x coordinate to check for a hit
	 * @param  y           Screen y coordinate to check for a hit
	 * @param  bAnyHit     If set to true, the first hit will be returned,
	 *                     otherwise the closest one will be returned
	 * @param  iObjStartID If both iObjStartID and iObjEndID are non-zero, then
	 *                     only objects whose ID is between iObjStartID and
	 *                     iObjEndID (inclusive) are considered for picking
	 * @param  iObjEndID   If both iObjStartID and iObjEndID are non-zero, then
	 *                     only objects whose ID is between iObjStartID and
	 *                     iObjEndID (inclusive) are considered for picking
	 * @param  pt          Optional pointer to a 3D vector which will be
	 *                     populated with the 3D coordinate at which the hit
	 *                     occurred, if a hit was found
	 * @param  uv          Optional pointer to a 2D vector which will be
	 *                     populated with the UV coordinates returned by
	 *                     WGeometry::Intersect(), if a hit was found
	 * @param  faceIndex   Optional pointer to an integer which will be
	 *                     populated with the index of the triangle into the
	 *                     buffer of the geometry of the object that was hit, if
	 *                     a hit was found
	 * @return             Object that was picked, nullptr if no Object was
	 *                     was picked
	 */
	WObject* PickObject(int x, int y, bool bAnyHit,
		unsigned int iObjStartID = 0,
		unsigned int iObjEndID = 0,
		WVector3* pt = nullptr, WVector2* uv = nullptr,
		unsigned int* faceIndex = nullptr) const;
};
