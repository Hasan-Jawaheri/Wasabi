#include "Animations/WSkeletalAnimation.h"
#include "Images/WImage.h"

const unsigned int sizeofBoneNoPtrs = 4 + 64 + sizeof(bool) + sizeof(WVector3) + 2 * sizeof(WMatrix);

WBone::WBone() {
	m_scale = WVector3(1.0f, 1.0f, 1.0f);
	m_worldM = WMatrix();
	m_invBindingPose = WMatrixInverse(WMatrix());
	m_parent = nullptr;
	ZeroMemory(m_name, 64);
}

WBone::~WBone() {
	//every bone destructs it's children, thus, only base bones should be deleteded and the rest will be deleted with it
	for (unsigned int i = 0; i < m_children.size(); i++)
		delete m_children[i];
}

unsigned int WBone::GetIndex() const {
	return m_index;
}

void WBone::SetIndex(unsigned int index) {
	m_index = index;
}

void WBone::GetName(char* name, unsigned int maxChars) const {
	strcpy_s(name, fmin(maxChars, 64), m_name);
}

void WBone::SetName(std::string name) {
	strcpy_s(m_name, 64, name.c_str());
}

WBone* WBone::GetParent() const {
	return m_parent;
}

void WBone::SetParent(WBone* parent) {
	m_parent = parent;
}

unsigned int WBone::GetNumChildren() const {
	return m_children.size();
}

WBone* WBone::GetChild(unsigned int index) const {
	if (index >= m_children.size())
		return nullptr;
	return m_children[index];
}

WBone* WBone::GetChild(std::string name) const {
	for (unsigned int i = 0; i < m_children.size(); i++)
		if (strcmp(m_children[i]->m_name, name.c_str()) == 0)
			return m_children[i];

	return nullptr;
}

void WBone::AddChild(WBone* child) {
	m_children.push_back(child);
}

void WBone::ClearChildren() {
	m_children.clear();
}

void WBone::SetInvBindingPose(WMatrix mtx) {
	m_invBindingPose = mtx;
}

void WBone::Scale(float x, float y, float z) {
	m_scale = WVector3(x, y, z);
	m_bAltered = true;
}

void WBone::Scale(WVector3 scale) {
	m_scale = scale;
	m_bAltered = true;
}

WVector3 WBone::GetScale() const {
	return m_scale;
}

WMatrix WBone::GetMatrix() {
	UpdateLocals();
	return m_worldM;
}

WMatrix WBone::GetRelativeMatrix() {
	UpdateLocals();
	WMatrix m = m_worldM;
	if (m_parent)
		m *= m_parent->GetRelativeMatrix();
	return m;
}

WMatrix WBone::GetInvBindingPose() const {
	return m_invBindingPose;
}

bool WBone::UpdateLocals() {
	if (m_bAltered) {
		m_bAltered = false;
		WVector3 _up = GetUVector();
		WVector3 _look = GetLVector();
		WVector3 _right = GetRVector();
		WVector3 _pos = m_parent == nullptr ? WVector3(0, GetPositionY(), 0) : GetPosition();

		//
		//the world matrix is the view matrix's inverse
		//so we build a normal view matrix and inverse it
		//

		//build world matrix
		float x = -WVec3Dot(_right, _pos);
		float y = -WVec3Dot(_up, _pos);
		float z = -WVec3Dot(_look, _pos);
		(m_worldM)(0, 0) = _right.x; (m_worldM)(0, 1) = _up.x; (m_worldM)(0, 2) = _look.x; (m_worldM)(0, 3) = 0.0f;
		(m_worldM)(1, 0) = _right.y; (m_worldM)(1, 1) = _up.y; (m_worldM)(1, 2) = _look.y; (m_worldM)(1, 3) = 0.0f;
		(m_worldM)(2, 0) = _right.z; (m_worldM)(2, 1) = _up.z; (m_worldM)(2, 2) = _look.z; (m_worldM)(2, 3) = 0.0f;
		(m_worldM)(3, 0) = x;        (m_worldM)(3, 1) = y;     (m_worldM)(3, 2) = z;       (m_worldM)(3, 3) = 1.0f;
		m_worldM = WMatrixInverse(m_worldM);

		//scale matrix
		//m_worldM = WTranslationMatrix ( -GetPosition ( ) ) * m_worldM; //remove offset after rotation
		m_worldM = WScalingMatrix(m_scale) * m_worldM;
		if (GetParent() == nullptr)
			for (int i = 0; i < 4; i++)
				m_worldM(2, i) = -m_worldM(2, i);

		return true;
	}

	return false;
}

