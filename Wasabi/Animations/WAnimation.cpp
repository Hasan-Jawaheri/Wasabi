#include "WAnimation.h"

WAnimationManager::WAnimationManager(Wasabi* const app) : WManager<WAnimation>(app) {
}
WAnimationManager::~WAnimationManager() {
}
void WAnimationManager::Update(float fDeltaTime) {
	for (unsigned int j = 0; j < W_HASHTABLESIZE; j++)
		for (unsigned int i = 0; i < m_entities[j].size(); i++)
			m_entities[j][i]->Update(fDeltaTime);
}
std::string WAnimationManager::GetTypeName() const {
	return "Animation";
}

WAnimation::WAnimation(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	AddSubAnimation();

	char name[256];
	static unsigned int i = 0;
	sprintf_s(name, 256, "Animation%3u", i++);
	SetName(name);

	m_bFramesOwner = true;
	m_totalTime = 0.0f;

	//register the object
	m_app->AnimationManager->AddEntity(this);
}

WAnimation::~WAnimation() {
	if (m_bFramesOwner)
		for (unsigned int i = 0; i < m_frames.size(); i++)
			delete m_frames[i];
	for (unsigned int i = 0; i < m_subAnimations.size(); i++)
		delete m_subAnimations[i];

	//unregister animation
	m_app->AnimationManager->RemoveEntity(this);
}

std::string WAnimation::GetTypeName() const {
	return "Animation";
}

