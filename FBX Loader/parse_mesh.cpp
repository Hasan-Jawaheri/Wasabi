#include "main.h"

MESHDATA ParseMesh(FbxMesh* pMesh)
{
	MESHDATA ret;

	FbxGeometryElementTangent* leTangent = nullptr;
	if (pMesh->GetElementTangentCount())
		leTangent = pMesh->GetElementTangent(0);

	FbxAMatrix FBXmeshMtx = pMesh->GetNode()->EvaluateGlobalTransform();
	FbxAMatrix mx1 = FBXmeshMtx;
	WMatrix meshMtx = WMatrix(
		mx1.Get(0, 0), mx1.Get(0, 1), mx1.Get(0, 2), mx1.Get(0, 3),
		mx1.Get(1, 0), mx1.Get(1, 1), mx1.Get(1, 2), mx1.Get(1, 3),
		mx1.Get(2, 0), mx1.Get(2, 1), mx1.Get(2, 2), mx1.Get(2, 3),
		mx1.Get(3, 0), mx1.Get(3, 1), mx1.Get(3, 2), mx1.Get(3, 3)
	);
	/*meshMtx = WMatrix (	1, 0, 0, 0,
								0, 0, -1, 0,
								0, 1, 0, 0,
								0, 0, 0, 1 );*/
	for (int i = 0; i < 12; i++)
		meshMtx.mat[i] *= -1;
	//meshMtx = WMatrixTranspose ( meshMtx );
	//meshMtx *= WRotationMatrixX ( Wm._PI );
	FBXmeshMtx.SetRow(0, FbxVector4(meshMtx(0, 0), meshMtx(1, 0), meshMtx(2, 0), meshMtx(3, 0)));
	FBXmeshMtx.SetRow(1, FbxVector4(meshMtx(0, 1), meshMtx(1, 1), meshMtx(2, 1), meshMtx(3, 1)));
	FBXmeshMtx.SetRow(2, FbxVector4(meshMtx(0, 2), meshMtx(1, 2), meshMtx(2, 2), meshMtx(3, 2)));
	FBXmeshMtx.SetRow(3, FbxVector4(meshMtx(0, 3), meshMtx(1, 3), meshMtx(2, 3), meshMtx(3, 3)));

	int lPolygonCount = pMesh->GetPolygonCount();
	int numControlPoints = pMesh->GetControlPointsCount();
	FbxVector4* allControlPointPositions = pMesh->GetControlPoints();
	FbxStringList uvSetNames;
	pMesh->GetUVSetNames(uvSetNames);

	char* uvSetName = NULL;
	if (uvSetNames.GetCount() > 0)
		uvSetName = uvSetNames.GetStringAt(uvSetNames.GetCount()-1);
	if (uvSetNames.GetCount() > 1) {
		string allSets;
		for (int i = 0; i < uvSetNames.GetCount(); i++)
			allSets += (i > 0 ? ", " : "") + string(uvSetNames.GetStringAt(i));
		printf("Warning: mesh contains multiple uv sets (%s), only set '%s' will be used\n", allSets.c_str(), uvSetName);
	}

	for (int i = 0; i < lPolygonCount; i++) {
		int lPolygonSize = pMesh->GetPolygonSize(i);
		if (lPolygonSize != 3) {
			printf("Error: non-triangular polygon found, skipping.");
			continue;
		}
		for (int j = 0; j < lPolygonSize; j++) {
			int lControlPointIndex = pMesh->GetPolygonVertex(i, j);
			FbxVector4 pos, norm, tang;
			FbxVector2 uv;
			bool dummy;
			pos = allControlPointPositions[lControlPointIndex];
			pMesh->GetPolygonVertexNormal(i, j, norm);
			pMesh->GetPolygonVertexUV(i, j, uvSetName, uv, dummy);

			/*if (leTangent) {
				if (leTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
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
			}*/

			WVector3 p = WVector3(pos[0], pos[1], pos[2]);
			WVector3 n = WVector3(norm[0], norm[1], norm[2]);
			WVector3 t = WVector3(tang[0], tang[1], tang[2]);
			p = WVec3TransformCoord(p, meshMtx);
			n = WVec3TransformNormal(n, meshMtx);
			t = WVec3TransformNormal(t, meshMtx);

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
			ret.vb.push_back(v);
			ret.ib.push_back(i*3+j);
		}
	}

	int lSkinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);

	if (lSkinCount)
	{
		if (lSkinCount > 1)
			printf("Warning: %d skins found, will only use the first\n", lSkinCount);

		vector<vector<pair<UINT, float>>> boneWeights(ret.vb.size()); // each element of this vector corresponds to a vertex. For each element, it has a vector of all bones that affect that vertex

		int skinIndex = 0;
		int lClusterCount = ((FbxSkin *)pMesh->GetDeformer(skinIndex, FbxDeformer::eSkin))->GetClusterCount();
		int parentIndex = -1;
		bool bNormalizeWeights = false;
		for (int j = 0; j < lClusterCount; j++)
		{
			FbxCluster* lCluster = ((FbxSkin *)pMesh->GetDeformer(skinIndex, FbxDeformer::eSkin))->GetCluster(j);
			if (lCluster->GetLink()->GetParent() == pMesh->GetScene()->GetRootNode())
				parentIndex = j;
		}
		for (int j = 0; j != lClusterCount; j++)
		{
			FbxCluster* lCluster = ((FbxSkin *)pMesh->GetDeformer(skinIndex, FbxDeformer::eSkin))->GetCluster(j);

			const char* lClusterModes[] = { "Normalize", "Additive", "Total1" };
			int mode = lCluster->GetLinkMode();
			if (mode == 0)
				bNormalizeWeights = true;

			int lIndexCount = lCluster->GetControlPointIndicesCount();
			int* lIndices = lCluster->GetControlPointIndices();
			double* lWeights = lCluster->GetControlPointWeights();

			const char* name = lCluster->GetLink()->GetName();

			UINT boneID = j;
			bool bAlreadyHasID = NodeExists(lCluster->GetLink());
			if (bAlreadyHasID)
				boneID = GetNodeID(lCluster->GetLink());

			if (parentIndex != -1 && !bAlreadyHasID) {
				if (j == 0)
					boneID = parentIndex;
				else if (j == parentIndex)
					boneID = 0;
			}

			BONEIDTABLEENTITY entity;
			entity.mapsTo = boneID;
			entity.node = lCluster->GetLink();
			FbxAMatrix globalMatrix;
			lCluster->GetTransformMatrix(globalMatrix);
			globalMatrix *= FBXmeshMtx;
			FbxAMatrix skin;
			lCluster->GetTransformLinkMatrix(skin);
			skin = globalMatrix.Inverse() * skin;
			skin = skin.Inverse();
			entity.bindingPose = skin;
			if (GetNodeByID(entity.mapsTo) == nullptr)
				CreateNewNode(entity);

			for (int k = 0; k < lIndexCount; k++)
			{
				float weight = lWeights[k];
				int ind = lIndices[k];

				boneWeights[ind].push_back(pair<UINT, float>(boneID, weight));
			}
		}

		ret.ab.resize(ret.vb.size());
		for (int i = 0; i < boneWeights.size(); i++) {
			sort(boneWeights[i].begin(), boneWeights[i].end(), [](pair<UINT, float> b1, pair<UINT, float> b2) -> bool {return b1.second > b2.second;});
			if (boneWeights[i].size() > 4)
				printf("Warning: vertex %d has %d weights (max is 4)\n", i, boneWeights[i].size());
			float fTotalWeight = 0.0f;
			for (int j = 0; j < min(4, boneWeights[i].size()); j++)
				fTotalWeight += boneWeights[i][j].second;
			if (!bNormalizeWeights)
				fTotalWeight = 1.0f; // this effectively negates normalization
			for (int j = 0; j < min(4, boneWeights[i].size()); j++) {
				ret.ab[i].bones[j] = boneWeights[i][j].first;
				ret.ab[i].weights[j] = boneWeights[i][j].second / fTotalWeight;
			}
		}
	}

	//reverse the winding order
	for (int i = 0; i < ret.ib.size(); i += 3) {
		DWORD t = ret.ib[i + 0];
		ret.ib[i + 0] = ret.ib[i + 2];
		ret.ib[i + 2] = t;
	}

	return ret;
}