void WBone::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type);
	m_bAltered = true;
}

WError WBone::Load(std::istream& inputStream) {
	WVector3 posULR[4];
	inputStream.read((char*)&m_worldM, sizeofBoneNoPtrs);
	inputStream.read((char*)posULR, sizeof(WVector3) * 4);
	SetPosition(posULR[0]);
	SetULRVectors(posULR[1], posULR[2], posULR[3]);
	unsigned int numChildren = 0;
	inputStream.read((char*)&numChildren, 4);
	for (unsigned int i = 0; i < numChildren; i++) {
		WBone* child = new WBone();
		child->SetParent(this);
		AddChild(child);
		child->Load(inputStream);
	}

	return WError(W_SUCCEEDED);
}

WError WBone::Save(std::ostream& outputStream) const {
	WVector3 posULR[4] = { GetPosition(), GetUVector(), GetLVector(), GetRVector() };
	outputStream.write((char*)&m_worldM, sizeofBoneNoPtrs);
	outputStream.write((char*)posULR, sizeof(WVector3) * 4);
	unsigned int numChildren = GetNumChildren();
	outputStream.write((char*)&numChildren, 4);
	for (unsigned int i = 0; i < numChildren; i++)
		m_children[i]->Save(outputStream);

	return WError(W_SUCCEEDED);
}

struct _WSkeletalFrame : public W_FRAME {
	WBone* baseBone;
	vector<WBone*> boneV;

	_WSkeletalFrame() {
		baseBone = nullptr;
	};
	~_WSkeletalFrame() {
		W_SAFE_DELETE(baseBone);
	}

	void ConstructHierarchyFromBase(WBone* base, WBone* currentBone, unsigned int* index) {
		base->UpdateLocals();
		boneV.push_back(currentBone);
		currentBone->SetIndex(base->GetIndex());
		char baseName[64];
		base->GetName(baseName, 64);
		currentBone->SetName(baseName);
		currentBone->SetToRotation(base);
		currentBone->SetPosition(base->GetPosition());
		currentBone->SetInvBindingPose(base->GetInvBindingPose());
		currentBone->UpdateLocals();
		if (index)
			currentBone->SetIndex(*index);
		WBone* curChild = nullptr;
		unsigned int i = -1;
		while (curChild = base->GetChild(++i)) {
			WBone* newBone = new WBone();
			newBone->SetParent(currentBone);
			currentBone->AddChild(newBone);
			if (index)
				(*index)++;
			ConstructHierarchyFromBase(base->GetChild(i), newBone, index);
		}
	}
};

WSkeleton::WSkeleton(Wasabi* const app, unsigned int ID) : WAnimation(app, ID) {
	//delete what the base had created, a skeleton has its unique subanimation structure
	for (unsigned int i = 0; i < m_subAnimations.size(); i++)
		delete m_subAnimations[i];
	m_subAnimations.clear();
	//add that new structure as a subanimation. casting these should be safe now
	m_subAnimations.push_back(new W_SKELETAL_SUB_ANIMATION());

	m_boneTex = nullptr;
	m_bindingScale = WVector3(1.0f, 1.0f, 1.0f);
	m_parentBonePos = WVector3(0, 0, 0);
}

