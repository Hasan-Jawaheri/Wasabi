#include "main.h"
#include "util.h"


/*
	Source: http://www.gamedev.net/topic/619416-fbx-skeleton-animation-need-help/

1. Get all bones from an animation for example
KFbxXMatrix matrix = node->EvaluateLocalTransform(time) * NodeHelper::getNodeTransform(node);
2. Then bine pose matrices for each corresponding mesh deformation cluster
KFbxXMatrix globalMatrix;
lCluster->GetTransformMatrix(globalMatrix);
KFbxXMatrix geomMatrix = NodeHelper::getNodeTransform(mesh->GetNode());
globalMatrix *= geomMatrix;
KFbxXMatrix skin;
lCluster->GetTransformLinkMatrix(skin);
skin = globalMatrix.Inverse() * skin;
skin = skin.Inverse();

bindMatrixArray[index] = skin;
3. Then i build the hierarchy with the node transforms
absoluteNodeMatrix[i] = localSpaceMatrix[i] * parentMatrix;
4. Apply the inverse bind pose matrix
for (unsigned int i = 0; i < bones; ++i)
{
ShaderReadyBoneMatrix[i] = bindMatrixArray[i] * absoluteNodeMatrix[i];
}
*/

vector<BONEIDTABLEENTITY> boneIDTable;
unordered_map<UINT, UINT> g_boneID_to_boneIndex;
unordered_map<FbxNode*, UINT> g_node_to_boneIndex;

void ClearBoneTable() {
	boneIDTable.clear();
	g_boneID_to_boneIndex.clear();
	g_node_to_boneIndex.clear();
}

bool NodeExists(FbxNode* pNode) {
	return g_node_to_boneIndex.find(pNode) != g_node_to_boneIndex.end();
}

void CreateNewNode(BONEIDTABLEENTITY entity) {
	boneIDTable.push_back(entity);
	g_boneID_to_boneIndex.insert(pair<UINT, UINT>(entity.mapsTo, boneIDTable.size() - 1));
	g_node_to_boneIndex.insert(pair<FbxNode*, UINT>(entity.node, boneIDTable.size() - 1));
}

UINT GetNodeID(FbxNode* pNode) {
	auto iter = g_node_to_boneIndex.find(pNode);
	if (iter != g_node_to_boneIndex.end())
		return boneIDTable[iter->second].mapsTo;

	//create a new mapping for it, it doesnt have one
	UINT newID = 0;
	while (g_boneID_to_boneIndex.find(++newID) != g_boneID_to_boneIndex.end()); //find the least unused id

	BONEIDTABLEENTITY e;
	e.mapsTo = newID;
	e.node = pNode;
	(e.bindingPose = FbxAMatrix()).SetIdentity();
	e.bindingPose = e.bindingPose.Inverse();
	CreateNewNode(e);
	return newID;
}

FbxNode* GetNodeByID(UINT ID) {
	auto iter = g_boneID_to_boneIndex.find(ID);
	if (iter != g_boneID_to_boneIndex.end())
		return boneIDTable[iter->second].node;
	return nullptr;
}

FbxAMatrix GetNodeBindingPose(FbxNode* pNode) {
	auto iter = g_node_to_boneIndex.find(pNode);
	if (iter != g_node_to_boneIndex.end())
		return boneIDTable[iter->second].bindingPose;
	return FbxAMatrix();
}

class FBXLoader : public Wasabi {
	void SaveMesh(MESHDATA m, LPCSTR filename) {
		printf("Writing mesh to file %s...\n", filename);
		WGeometry* g = new WGeometry(this);
		g->CreateFromData(m.vb.data(), m.vb.size(), m.ib.data(), m.ib.size(), true, false, !m.bTangents);
		if (m.ab.size())
			g->CreateAnimationData(m.ab.data());
		WFile file(this);
		file.Open(filename);
		file.SaveAsset(g, nullptr);
		file.Close();
		g->RemoveReference();
	}

