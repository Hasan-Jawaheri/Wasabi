/** @file WSkeletalAnimation.h
 *  @brief Skeletal animation implementation
 *
 *  
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */
#pragma once

#include "../Core/Core.h"
#include "WAnimation.h"

class WBone : public WOrientation {
public:
	WBone();
	~WBone();

	unsigned int	GetIndex() const;
	void			SetIndex(unsigned int index);
	void			GetName(char* name, unsigned int maxChars) const;
	void			SetName(std::string name);

	WBone*			GetParent() const;
	void			SetParent(WBone* parent);

	unsigned int	GetNumChildren() const;
	WBone*			GetChild(unsigned int index) const;
	WBone*			GetChild(std::string name) const;
	void			AddChild(WBone* child);
	void			ClearChildren();

	void			SetInvBindingPose(WMatrix mtx);
	void			Scale(float x, float y, float z);
	void			Scale(WVector3 scale);
	void			ScaleX(float scale);
	void			ScaleY(float scale);
	void			ScaleZ(float scale);
	WVector3		GetScale() const;
	float			GetScaleX() const;
	float			GetScaleY() const;
	float			GetScaleZ() const;

	WMatrix			GetMatrix();
	WMatrix			GetRelativeMatrix();
	WMatrix			GetInvBindingPose() const;
	WQuaternion		GetRotationQuat();
	bool			UpdateLocals();
	void			OnStateChange(STATE_CHANGE_TYPE type);

	WError			LoadFromWS(basic_filebuf<char>* buff, unsigned int pos);
	WError			SaveToWS(basic_filebuf<char>* buff, unsigned int pos) const;

private:
	// THE FOLLOWING VARIABLES MUST REMAIN IN THIS ORDER
	WMatrix m_worldM;
	WMatrix m_invBindingPose;
	unsigned int m_index;
	char m_name[64];
	WVector3 m_scale;
	bool m_bAltered;
	WBone* m_parent;
	vector<WBone*> m_children;
	// ==================================================
};

struct W_SKELETAL_SUB_ANIMATION : public W_SUB_ANIMATION {
	vector<unsigned int> boneIndices;
	unsigned int parentIndex;
	unsigned int parentSubAnimation;

	W_SKELETAL_SUB_ANIMATION() {
		parentIndex = -1;
		parentSubAnimation = -1;
		boneIndices.clear();
	}

	void BuildIndices(WBone* curBone) {
		boneIndices.push_back(curBone->GetIndex());

		WBone* curChild = nullptr;
		unsigned int i = -1;
		while (curChild = curBone->GetChild(++i))
			BuildIndices(curBone->GetChild(i));
	}
};

/*********************************************************************
********************************WSkeleton****************************
skeletal Hierarchy handler class
*********************************************************************/
class WSkeleton : public WAnimation {
public:
	WSkeleton(Wasabi* const app, unsigned int ID = 0);
	~WSkeleton();

	WError CreateKeyFrame(WBone* baseBone, float fTime);
	WError DeleteKeyFrame(unsigned int frame);
	WBone*	GetKeyFrame(unsigned int frame);

	virtual void AddSubAnimation();
	void SetSubAnimationBaseBone(unsigned int subAnimation, unsigned int boneIndex, unsigned int parentSubAnimation = -1);

	virtual void Update(float fDeltaTime);

	virtual WImage* GetTexture() const;
	virtual WBone* GetBone(unsigned int frame, unsigned int index) const;
	virtual WBone* GetBone(unsigned int frame, std::string name) const;

	void Scale(float scale);
	void Scale(float x, float y, float z);
	void Scale(WVector3 scale);

	void BindToBone(WOrientation* obj, unsigned int boneID);
	void UnbindFromBone(WOrientation* obj, unsigned int boneID);
	void UnbindFromBone(unsigned int boneID);
	void SetBindingScale(float scale);
	void SetBindingScale(float x, float y, float z);
	void SetBindingScale(WVector3 scale);

	WVector3 GetCurrentParentBonePosition();

	virtual WError	LoadFromWS(std::string Filename);
	virtual WError	LoadFromWS(basic_filebuf<char>* buff = nullptr, unsigned int pos = 0);
	virtual WError	SaveToWS(basic_filebuf<char>* buff = nullptr, unsigned int pos = 0) const;
	virtual WError	SaveToWS(std::string Filename) const;
	virtual WError	CopyFrom(const WAnimation* const from);
	virtual WError	UseAnimationFrames(const WAnimation* const anim);

	bool Valid() const;

private:
	struct BONEBIND {
		WOrientation* obj;
		unsigned int boneID;
		BONEBIND(WOrientation* o, unsigned int id) : obj(o), boneID(id) {}
	};

	WImage* m_boneTex;
	vector<BONEBIND> m_bindings;
	WVector3 m_bindingScale, m_parentBonePos;
};