WSkeleton::~WSkeleton() {
	for (unsigned int i = 0; i < m_bindings.size(); i++)
		m_bindings[i].obj->RemoveBinding();
	W_SAFE_REMOVEREF(m_boneTex);
}

WError WSkeleton::CreateKeyFrame(WBone* baseBone, float fTime) {
	if (!WAnimation::m_bFramesOwner)
		return WError(W_ERRORUNK);
	if (!baseBone)
		return WError(W_INVALIDPARAM);
	if (baseBone->GetParent()) //parent must be nullptr, this is the base bone
		return WError(W_INVALIDPARAM);
	if (fTime < 0.01f)
		fTime += 0.01f;

	_WSkeletalFrame* f = new _WSkeletalFrame();
	f->fTime = fTime;
	WAnimation::m_frames.push_back(f);
	m_totalTime += fTime;

	f->baseBone = new WBone();
	unsigned int index = 0;
	f->ConstructHierarchyFromBase(baseBone, f->baseBone, baseBone->GetIndex() == -1 ? &index : nullptr);

	WError err = WError(W_SUCCEEDED);

	if (!m_boneTex) { //first frame will create the texture
		float fExactWidth = sqrtf(f->boneV.size() * 4);
		unsigned int texWidth = 2;
		while (fExactWidth > texWidth)
			texWidth *= 2;

		float* texData = new float[texWidth * texWidth * 4];
		for (unsigned int i = 0; i < f->boneV.size(); i++) {
			WMatrix mtx = f->boneV[i]->GetInvBindingPose() * f->boneV[i]->GetRelativeMatrix();
			memcpy(&((char*)texData)[i * sizeof(WMatrix)], &mtx, sizeof(WMatrix) - 4 * sizeof(float));
		}
		int oldMips = m_app->GetEngineParam<int>("numGeneratedMips");
		m_app->SetEngineParam<int>("numGeneratedMips", 1);
		m_boneTex = m_app->ImageManager->CreateImage(texData, texWidth, texWidth, VK_FORMAT_R32G32B32A32_SFLOAT, W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_DYNAMIC | W_IMAGE_CREATE_REWRITE_EVERY_FRAME);
		m_app->SetEngineParam<int>("numGeneratedMips", oldMips);
		W_SAFE_DELETE_ARRAY(texData);

		if (!m_boneTex)
			err = WError(W_OUTOFMEMORY);
	}

	return err;
}

WError WSkeleton::DeleteKeyFrame(unsigned int frame) {
	if (frame >= WAnimation::m_frames.size())
		return WError(W_INVALIDPARAM);

	if (WAnimation::m_bFramesOwner)
		delete WAnimation::m_frames[frame];
	WAnimation::m_frames.erase(WAnimation::m_frames.begin() + frame);

	return WError(W_SUCCEEDED);
}

WBone* WSkeleton::GetKeyFrame(unsigned int frame) {
	if (frame >= WAnimation::m_frames.size())
		return nullptr;

	return ((_WSkeletalFrame*)WAnimation::m_frames[frame])->baseBone;
}

void WSkeleton::AddSubAnimation() {
	WAnimation::m_subAnimations.push_back(new W_SKELETAL_SUB_ANIMATION);
}

