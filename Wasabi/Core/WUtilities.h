#pragma once

#include "WMath.h"

class Wasabi;

namespace WUtil {
	bool Point3DToScreen2D(Wasabi* app, WVector3 point, int* x, int* y);

	bool RayIntersectCube(float cubeHalfSize, WVector3 rayPos, WVector3 rayDir, WVector3 cubePos = WVector3(0, 0, 0));
	bool RayIntersectBox(WVector3 halfDimensions, WVector3 rayPos, WVector3 rayDir, WVector3 boxPos = WVector3(0, 0, 0));
};