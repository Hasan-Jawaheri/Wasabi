#include "Wasabi/Animations/WAnimation.hpp"

WAnimationManager::WAnimationManager(Wasabi* const app) : WManager<WAnimation>(app) {
}
WAnimationManager::~WAnimationManager() {
}
void WAnimationManager::Update(float fDeltaTime) {
	uint32_t entitiyCount = GetEntitiesCount();
	for (uint32_t i = 0; i < entitiyCount; i++)
		GetEntityByIndex(i)->Update(fDeltaTime);
}
std::string WAnimationManager::GetTypeName() const {
	return "Animation";
}

WAnimation::WAnimation(Wasabi* const app, uint32_t ID) : WFileAsset(app, ID) {
	AddSubAnimation();

	m_bFramesOwner = true;
	m_totalTime = 0.0f;

	//register the object
	m_app->AnimationManager->AddEntity(this);
}

WAnimation::~WAnimation() {
	if (m_bFramesOwner)
		for (uint32_t i = 0; i < m_frames.size(); i++)
			delete m_frames[i];
	for (uint32_t i = 0; i < m_subAnimations.size(); i++)
		delete m_subAnimations[i];

	//unregister animation
	m_app->AnimationManager->RemoveEntity(this);
}

std::string WAnimation::_GetTypeName() {
	return "Animation";
}

std::string WAnimation::GetTypeName() const {
	return _GetTypeName();
}

void WAnimation::SetID(uint32_t newID) {
	m_app->AnimationManager->RemoveEntity(this);
	m_ID = newID;
	m_app->AnimationManager->AddEntity(this);
}

void WAnimation::SetName(std::string newName) {
	m_name = newName;
	m_app->AnimationManager->OnEntityNameChanged(this, newName);
}