void WSkeleton::SetSubAnimationBaseBone(unsigned int subAnimation, unsigned int boneIndex, unsigned int parentSubAnimation) {
	if (!WAnimation::m_frames.size())
		return;
	if (subAnimation >= WAnimation::m_subAnimations.size())
		return;

	if (boneIndex != -1) {
		unsigned int indexInVector = -1;
		for (unsigned int i = 0; i < ((_WSkeletalFrame*)WAnimation::m_frames[0])->boneV.size() && indexInVector == -1; i++)
			if (((_WSkeletalFrame*)WAnimation::m_frames[0])->boneV[i]->GetIndex() == boneIndex)
				indexInVector = i;
		if (indexInVector == -1)
			return;

		//get all involved bones, start with the parent (at boneIndex) and store all children indices
		WBone* baseBone = ((_WSkeletalFrame*)WAnimation::m_frames[0])->boneV[indexInVector];
		((W_SKELETAL_SUB_ANIMATION*)WAnimation::m_subAnimations[subAnimation])->BuildIndices(baseBone);
		if (parentSubAnimation != -1) {
			if (baseBone->GetParent()) {
				unsigned int parentIndex = baseBone->GetParent()->GetIndex();
				((W_SKELETAL_SUB_ANIMATION*)WAnimation::m_subAnimations[subAnimation])->parentIndex = parentIndex;
				((W_SKELETAL_SUB_ANIMATION*)WAnimation::m_subAnimations[subAnimation])->parentSubAnimation = parentSubAnimation;
			}
		}
	} else // -1 means all bones are involved, so clear the indices to specify that
		((W_SKELETAL_SUB_ANIMATION*)WAnimation::m_subAnimations[subAnimation])->boneIndices.clear();
}

