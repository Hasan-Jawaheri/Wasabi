/** @file WAnimation.hpp
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

#include "Wasabi/Core/WCore.hpp"

/**
 * Represents an animation frame.
 */
struct W_FRAME {
	/** The duration of this frame. */
	float fTime;
	W_FRAME() : fTime(0.0f) {}
	~W_FRAME() {}
};

/**
 * Represents a subanimation.
 */
struct W_SUB_ANIMATION {
	/** The frame this subanimation is currently in. */
	uint32_t curFrame;
	/** The next frame this subanimation is merging to. */
	uint32_t nextFrame;
	/** The first frame within the play boundaries. */
	uint32_t firstFrame;
	/** true if the subanimation is playing (or looping). */
	bool bPlaying;
	/** true if the animation is looping. */
	bool bLoop;
	/** The current time in the subanimation. */
	float fCurrentTime;
	/** The speed multiplier for the subanimation. */
	float fSpeed;
	/** Starting time for the play boundaries. */
	float fPlayStartTime;
	/** Ending time for the play boundaries. */
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

/**
 * @ingroup engineclass
 * This is an abstract base class that represents an animation which could be
 * attached to an object. This base class is responsible for maintaining
 * subanimations and managing their playing and looping.
 */
class WAnimation : public WFileAsset {
protected:
	virtual ~WAnimation();

public:
	/**
	 * Returns "Animation" string.
	 * @return Returns "Animation" string
	 */
	static std::string _GetTypeName();
	virtual std::string GetTypeName() const override;
	virtual void SetID(uint32_t newID) override;
	virtual void SetName(std::string newName) override;

	WAnimation(class Wasabi* const app, uint32_t ID = 0);

	/**
	 * Steps the state of the playing (or looping) subanimations forward. This is
	 * usually called by the engine during normal execution each frame.
	 * @param fDeltaTime step time in seconds
	 */
	virtual void Update(float fDeltaTime);

	/**
	 * Retrieves the texture that corresponds to the animation. This depends on
	 * the implementation. For example, a skeletal animation implementation may
	 * return a texture that contains bone matrices encoded in its pixels.
	 * @return The animation texture
	 */
	virtual class WImage* GetTexture() const = 0;

	/**
	 * Appends a subanimation to this animation.
	 */
	virtual void AddSubAnimation();

	/**
	 * Removes a subanimation from this animation. Subanimation at index 0 cannot
	 * be removed.
	 * @param index The index of the subanimation to remove
	 */
	virtual void RemoveSubAnimation(uint32_t index);

	/**
	 * Sets the time interval for a keyframe in the animation frames.
	 * @param  frame Frame index
	 * @param  fTime Time to set for the frame
	 * @return       Error code, see WError.h
	 */
	WError SetKeyFrameTime(uint32_t frame, float fTime);

	/**
	 * Sets the play speed multiplier for the selected subanimation.
	 * @param fSpeedMultiplier Speed multiplier
	 * @param subAnimation     subanimation index, MAX will set the speed for all
	 *                         subanimations
	 */
	void SetPlaySpeed(float fSpeedMultiplier, uint32_t subAnimation = std::numeric_limits<uint32_t>::max());

	/**
	 * Immediately sets the current frame in the subanimation.
	 * @param frame        The frame index to go to
	 * @param subAnimation The subanimation to set its frame, MAX for all
	 *                     subanimations
	 */
	void SetCurrentFrame(uint32_t frame, uint32_t subAnimation = 0);

	/**
	 * Immediately sets the current time in the subanimation.
	 * @param fTime        The time to set the subanimation to
	 * @param subAnimation The subanimation to set its time, MAX for all
	 *                     subanimations
	 */
	void SetCurrentTime(float fTime, uint32_t subAnimation = 0);

	/**
	 * Sets the range in which the subanimation can loop.
	 * @param startFrame   The first frame to start looping
	 * @param endFrame     The last frame for the loop
	 * @param subAnimation The subanimation to set its boundaries, MAX for all
	 *                     subanimations
	 */
	void SetPlayingBounds(uint32_t startFrame, uint32_t endFrame, uint32_t subAnimation = 0);

