#include "WUtilities.h"
#include "../Core/WCore.h"
#include "../Cameras/WCamera.h"
#include "../Renderers/WRenderer.h"
#include "../Images/WRenderTarget.h"
#include "../Windows/WWindowComponent.h"

bool WUtil::Point3DToScreen2D(Wasabi* app, WVector3 point, int* _x, int* _y) {
	WCamera* cam = app->Renderer->GetDefaultRenderTarget()->GetCamera();
	float width = app->WindowComponent->GetWindowWidth();
	float height = app->WindowComponent->GetWindowHeight();
	WVector3 pos = WVec3TransformCoord(point, cam->GetViewMatrix());

	float fX = pos.x * cam->GetMinRange() / pos.z;
	float fY = pos.y * cam->GetMinRange() / pos.z;
	float fTriHalfSideX = cam->GetMinRange() * tanf(0.5f * W_DEGTORAD(cam->GetFOV()));
	float fTriHalfSideY = cam->GetMinRange() * tanf(0.5f * W_DEGTORAD(cam->GetFOV()));

	fTriHalfSideX *= cam->GetAspect();

	fX = (fX + fTriHalfSideX) / 2;
	fY = (fY + fTriHalfSideY) / 2;
	fX = (fX / fTriHalfSideX) * width;
	fY = (1 - (fY / fTriHalfSideY)) * height;

	int x = (int)fX;
	int y = (int)fY;

	if (_x) *_x = x;
	if (_y) *_y = y;

	if (x >= 0 && x <= width && y >= 0 && y <= height)
		return true;

	return false;
}

bool WUtil::RayIntersectCube(float cubeHalfSize, WVector3 rayPos, WVector3 rayDir, WVector3 cubePos) {
	//if the ray position is inside the box, it intersects then
	if (fabs(rayPos.x) < cubeHalfSize && fabs(rayPos.y) < cubeHalfSize && fabs(rayPos.z) < cubeHalfSize)
		return true;

	//allocate 24 vertices for the cube
	WVector3 verts[24];

	//fill the vertex buffer data
	verts[0] = WVector3(-0.5f, -0.5f, -0.5f);
	verts[1] = WVector3(-0.5f, 0.5f, -0.5f);
	verts[2] = WVector3(0.5f, 0.5f, -0.5f);
	verts[3] = WVector3(0.5f, -0.5f, -0.5f);
	verts[4] = WVector3(-0.5f, -0.5f, 0.5f);
	verts[5] = WVector3(0.5f, -0.5f, 0.5f);
	verts[6] = WVector3(0.5f, 0.5f, 0.5f);
	verts[7] = WVector3(-0.5f, 0.5f, 0.5f);
	verts[8] = WVector3(-0.5f, 0.5f, -0.5f);
	verts[9] = WVector3(-0.5f, 0.5f, 0.5f);
	verts[10] = WVector3(0.5f, 0.5f, 0.5f);
	verts[11] = WVector3(0.5f, 0.5f, -0.5f);
	verts[12] = WVector3(-0.5f, -0.5f, -0.5f);
	verts[13] = WVector3(0.5f, -0.5f, -0.5f);
	verts[14] = WVector3(0.5f, -0.5f, 0.5f);
	verts[15] = WVector3(-0.5f, -0.5f, 0.5f);
	verts[16] = WVector3(-0.5f, -0.5f, 0.5f);
	verts[17] = WVector3(-0.5f, 0.5f, 0.5f);
	verts[18] = WVector3(-0.5f, 0.5f, -0.5f);
	verts[19] = WVector3(-0.5f, -0.5f, -0.5f);
	verts[20] = WVector3(0.5f, -0.5f, -0.5f);
	verts[21] = WVector3(0.5f, 0.5f, -0.5f);
	verts[22] = WVector3(0.5f, 0.5f, 0.5f);
	verts[23] = WVector3(0.5f, -0.5f, 0.5f);

	//scale the vertices
	for (int i = 0; i < 24; i++) {
		verts[i] *= cubeHalfSize * 2;
		verts[i] += cubePos;
	}

	//allocate 36 unsigned ints for the indices of the cube
	unsigned int _indices[36];

	//fill cube's index data
	_indices[0] = 0; _indices[1] = 1; _indices[2] = 2;
	_indices[3] = 0; _indices[4] = 2; _indices[5] = 3;
	_indices[6] = 4; _indices[7] = 5; _indices[8] = 6;
	_indices[9] = 4; _indices[10] = 6; _indices[11] = 7;
	_indices[12] = 8; _indices[13] = 9; _indices[14] = 10;
	_indices[15] = 8; _indices[16] = 10; _indices[17] = 11;
	_indices[18] = 12; _indices[19] = 13; _indices[20] = 14;
	_indices[21] = 12; _indices[22] = 14; _indices[23] = 15;
	_indices[24] = 16; _indices[25] = 17; _indices[26] = 18;
	_indices[27] = 16; _indices[28] = 18; _indices[29] = 19;
	_indices[30] = 20; _indices[31] = 21; _indices[32] = 22;
	_indices[33] = 20; _indices[34] = 22; _indices[35] = 23;

	for (unsigned int i = 0; i < 12; i++) {
		WVector3 v0 = verts[_indices[i * 3 + 0]];
		WVector3 v1 = verts[_indices[i * 3 + 1]];
		WVector3 v2 = verts[_indices[i * 3 + 2]];

		WVector3 e1, e2, h, s, q;
		float a, f, u, v;

		e1 = v1 - v0; //vector ( e1, v1, v0 );
		e2 = v2 - v0; //vector ( e2, v2, v0 );
		h = WVec3Cross(rayDir, e2);
		a = WVec3Dot(e1, h);

		if (a > -0.00001 && a < 0.00001)
			continue; //no intersection

		f = 1 / a;
		s = rayPos - v0; //vector ( s, p, v0 );
		u = f * (WVec3Dot(s, h));

		if (u < 0.0 || u > 1.0)
			continue; //no intersection

		q = WVec3Cross(s, e1);
		v = f * WVec3Dot(rayDir, q);
		if (v < 0.0 || u + v > 1.0)
			continue; //no intersection
					  // at this stage we can compute t to find out where 
					  // the intersection point is on the line
		float t = f * WVec3Dot(e2, q);
		if (t > 0.00001) // ray intersection
			return true;
	}

	return false;
}