void WSkeleton::Update(float fDeltaTime) {
	WAnimation::Update(fDeltaTime);

	if (WAnimation::m_frames.size()) {
		unsigned int texWidth = m_boneTex->GetWidth();
		float* texData = nullptr;
		m_boneTex->MapPixels((void**)&texData, W_MAP_WRITE);
		if (!texData)
			return;

		for (unsigned int anim = 0; anim < WAnimation::m_subAnimations.size(); anim++) {
			W_SKELETAL_SUB_ANIMATION* curSubAnim = ((W_SKELETAL_SUB_ANIMATION*)WAnimation::m_subAnimations[anim]);
			unsigned int curFrameIndex = curSubAnim->curFrame;
			unsigned int nextFrameIndex = curSubAnim->nextFrame;
			_WSkeletalFrame* curFrame = (_WSkeletalFrame*)(WAnimation::m_frames[curFrameIndex]);
			_WSkeletalFrame* nextFrame = (_WSkeletalFrame*)(WAnimation::m_frames[nextFrameIndex]);

			/*if ( curFrameIndex + 1 < WAnimation::m_frames.size ( ) ) {
			nextFrame = (_WSkeletalFrame*)(WAnimation::m_frames[curFrameIndex+1]);
			else //we're on last frame, next is the first
			nextFrame = (_WSkeletalFrame*)(WAnimation::m_frames[curSubAnim->firstFrame]);*/

			float fTimeBeforeFrame = 0.0f;
			for (unsigned int i = 0; i < curFrameIndex; i++)
				fTimeBeforeFrame += ((_WSkeletalFrame*)WAnimation::m_frames[i])->fTime;

			float fLerpValue = (curSubAnim->fCurrentTime - fTimeBeforeFrame) / curFrame->fTime;
			if (fLerpValue >= 1.0f)
				fLerpValue = 1.0f;

			if (curSubAnim->boneIndices.size()) //if we have any specified indices
			{
				//first, ajust parents to synchronize with the parent sub animation
				WBone* curFrameOldParent = nullptr;
				WBone* nextFrameOldParent = nullptr;
				if (curSubAnim->parentSubAnimation != -1) {
					unsigned int boneIndex = curSubAnim->boneIndices[0];
					unsigned int boneInVector = -1;
					for (unsigned int k = 0; k < curFrame->boneV.size() && boneInVector == -1; k++)
						if (curFrame->boneV[k]->GetIndex() == boneIndex)
							boneInVector = k;
					WBone* curFrameBone = curFrame->boneV[boneInVector];
					WBone* nextFrameBone = nextFrame->boneV[boneInVector];
					curFrameOldParent = curFrameBone->GetParent();
					nextFrameOldParent = nextFrameBone->GetParent();
					W_SKELETAL_SUB_ANIMATION* parentSubAnim =
						((W_SKELETAL_SUB_ANIMATION*)WAnimation::m_subAnimations[curSubAnim->parentSubAnimation]);
					_WSkeletalFrame* parentFrame =
						(_WSkeletalFrame*)(WAnimation::m_frames[parentSubAnim->curFrame]);
					for (unsigned int k = 0; k < parentFrame->boneV.size(); k++)
						if (parentFrame->boneV[k]->GetIndex() == curSubAnim->parentIndex) {
							curFrameBone->SetParent(parentFrame->boneV[k]);
							nextFrameBone->SetParent(parentFrame->boneV[k]);
						}
				}

				for (unsigned int i = 0; i < curSubAnim->boneIndices.size(); i++) {
					unsigned int boneIndex = curSubAnim->boneIndices[i];
					unsigned int boneInVector = -1;
					for (unsigned int k = 0; k < curFrame->boneV.size() && boneInVector == -1; k++)
						if (curFrame->boneV[k]->GetIndex() == boneIndex)
							boneInVector = k;

					WBone* curFrameBone = curFrame->boneV[boneInVector];
					WBone* nextFrameBone = nextFrame->boneV[boneInVector];
					//update each frame's bones
					curFrameBone->UpdateLocals();
					nextFrameBone->UpdateLocals();

					//lerp the bones' matrices to get the matrix for the current, in-between frame
					WMatrix curFrameMtx = curFrameBone->GetRelativeMatrix();
					WMatrix nextFrameMtx = nextFrameBone->GetRelativeMatrix();
					WMatrix curFrameMtxF = curFrameBone->GetInvBindingPose() * curFrameMtx;
					WMatrix nextFrameMtxF = nextFrameBone->GetInvBindingPose() * nextFrameMtx;
					WMatrix finalMatrix = curFrameMtxF * (1.0f - fLerpValue) + nextFrameMtxF * fLerpValue;

					if (m_bindings.size()) {
						WMatrix bindMtx = curFrameMtx * (1.0f - fLerpValue) + nextFrameMtx * fLerpValue;
						for (int j = 0; j < 4; j++)
							bindMtx(2, j) = -bindMtx(2, j);
						for (unsigned int j = 0; j < m_bindings.size(); j++)
							if (m_bindings[j].boneID == boneIndex)
								m_bindings[j].obj->SetBindingMatrix(bindMtx * WScalingMatrix(m_bindingScale));
					}

					//encode the matrix (decoded in the shader to save a texture fetch in the VS)
					finalMatrix(0, 3) = finalMatrix(3, 0);
					finalMatrix(1, 3) = finalMatrix(3, 1);
					finalMatrix(2, 3) = finalMatrix(3, 2);
					memcpy(&((char*)texData)[boneIndex * sizeof(WMatrix)], &finalMatrix, sizeof(WMatrix) - sizeof(float) * 4);
				}

				//set parents back to normal
				if (curSubAnim->parentSubAnimation != -1) {
					unsigned int boneIndex = curSubAnim->boneIndices[0];
					unsigned int boneInVector = -1;
					for (unsigned int k = 0; k < curFrame->boneV.size() && boneInVector == -1; k++)
						if (curFrame->boneV[k]->GetIndex() == boneIndex)
							boneInVector = k;
					curFrame->boneV[boneInVector]->SetParent(curFrameOldParent);
					nextFrame->boneV[boneInVector]->SetParent(nextFrameOldParent);
				}
			} else //no indices means all bones
			{
				for (unsigned int i = 0; i < curFrame->boneV.size(); i++) {
					//update parent position
					if (curFrame->boneV[i]->GetParent() == nullptr)
						m_parentBonePos = curFrame->boneV[i]->GetPosition();

					unsigned int boneIndex = curFrame->boneV[i]->GetIndex();

					WBone* curFrameBone = curFrame->boneV[i];
					WBone* nextFrameBone = nextFrame->boneV[i];
					//update each frame's bones
					curFrameBone->UpdateLocals();
					nextFrameBone->UpdateLocals();
					//lerp the bones' matrices to get the matrix for the current, in-between frame
					WMatrix curFrameMtx = curFrameBone->GetRelativeMatrix();
					WMatrix nextFrameMtx = nextFrameBone->GetRelativeMatrix();
					WMatrix curFrameMtxF = curFrameBone->GetInvBindingPose() * curFrameMtx;
					WMatrix nextFrameMtxF = nextFrameBone->GetInvBindingPose() * nextFrameMtx;
					WMatrix finalMatrix = curFrameMtxF * (1.0f - fLerpValue) + nextFrameMtxF * fLerpValue;

					if (m_bindings.size()) {
						WMatrix bindMtx = curFrameMtx * (1.0f - fLerpValue) + nextFrameMtx * fLerpValue;
						for (int j = 0; j < 4; j++)
							bindMtx(2, j) = -bindMtx(2, j);
						for (unsigned int j = 0; j < m_bindings.size(); j++)
							if (m_bindings[j].boneID == boneIndex)
								m_bindings[j].obj->SetBindingMatrix(bindMtx * WScalingMatrix(m_bindingScale));
					}

					//encode the matrix (decoded in the shader to save a texture fetch in the VS)
					finalMatrix(0, 3) = finalMatrix(3, 0);
					finalMatrix(1, 3) = finalMatrix(3, 1);
					finalMatrix(2, 3) = finalMatrix(3, 2);
					memcpy(&((char*)texData)[boneIndex * sizeof(WMatrix)], &finalMatrix, sizeof(WMatrix) - sizeof(float) * 4);
				}
			}
		}

		m_boneTex->UnmapPixels();
	}
}