	/**
	 * Sets the range in which the subanimation can loop.
	 * @param fStartTime   The time at which looping begins
	 * @param fEndTime     The time at which the loop restarts
	 * @param subAnimation The subanimation to set its boundaries, MAX for all
	 *                     subanimations
	 */
	void SetPlayingBounds_Time(float fStartTime, float fEndTime, uint32_t subAnimation = 0);

	/**
	 * Starts playing the subanimation.
	 * @param subAnimation subanimation to play, MAX for all subanimations
	 */
	void Play(uint32_t subAnimation = std::numeric_limits<uint32_t>::max());

	/**
	 * Starts looping the subanimation.
	 * @param subAnimation subanimation to loop, MAX for all subanimations
	 */
	void Loop(uint32_t subAnimation = std::numeric_limits<uint32_t>::max());

	/**
	 * Stops playing (and looping) the subanimation.
	 * @param subAnimation subanimation to stop, MAX for all subanimations
	 */
	void Stop(uint32_t subAnimation = std::numeric_limits<uint32_t>::max());

	/**
	 * Resets the subanimation by setting its current time and frame to the
	 * beginning of the boundaries set for it (Set boundaries using
	 * SetPlayingBounds and SetPlayingBounds_Time).
	 * @param subAnimation subanimation to reset, MAX for all subanimations
	 */
	void Reset(uint32_t subAnimation = std::numeric_limits<uint32_t>::max());

	/**
	 * Whether or not the subanimation is playing (or looping).
	 * @param subAnimation subanimation to check
	 * @return             true if the subanimation is playing (or looping),
	 *                     false otherwise
	 */
	bool Playing(uint32_t subAnimation = 0) const;

	/**
	 * Whether or not the subanimation is looping.
	 * @param subAnimation subanimation to check
	 * @return             true if the subanimation is looping, false otherwise
	 */
	bool Looping(uint32_t subAnimation = 0) const;

	/**
	 * Retrieves the current time in the subanimation.
	 * @param subAnimation subanimation to check
	 * @return             The current time of the subanimation
	 */
	float GetTime(uint32_t subAnimation = 0) const;

	/**
	 * Copies another WAnimation. This function is specific to the
	 * implementation.
	 * @param  from Animation to copy from
	 * @return      Error code, see WError.h
	 */
	virtual WError CopyFrom(const WAnimation* const from) = 0;

	/**
	 * Use the animation frames of another animation object. This should be more
	 * efficient than copying in time and memory usage. This function depends on
	 * the implementation.
	 * @param  anim Animation to use its frames
	 * @return      Error code, see WError.h
	 */
	virtual WError UseAnimationFrames(const WAnimation* const anim) = 0;

protected:
	/**
	 * Updates W_SUB_ANIMATION::firstFrame for the given subanimation to match
	 * its' W_SUB_ANIMATION::fPlayStartTime.
	 * @param subAnimation subanimation to update, MAX to update all subanimations
	 */
	void m_UpdateFirstFrame(uint32_t subAnimation = std::numeric_limits<uint32_t>::max());

	/**
	 * true if this object owns the frames in m_frames, and can thus free them.
	 * The owner of the frames is the one who allocated them.
	 */
	bool m_bFramesOwner;
	/** Total time of all the frames */
	float m_totalTime;
	/** The frames of this animation */
	vector<W_FRAME*> m_frames;
	/** The subanimations of this animation */
	vector<W_SUB_ANIMATION*> m_subAnimations;
};

/**
 * @ingroup engineclass
 * Manager class for WAnimation.
 */
class WAnimationManager : public WManager<WAnimation> {
	friend class WAnimation;

	/**
	 * Returns "Animation" string.
	 * @return Returns "Animation" string
	 */
	virtual std::string GetTypeName() const;

public:
	WAnimationManager(class Wasabi* const app);
	~WAnimationManager();

	/**
	 * Update (step) all registered animations.
	 * @param fDeltaTime Time to step each animation
	 */
	void Update(float fDeltaTime);
};
