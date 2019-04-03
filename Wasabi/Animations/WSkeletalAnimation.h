/** @file WSkeletalAnimation.h
 *  @brief Skeletal animation implementation
 *
 *  Skeletal animation is implemented as follows:
 *  * WSkeleton class is responsible for loading skeleton data from a .WA file
 *  * A skeleton contains information about the bones of an animation, while a
 *  	geometry (WGeometry) should contain animation data in the form of a
 *  	vertex buffer (defined by WDefaultVertex_Animation).
 *  * WSkeleton will create a texture that will represent how the bones are
 *  	positioned at a point in the animation. This texture is updated every
 *  	frame to reflect the animation of the bones. Every 4 pixels represent
 *  	a matrix for a bone at that index. The matrix is stored row by row, but
 *  	the last row is omitted and its' components are encoded in the first 3
 *  	rows' w component.
 *  * The bone texture and the animation vertex buffer are passed to the vertex
 *  	shader, which is free to use in any way. The default implementation
 *  	uses the boneIDs of every vertex (supplied by the animation vertex
 *  	buffer) to fetch the matrix of that bone (3 texture fetches) and then
 *  	applies the matrix in the vertex transformation.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */
#pragma once

#include "../Core/WCore.h"
#include "WAnimation.h"
#include <iostream>

/**
 * @ingroup engineclass
 * This class represents a single bone in a skeleton. The final matrix of this
 * bone is calculated by GetInvBindingPose() * GetRelativeMatrix().
 */
class WBone : public WOrientation {
public:
	WBone();
	~WBone();

	/**
	 * Retrieves the index of this bone.
	 * @return Index of this bone
	 */
	unsigned int GetIndex() const;

	/**
	 * Sets the index of this bone.
	 * @param Index new index
	 */
	void SetIndex(unsigned int index);

	/**
	 * Retrieves the name of this bone.
	 * @param name     A character array to be filled with the name
	 * @param maxChars Maximum number of characters to fill in name
	 */
	void GetName(char* name, unsigned int maxChars) const;

	/**
	 * Sets the name of this bone.
	 * @param name New name
	 */
	void SetName(std::string name);

	/**
	 * Retrieves a pointer to the parent of this bone.
	 * @return Parent of this bone
	 */
	WBone* GetParent() const;

	/**
	 * Sets the parent of this bone.
	 * @param Parent pointer to the new parent
	 */
	void SetParent(WBone* parent);

	/**
	 * Retrieves the number of children this bone has.
	 * @return Number of children for this bone
	 */
	unsigned int GetNumChildren() const;

	/**
	 * Retrieve a pointer to the child with the given index.
	 * @param  index Index of the child
	 * @return       The child at index, or nullptr if index is invalid
	 */
	WBone* GetChild(unsigned int index) const;

	/**
	 * Retrieves a child by name.
	 * @param  name Name of the child to retrieve
	 * @return      The child with the name name, or nullptr if it is invalid
	 */
	WBone* GetChild(std::string name) const;

	/**
	 * Adds a child to this bone.
	 * @param child Pointer to the new child
	 */
	void AddChild(WBone* child);

	/**
	 * Clears all children of this bone.
	 */
	void ClearChildren();

	/**
	 * Sets the inverse matrix of the binding pose for this bone.
	 * @param mtx Inverse binding pose matrix
	 */
	void SetInvBindingPose(WMatrix mtx);

	/**
	 * Sets the scale of this bone.
	 * @param x Scale x multiplier
	 * @param y Scale y multiplier
	 * @param z Scale z multiplier
	 */
	void Scale(float x, float y, float z);

	/**
	 * Sets the scale of this bone.
	 * @param scale A 3D vector representing the scale components
	 */
	void Scale(WVector3 scale);

	/**
	 * Retrieves the scale of this bone.
	 * @return A 3D vector representing the scale components
	 */
	WVector3 GetScale() const;

	/**
	 * Retrieves the local matrix of this bone.
	 * @return Local matrix of this bone
	 */
	WMatrix GetMatrix();

	/**
	 * Retrieves the local matrix of this bone multiplied by the relative
	 * matrix of the parent bone (will recursively produce the final matrix for
	 * this bone).
	 * @return Relative (final) matrix of this bone.
	 */
	WMatrix GetRelativeMatrix();

	/**
	 * Retrieves the inverse binding pose for this bone.
	 * @return The inverse binding pose matrix.
	 */
	WMatrix GetInvBindingPose() const;