WImage* WSkeleton::GetTexture() const {
	return m_boneTex;
}

WBone* WSkeleton::GetBone(unsigned int frame, unsigned int index) const {
	if (frame < WAnimation::m_frames.size()) {
		for (unsigned int i = 0; i < ((_WSkeletalFrame*)WAnimation::m_frames[frame])->boneV.size(); i++)
			if (((_WSkeletalFrame*)WAnimation::m_frames[frame])->boneV[i]->GetIndex() == index)
				return ((_WSkeletalFrame*)WAnimation::m_frames[frame])->boneV[i];
	}
	return nullptr;
}

WBone* WSkeleton::GetBone(unsigned int frame, std::string name) const {
	if (frame < WAnimation::m_frames.size()) {
		for (unsigned int i = 0; i < ((_WSkeletalFrame*)WAnimation::m_frames[frame])->boneV.size(); i++) {
			char curName[64];
			((_WSkeletalFrame*)WAnimation::m_frames[frame])->boneV[i]->GetName(curName, 64);
			if (strcmp(curName, name.c_str()) == 0)
				return ((_WSkeletalFrame*)WAnimation::m_frames[frame])->boneV[i];
		}
	}
	return nullptr;
}

void WSkeleton::Scale(float scale) {
	for (int i = 0; i < WAnimation::m_frames.size(); i++)
		((_WSkeletalFrame*)WAnimation::m_frames[i])->baseBone->Scale(scale, scale, scale);
}

void WSkeleton::Scale(float x, float y, float z) {
	for (int i = 0; i < WAnimation::m_frames.size(); i++)
		((_WSkeletalFrame*)WAnimation::m_frames[i])->baseBone->Scale(x, y, z);
}

void WSkeleton::Scale(WVector3 scale) {
	for (int i = 0; i < WAnimation::m_frames.size(); i++)
		((_WSkeletalFrame*)WAnimation::m_frames[i])->baseBone->Scale(scale);
}

void WSkeleton::BindToBone(WOrientation* obj, unsigned int boneID) {
	m_bindings.push_back(WSkeleton::BONEBIND(obj, boneID));
}

