#pragma once

#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <fbxsdk.h>
#include <unordered_map>
#pragma comment ( lib, "libfbxsdk-md.lib" )

#include "Wasabi/Wasabi.h"

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
	uint32_t texIndex;
};

struct ANIMVERTEX {
	uint32_t bones[4];
	float weights[4];
	ANIMVERTEX() { ZeroMemory(this, sizeof ANIMVERTEX); }
};

struct MESHDATA {
	std::string name;
	WMatrix transform;
	vector<VERTEX> vb;
	vector<DWORD> ib;
	vector<ANIMVERTEX> ab;
	vector<const char*> textures;
	bool bTangents;
};

struct ANIMDATA {
	std::string name;
	vector<WBone*> frames;
};


struct BONEIDTABLEENTITY {
	FbxNode* node;
	FbxAMatrix bindingPose;
	uint32_t mapsTo;
};

bool NodeExists(FbxNode* pNode);
uint32_t GetNodeID(FbxNode* pNode);
void CreateNewNode(BONEIDTABLEENTITY entity);
FbxNode* GetNodeByID(uint32_t ID);
FbxAMatrix GetNodeBindingPose(FbxNode* node);

MESHDATA ParseMesh(FbxMesh* pMesh);
ANIMDATA ParseAnimation(FbxScene* pScene, FbxNode* node);
