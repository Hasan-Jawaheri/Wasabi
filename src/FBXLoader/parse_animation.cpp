#include "main.h"

struct RAWBONE {
	float vals[9];
	bool bIsKeyFrame[9];
	char name[64];
	uint32_t id;
	EFbxRotationOrder lRotationOrder;
	FbxAMatrix matrix;
	RAWBONE* parent;
	vector<RAWBONE*> children;
	vector<RAWBONE*> siblings;

	RAWBONE() { parent = nullptr; ZeroMemory(&vals, sizeof(float) * 9 + sizeof(bool) * 9); }
	~RAWBONE() { for (int i = 0; i < children.size(); i++) delete children[i]; }
};

struct KEYPOINT {
	float* valPtr;
	bool bValIsKey;
};

int GetCurveMaxFrame(FbxAnimCurve* pCurve) {
	int lKeyCount = pCurve->KeyGetCount();
	int maxFrame = 0;

	for (int lCount = 0; lCount < lKeyCount; lCount++) {
		FbxTime lKeyTime = pCurve->KeyGetTime(lCount);
		long long frame = lKeyTime.GetFrameCount();
		maxFrame = max(maxFrame, frame);
	}

	return maxFrame;
}
int GetNodeMaxFrame(FbxNode* pNode, FbxAnimLayer* pAnimLayer) {
	FbxProperty* props[3] = { &pNode->LclTranslation, &pNode->LclRotation, &pNode->LclScaling };
	const char* comp[3] = { "X", "Y", "Z" };
	int maxFrame = 0;

	for (int i = 0; i < 9; i++) {
		FbxAnimCurve* lAnimCurve = props[i / 3]->GetCurve(pAnimLayer, comp[i % 3]);
		if (lAnimCurve)
			maxFrame = max(maxFrame, GetCurveMaxFrame(lAnimCurve));
	}

	return maxFrame;
}
int GetLayerMaxFrame(FbxAnimLayer* pAnimLayer, FbxNode* pNode, RAWBONE* bone) {
	int maxFrame = GetNodeMaxFrame(pNode, pAnimLayer);
	pNode->GetRotationOrder(FbxNode::eSourcePivot, bone->lRotationOrder);
	strcpy_s(bone->name, 64, pNode->GetName());
	bone->id = GetNodeID(pNode);

	int numChildren = pNode->GetChildCount();
	for (int i = 0; i < numChildren; i++) {
		RAWBONE* child = new RAWBONE;
		child->parent = bone;
		bone->children.push_back(child);
		int newMax = GetLayerMaxFrame(pAnimLayer, pNode->GetChild(i), child);
		maxFrame = max(maxFrame, newMax);
	}

	return maxFrame;
}
int GetCurveMinFrame(FbxAnimCurve* pCurve) {
	int lKeyCount = pCurve->KeyGetCount();
	int minFrame = INT_MAX;

	for (int lCount = 0; lCount < lKeyCount; lCount++) {
		FbxTime lKeyTime = pCurve->KeyGetTime(lCount);
		long long frame = lKeyTime.GetFrameCount();
		minFrame = min(minFrame, frame);
	}

	return minFrame;
}
int GetNodeMinFrame(FbxNode* pNode, FbxAnimLayer* pAnimLayer) {
	FbxProperty* props[3] = { &pNode->LclTranslation, &pNode->LclRotation, &pNode->LclScaling };
	const char* comp[3] = { "X", "Y", "Z" };
	int minFrame = INT_MAX;

	for (int i = 0; i < 9; i++) {
		FbxAnimCurve* lAnimCurve = props[i / 3]->GetCurve(pAnimLayer, comp[i % 3]);
		if (lAnimCurve)
			minFrame = min(minFrame, GetCurveMinFrame(lAnimCurve));
	}

	return minFrame;
}
int GetLayerMinFrame(FbxAnimLayer* pAnimLayer, FbxNode* pNode, RAWBONE* bone) {
	int minFrame = GetNodeMinFrame(pNode, pAnimLayer);
	pNode->GetRotationOrder(FbxNode::eSourcePivot, bone->lRotationOrder);
	strcpy_s(bone->name, 64, pNode->GetName());
	bone->id = GetNodeID(pNode);

	int numChildren = pNode->GetChildCount();
	for (int i = 0; i < numChildren; i++) {
		int newMin = GetLayerMinFrame(pAnimLayer, pNode->GetChild(i), bone->children[i]);
		minFrame = min(minFrame, newMin);
	}

	return minFrame;
}

