#pragma once

#include "Wasabi.h"
#include "WBase.h"
#include "WMath.h"

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


struct W_FRAME {
	float fTime;
	W_FRAME() : fTime(0.0f) {}
	~W_FRAME() {}
};

struct W_SUB_ANIMATION {
	unsigned int curFrame;
	unsigned int nextFrame;
	unsigned int firstFrame;
	bool bPlaying;
	bool bLoop;
	float fCurrentTime;
	float fSpeed;
	float fPlayStartTime;
	float fPlayEndTime;

	W_SUB_ANIMATION() {
		bPlaying = false;
		bLoop = false;
		fCurrentTime = 0.0f;
		curFrame = 0;
		firstFrame = 0;
		fSpeed = 1.0f;

		fPlayStartTime = 0.0f;
		fPlayEndTime = FLT_MAX;
	}
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
*******************************WAnimation****************************
Animation handler, this is either a skeletal or a keyframed animation
*********************************************************************/
class WAnimation : public WBase {
	std::string GetTypeName() const;

public:
	WAnimation(Wasabi* const app, unsigned int ID = 0);
	~WAnimation();

	virtual void Update(float fDeltaTime);

	virtual WImage* GetTexture() const = 0;

	virtual void AddSubAnimation();

	WError	SetKeyFrameTime(unsigned int frame, float fTime);
	void SetPlaySpeed(float fSpeedMultiplier, unsigned int subAnimation = 0);
	void SetCurrentFrame(unsigned int frame, unsigned int subAnimation = 0);
	void SetCurrentTime(float fTime, unsigned int subAnimation = 0);
	void SetPlayingBounds(unsigned int startFrame, unsigned int endFrame, unsigned int subAnimation = 0);
	void SetPlayingBounds_Time(float fStartTime, float fEndTime, unsigned int subAnimation = 0);
	void Play(unsigned int subAnimation = -1);
	void Loop(unsigned int subAnimation = -1);
	void Stop(unsigned int subAnimation = -1);
	void Reset(unsigned int subAnimation = -1);
	bool Playing(unsigned int subAnimation = 0) const;
	bool Looping(unsigned int subAnimation = 0) const;
	float GetTime(unsigned int subAnimation = 0) const;

	virtual WError	LoadFromWS(std::string Filename) = 0;
	virtual WError	LoadFromWS(basic_filebuf<char>* buff = nullptr, unsigned int pos = 0) = 0;
	virtual WError	SaveToWS(basic_filebuf<char>* buff = nullptr, unsigned int pos = 0) const = 0;
	virtual WError	SaveToWS(std::string Filename) const = 0;
	virtual WError	CopyFrom(const WAnimation* const from) = 0;
	virtual WError	UseAnimationFrames(const WAnimation* const anim) = 0;

protected:
	void m_UpdateFirstFrame(unsigned int subAnimation);

	bool m_bFramesOwner;
	vector<W_FRAME*> m_frames;
	vector<W_SUB_ANIMATION*> m_subAnimations;
};

class WAnimationManager : public WManager<WAnimation> {
	friend class WAnimation;

	virtual std::string GetTypeName() const;

public:
	WAnimationManager(class Wasabi* const app);
	~WAnimationManager();

	void Update(float fDeltaTime);
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
