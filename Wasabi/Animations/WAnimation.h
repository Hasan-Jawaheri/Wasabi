/** @file WAnimation.h
 *  @brief Animation implementation
 *
 *  Each animation contains information to allow a WObject to render with
 *  animations. An animation contains several subanimations, all happening at
 *  the same time. For instance, an animation for a character can have two
 *  subanimations, one controlling the upper body, and one controlling the
 *  lower body, so a character can shoot a gun and run at the same time using
 *  frames for running in addition to frames for shooting.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */
#pragma once

#include "../Core/Core.h"

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


/*********************************************************************
*******************************WAnimation****************************
Animation handler, this is either a skeletal or a keyframed animation
*********************************************************************/
class WAnimation : public WBase {
	std::string GetTypeName() const;

public:
	WAnimation(class Wasabi* const app, unsigned int ID = 0);
	~WAnimation();

	virtual void Update(float fDeltaTime);

	virtual class WImage* GetTexture() const = 0;

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
	float m_totalTime;
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