void EnumNode(FbxNode* pNode, FbxAnimLayer* pAnimLayer, RAWBONE* bone, int minFrame = 0) {
	FbxProperty* props[3] = { &pNode->LclTranslation, &pNode->LclRotation, &pNode->LclScaling };
	const char* comp[3] = { "X", "Y", "Z" };

	for (int i = 0; i < 9; i++) {
		FbxAnimCurve* lAnimCurve = props[i / 3]->GetCurve(pAnimLayer, comp[i % 3]);
		if (lAnimCurve) {
			int lKeyCount = lAnimCurve->KeyGetCount();

			for (int lCount = 0; lCount < lKeyCount; lCount++) {
				float lKeyValue = static_cast<float> (lAnimCurve->KeyGetValue(lCount));
				FbxTime lKeyTime = lAnimCurve->KeyGetTime(lCount);
				//keyframes dont store positions so do it manually
				FbxAMatrix m = pNode->EvaluateGlobalTransform(lKeyTime);
				m = m.Inverse();
				FbxVector4 curPos = FbxVector4(-m.Get(3, 0), -m.Get(3, 1), -m.Get(3, 2));//pNode->EvaluateLocalTranslation ( lKeyTime );
				FbxVector4 curRot = pNode->EvaluateLocalRotation(lKeyTime);
				long long frame = lKeyTime.GetFrameCount() - minFrame;
				if (frame == 0) {
					bone->vals[0] = curPos[0];
					bone->vals[1] = curPos[1];
					bone->vals[2] = curPos[2];
					bone->vals[3] = curRot[0];
					bone->vals[4] = curRot[1];
					bone->vals[5] = curRot[2];
					//bone->vals[i] = lKeyValue;
					bone->bIsKeyFrame[i] = true;
				} else {
					bone->siblings[frame - 1]->vals[0] = curPos[0];
					bone->siblings[frame - 1]->vals[1] = curPos[1];
					bone->siblings[frame - 1]->vals[2] = curPos[2];
					bone->siblings[frame - 1]->vals[3] = curRot[0];
					bone->siblings[frame - 1]->vals[4] = curRot[1];
					bone->siblings[frame - 1]->vals[5] = curRot[2];
					//bone->siblings[frame-1]->vals[i] = lKeyValue;
					bone->siblings[frame - 1]->bIsKeyFrame[i] = true;
				}
			}
		}
	}
}

void ParseAnimationLayer(FbxAnimLayer* pAnimLayer, FbxNode* pNode, RAWBONE* bone, int minFrame) {
	EnumNode(pNode, pAnimLayer, bone, minFrame);

	int numChildren = pNode->GetChildCount();
	for (int i = 0; i < numChildren; i++)
		ParseAnimationLayer(pAnimLayer, pNode->GetChild(i), bone->children[i], minFrame);
}