void WAnimation::m_UpdateFirstFrame(unsigned int subAnimation) {
	if (subAnimation == -1) {
		for (int i = 0; i < m_subAnimations.size(); i++) {
			float fStartTime = ((W_SUB_ANIMATION*)m_subAnimations[i])->fPlayStartTime;
			float fCurTime = 0.0f;
			for (int n = 0; n < m_frames.size(); n++) {
				if (fCurTime > fStartTime) {
					((W_SUB_ANIMATION*)m_subAnimations[i])->firstFrame = n;
					break;
				}
				fCurTime += ((W_FRAME*)m_frames[n])->fTime;
			}
		}
	} else {
		float fStartTime = ((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fPlayStartTime;
		float fCurTime = 0.0f;
		for (int n = 0; n < m_frames.size(); n++) {
			if (fCurTime >= fStartTime) {
				((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->firstFrame = n;
				break;
			}
			fCurTime += ((W_FRAME*)m_frames[n])->fTime;
		}
	}
}

void WAnimation::Update(float fDeltaTime) {
	for (int anim = 0; anim < m_subAnimations.size(); anim++) {
		W_SUB_ANIMATION* curSubAnimation = (W_SUB_ANIMATION*)m_subAnimations[anim];
		if (curSubAnimation->bPlaying)
			curSubAnimation->fCurrentTime += fDeltaTime * curSubAnimation->fSpeed;

		while (true) {
			//if we passed the end time provided, don't search for next frame because we're done already
			if (curSubAnimation->fCurrentTime < curSubAnimation->fPlayEndTime) {
				float fTotalTime = 0.0f;
				bool bContinue = true;
				for (unsigned int i = 0; i < m_frames.size(); i++) {
					fTotalTime += ((W_FRAME*)(m_frames[i]))->fTime;
					if (curSubAnimation->fCurrentTime < fTotalTime) {
						curSubAnimation->curFrame = i;
						curSubAnimation->nextFrame = i + 1;
						//if its the last frame, loop the next to the first
						if (fTotalTime > curSubAnimation->fPlayEndTime - 0.001f || curSubAnimation->nextFrame == m_frames.size())
							curSubAnimation->nextFrame = curSubAnimation->firstFrame;
						bContinue = false;
						break;
					}
				}
				if (!bContinue)
					break; // done with this animation for this iteration
			}

			//passed the total time, loop or not?
			if (curSubAnimation->bLoop) {
				float fEndTime = min(curSubAnimation->fPlayEndTime, m_totalTime);
				float fOverflow = curSubAnimation->fCurrentTime - fEndTime;
				SetCurrentTime(curSubAnimation->fPlayStartTime, anim);
				curSubAnimation->fCurrentTime += fOverflow;
			} else {
				Stop(anim);
				break; // done with this animation
			}
		}
	}
}

void WAnimation::AddSubAnimation() {
	m_subAnimations.push_back(new W_SUB_ANIMATION);
}

WError WAnimation::SetKeyFrameTime(unsigned int frame, float fTime) {
	if (frame >= m_frames.size())
		return WError(W_INVALIDPARAM);
	if (!fTime)
		fTime += 0.01f;

	((W_FRAME*)m_frames[frame])->fTime = fTime;

	return WError(W_SUCCEEDED);
}

void WAnimation::SetPlaySpeed(float fSpeedMultiplier, unsigned int subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;
	if (subAnimation == -1)
		for (unsigned int i = 0; i < m_subAnimations.size(); i++)
			((W_SUB_ANIMATION*)m_subAnimations[i])->fSpeed = fSpeedMultiplier;
	else
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fSpeed = fSpeedMultiplier;
}

void WAnimation::SetCurrentFrame(unsigned int frame, unsigned int subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;
	if (frame >= m_frames.size())
		return;

	float fTotalTime = 0.0f;
	for (unsigned int i = 0; i < m_frames.size(); i++) {
		if (frame == i) {
			if (subAnimation == -1) {
				for (unsigned int n = 0; n < m_subAnimations.size(); n++) {
					((W_SUB_ANIMATION*)m_subAnimations[n])->fCurrentTime = fTotalTime;
					((W_SUB_ANIMATION*)m_subAnimations[n])->curFrame = i;
				}
			} else {
				((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fCurrentTime = fTotalTime;
				((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->curFrame = i;
			}
			return;
		}
		fTotalTime += ((W_FRAME*)(m_frames[i]))->fTime;
	}
}

void WAnimation::SetCurrentTime(float fTime, unsigned int subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;

	float fTotalTime = 0.0f;
	for (unsigned int i = 0; i < m_frames.size(); i++) {
		fTotalTime += ((W_FRAME*)(m_frames[i]))->fTime;
		if (fTime < fTotalTime) {
			if (subAnimation == -1) {
				for (unsigned int n = 0; n < m_subAnimations.size(); n++) {
					((W_SUB_ANIMATION*)m_subAnimations[n])->fCurrentTime = fTime;
					((W_SUB_ANIMATION*)m_subAnimations[n])->curFrame = i;
				}
			} else {
				((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fCurrentTime = fTime;
				((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->curFrame = i;
			}
			return;
		}
	}
}

void WAnimation::SetPlayingBounds(unsigned int startFrame, unsigned int endFrame, unsigned int subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;
	if (endFrame >= m_frames.size())
		endFrame = m_frames.size() - 1;
	if (startFrame > endFrame)
		return;

	float fTotalTime = 0.0f;
	for (unsigned int i = 0; i < m_frames.size(); i++) {
		if (startFrame == i) {
			if (subAnimation == -1)
				for (unsigned int n = 0; n < m_subAnimations.size(); n++)
					((W_SUB_ANIMATION*)m_subAnimations[n])->fPlayStartTime = fTotalTime;
			else
				((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fPlayStartTime = fTotalTime;
		}

		fTotalTime += ((W_FRAME*)(m_frames[i]))->fTime;

		if (endFrame == i) {
			if (subAnimation == -1)
				for (unsigned int n = 0; n < m_subAnimations.size(); n++)
					((W_SUB_ANIMATION*)m_subAnimations[n])->fPlayEndTime = fTotalTime;
			else
				((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fPlayEndTime = fTotalTime;
		}
	}

	m_UpdateFirstFrame(subAnimation);
}

void WAnimation::SetPlayingBounds_Time(float fStartTime, float fEndTime, unsigned int subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;
	if (fStartTime >= fEndTime)
		return;

	if (subAnimation == -1) {
		for (unsigned int n = 0; n < m_subAnimations.size(); n++) {
			((W_SUB_ANIMATION*)m_subAnimations[n])->fPlayStartTime = fStartTime;
			((W_SUB_ANIMATION*)m_subAnimations[n])->fPlayEndTime = fEndTime;
		}
	} else {
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fPlayStartTime = fStartTime;
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fPlayEndTime = fEndTime;
	}

	m_UpdateFirstFrame(subAnimation);
}

void WAnimation::Play(unsigned int subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;

	if (subAnimation == -1) {
		for (unsigned int n = 0; n < m_subAnimations.size(); n++) {
			((W_SUB_ANIMATION*)m_subAnimations[n])->bPlaying = true;
			((W_SUB_ANIMATION*)m_subAnimations[n])->bLoop = false;
		}
	} else {
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bPlaying = true;
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bLoop = false;
	}
}

void WAnimation::Loop(unsigned int subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;

	if (subAnimation == -1) {
		for (unsigned int n = 0; n < m_subAnimations.size(); n++) {
			((W_SUB_ANIMATION*)m_subAnimations[n])->bPlaying = true;
			((W_SUB_ANIMATION*)m_subAnimations[n])->bLoop = true;
		}
	} else {
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bPlaying = true;
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bLoop = true;
	}
}

void WAnimation::Stop(unsigned int subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;

	if (subAnimation == -1) {
		for (unsigned int n = 0; n < m_subAnimations.size(); n++) {
			((W_SUB_ANIMATION*)m_subAnimations[n])->bPlaying = false;
			((W_SUB_ANIMATION*)m_subAnimations[n])->bLoop = false;
		}
	} else {
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bPlaying = false;
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bLoop = false;
	}
}

void WAnimation::Reset(unsigned int subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;

	if (subAnimation == -1) {
		for (unsigned int n = 0; n < m_subAnimations.size(); n++) {
			((W_SUB_ANIMATION*)m_subAnimations[n])->fCurrentTime = 0.0f;
			((W_SUB_ANIMATION*)m_subAnimations[n])->curFrame = 0;
		}
	} else {
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fCurrentTime = 0.0f;
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->curFrame = 0;
	}
}

bool WAnimation::Playing(unsigned int subAnimation) const {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return false;

	if (subAnimation == -1) {
		bool bPlaying = true;
		for (unsigned int n = 0; n < m_subAnimations.size(); n++)
			if (!((W_SUB_ANIMATION*)m_subAnimations[n])->bPlaying)
				bPlaying = false;
		return bPlaying;
	}

	return ((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bPlaying;
}

bool WAnimation::Looping(unsigned int subAnimation) const {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return false;

	if (subAnimation == -1) {
		bool bLooping = true;
		for (unsigned int n = 0; n < m_subAnimations.size(); n++)
			if (!((W_SUB_ANIMATION*)m_subAnimations[n])->bLoop)
				bLooping = false;
		return bLooping;
	}

	return ((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bLoop;
}

float WAnimation::GetTime(unsigned int subAnimation) const {
	if (subAnimation >= m_subAnimations.size())
		return false;

	return ((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fCurrentTime;
}
