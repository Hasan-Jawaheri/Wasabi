#pragma once

#include "Wasabi.h"
#include "WBase.h"
#include "WMath.h"

struct WVertex {
	WVertex(void) {}
	WVertex(float x, float y, float z,
		float tx, float ty, float tz,
		float nx, float ny, float nz,
		float u, float v)
		: pos(x, y, z), tang(tx, ty, tz), norm(nx, ny, nz), texC(u, v) {
	};

	WVector3 pos;
	WVector3 tang;
	WVector3 norm;
	WVector2 texC;
};

class WGeometry : public WBase {
public:
	WGeometry(Wasabi* app);
	~WGeometry();

	WError				CreateFromBuffers(const void* const vertices, const DWORD* const indices,
											UINT vbSize, UINT ibSize, UINT structSize = sizeof WVertex);
	WError				CreateCube(float fSize, bool bDynamic = false);
	WError				CreateBox(WVector3 dimensions, bool bDynamic = false);
	WError				CreatePlain(float fSize, int xsegs, int zsegs, bool bDynamic = false);
	WError				CreateSphere(float Radius, UINT VRes, UINT URes, bool bDynamic = false);
	WError				CreateCone(float fRadius, float fHeight, UINT hsegs, UINT csegs, bool bDynamic = false);
	WError				CreateCylinder(float fRadius, float fHeight, UINT hsegs, UINT csegs, bool bDynamic = false);
	WError				CopyFrom(const WGeometry* const geom);

	WError				LoadFromHXM(LPCSTR Filename);
	WError				LoadFromHXMString(LPCSTR string);
	WError				LoadFromHXM(basic_filebuf<char>* buff = NULL, UINT pos = 0);
	WError				SaveToHXM(basic_filebuf<char>* buff = NULL, UINT pos = 0) const;
	WError				SaveToHXM(LPCSTR Filename) const;

	bool				Intersect(WVector3 p1, WVector3 p2, UINT* triangleIndex, float* u, float* v, WVector3* pt) const;
	void				Scale(float mulFactor);
	void				ScaleX(float mulFactor);
	void				ScaleY(float mulFactor);
	void				ScaleZ(float mulFactor);
	void				ApplyOffset(float x, float y, float z);
	void				ApplyOffset(WVector3 offset);
	void				ApplyRotation(float x, float y, float z);
	void				ApplyRotation(WVector3 angle);
	UINT				GetNumVertices(void) const;
	UINT				GetNumIndices(void) const;
	WError				MapVertexBuffer(void** const vb, bool bReadOnly = false) const;
	WError				MapIndexBuffer(DWORD** const ib, bool bReadOnly = false) const;
	WError				UnmapVertexBuffer(void) const;
	WError				UnmapIndexBuffer(void) const;
	bool				Valid(void) const;

	WVector3			GetMinPoint(void) const;
	WVector3			GetMaxPoint(void) const;
};
