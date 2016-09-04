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

struct VEC3 {
	VEC3 ( void ) : x ( 0 ), y ( 0 ), z ( 0 ) { }
	VEC3 ( float fx, float fy, float fz ) : x ( fx ), y ( fy ), z ( fz ) { }
	float x, y, z;
};
struct VEC2 {
	float x, y;
};
struct VERTEX {
	VEC3 pos;
	VEC3 tang;
	VEC3 norm;
	VEC2 uv;
};
struct ANIMVERTEX {
	UINT bones[4];
	float weights[4];
	ANIMVERTEX() { ZeroMemory ( this, sizeof ANIMVERTEX ); }
};
struct MESHDATA {
	vector<VERTEX> vb;
	vector<DWORD> ib;
	vector<ANIMVERTEX> ab;
	bool bTangents;
};
struct RAWBONE {
	float vals[9];
	bool bIsKeyFrame[9];
	char name[64];
	UINT id;
	EFbxRotationOrder lRotationOrder;
	FbxAMatrix matrix;
	RAWBONE* parent;
	vector<RAWBONE*> children;
	vector<RAWBONE*> siblings;

	RAWBONE() { parent = nullptr; ZeroMemory ( &vals, sizeof ( float ) * 9 + sizeof ( bool ) * 9 ); }
	~RAWBONE() { for ( int i = 0; i < children.size ( ); i++ ) delete children[i]; }
};
struct KEYPOINT {
	float* valPtr;
	bool bValIsKey;
};
struct ANIMDATA {
	vector<WBone*> frames;
};


struct BONEIDTABLEENTITY {
	FbxNode* node;
	FbxAMatrix bindingPose;
	UINT mapsTo;
};
vector<BONEIDTABLEENTITY> boneIDTable;
UINT GetNodeID ( FbxNode* pNode ) {
	for ( int i = 0; i < boneIDTable.size ( ); i++ )
		if ( boneIDTable[i].node == pNode )
			return boneIDTable[i].mapsTo;

	//create a new mapping for it, it doesnt have one
	UINT newID = 1;
	for ( int i = 0; i < boneIDTable.size ( ); i++ )
	{
		if ( boneIDTable[i].mapsTo == newID )
		{
			newID++;
			i = -1;
		}
	}

	BONEIDTABLEENTITY e;
	e.mapsTo = newID;
	e.node = pNode;
	(e.bindingPose = FbxAMatrix()).SetIdentity ( );
	e.bindingPose = e.bindingPose.Inverse ( );
	boneIDTable.push_back ( e );
	return newID;
}
FbxNode* GetIDNode ( UINT ID ) {
	for ( int i = 0; i < boneIDTable.size ( ); i++ )
		if ( boneIDTable[i].mapsTo == ID )
			return boneIDTable[i].node;

	return nullptr;
}
FbxAMatrix GetNodeBindingPose ( FbxNode* node ) {
	for ( int i = 0; i < boneIDTable.size ( ); i++ )
		if ( boneIDTable[i].node == node )
			return boneIDTable[i].bindingPose;

	return FbxAMatrix ( );
}

VEC3 VEC3CROSS ( VEC3 v1, VEC3 v2 ) {
	VEC3 v;
	v.x = v1.y * v2.z - v1.z * v2.y;
	v.y = v1.z * v2.x - v1.x * v2.z;
	v.z = v1.x * v2.y - v1.y * v2.x;
	return v;
}
float VEC3LENGTHSQ ( VEC3 v ) {
	return (v.x * v.x + v.y * v.y + v.z * v.z);
}
VEC3 VEC3NORMALIZE ( VEC3 v ) {
	float len = VEC3LENGTHSQ ( v );
	if ( len == 0 )
		return VEC3 ( );
	return VEC3 ( v.x / len, v.y / len, v.z / len );
}   
inline VEC3 GetRotation ( FbxAMatrix m )
{
	VEC3 ret;
	if ( m.Get(0,0) == 1.0f )
	{
		ret.x = atan2f(m.Get(0,2), m.Get(2,3));
		ret.y = 0;
		ret.z = 0;
	} else if (m.Get(0,0) == -1.0f)
	{
		ret.x = atan2f(m.Get(0,2), m.Get(2,3));
		ret.y = 0;
		ret.z = 0;
	} else {
		ret.x = atan2(-m.Get(2,0),m.Get(0,0));
		ret.y = asin(m.Get(1,0));
		ret.z = atan2(-m.Get(1,2),m.Get(1,1));
	}
	return ret;
}

MESHDATA ParseMesh ( FbxMesh* pMesh );
ANIMDATA ParseAnimation ( FbxScene* pScene, FbxNode* node );

class FBXLoader : public Wasabi {
	void SaveMesh(MESHDATA m, LPCSTR filename) {
		printf("Writing to file %s...", filename);
		WGeometry* g = new WGeometry(this);
		g->CreateFromData(m.vb.data(), m.vb.size(), m.ib.data(), m.ib.size(), true, false, !m.bTangents);
		if (m.ab.size())
			g->CreateAnimationData(m.ab.data());
		g->SaveToWGM(filename);
		g->RemoveReference();
		printf("done\n");
	}