void WSkeleton::UnbindFromBone(WOrientation* obj, unsigned int boneID) {
	for (unsigned int i = 0; i < m_bindings.size(); i++)
		if (m_bindings[i].obj == obj && m_bindings[i].boneID == boneID)
			m_bindings.erase(m_bindings.begin() + i);
	obj->RemoveBinding();
}

void WSkeleton::UnbindFromBone(unsigned int boneID) {
	for (unsigned int i = 0; i < m_bindings.size(); i++)
		if (m_bindings[i].boneID == boneID) {
			m_bindings[i].obj->RemoveBinding();
			m_bindings.erase(m_bindings.begin() + i);
		}
}

void WSkeleton::SetBindingScale(float scale) {
	m_bindingScale = WVector3(scale, scale, scale);
}

void WSkeleton::SetBindingScale(float x, float y, float z) {
	m_bindingScale = WVector3(x, y, z);
}

void WSkeleton::SetBindingScale(WVector3 scale) {
	m_bindingScale = scale;
}

WVector3 WSkeleton::GetCurrentParentBonePosition() {
	return m_parentBonePos;
}

WError	WSkeleton::CopyFrom(const WAnimation* const from) {
	if (!from)
		return WError(W_INVALIDPARAM);
	if (!from->Valid())
		return WError(W_INVALIDPARAM);

	//delete the bone texture and any existing frames
	W_SAFE_REMOVEREF(m_boneTex);
	if (WAnimation::m_bFramesOwner)
		for (unsigned int i = 0; i < WAnimation::m_frames.size(); i++)
			W_SAFE_DELETE(WAnimation::m_frames[i]);
	m_frames.clear();
	m_totalTime = 0.0f;
	//delete all subanimations - start a fresh object with only 1
	for (unsigned int i = 0; i < m_subAnimations.size(); i++)
		delete m_subAnimations[i];
	m_subAnimations.clear();
	//add that new structure as a subanimation. casting these should be safe now
	m_subAnimations.push_back(new W_SKELETAL_SUB_ANIMATION());
	WAnimation::m_bFramesOwner = true;

	WSkeleton* fromS = (WSkeleton*)from;
	m_bindingScale = fromS->m_bindingScale;
	for (unsigned int i = 0; i < fromS->WAnimation::m_frames.size(); i++) {
		_WSkeletalFrame* frame = (_WSkeletalFrame*)fromS->WAnimation::m_frames[i];
		WError err = CreateKeyFrame(frame->baseBone, frame->fTime);
		if (!err)
			return err;
	}

	return WError(W_SUCCEEDED);
}

WError WSkeleton::UseAnimationFrames(const WAnimation* const anim) {
	if (!anim)
		return WError(W_INVALIDPARAM);
	if (!anim->Valid())
		return WError(W_INVALIDPARAM);

	//delete the bone texture and any existing frames
	W_SAFE_REMOVEREF(m_boneTex);
	if (WAnimation::m_bFramesOwner)
		for (unsigned int i = 0; i < WAnimation::m_frames.size(); i++)
			W_SAFE_DELETE(WAnimation::m_frames[i]);
	m_frames.clear();
	m_totalTime = 0.0f;
	//delete all subanimations - start a fresh object with only 1
	for (unsigned int i = 0; i < m_subAnimations.size(); i++)
		delete m_subAnimations[i];
	m_subAnimations.clear();
	//add that new structure as a subanimation. casting these should be safe now
	m_subAnimations.push_back(new W_SKELETAL_SUB_ANIMATION());
	WAnimation::m_bFramesOwner = false;

	WSkeleton* fromS = (WSkeleton*)anim;
	m_bindingScale = fromS->m_bindingScale;
	for (unsigned int i = 0; i < fromS->WAnimation::m_frames.size(); i++) {
		WAnimation::m_frames.push_back(fromS->WAnimation::m_frames[i]);
		m_totalTime += fromS->WAnimation::m_frames[i]->fTime;
	}
	int oldMips = m_app->GetEngineParam<int>("numGeneratedMips");
	m_app->SetEngineParam<int>("numGeneratedMips", 1);
	m_boneTex = m_app->ImageManager->CreateImage(fromS->m_boneTex, W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_DYNAMIC | W_IMAGE_CREATE_REWRITE_EVERY_FRAME);
	m_app->SetEngineParam<int>("numGeneratedMips", oldMips);
	if (!m_boneTex)
		return WError(W_OUTOFMEMORY);

	return WError(W_SUCCEEDED);
}

