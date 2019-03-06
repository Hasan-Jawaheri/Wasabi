#pragma once

#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <fbxsdk.h>
#include <unordered_map>
#pragma comment ( lib, "libfbxsdk-md.lib" )

#include "Wasabi.h"

using namespace std;

struct VEC3 {
	VEC3(void) : x(0), y(0), z(0) { }
	VEC3(float fx, float fy, float fz) : x(fx), y(fy), z(fz) { }
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
	ANIMVERTEX() { ZeroMemory(this, sizeof ANIMVERTEX); }
};

struct MESHDATA {
	vector<VERTEX> vb;
	vector<DWORD> ib;
	vector<ANIMVERTEX> ab;
	bool bTangents;
};

struct ANIMDATA {
	vector<WBone*> frames;
};


struct BONEIDTABLEENTITY {
	FbxNode* node;
	FbxAMatrix bindingPose;
	UINT mapsTo;
};

bool NodeExists(FbxNode* pNode);
UINT GetNodeID(FbxNode* pNode);
void CreateNewNode(BONEIDTABLEENTITY entity);
FbxNode* GetNodeByID(UINT ID);
FbxAMatrix GetNodeBindingPose(FbxNode* node);

MESHDATA ParseMesh(FbxMesh* pMesh);
ANIMDATA ParseAnimation(FbxScene* pScene, FbxNode* node);