	void SaveAnimation(ANIMDATA anim, LPCSTR filename) {
		//open the file for writing
		fstream file;
		file.open(filename, ios::out | ios::binary);
		if (!file.is_open())
			return;

		printf("Writing to file %s...", filename);
		//Format: <NUMFRAMES><FRAME 1><FRAME 2>...<FRAME NUMFRAMES-1>
		//Format: where <FRAME n>: <fTime><BASICBONE:sizeofBoneNoPtrs><NUMCHILDREN><BASICBONE><NUMCHILDREN> (recursize)
		UINT numFrames = anim.frames.size();
		file.write((char*)&numFrames, 4);
		for (UINT i = 0; i < numFrames; i++) {
			//_WSkeletalFrame* curFrame = (_WSkeletalFrame*)WAnimation::__Impl__->frames[i];
			float fTime = 1.0f;//curFrame->fTime;
			file.write((char*)&fTime, 4);
			//update all the matrices
			vector<WBone*> boneStack;
			boneStack.push_back(anim.frames[i]);
			while (boneStack.size()) {
				WBone* b = boneStack[boneStack.size() - 1];
				boneStack.pop_back();
				b->UpdateLocals();
				for (UINT i = 0; i < b->GetNumChildren(); i++)
					boneStack.push_back(b->GetChild(i));
			}
			anim.frames[i]->SaveToWA(file.rdbuf(), file.tellp());
		}
		printf("done\n");

		//close the file
		file.close();
	}

public:
	WError Setup() {
		WError ret = StartEngine(640, 480);
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
			if (h) {
				do {
					if (LoadScene(pManager, pScene, data.cFileName)) {
						FbxAxisSystem::DirectX.ConvertScene(pScene);
						FbxAxisSystem::ECoordSystem sys = pScene->GetGlobalSettings().GetAxisSystem().GetCoorSystem();

						FbxNode* lNode = pScene->GetRootNode();

						if (lNode) {
							boneIDTable.clear();

							UINT numMeshes = 0;
							for (int i = 0; i < lNode->GetChildCount(); i++) {
								FbxNode* pNode = lNode->GetChild(i);
								FbxNodeAttribute::EType lAttributeType;
								if (pNode->GetNodeAttribute() != NULL) {
									lAttributeType = pNode->GetNodeAttribute()->GetAttributeType();
									if (lAttributeType == FbxNodeAttribute::eMesh) {
										numMeshes++;
									}
								}
							}
							printf("Found %d mesh(es) in the file\n", numMeshes);
							for (int i = 0; i < lNode->GetChildCount(); i++) {
								FbxNode* pNode = lNode->GetChild(i);
								FbxNodeAttribute::EType lAttributeType;
								if (pNode->GetNodeAttribute() != NULL) {
									lAttributeType = pNode->GetNodeAttribute()->GetAttributeType();
									if (lAttributeType == FbxNodeAttribute::eMesh) {
										FbxMesh* lMesh = (FbxMesh*)pNode->GetNodeAttribute();

										char meshName[256];
										strcpy_s(meshName, 256, (char*)pNode->GetName());
										MESHDATA mesh = ParseMesh(lMesh);
										if (mesh.vb.size()) {
											if (mesh.ib.size()) {
												char outFile[MAX_PATH];
												if (numMeshes > 1)
													strcpy_s(outFile, MAX_PATH, meshName);
												else {
													strcpy_s(outFile, MAX_PATH, data.cFileName);
													for (UINT i = 0; i < strlen(outFile); i++)
														if (outFile[i] == '.')
															outFile[i] = '\0';
												}
												strcat_s(outFile, MAX_PATH, ".HXM");
												SaveMesh(mesh, outFile);
											}
										}
									}
								}
							}
							for (int i = 0; i < lNode->GetChildCount(); i++) {
								FbxNode* pNode = lNode->GetChild(i);
								FbxNodeAttribute::EType lAttributeType;
								if (pNode->GetNodeAttribute() != NULL) {
									lAttributeType = pNode->GetNodeAttribute()->GetAttributeType();
									if (lAttributeType == FbxNodeAttribute::eSkeleton) {
										ANIMDATA anim = ParseAnimation(pScene, pNode);
										if (anim.frames.size()) {
											//save the animation data
											char skeletonName[256];
											strcpy_s(skeletonName, 256, pNode->GetName());
											strcat_s(skeletonName, 256, ".HXS");
											SaveAnimation(anim, skeletonName);
											for (int i = 0; i < anim.frames.size(); i++)
												delete anim.frames[i];
										}
									}
								}
							}
						}
					}
				} while (FindNextFileA(h, &data));
			}

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

MESHDATA ParseMesh ( FbxMesh* pMesh )
{
	MESHDATA ret;

	FbxAMatrix FBXmeshMtx = pMesh->GetNode()->EvaluateGlobalTransform ( );
	FbxAMatrix mx1 = FBXmeshMtx;
	WMatrix meshMtx = WMatrix (	mx1.Get ( 0, 0 ), mx1.Get ( 0, 1 ), mx1.Get ( 0, 2 ), mx1.Get ( 0, 3 ),
											mx1.Get ( 1, 0 ), mx1.Get ( 1, 1 ), mx1.Get ( 1, 2 ), mx1.Get ( 1, 3 ),
											mx1.Get ( 2, 0 ), mx1.Get ( 2, 1 ), mx1.Get ( 2, 2 ), mx1.Get ( 2, 3 ),
											mx1.Get ( 3, 0 ), mx1.Get ( 3, 1 ), mx1.Get ( 3, 2 ), mx1.Get ( 3, 3 ) );
	/*meshMtx = WMatrix (	1, 0, 0, 0,
								0, 0, -1, 0,
								0, 1, 0, 0,
								0, 0, 0, 1 );*/
	for ( int i = 0; i < 12; i++ )
		meshMtx.mat[i] *= -1;
	//meshMtx = WMatrixTranspose ( meshMtx );
	//meshMtx *= WRotationMatrixX ( Wm._PI );
	FBXmeshMtx.SetRow(0, FbxVector4(meshMtx(0, 0), meshMtx(1, 0), meshMtx(2, 0), meshMtx(3, 0)));
	FBXmeshMtx.SetRow(1, FbxVector4(meshMtx(0, 1), meshMtx(1, 1), meshMtx(2, 1), meshMtx(3, 1)));
	FBXmeshMtx.SetRow(2, FbxVector4(meshMtx(0, 2), meshMtx(1, 2), meshMtx(2, 2), meshMtx(3, 2)));
	FBXmeshMtx.SetRow(3, FbxVector4(meshMtx(0, 3), meshMtx(1, 3), meshMtx(2, 3), meshMtx(3, 3)));

	int lPolygonCount = pMesh->GetPolygonCount ( );
	FbxVector4* lControlPoints = pMesh->GetControlPoints ( );

	FbxVector4* allVerts = new FbxVector4[pMesh->GetControlPointsCount ( )];

	int vbSize = 0;
	for ( int i = 0; i < lPolygonCount; i++ ) {
		int lPolygonSize = pMesh->GetPolygonSize ( i );
		if ( lPolygonSize != 3 ) {
			printf ( "Error: non-triangular polygon found, skipping." );
			continue;
		}
		//for ( int j = lPolygonSize-1; j >= 0; j-- ) {
		for ( int j = 0; j < lPolygonSize; j++ ) {
			int lControlPointIndex = pMesh->GetPolygonVertex(i, j);
			FbxVector4 norm;
			pMesh->GetPolygonVertexNormal ( i, j, norm );
			allVerts[lControlPointIndex] = norm;
			ret.ib.push_back ( lControlPointIndex );
			vbSize = max ( vbSize, lControlPointIndex + 1 );
		}
	}

	for ( int i = 0; i < vbSize; i++ )
		ret.vb.push_back ( VERTEX ( ) );

	vector<DWORD> repeatedVerts;
	for ( UINT i = 0; i < ret.ib.size ( ); i++ ) {
		int lControlPointIndex = ret.ib[i];
		bool bRepeated = false;
		for ( UINT k = 0; k < repeatedVerts.size ( ) && !bRepeated; k++ )
			if ( repeatedVerts[k] == lControlPointIndex )
				bRepeated = true;
		if ( bRepeated )
			continue;
		repeatedVerts.push_back ( lControlPointIndex );

		FbxVector4 pos = lControlPoints[lControlPointIndex];
		FbxVector2 uv ( 0, 0 );
		FbxVector4 norm ( 0, 1, 0, 0 );
		FbxVector4 tang ( 1, 0, 0, 0 );

		if ( pMesh->GetElementUVCount ( ) ) {
			FbxGeometryElementUV* leUV = pMesh->GetElementUV( 0 );

			switch (leUV->GetMappingMode())
			{
			case FbxGeometryElement::eByControlPoint:
				switch (leUV->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
					uv = leUV->GetDirectArray().GetAt(lControlPointIndex);
					break;
				case FbxGeometryElement::eIndexToDirect:
					{
						int id = leUV->GetIndexArray().GetAt(lControlPointIndex);
						uv = leUV->GetDirectArray().GetAt(id);
					}
					break;
				default:
					break; // other reference modes not shown here!
				}
				break;

			case FbxGeometryElement::eByPolygonVertex:
				{
					int lTextureUVIndex = pMesh->GetTextureUVIndex(i / 3, i % 3);
					switch (leUV->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					case FbxGeometryElement::eIndexToDirect:
						{
							uv = leUV->GetDirectArray().GetAt(lTextureUVIndex);
						}
						break;
					default:
						break; // other reference modes not shown here!
					}
				}
				break;

			case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
			case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
			case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
				break;
			}
		}
		norm = allVerts[lControlPointIndex];
		/*if ( pMesh->GetElementNormalCount ( ) ) {
			FbxGeometryElementNormal* leNormal = pMesh->GetElementNormal ( 0 );

			if(leNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
			{
				switch (leNormal->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
					norm = leNormal->GetDirectArray().GetAt(lControlPointIndex);
					break;
				case FbxGeometryElement::eIndexToDirect:
					{
						int id = leNormal->GetIndexArray().GetAt(lControlPointIndex);
						norm = leNormal->GetDirectArray().GetAt(id);
					}
					break;
				default:
					break; // other reference modes not shown here!
				}
			}

		}*/
		if ( pMesh->GetElementTangentCount ( ) ) {
			FbxGeometryElementTangent* leTangent = pMesh->GetElementTangent ( 0 );

			if(leTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
			{
				switch (leTangent->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
					tang = leTangent->GetDirectArray().GetAt(lControlPointIndex);
					ret.bTangents = true;
					break;
				case FbxGeometryElement::eIndexToDirect:
					{
						int id = leTangent->GetIndexArray().GetAt(lControlPointIndex);
						tang = leTangent->GetDirectArray().GetAt(id);
						ret.bTangents = true;
					}
					break;
				default:
					break; // other reference modes not shown here!
				}
			}

		}
		
		WVector3 p = WVector3 ( pos[0], pos[1], pos[2] );
		WVector3 n = WVector3 ( norm[0], norm[1], norm[2] );
		WVector3 t = WVector3 ( tang[0], tang[1], tang[2] );
		p = WVec3TransformCoord ( p, meshMtx );
		n = WVec3TransformNormal ( n, meshMtx );
		t = WVec3TransformNormal ( t, meshMtx );

		VERTEX v;
		v.pos.x = p.x;
		v.pos.y = p.y;
		v.pos.z = p.z;
		v.uv.x = uv[0];
		v.uv.y = 1.0f - uv[1]; //inverse v coordinate
		v.norm.x = n.x;
		v.norm.y = n.y;
		v.norm.z = n.z;
		v.tang.x = t.x;
		v.tang.y = t.y;
		v.tang.z = t.z;
		ret.vb[ret.ib[i]] = v;
	}

	delete[] allVerts;
	
	int lSkinCount = pMesh->GetDeformerCount ( FbxDeformer::eSkin );

	if ( lSkinCount )
	{
		for ( int i = 0; i < ret.vb.size ( ); i++ )
			ret.ab.push_back ( ANIMVERTEX ( ) );

		for ( int i = 0; i != lSkinCount; i++ )
		{
			int lClusterCount = ((FbxSkin *) pMesh->GetDeformer(i, FbxDeformer::eSkin))->GetClusterCount ( );
			int parentIndex = -1;
			for ( int j = 0; j < lClusterCount; j++ )
			{
				FbxCluster* lCluster=((FbxSkin *) pMesh->GetDeformer(i, FbxDeformer::eSkin))->GetCluster(j);
				if ( lCluster->GetLink()->GetParent() == pMesh->GetScene()->GetRootNode ( ) )
					parentIndex = j;
			}
			for ( int j = 0; j != lClusterCount; j++ )
			{
				FbxCluster* lCluster=((FbxSkin *) pMesh->GetDeformer(i, FbxDeformer::eSkin))->GetCluster(j);

				const char* lClusterModes[] = { "Normalize", "Additive", "Total1" };

				int mode = lCluster->GetLinkMode ( );

				int lIndexCount = lCluster->GetControlPointIndicesCount ( );
				int* lIndices = lCluster->GetControlPointIndices ( );
				double* lWeights = lCluster->GetControlPointWeights ( );

				const char* name = lCluster->GetLink()->GetName ( );
				
				UINT boneID = j;
				bool bAlreadyHasID = false;
				for ( UINT i = 0; i < boneIDTable.size ( ); i++ ) {
					if ( boneIDTable[i].node == lCluster->GetLink ( ) ) {
						boneID = boneIDTable[i].mapsTo;
						bAlreadyHasID = true;
						break;
					}
				}
				if ( parentIndex != -1 && !bAlreadyHasID ) {
					if ( j == 0 )
						boneID = parentIndex;
					else if ( j == parentIndex )
						boneID = 0;
				}

				BONEIDTABLEENTITY entity;
				entity.mapsTo = boneID;
				entity.node = lCluster->GetLink ( );
				FbxAMatrix globalMatrix;
				lCluster->GetTransformMatrix ( globalMatrix );
				globalMatrix *= FBXmeshMtx;
				FbxAMatrix skin;
				lCluster->GetTransformLinkMatrix ( skin );
				skin = globalMatrix.Inverse ( ) * skin;
				skin = skin.Inverse();
				entity.bindingPose = skin;
				if ( GetIDNode ( entity.mapsTo ) == nullptr )
					boneIDTable.push_back ( entity );

				for ( int k = 0; k < lIndexCount; k++ )
				{
					float wei = lWeights[k];
					int ind = lIndices[k];

					int curIndex = 0;
					for ( int i = 0; i < 4; i++ ) {
						if ( ret.ab[lIndices[k]].weights[i] < 0.01f )
							break;
						curIndex++;
					}
					if ( curIndex == 4 )
					{
						int leastInfluencing = 0;
						for ( int n = 1; n < 4; n++ )
							if ( ret.ab[lIndices[k]].weights[n] < ret.ab[lIndices[k]].weights[leastInfluencing] )
								leastInfluencing = n;
						int arr[3];
						int posInArray = 0;
						for ( int n = 0; n < 4; n++ )
							if ( n != leastInfluencing )
								arr[posInArray++] = n;
						ret.ab[lIndices[k]].weights[leastInfluencing] = 1.0f - (ret.ab[lIndices[k]].weights[arr[0]] +
							ret.ab[lIndices[k]].weights[arr[1]] + ret.ab[lIndices[k]].weights[arr[2]]);
						ret.ab[lIndices[k]].bones[leastInfluencing] = boneID;
						printf ( "Vertex %d has more than 4 bones associated with it which is not supported.\nExtra bone causing error: %s\n",
							lIndices[k], entity.node->GetName ( ) );
						continue; //cannot use this index, this vertex has more than 4 bones
									//which is not supported
					}
					ret.ab[lIndices[k]].bones[curIndex] = boneID;
					ret.ab[lIndices[k]].weights[curIndex] = (float) lWeights[k];
				}
			}
		}
	}

	//weights sometimes get messed up, fix it.
	for ( int i = 0; i < ret.ab.size ( ); i++ )
	{
		int mostWeight = 0;
		float fWeight = 0;
		for ( int j = 0; j < 4; j++ )
		{
			fWeight += ret.ab[i].weights[j];
			if ( ret.ab[i].weights[j] > ret.ab[i].weights[mostWeight] )
				mostWeight = j;
		}
		if ( abs ( fWeight - 1.0f ) > 0.01f )
		{
			printf ( "Vertex %d has total weights not equal to 1.\n", i );
			int arr[3];
			int posInArray = 0;
			for ( int k = 0; k < 4; k++ )
				if ( k != mostWeight )
					arr[posInArray++] = k;
			ret.ab[i].weights[mostWeight] = 1.0f - (ret.ab[i].weights[arr[0]] +
				ret.ab[i].weights[arr[1]] + ret.ab[i].weights[arr[2]]);
		}
		//bubble sort, the highest better be first
		for ( int a = 0; a < ret.ab.size ( ); a++ )
		{
			for ( int i = 0; i < 3; i++ )
			{
				if ( ret.ab[a].weights[i] < ret.ab[a].weights[i+1] )
				{
					float t1 = ret.ab[a].weights[i];
					UINT t2 = ret.ab[a].bones[i];
					ret.ab[a].weights[i] = ret.ab[a].weights[i+1];
					ret.ab[a].weights[i+1] = t1;
					ret.ab[a].bones[i] = ret.ab[a].bones[i+1];
					ret.ab[a].bones[i+1] = t2;
					i--;
				}
			}
		}
	}

	//reverse the winding order
	for ( int i = 0; i < ret.ib.size ( ); i += 3 ) {
		DWORD t = ret.ib[i+0];
		ret.ib[i+0] = ret.ib[i+2];
		ret.ib[i+2] = t;
	}

	return ret;
}

int GetCurveMaxFrame ( FbxAnimCurve* pCurve )
{
	int lKeyCount = pCurve->KeyGetCount ( );
	int maxFrame = 0;

	for ( int lCount = 0; lCount < lKeyCount; lCount++ )
	{
		FbxTime lKeyTime  = pCurve->KeyGetTime ( lCount );
		long long frame = lKeyTime.GetFrameCount ( );
		maxFrame = max ( maxFrame, frame );
	}

	return maxFrame;
}
int GetNodeMaxFrame ( FbxNode* pNode, FbxAnimLayer* pAnimLayer )
{
	FbxProperty* props[3] = { &pNode->LclTranslation, &pNode->LclRotation, &pNode->LclScaling };
	const char* comp[3] = { "X", "Y", "Z" };
	int maxFrame = 0;

	for ( int i = 0; i < 9; i++ )
	{
		FbxAnimCurve* lAnimCurve = props[i/3]->GetCurve ( pAnimLayer, comp[i%3] );
		if (lAnimCurve)
			maxFrame = max ( maxFrame, GetCurveMaxFrame ( lAnimCurve ) );
	}

	return maxFrame;
}
int GetLayerMaxFrame ( FbxAnimLayer* pAnimLayer, FbxNode* pNode, RAWBONE* bone )
{
	int maxFrame = GetNodeMaxFrame ( pNode, pAnimLayer );
	pNode->GetRotationOrder ( FbxNode::eSourcePivot, bone->lRotationOrder );
	strcpy_s ( bone->name, 64, pNode->GetName ( ) );
	bone->id = GetNodeID ( pNode );

	int numChildren = pNode->GetChildCount ( );
	for ( int i = 0; i < numChildren; i++ )
	{
		RAWBONE* child = new RAWBONE;
		child->parent = bone;
		bone->children.push_back ( child );
		int newMax = GetLayerMaxFrame ( pAnimLayer, pNode->GetChild ( i ), child );
		maxFrame = max ( maxFrame, newMax );
	}

	return maxFrame;
}
int GetCurveMinFrame ( FbxAnimCurve* pCurve )
{
	int lKeyCount = pCurve->KeyGetCount ( );
	int minFrame = INT_MAX;

	for ( int lCount = 0; lCount < lKeyCount; lCount++ )
	{
		FbxTime lKeyTime  = pCurve->KeyGetTime ( lCount );
		long long frame = lKeyTime.GetFrameCount ( );
		minFrame = min ( minFrame, frame );
	}

	return minFrame;
}
int GetNodeMinFrame ( FbxNode* pNode, FbxAnimLayer* pAnimLayer )
{
	FbxProperty* props[3] = { &pNode->LclTranslation, &pNode->LclRotation, &pNode->LclScaling };
	const char* comp[3] = { "X", "Y", "Z" };
	int minFrame = INT_MAX;

	for ( int i = 0; i < 9; i++ )
	{
		FbxAnimCurve* lAnimCurve = props[i/3]->GetCurve ( pAnimLayer, comp[i%3] );
		if (lAnimCurve)
			minFrame = min ( minFrame, GetCurveMinFrame ( lAnimCurve ) );
	}

	return minFrame;
}
int GetLayerMinFrame ( FbxAnimLayer* pAnimLayer, FbxNode* pNode, RAWBONE* bone )
{
	int minFrame = GetNodeMinFrame ( pNode, pAnimLayer );
	pNode->GetRotationOrder ( FbxNode::eSourcePivot, bone->lRotationOrder );
	strcpy_s ( bone->name, 64, pNode->GetName ( ) );
	bone->id = GetNodeID ( pNode );

	int numChildren = pNode->GetChildCount ( );
	for ( int i = 0; i < numChildren; i++ )
	{
		int newMin = GetLayerMinFrame ( pAnimLayer, pNode->GetChild ( i ), bone->children[i] );
		minFrame = min ( minFrame, newMin );
	}

	return minFrame;
}

void EnumNode ( FbxNode* pNode, FbxAnimLayer* pAnimLayer, RAWBONE* bone, int minFrame = 0 )
{
	FbxProperty* props[3] = { &pNode->LclTranslation, &pNode->LclRotation, &pNode->LclScaling };
	const char* comp[3] = { "X", "Y", "Z" };

	for ( int i = 0; i < 9; i++ )
	{
		FbxAnimCurve* lAnimCurve = props[i/3]->GetCurve ( pAnimLayer, comp[i%3] );
		if ( lAnimCurve )
		{
			int lKeyCount = lAnimCurve->KeyGetCount ( );

			for ( int lCount = 0; lCount < lKeyCount; lCount++ )
			{
				float lKeyValue = static_cast<float> ( lAnimCurve->KeyGetValue ( lCount ) );
				FbxTime lKeyTime  = lAnimCurve->KeyGetTime ( lCount );
				//keyframes dont store positions so do it manually
				FbxAMatrix m = pNode->EvaluateGlobalTransform ( lKeyTime );
				m = m.Inverse ( );
				FbxVector4 curPos = FbxVector4 ( -m.Get(3,0), -m.Get(3,1), -m.Get(3,2) );//pNode->EvaluateLocalTranslation ( lKeyTime );
				FbxVector4 curRot = pNode->EvaluateLocalRotation ( lKeyTime );
				long long frame = lKeyTime.GetFrameCount ( ) - minFrame;
				if ( frame == 0 )
				{
					bone->vals[0] = curPos[0];
					bone->vals[1] = curPos[1];
					bone->vals[2] = curPos[2];
					bone->vals[3] = curRot[0];
					bone->vals[4] = curRot[1];
					bone->vals[5] = curRot[2];
					//bone->vals[i] = lKeyValue;
					bone->bIsKeyFrame[i] = true;
				}
				else
				{
					bone->siblings[frame-1]->vals[0] = curPos[0];
					bone->siblings[frame-1]->vals[1] = curPos[1];
					bone->siblings[frame-1]->vals[2] = curPos[2];
					bone->siblings[frame-1]->vals[3] = curRot[0];
					bone->siblings[frame-1]->vals[4] = curRot[1];
					bone->siblings[frame-1]->vals[5] = curRot[2];
					//bone->siblings[frame-1]->vals[i] = lKeyValue;
					bone->siblings[frame-1]->bIsKeyFrame[i] = true;
				}
			}
		}
	}
}

void ParseAnimationLayer ( FbxAnimLayer* pAnimLayer, FbxNode* pNode, RAWBONE* bone, int minFrame )
{
	EnumNode ( pNode, pAnimLayer, bone, minFrame );

	int numChildren = pNode->GetChildCount ( );
	for ( int i = 0; i < numChildren; i++ )
		ParseAnimationLayer ( pAnimLayer, pNode->GetChild ( i ), bone->children[i], minFrame );
}

void CreateSiblings ( RAWBONE* bone, int numSiblings )
{
	for ( int i = 0; i < numSiblings; i++ )
	{
		RAWBONE* sib = new RAWBONE;
		strcpy_s ( sib->name, 64, bone->name );
		sib->id = bone->id;
		sib->lRotationOrder = bone->lRotationOrder;
		RAWBONE* parent = nullptr;
		if ( bone->parent )
		{
			parent = bone->parent->siblings[i];
			bone->parent->siblings[i]->children.push_back ( sib );
		}
		bone->siblings.push_back ( sib );
	}

	for ( int i = 0; i < bone->children.size ( ); i++ )
		CreateSiblings ( bone->children[i], numSiblings );
}
double LinearInterpolate (
	double y1, double y2,
	double mu )
{
	return y1 * (1 - mu)+ y2 * mu;
}
 double CubicInterpolate (
	double y0, double y1,
	double y2, double y3,
	double mu )
{
	double a0, a1, a2, a3, mu2;

	mu2 = mu * mu;
	a0 = y3 - y2 - y0 + y1;
	a1 = y0 - y1 - a0;
	a2 = y2 - y0;
	a3 = y1;

	return a0 *mu*mu2 + a1*mu2 + a2*mu + a3;
}
void ConnectRawBoneKeys ( RAWBONE* rawBone )
{
	//for each one of the 9 components, do the curve interpolation
	for ( int component = 0; component < 9; component++ )
	{
		vector<KEYPOINT> keypoints;
		KEYPOINT k;
		k.valPtr = &rawBone->vals[component];
		k.bValIsKey = rawBone->bIsKeyFrame[component];
		keypoints.push_back ( k );
		for ( int sib = 0; sib < rawBone->siblings.size ( ); sib++ )
		{
			k.valPtr = &rawBone->siblings[sib]->vals[component];
			k.bValIsKey = rawBone->siblings[sib]->bIsKeyFrame[component];
			keypoints.push_back ( k );
		}
		//first, extend the keys before the first and after last
		int veryFirstKey = -1, veryLastKey = -1;
		for ( int i = 0; i < keypoints.size ( ); i++ )
		{
			if ( keypoints[i].bValIsKey )
			{
				veryLastKey= i;
				if ( veryFirstKey == -1 )
					veryFirstKey = i;
			}
		}
		if ( veryFirstKey == -1 || veryLastKey == -1 )
			veryFirstKey = veryLastKey = 0;
		for ( int i = 0; i < veryFirstKey; i++ )
			*keypoints[i].valPtr = *keypoints[veryFirstKey].valPtr;
		for ( int i = veryLastKey + 1; i < keypoints.size ( ); i++ )
			*keypoints[i].valPtr = *keypoints[veryLastKey].valPtr;

		//interpolate this component's keys
		int pt1 = -1, pt2 = -1;
		for ( int i = 0; i < keypoints.size ( ); i++ )
		{
			bool bInterpolate = false;
			if ( keypoints[i].bValIsKey )
			{
				if ( pt2 == -1 )
				{
					if ( pt1 == -1 )
						pt1 = i;
					else
					{
						pt2 = i;
						bInterpolate = true;
					}
				}
				else
				{
					pt1 = pt2;
					pt2 = i;
					bInterpolate = true;
				}
			}
			if ( bInterpolate )
			{
				float p1 = *keypoints[pt1].valPtr;
				float p2 = *keypoints[pt2].valPtr;
				int range = pt2 - pt1;
				for ( int i = pt1 + 1; i < pt2; i++ )
				{
					float lerpVal = (float)(i - pt1) / (float)range;
					*keypoints[i].valPtr = LinearInterpolate ( p1, p2, lerpVal );
				}
			}
		}
	}

	for ( int i = 0; i < rawBone->children.size ( ); i++ )
		ConnectRawBoneKeys ( rawBone->children[i] );
}

WBone* CreateBoneFromRawBone ( RAWBONE* rawBone, FbxTime time )
{
	WBone* bone = new WBone ( );
	bone->SetName ( rawBone->name );
	bone->SetIndex ( rawBone->id );
	bone->SetParent ( nullptr );

	FbxNode* node = GetIDNode ( rawBone->id );
	if ( node )
	{
		bool bIsParent = true;
		if ( node->GetParent ( ) )
			if ( node->GetParent()->GetNodeAttribute ( ) )
				bIsParent = false;
		rawBone->matrix = node->EvaluateLocalTransform ( time );

		
		FbxAMatrix ibp = GetNodeBindingPose ( node );
		FbxVector4 p = node->EvaluateLocalTranslation ( );

		if ( bIsParent ) {
			p = FbxVector4 ( rawBone->vals[1], -rawBone->vals[2], -rawBone->vals[0] );
			FbxAMatrix nodeGlobal = node->EvaluateGlobalTransform ( );
			rawBone->matrix = nodeGlobal.Inverse ( ) * rawBone->matrix;
		}
		FbxAMatrix inv = rawBone->matrix.Inverse ( );
		
		WMatrix invBindingPose = WMatrix (	ibp.Get ( 0, 0 ), ibp.Get ( 0, 1 ), ibp.Get ( 0, 2 ), ibp.Get ( 0, 3 ),
												ibp.Get ( 1, 0 ), ibp.Get ( 1, 1 ), ibp.Get ( 1, 2 ), ibp.Get ( 1, 3 ),
												ibp.Get ( 2, 0 ), ibp.Get ( 2, 1 ), ibp.Get ( 2, 2 ), ibp.Get ( 2, 3 ),
												ibp.Get ( 3, 0 ), ibp.Get ( 3, 1 ), ibp.Get ( 3, 2 ), ibp.Get ( 3, 3 ) );
		WVector3 pos = WVector3 ( p[0], p[1], p[2] );
		WVector3 r = WVector3 ( inv.Get ( 0, 0 ), inv.Get ( 1, 0 ), inv.Get ( 2, 0 ) );
		WVector3 u = WVector3 ( inv.Get ( 0, 1 ), inv.Get ( 1, 1 ), inv.Get ( 2, 1 ) );
		WVector3 l = WVector3 ( inv.Get ( 0, 2 ), inv.Get ( 1, 2 ), inv.Get ( 2, 2 ) );
		if ( bIsParent ) {
			/*WVector3 t = u;
			u = r;
			r = t;*/
			WVector3 t = l;
			l = -u;
			u = r;
			r = t;
		}
		bone->SetInvBindingPose ( invBindingPose );
		bone->SetULRVectors ( u, l, r );
		bone->SetPosition ( pos );
		bone->UpdateLocals ( );
	}

	for ( int i = 0; i < rawBone->children.size ( ); i++ )
	{
		WBone* child = CreateBoneFromRawBone ( rawBone->children[i], time );
		child->SetParent ( bone );
		bone->AddChild ( child );
	}

	return bone;
}

ANIMDATA CreateAnimationData ( RAWBONE* rawBase )
{
	ANIMDATA data;
	int numFrames = rawBase->siblings.size ( );
	FbxTime time;
	time.SetFrame ( 0 );
	data.frames.push_back ( CreateBoneFromRawBone ( rawBase, time ) ); //frame 0 is not the siblings[0]
	for ( int i = 0; i < numFrames; i++ )
	{
		time.SetFrame ( i + 1 );
		data.frames.push_back ( CreateBoneFromRawBone ( rawBase->siblings[i], time ) );
	}
	return data;
}

bool IsBoneAnimated ( RAWBONE* bone )
{
	FbxAMatrix m = bone->matrix;
	for ( int i = 0; i < bone->siblings.size ( ); i++ )
		for ( int j = 0; j < 4; j++ ) {
			FbxDouble4 d1 = m[j];
			FbxDouble4 d2 = bone->siblings[i]->matrix[j];
			for ( int k = 0; k < 4; k++ )
				if ( abs ( d1[k] - d2[k] ) > 0.01f )
					return true;
		}
	return false;
}
void PrintAnimatedBones ( RAWBONE* bone )
{
	if ( IsBoneAnimated ( bone ) )
		printf ( "Animated bone: %s with ID %d\n", bone->name, bone->id );

	for ( int i = 0; i < bone->children.size ( ); i++ )
		PrintAnimatedBones ( bone->children[i] );
}
ANIMDATA ParseAnimation ( FbxScene* pScene, FbxNode* node )
{
	ANIMDATA ret;
	if ( pScene->GetSrcObjectCount<FbxAnimStack> ( ) )
	{
		FbxAnimStack* lAnimStack = pScene->GetSrcObject<FbxAnimStack> ( 0 );

		int nbAnimLayers = lAnimStack->GetMemberCount<FbxAnimLayer> ( );

		if ( nbAnimLayers )
		{
			FbxAnimLayer* lAnimLayer = lAnimStack->GetMember<FbxAnimLayer> ( 0 );
			RAWBONE* rawBone = new RAWBONE;
			int maxFrame = GetLayerMaxFrame ( lAnimLayer, node, rawBone );
			int minFrame = GetLayerMinFrame ( lAnimLayer, node, rawBone );
			CreateSiblings ( rawBone, maxFrame - minFrame );
			//ParseAnimationLayer ( lAnimLayer, node, rawBone, minFrame );
			//only parent node needs to be explicitly parsed
			EnumNode ( node, lAnimLayer, rawBone, minFrame );
			ConnectRawBoneKeys ( rawBone );
			ret = CreateAnimationData ( rawBone );

			PrintAnimatedBones ( rawBone );

			for ( int i = 0; i < rawBone->siblings.size ( ); i++ )
				delete rawBone->siblings[i];
			delete rawBone;
		}
	}
	return ret;
}
