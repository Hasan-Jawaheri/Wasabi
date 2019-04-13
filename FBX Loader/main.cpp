#include "main.h"
#include "util.h"

#include <Renderers/ForwardRenderer/WForwardRenderer.h>
#include <Renderers/DeferredRenderer/WDeferredRenderer.h>

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
	WFile* m_file;

	void SaveMesh(MESHDATA m, LPCSTR filename) {
		printf("Writing mesh %s to file %s...\n", m.name, filename);
		WGeometry* g = new WGeometry(this);
		g->SetName(std::string(m.name) + "-geometry");
		W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_DYNAMIC;
		if (!m.bTangents)
			flags |= W_GEOMETRY_CREATE_CALCULATE_TANGENTS;
		g->CreateFromData(m.vb.data(), m.vb.size(), m.ib.data(), m.ib.size(), flags);
		if (m.ab.size())
			g->CreateAnimationData(m.ab.data());

		WObject* obj = ObjectManager->CreateObject();
		obj->SetName(m.name);
		obj->SetGeometry(g);
		g->RemoveReference();

		for (uint i = 0; i < m.textures.size(); i++) {
			char drive[64];
			char ext[64];
			char dir[512];
			char filename[512];
			_splitpath_s(m.textures[i], drive, 64, dir, 512, filename, 512, ext, 64);

			WImage* img = new WImage(this);
			WError err = img->Load(m.textures[i], W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_DYNAMIC);
			if (err.m_error == W_FILENOTFOUND) {
				err = img->Load(std::string(filename) + std::string(ext), W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_DYNAMIC);
			}
			if (err) {
				img->SetName(std::string(filename));
				obj->GetMaterial()->SetTexture("diffuseTexture", img, i);
				img->RemoveReference();
			} else {
				printf("Failed to load image %s\n", m.textures[i]);
			}
		}
		obj->GetMaterial()->SetName(std::string(m.name) + "-material");

		m_file->SaveAsset(obj);
		obj->RemoveReference();
	}

	void SaveAnimation(ANIMDATA anim, LPCSTR filename) {
		printf("Writing animation %s to file %s...\n", anim.name, filename);
		WSkeleton* s = new WSkeleton(this);
		for (auto it = anim.frames.begin(); it != anim.frames.end(); it++)
			s->CreateKeyFrame(*it, 1.0f);
		m_file->SaveAsset(s);
		s->RemoveReference();
	}

public:

	virtual WRenderer* CreateRenderer() {
		if (true)
			return new WForwardRenderer(this);
		else
			return new WDeferredRenderer(this);
	}

	WWindowAndInputComponent* CreateWindowAndInputComponent() {
		WWindowAndInputComponent* component = Wasabi::CreateWindowAndInputComponent();
		engineParams["windowStyle"] = (void*)((DWORD)engineParams["windowStyle"] & (~WS_VISIBLE));
		return component;
	}

	WError Setup() {
		WError ret = StartEngine(2, 2);
		if (ret) {
			RedirectIOToConsole();

			std::fstream f;
			f.open("data.WSBI", ios::out);
			f.close();

			m_file = new WFile(this);
			ret = m_file->Open("data.WSBI");
			if (!ret) {
				printf("Failed to open output file: %s\n", ret.AsString().c_str());
				return ret;
			}

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

			m_file->Close();
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