void WAnimation::m_UpdateFirstFrame(uint32_t subAnimation) {
	if (subAnimation == std::numeric_limits<uint32_t>::max()) {
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
				for (uint32_t i = 0; i < m_frames.size(); i++) {
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
				float fEndTime = fmin(curSubAnimation->fPlayEndTime, m_totalTime);
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

void WAnimation::RemoveSubAnimation(uint32_t index) {
	if (index > 0 && index < m_subAnimations.size()) {
		delete m_subAnimations[index];
		m_subAnimations.erase(m_subAnimations.begin() + index);
	}
}

WError WAnimation::SetKeyFrameTime(uint32_t frame, float fTime) {
	if (frame >= m_frames.size())
		return WError(W_INVALIDPARAM);
	if (!fTime)
		fTime += 0.01f;

	((W_FRAME*)m_frames[frame])->fTime = fTime;

	return WError(W_SUCCEEDED);
}

void WAnimation::SetPlaySpeed(float fSpeedMultiplier, uint32_t subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;
	if (subAnimation == std::numeric_limits<uint32_t>::max())
		for (uint32_t i = 0; i < m_subAnimations.size(); i++)
			((W_SUB_ANIMATION*)m_subAnimations[i])->fSpeed = fSpeedMultiplier;
	else
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fSpeed = fSpeedMultiplier;
}

void WAnimation::SetCurrentFrame(uint32_t frame, uint32_t subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;
	if (frame >= m_frames.size())
		return;

	float fTotalTime = 0.0f;
	for (uint32_t i = 0; i < m_frames.size(); i++) {
		if (frame == i) {
			if (subAnimation == std::numeric_limits<uint32_t>::max()) {
				for (uint32_t n = 0; n < m_subAnimations.size(); n++) {
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

void WAnimation::SetCurrentTime(float fTime, uint32_t subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;

	float fTotalTime = 0.0f;
	for (uint32_t i = 0; i < m_frames.size(); i++) {
		fTotalTime += ((W_FRAME*)(m_frames[i]))->fTime;
		if (fTime < fTotalTime) {
			if (subAnimation == std::numeric_limits<uint32_t>::max()) {
				for (uint32_t n = 0; n < m_subAnimations.size(); n++) {
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

void WAnimation::SetPlayingBounds(uint32_t startFrame, uint32_t endFrame, uint32_t subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;
	if (endFrame >= m_frames.size())
		endFrame = (uint32_t)m_frames.size() - 1;
	if (startFrame > endFrame)
		return;

	float fTotalTime = 0.0f;
	for (uint32_t i = 0; i < m_frames.size(); i++) {
		if (startFrame == i) {
			if (subAnimation == std::numeric_limits<uint32_t>::max())
				for (uint32_t n = 0; n < m_subAnimations.size(); n++)
					((W_SUB_ANIMATION*)m_subAnimations[n])->fPlayStartTime = fTotalTime;
			else
				((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fPlayStartTime = fTotalTime;
		}

		fTotalTime += ((W_FRAME*)(m_frames[i]))->fTime;

		if (endFrame == i) {
			if (subAnimation == std::numeric_limits<uint32_t>::max())
				for (uint32_t n = 0; n < m_subAnimations.size(); n++)
					((W_SUB_ANIMATION*)m_subAnimations[n])->fPlayEndTime = fTotalTime;
			else
				((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fPlayEndTime = fTotalTime;
		}
	}

	m_UpdateFirstFrame(subAnimation);
}

void WAnimation::SetPlayingBounds_Time(float fStartTime, float fEndTime, uint32_t subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;
	if (fStartTime >= fEndTime)
		return;

	if (subAnimation == std::numeric_limits<uint32_t>::max()) {
		for (uint32_t n = 0; n < m_subAnimations.size(); n++) {
			((W_SUB_ANIMATION*)m_subAnimations[n])->fPlayStartTime = fStartTime;
			((W_SUB_ANIMATION*)m_subAnimations[n])->fPlayEndTime = fEndTime;
		}
	} else {
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fPlayStartTime = fStartTime;
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fPlayEndTime = fEndTime;
	}

	m_UpdateFirstFrame(subAnimation);
}

void WAnimation::Play(uint32_t subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;

	if (subAnimation == std::numeric_limits<uint32_t>::max()) {
		for (uint32_t n = 0; n < m_subAnimations.size(); n++) {
			((W_SUB_ANIMATION*)m_subAnimations[n])->bPlaying = true;
			((W_SUB_ANIMATION*)m_subAnimations[n])->bLoop = false;
		}
	} else {
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bPlaying = true;
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bLoop = false;
	}
}

void WAnimation::Loop(uint32_t subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;

	if (subAnimation == std::numeric_limits<uint32_t>::max()) {
		for (uint32_t n = 0; n < m_subAnimations.size(); n++) {
			((W_SUB_ANIMATION*)m_subAnimations[n])->bPlaying = true;
			((W_SUB_ANIMATION*)m_subAnimations[n])->bLoop = true;
		}
	} else {
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bPlaying = true;
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bLoop = true;
	}
}

void WAnimation::Stop(uint32_t subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;

	if (subAnimation == std::numeric_limits<uint32_t>::max()) {
		for (uint32_t n = 0; n < m_subAnimations.size(); n++) {
			((W_SUB_ANIMATION*)m_subAnimations[n])->bPlaying = false;
			((W_SUB_ANIMATION*)m_subAnimations[n])->bLoop = false;
		}
	} else {
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bPlaying = false;
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bLoop = false;
	}
}

void WAnimation::Reset(uint32_t subAnimation) {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return;

	if (subAnimation == std::numeric_limits<uint32_t>::max()) {
		for (uint32_t n = 0; n < m_subAnimations.size(); n++) {
			((W_SUB_ANIMATION*)m_subAnimations[n])->fCurrentTime = ((W_SUB_ANIMATION*)m_subAnimations[n])->fPlayStartTime;
			((W_SUB_ANIMATION*)m_subAnimations[n])->curFrame = ((W_SUB_ANIMATION*)m_subAnimations[n])->firstFrame;
		}
	} else {
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fCurrentTime = ((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fPlayStartTime;
		((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->curFrame = ((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->firstFrame;
	}
}

bool WAnimation::Playing(uint32_t subAnimation) const {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return false;

	if (subAnimation == std::numeric_limits<uint32_t>::max()) {
		bool bPlaying = true;
		for (uint32_t n = 0; n < m_subAnimations.size(); n++)
			if (!((W_SUB_ANIMATION*)m_subAnimations[n])->bPlaying)
				bPlaying = false;
		return bPlaying;
	}

	return ((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bPlaying;
}

bool WAnimation::Looping(uint32_t subAnimation) const {
	if (subAnimation >= m_subAnimations.size() && subAnimation != -1)
		return false;

	if (subAnimation == std::numeric_limits<uint32_t>::max()) {
		bool bLooping = true;
		for (uint32_t n = 0; n < m_subAnimations.size(); n++)
			if (!((W_SUB_ANIMATION*)m_subAnimations[n])->bLoop)
				bLooping = false;
		return bLooping;
	}

	return ((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->bLoop;
}

float WAnimation::GetTime(uint32_t subAnimation) const {
	if (subAnimation >= m_subAnimations.size())
		return false;

	return ((W_SUB_ANIMATION*)m_subAnimations[subAnimation])->fCurrentTime;
}