	void SaveAnimation(ANIMDATA anim, LPCSTR filename) {
		printf("Writing animation to file %s...", filename);
		WSkeleton* s = new WSkeleton(this);
		for (auto it = anim.frames.begin(); it != anim.frames.end(); it++)
			s->CreateKeyFrame(*it, 1.0f);
		WFile file(this);
		file.Open(filename);
		file.SaveAsset(s, nullptr);
		file.Close();
		s->RemoveReference();
	}

public:

	WWindowAndInputComponent* CreateWindowAndInputComponent() {
		WWindowAndInputComponent* component = Wasabi::CreateWindowAndInputComponent();
		engineParams["windowStyle"] = (void*)((DWORD)engineParams["windowStyle"] & (~WS_VISIBLE));
		return component;
	}

	WError Setup() {
		WError ret = StartEngine(2, 2);
		if (ret) {
			RedirectIOToConsole();

			FbxManager* pManager = nullptr;
			FbxScene* pScene = nullptr;
			InitializeSdkObjects(pManager, pScene);

			/*
				Loop through the FBX objects and find meshes and skeletons to load.
			*/
			_WIN32_FIND_DATAA data;
			HANDLE h = FindFirstFileA("*.FBX", &data);
			if (h != INVALID_HANDLE_VALUE) {
				do {
					printf("Loading %s...\n", data.cFileName);
					if (LoadScene(pManager, pScene, data.cFileName)) {
						FbxAxisSystem::DirectX.ConvertScene(pScene);
						FbxAxisSystem::ECoordSystem sys = pScene->GetGlobalSettings().GetAxisSystem().GetCoorSystem();

						FbxNode* lNode = pScene->GetRootNode();

						if (lNode) {
							ClearBoneTable();

							vector<FbxNode*> meshes;
							vector<FbxNode*> skeletons;
							for (int i = 0; i < lNode->GetChildCount(); i++) {
								FbxNode* pNode = lNode->GetChild(i);
								if (pNode->GetNodeAttribute() != NULL) {
									switch (pNode->GetNodeAttribute()->GetAttributeType()) {
										case FbxNodeAttribute::eMesh:
											meshes.push_back(pNode);
											break;
										case FbxNodeAttribute::eSkeleton:
											skeletons.push_back(pNode);
											break;
									}
								}
							}

							printf("Found %d mesh(es) and %d skeletons in the file\n", meshes.size(), skeletons.size());

							for (auto it = meshes.begin(); it != meshes.end(); it++) {
								FbxNode* pNode = *it;
								FbxMesh* lMesh = (FbxMesh*)pNode->GetNodeAttribute();

								char meshName[256];
								strcpy_s(meshName, 256, (char*)pNode->GetName());
								MESHDATA mesh = ParseMesh(lMesh);
								if (mesh.vb.size()) {
									if (mesh.ib.size()) {
										char outFile[MAX_PATH];
										if (meshes.size() > 1)
											strcpy_s(outFile, MAX_PATH, (char*)pNode->GetName());
										else {
											strcpy_s(outFile, MAX_PATH, data.cFileName);
											for (UINT i = 0; i < strlen(outFile); i++)
												if (outFile[i] == '.')
													outFile[i] = '\0';
										}
										strcat_s(outFile, MAX_PATH, ".WSBI");
										SaveMesh(mesh, outFile);
									}
								}
							}

							for (auto it = skeletons.begin(); it != skeletons.end(); it++) {
								FbxNode* pNode = *it;
								ANIMDATA anim = ParseAnimation(pScene, pNode);
								if (anim.frames.size()) {
									//save the animation data
									char skeletonName[256];
									strcpy_s(skeletonName, 256, pNode->GetName());
									strcat_s(skeletonName, 256, ".WSBI");
									SaveAnimation(anim, skeletonName);
									for (int i = 0; i < anim.frames.size(); i++)
										delete anim.frames[i];
								}
							}
						}
					}
				} while (FindNextFileA(h, &data));
			} else
				printf("No .FBX files found in this directory.\n");

			DestroySdkObjects(pManager, 0);
		}
		return ret;
	}

	bool Loop(float) {
		return true;
	}

	void Cleanup() {

	}
};

Wasabi* WInitialize() {
	return new FBXLoader();
}