void CreateSiblings(RAWBONE* bone, int numSiblings) {
	for (int i = 0; i < numSiblings; i++) {
		RAWBONE* sib = new RAWBONE;
		strcpy_s(sib->name, 64, bone->name);
		sib->id = bone->id;
		sib->lRotationOrder = bone->lRotationOrder;
		RAWBONE* parent = nullptr;
		if (bone->parent) {
			parent = bone->parent->siblings[i];
			bone->parent->siblings[i]->children.push_back(sib);
		}
		bone->siblings.push_back(sib);
	}

	for (int i = 0; i < bone->children.size(); i++)
		CreateSiblings(bone->children[i], numSiblings);
}
double LinearInterpolate(
	double y1, double y2,
	double mu) {
	return y1 * (1 - mu) + y2 * mu;
}
double CubicInterpolate(
	double y0, double y1,
	double y2, double y3,
	double mu) {
	double a0, a1, a2, a3, mu2;

	mu2 = mu * mu;
	a0 = y3 - y2 - y0 + y1;
	a1 = y0 - y1 - a0;
	a2 = y2 - y0;
	a3 = y1;

	return a0 * mu* mu2 + a1 * mu2 + a2 * mu + a3;
}
void ConnectRawBoneKeys(RAWBONE* rawBone) {
	//for each one of the 9 components, do the curve interpolation
	for (int component = 0; component < 9; component++) {
		vector<KEYPOINT> keypoints;
		KEYPOINT k;
		k.valPtr = &rawBone->vals[component];
		k.bValIsKey = rawBone->bIsKeyFrame[component];
		keypoints.push_back(k);
		for (int sib = 0; sib < rawBone->siblings.size(); sib++) {
			k.valPtr = &rawBone->siblings[sib]->vals[component];
			k.bValIsKey = rawBone->siblings[sib]->bIsKeyFrame[component];
			keypoints.push_back(k);
		}
		//first, extend the keys before the first and after last
		int veryFirstKey = -1, veryLastKey = -1;
		for (int i = 0; i < keypoints.size(); i++) {
			if (keypoints[i].bValIsKey) {
				veryLastKey = i;
				if (veryFirstKey == -1)
					veryFirstKey = i;
			}
		}
		if (veryFirstKey == -1 || veryLastKey == -1)
			veryFirstKey = veryLastKey = 0;
		for (int i = 0; i < veryFirstKey; i++)
			* keypoints[i].valPtr = *keypoints[veryFirstKey].valPtr;
		for (int i = veryLastKey + 1; i < keypoints.size(); i++)
			* keypoints[i].valPtr = *keypoints[veryLastKey].valPtr;

		//interpolate this component's keys
		int pt1 = -1, pt2 = -1;
		for (int i = 0; i < keypoints.size(); i++) {
			bool bInterpolate = false;
			if (keypoints[i].bValIsKey) {
				if (pt2 == -1) {
					if (pt1 == -1)
						pt1 = i;
					else {
						pt2 = i;
						bInterpolate = true;
					}
				} else {
					pt1 = pt2;
					pt2 = i;
					bInterpolate = true;
				}
			}
			if (bInterpolate) {
				float p1 = *keypoints[pt1].valPtr;
				float p2 = *keypoints[pt2].valPtr;
				int range = pt2 - pt1;
				for (int i = pt1 + 1; i < pt2; i++) {
					float lerpVal = (float)(i - pt1) / (float)range;
					*keypoints[i].valPtr = LinearInterpolate(p1, p2, lerpVal);
				}
			}
		}
	}

	for (int i = 0; i < rawBone->children.size(); i++)
		ConnectRawBoneKeys(rawBone->children[i]);
}

