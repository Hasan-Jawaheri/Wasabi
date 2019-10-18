#pragma once

#include "main.hpp"
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <fbxsdk.h>

#pragma comment ( lib, "libfbxsdk-md.lib" )

void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
void DestroySdkObjects(FbxManager* pManager, bool pExitStatus);
bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);

void RedirectIOToConsole();