bool WSkeleton::Valid() const {
	return WAnimation::m_frames.size() && m_boneTex;
}

WError WSkeleton::SaveToStream(WFile* file, std::ostream& outputStream) {
	if (!Valid())
		return WError(W_NOTVALID);

	//Format: <NUMFRAMES><FRAME 1><FRAME 2>...<FRAME NUMFRAMES-1>
	//Format: where <FRAME n>: <fTime><BASICBONE:sizeofBoneNoPtrs><NUMCHILDREN><BASICBONE><NUMCHILDREN> (recursize)
	unsigned int numFrames = WAnimation::m_frames.size();
	outputStream.write((char*)&numFrames, 4);
	for (unsigned int i = 0; i < numFrames; i++) {
		_WSkeletalFrame* curFrame = (_WSkeletalFrame*)WAnimation::m_frames[i];
		float fTime = curFrame->fTime;
		outputStream.write((char*)&fTime, 4);
		//update all the matrices
		vector<WBone*> boneStack;
		boneStack.push_back(curFrame->baseBone);
		while (boneStack.size()) {
			WBone* b = boneStack[boneStack.size() - 1];
			boneStack.pop_back();
			b->UpdateLocals();
			for (unsigned int i = 0; i < b->GetNumChildren(); i++)
				boneStack.push_back(b->GetChild(i));
		}
		WError err = curFrame->baseBone->Save(outputStream);
		if (!err)
			return err;
	}

	return WError(W_SUCCEEDED);
}

std::vector<void*> WSkeleton::LoadArgs() {
	return std::vector<void*>();
}

WError WSkeleton::LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args) {
	//delete the bone texture and any existing frames
	W_SAFE_REMOVEREF(m_boneTex);
	if (WAnimation::m_bFramesOwner)
		for (unsigned int i = 0; i < WAnimation::m_frames.size(); i++)
			W_SAFE_DELETE(WAnimation::m_frames[i]);
	m_frames.clear();
	m_totalTime = 0.0f;
	//delete all subanimations - start a fresh object with only 1
	for (unsigned int i = 0; i < m_subAnimations.size(); i++)
		delete m_subAnimations[i];
	m_subAnimations.clear();
	//add that new structure as a subanimation. casting these should be safe now
	m_subAnimations.push_back(new W_SKELETAL_SUB_ANIMATION());
	WAnimation::m_bFramesOwner = true;

	WError ret = WError(W_SUCCEEDED);

	//Format: <NUMFRAMES><FRAME 1><FRAME 2>...<FRAME NUMFRAMES-1>
	//Format: where <FRAME n>: <fTime><BASICBONE:sizeofBoneNoPtrs><NUMCHILDREN><BASICBONE><NUMCHILDREN> (recursize)
	unsigned int numFrames = 0;
	inputStream.read((char*)&numFrames, 4);
	for (unsigned int i = 0; i < numFrames; i++) {
		float fTime = 0.0f;
		inputStream.read((char*)&fTime, 4);

		WBone* baseBone = new WBone();
		baseBone->Load(inputStream);

		ret = CreateKeyFrame(baseBone, fTime);
		delete baseBone;

		if (!ret) {
			break;
		}
	}

	return ret;
}