	/**
	 * Updates the local matrix of this bone. This function should only be called
	 * by WSkeleton.
	 * @return true if the local matrix has changed, false if not.
	 */
	bool UpdateLocals();

	/**
	 * This is a callback (inherited from WOrientation) to inform this object of
	 * a change in orientation.
	 * @param type Orientation change type (rotation or motion)
	 */
	void OnStateChange(STATE_CHANGE_TYPE type);

	/**
	 * Load this bone from a file stream as a bone format.
	 * @param  inputStream Stream to read the data from
	 * @return             Error code, see WError.h
	 */
	WError Load(std::istream& inputStream);

	/**
	 * Save this bone to a file stream as a bone format.
	 * @param  outputStream Stream to read the data from
	 * @return              Error code, see WError.h
	 */
	WError Save(std::ostream& outputStream) const;

private:
	// THE FOLLOWING VARIABLES MUST REMAIN IN THIS ORDER
	/** Local world matrix */
	WMatrix m_worldM;
	/** Inverse binding pose */
	WMatrix m_invBindingPose;
	/** Bone index */
	unsigned int m_index;
	/** Bone name */
	char m_name[64];
	/** Bone scale */
	WVector3 m_scale;
	/** true if the local matrix needs to be updated, false otherwise */
	bool m_bAltered;
	/** Parent of this bone */
	WBone* m_parent;
	/** Children of this bone */
	vector<WBone*> m_children;
	// ==================================================
};

/**
 * An extended W_SUB_ANIMATION, representing a subanimation for a skeleton.
 */
struct W_SKELETAL_SUB_ANIMATION : public W_SUB_ANIMATION {
	/** List of bone indices that are affected by this subanimation */
	vector<unsigned int> boneIndices;
	/** Index of the parent bone of this subanimation, -1 if none */
	unsigned int parentIndex;
	/** Index of the parent subanimation, -1 if none */
	unsigned int parentSubAnimation;

	W_SKELETAL_SUB_ANIMATION() {
		parentIndex = -1;
		parentSubAnimation = -1;
		boneIndices.clear();
	}

	/**
	 * Recursively goes through the children of a given bone to add their indices
	 * to boneIndices.
	 * @param curBone The root bone
	 */
	void BuildIndices(WBone* curBone) {
		boneIndices.push_back(curBone->GetIndex());

		WBone* curChild = nullptr;
		unsigned int i = -1;
		while (curChild = curBone->GetChild(++i))
			BuildIndices(curBone->GetChild(i));
	}
};

/**
 * @ingroup engineclass
 * This is the class implementing skeletal animation, as described in
 * WSkeletalAnimation.h.
 */
class WSkeleton : public WAnimation {
protected:
	virtual ~WSkeleton();

public:

	WSkeleton(Wasabi* const app, unsigned int ID = 0);

	/**
	 * Appends a keyframe to the frames of this animation.
	 * @param  baseBone The root of the bone structure for this keyframe
	 * @param  fTime    The duration of this keyframe
	 * @return          Error code, see WError.h
	 */
	WError CreateKeyFrame(WBone* baseBone, float fTime);

	/**
	 * Delete a an existing keyframe.
	 * @param  frame Index of the keyframe to delete
	 * @return       Error code, see WError.h
	 */
	WError DeleteKeyFrame(unsigned int frame);

	/**
	 * Retrieves an existing keyframe.
	 * @param  frame Index of the keyframe to retrieve
	 * @return       The root of the bone structure of thi keyframe
	 */
	WBone* GetKeyFrame(unsigned int frame);

	/**
	 * Appends a subanimation to this animation. This is inherited from
	 * WAnimation.
	 */
	virtual void AddSubAnimation();

	/**
	 * Sets the base bone for a subanimation. A subanimation's transformations
	 * will affect its base bone and all its children. If a parent subanimation
	 * is present, then this subanimation will be affected by the motion of that
	 * parent subanimation.
	 * @param subAnimation       The subanimation to set its base bane
	 * @param boneIndex          The index of the bone to be set as base
	 * @param parentSubAnimation The parent subanimation, -1 if none
	 */
	void SetSubAnimationBaseBone(unsigned int subAnimation,
								 unsigned int boneIndex,
								 unsigned int parentSubAnimation = -1);

	/**
	 * Steps the state of the playing (or looping) subanimations forward. This is
	 * usually called by the engine during normal execution each frame.
	 * @param fDeltaTime step time in seconds
	 */
	virtual void Update(float fDeltaTime);

	/**
	 * Retrieves the animation texture, as described in WSkeletalAnimation.h
	 * @return The animation texture
	 */
	virtual WImage* GetTexture() const;