bool WUtil::RayIntersectBox(WVector3 boxDimensions, WVector3 rayPos, WVector3 rayDir, WVector3 boxPos) {
	//if the ray position is inside the box, it intersects then
	if (fabs(rayPos.x) < boxDimensions.x && fabs(rayPos.y) < boxDimensions.y && fabs(rayPos.z) < boxDimensions.z)
		return true;

	//allocate 24 vertices for the cube
	WVector3 verts[24];

	//fill the vertex buffer data
	verts[0] = WVector3(-0.5f, -0.5f, -0.5f);
	verts[1] = WVector3(-0.5f, 0.5f, -0.5f);
	verts[2] = WVector3(0.5f, 0.5f, -0.5f);
	verts[3] = WVector3(0.5f, -0.5f, -0.5f);
	verts[4] = WVector3(-0.5f, -0.5f, 0.5f);
	verts[5] = WVector3(0.5f, -0.5f, 0.5f);
	verts[6] = WVector3(0.5f, 0.5f, 0.5f);
	verts[7] = WVector3(-0.5f, 0.5f, 0.5f);
	verts[8] = WVector3(-0.5f, 0.5f, -0.5f);
	verts[9] = WVector3(-0.5f, 0.5f, 0.5f);
	verts[10] = WVector3(0.5f, 0.5f, 0.5f);
	verts[11] = WVector3(0.5f, 0.5f, -0.5f);
	verts[12] = WVector3(-0.5f, -0.5f, -0.5f);
	verts[13] = WVector3(0.5f, -0.5f, -0.5f);
	verts[14] = WVector3(0.5f, -0.5f, 0.5f);
	verts[15] = WVector3(-0.5f, -0.5f, 0.5f);
	verts[16] = WVector3(-0.5f, -0.5f, 0.5f);
	verts[17] = WVector3(-0.5f, 0.5f, 0.5f);
	verts[18] = WVector3(-0.5f, 0.5f, -0.5f);
	verts[19] = WVector3(-0.5f, -0.5f, -0.5f);
	verts[20] = WVector3(0.5f, -0.5f, -0.5f);
	verts[21] = WVector3(0.5f, 0.5f, -0.5f);
	verts[22] = WVector3(0.5f, 0.5f, 0.5f);
	verts[23] = WVector3(0.5f, -0.5f, 0.5f);

	//scale the vertices
	for (int i = 0; i < 24; i++) {
		verts[i].x *= boxDimensions.x * 2;
		verts[i].y *= boxDimensions.y * 2;
		verts[i].z *= boxDimensions.z * 2;
		verts[i] += boxPos;
	}

	//allocate 36 unsigned ints for the indices of the cube
	unsigned int _indices[36];

	//fill cube's index data
	_indices[0] = 0; _indices[1] = 1; _indices[2] = 2;
	_indices[3] = 0; _indices[4] = 2; _indices[5] = 3;
	_indices[6] = 4; _indices[7] = 5; _indices[8] = 6;
	_indices[9] = 4; _indices[10] = 6; _indices[11] = 7;
	_indices[12] = 8; _indices[13] = 9; _indices[14] = 10;
	_indices[15] = 8; _indices[16] = 10; _indices[17] = 11;
	_indices[18] = 12; _indices[19] = 13; _indices[20] = 14;
	_indices[21] = 12; _indices[22] = 14; _indices[23] = 15;
	_indices[24] = 16; _indices[25] = 17; _indices[26] = 18;
	_indices[27] = 16; _indices[28] = 18; _indices[29] = 19;
	_indices[30] = 20; _indices[31] = 21; _indices[32] = 22;
	_indices[33] = 20; _indices[34] = 22; _indices[35] = 23;

	for (unsigned int i = 0; i < 12; i++) {
		WVector3 v0 = verts[_indices[i * 3 + 0]];
		WVector3 v1 = verts[_indices[i * 3 + 1]];
		WVector3 v2 = verts[_indices[i * 3 + 2]];

		WVector3 e1, e2, h, s, q;
		float a, f, u, v;

		e1 = v1 - v0; //vector ( e1, v1, v0 );
		e2 = v2 - v0; //vector ( e2, v2, v0 );
		h = WVec3Cross(rayDir, e2);
		a = WVec3Dot(e1, h);

		if (a > -0.00001 && a < 0.00001)
			continue; //no intersection

		f = 1 / a;
		s = rayPos - v0; //vector ( s, p, v0 );
		u = f * (WVec3Dot(s, h));

		if (u < 0.0 || u > 1.0)
			continue; //no intersection

		q = WVec3Cross(s, e1);
		v = f * WVec3Dot(rayDir, q);
		if (v < 0.0 || u + v > 1.0)
			continue; //no intersection
					  // at this stage we can compute t to find out where 
					  // the intersection point is on the line
		float t = f * WVec3Dot(e2, q);
		if (t > 0.00001) // ray intersection
			return true;
	}

	return false;
}

float WUtil::frand_0_1() {
	return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
};
float WUtil::flerp(float x, float y, float f) {
	return x * (1 - f) + y * f;
}