WBone* CreateBoneFromRawBone(RAWBONE* rawBone, FbxTime time) {
	WBone* bone = new WBone();
	bone->SetName(rawBone->name);
	bone->SetIndex(rawBone->id);
	bone->SetParent(nullptr);

	FbxNode* node = GetNodeByID(rawBone->id);
	if (node) {
		bool bIsParent = true;
		if (node->GetParent())
			if (node->GetParent()->GetNodeAttribute())
				bIsParent = false;
		rawBone->matrix = node->EvaluateLocalTransform(time);


		FbxAMatrix ibp = GetNodeBindingPose(node);
		FbxVector4 p = node->EvaluateLocalTranslation();

		if (bIsParent) {
			p = FbxVector4(rawBone->vals[1], -rawBone->vals[2], -rawBone->vals[0]);
			FbxAMatrix nodeGlobal = node->EvaluateGlobalTransform();
			rawBone->matrix = nodeGlobal.Inverse() * rawBone->matrix;
		}
		FbxAMatrix inv = rawBone->matrix.Inverse();

		WMatrix invBindingPose = WMatrix(ibp.Get(0, 0), ibp.Get(0, 1), ibp.Get(0, 2), ibp.Get(0, 3),
			ibp.Get(1, 0), ibp.Get(1, 1), ibp.Get(1, 2), ibp.Get(1, 3),
			ibp.Get(2, 0), ibp.Get(2, 1), ibp.Get(2, 2), ibp.Get(2, 3),
			ibp.Get(3, 0), ibp.Get(3, 1), ibp.Get(3, 2), ibp.Get(3, 3));
		WVector3 pos = WVector3(p[0], p[1], p[2]);
		WVector3 r = WVector3(inv.Get(0, 0), inv.Get(1, 0), inv.Get(2, 0));
		WVector3 u = WVector3(inv.Get(0, 1), inv.Get(1, 1), inv.Get(2, 1));
		WVector3 l = WVector3(inv.Get(0, 2), inv.Get(1, 2), inv.Get(2, 2));
		if (bIsParent) {
			/*WVector3 t = u;
			u = r;
			r = t;*/
			WVector3 t = l;
			l = -u;
			u = r;
			r = t;
		}
		bone->SetInvBindingPose(invBindingPose);
		bone->SetULRVectors(u, l, r);
		bone->SetPosition(pos);
		bone->UpdateLocals();
	}

	for (int i = 0; i < rawBone->children.size(); i++) {
		WBone* child = CreateBoneFromRawBone(rawBone->children[i], time);
		child->SetParent(bone);
		bone->AddChild(child);
	}

	return bone;
}

ANIMDATA CreateAnimationData(RAWBONE* rawBase) {
	ANIMDATA data;
	int numFrames = rawBase->siblings.size();
	FbxTime time;
	time.SetFrame(0);
	data.frames.push_back(CreateBoneFromRawBone(rawBase, time)); //frame 0 is not the siblings[0]
	for (int i = 0; i < numFrames; i++) {
		time.SetFrame(i + 1);
		data.frames.push_back(CreateBoneFromRawBone(rawBase->siblings[i], time));
	}
	return data;
}

bool IsBoneAnimated(RAWBONE* bone) {
	FbxAMatrix m = bone->matrix;
	for (int i = 0; i < bone->siblings.size(); i++)
		for (int j = 0; j < 4; j++) {
			FbxDouble4 d1 = m[j];
			FbxDouble4 d2 = bone->siblings[i]->matrix[j];
			for (int k = 0; k < 4; k++)
				if (abs(d1[k] - d2[k]) > 0.01f)
					return true;
		}
	return false;
}
void PrintAnimatedBones(RAWBONE* bone) {
	if (IsBoneAnimated(bone))
		printf("Animated bone: %s with ID %d\n", bone->name, bone->id);

	for (int i = 0; i < bone->children.size(); i++)
		PrintAnimatedBones(bone->children[i]);
}

ANIMDATA ParseAnimation(FbxScene* pScene, FbxNode* node) {
	ANIMDATA ret;
	if (pScene->GetSrcObjectCount<FbxAnimStack>()) {
		FbxAnimStack* lAnimStack = pScene->GetSrcObject<FbxAnimStack>(0);
		const char* name = lAnimStack->GetName();

		int nbAnimLayers = lAnimStack->GetMemberCount<FbxAnimLayer>();

		if (nbAnimLayers) {
			FbxAnimLayer* lAnimLayer = lAnimStack->GetMember<FbxAnimLayer>(0);
			RAWBONE* rawBone = new RAWBONE;
			int maxFrame = GetLayerMaxFrame(lAnimLayer, node, rawBone);
			int minFrame = GetLayerMinFrame(lAnimLayer, node, rawBone);
			CreateSiblings(rawBone, maxFrame - minFrame);
			//ParseAnimationLayer ( lAnimLayer, node, rawBone, minFrame );
			//only parent node needs to be explicitly parsed
			EnumNode(node, lAnimLayer, rawBone, minFrame);
			ConnectRawBoneKeys(rawBone);
			ret = CreateAnimationData(rawBone);

			PrintAnimatedBones(rawBone);

			for (int i = 0; i < rawBone->siblings.size(); i++)
				delete rawBone->siblings[i];
			delete rawBone;
		}
	}
	ret.name = node->GetName();
	return ret;
}