	/**
	 * Retrieves a pointer to a bone from a frame. Changing the returned bone
	 * will impact the frame in real-time without any re-initialization.
	 * @param  frame Frame to retrieve from
	 * @param  index Index of the bone to retrieve
	 * @return       A pointer to the retrieved bone, nullptr if it doesnt exist
	 */
	virtual WBone* GetBone(unsigned int frame, unsigned int index) const;

	/**
	 * Retrieves a pointer to a bone from a frame. Changing the returned bone
	 * will impact the frame in real-time without any re-initialization.
	 * @param  frame Frame to retrieve from
	 * @param  index Name of the bone to retrieve
	 * @return       A pointer to the retrieved bone, nullptr if it doesnt exist
	 */
	virtual WBone* GetBone(unsigned int frame, std::string name) const;

	/**
	 * Sets the scale of the the skeleton.
	 * @param scale Scale multiplier
	 */
	void Scale(float scale);

	/**
	 * Sets the scale of the the skeleton.
	 * @param x Scale multiplier on X
	 * @param y Scale multiplier on Y
	 * @param z Scale multiplier on Z
	 */
	void Scale(float x, float y, float z);

	/**
	 * Scales the skeleton.
	 * @param scale Scale multiplier components
	 */
	void Scale(WVector3 scale);

	/**
	 * Binds any orientation-enabled entity (WObject, WLight, etc...) to a bone
	 * in the skeleton. The user \b MUST ensure that the bound object remains
	 * valid while it is bound to a skeleton.
	 * @param obj    Pointer to the object to bind
	 * @param boneID Index of the bone to bind obj to
	 */
	void BindToBone(WOrientation* obj, unsigned int boneID);

	/**
	 * Unbind an object from a bone.
	 * @param obj    Pointer to the object to unbind
	 * @param boneID Index of the bone to unbind obj from
	 */
	void UnbindFromBone(WOrientation* obj, unsigned int boneID);

	/**
	 * Unbind all objects from a bone.
	 * @param boneID Index of the bone to unbind objects from
	 */
	void UnbindFromBone(unsigned int boneID);

	/**
	 * Set the binding scale, which is applied to the binding matrix before it
	 * is set on bound objects.
	 * @param scale Scale multiplier
	 */
	void SetBindingScale(float scale);

	/**
	 * Set the binding scale, which is applied to the binding matrix before it
	 * is set on bound objects.
	 * @param x Scale multiplier on X
	 * @param y Scale multiplier on Y
	 * @param z Scale multiplier on Z
	 */
	void SetBindingScale(float x, float y, float z);

	/**
	 * Set the binding scale, which is applied to the binding matrix before it
	 * is set on bound objects.
	 * @param scale Scale multiplier components
	 */
	void SetBindingScale(WVector3 scale);

	/**
	 * Retrieves the world-space position of the root of this skeleton at
	 * this point in the animation.
	 * @return A 3D point whee the skeleton is rooted at this time
	 */
	WVector3 GetCurrentParentBonePosition();

	/**
	 * Copies another WSkeleton.
	 * @param  from Sekelton to copy from
	 * @return      Error code, see WError.h
	 */
	virtual WError CopyFrom(const WAnimation* const from);

	/**
	 * Use the animation frames of another animation object. This is more
	 * efficient than copying in time and memory usage.
	 * @param  anim Animation to use its frames
	 * @return      Error code, see WError.h
	 */
	virtual WError UseAnimationFrames(const WAnimation* const anim);

	/**
	 * Returns whether or not this skeleton is valid. A valid skeleton is one
	 * that has animation keyframes and a bone texture.
	 * @return true if the skeleton is valid, false otherwise
	 */
	bool Valid() const;

	virtual WError SaveToStream(class WFile* file, std::ostream& outputStream);
	virtual WError LoadFromStream(class WFile* file, std::istream& inputStream);

private:
	/**
	 * Represents the binding of an object to a bone.
	 */
	struct BONEBIND {
		/** Bound orientation object */
		WOrientation* obj;
		/** Bone index that obj is bound to */
		unsigned int boneID;
		BONEBIND(WOrientation* o, unsigned int id) : obj(o), boneID(id) {}
	};

	/** The animation texture */
	WImage* m_boneTex;
	/** The object bindings to the bones */
	vector<BONEBIND> m_bindings;
	/** The binding scale */
	WVector3 m_bindingScale;
	/** The world-space position of the root of the skeleton */
	WVector3 m_parentBonePos;
};
