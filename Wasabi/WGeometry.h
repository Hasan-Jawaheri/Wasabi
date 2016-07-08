#pragma once

#include "Wasabi.h"
#include "WBase.h"
#include "WMath.h"

#define W_ATTRIBUTE_POSITION	W_VERTEX_ATTRIBUTE(std::string("position"), 3)
#define W_ATTRIBUTE_NORMAL		W_VERTEX_ATTRIBUTE(std::string("normal"), 3)
#define W_ATTRIBUTE_TANGENT		W_VERTEX_ATTRIBUTE(std::string("tangent"), 3)
#define W_ATTRIBUTE_UV			W_VERTEX_ATTRIBUTE(std::string("uv"), 2)

struct W_VERTEX_ATTRIBUTE {
	W_VERTEX_ATTRIBUTE(std::string n, unsigned char s) : name(n), size(s) {}

	std::string name;
	unsigned char size;
};

struct W_VERTEX_DESCRIPTION {
	W_VERTEX_DESCRIPTION(std::vector<W_VERTEX_ATTRIBUTE> attribs) : attributes(attribs) {}

	std::vector<W_VERTEX_ATTRIBUTE> attributes;

	size_t GetSize() const;
	size_t GetOffset(unsigned int attrib_index) const;
	size_t GetOffset(std::string attrib_name) const;
	unsigned int GetIndex(std::string attrib_name) const;
};

struct WDefaultVertex {
	WDefaultVertex(void) {}
	WDefaultVertex( float x, float y, float z,
					float tx, float ty, float tz,
					float nx, float ny, float nz,
					float u, float v)
					: pos(x, y, z), norm(nx, ny, nz), tang(tx, ty, tz), texC(u, v) {
	};
	WVector3 pos;
	WVector3 norm;
	WVector3 tang;
	WVector2 texC;
};

class WGeometry : public WBase {
	virtual std::string GetTypeName() const;

	struct {
		int count;
		VkBuffer buf;
		VkDeviceMemory mem;
	} m_vertices;

	struct {
		int count;
		VkBuffer buf;
		VkDeviceMemory mem;
	} m_indices;

	WVector3		m_maxPt;
	WVector3		m_minPt;

	void _DestroyResources();
	void _CalcMinMax(void* vb, unsigned int num_vert);
	void _CalcNormals(void* vb, unsigned int num_verts, void* ib, unsigned int num_indices);
	void _CalcTangents(void* vb, unsigned int num_verts);

public:
	WGeometry(Wasabi* const app, unsigned int ID = 0);
	~WGeometry();

	virtual W_VERTEX_DESCRIPTION GetVertexDescription() const {
		return W_VERTEX_DESCRIPTION({
			W_ATTRIBUTE_POSITION,
			W_ATTRIBUTE_NORMAL,
			W_ATTRIBUTE_TANGENT,
			W_ATTRIBUTE_UV,
		});
	}

	/*
		CopyFrom only works if *from is dynamic. Scale*, Apply*, Map* and Unmap* functions
		only work if the object is created with bDynamic = true.
		TODO: remove the dynamic constraint by having backup data buffers in RAM
	*/
	WError				CreateFromData(	void* vb, unsigned int num_verts,
										void* ib, unsigned int num_indices, bool bDynamic = false,
										bool bCalcNormals = false, bool bCalcTangents = false);
	WError				CreateCube(float fSize, bool bDynamic = false);
	WError				CreateBox(WVector3 dimensions, bool bDynamic = false);
	WError				CreatePlain(float fSize, int xsegs, int zsegs, bool bDynamic = false);
	WError				CreateSphere(float Radius, unsigned int VRes, unsigned int URes, bool bDynamic = false);
	WError				CreateCone(float fRadius, float fHeight, unsigned int hsegs, unsigned int csegs, bool bDynamic = false);
	WError				CreateCylinder(float fRadius, float fHeight, unsigned int hsegs, unsigned int csegs, bool bDynamic = false);
	WError				CopyFrom(WGeometry* const from, bool bDynamic = false);

	WError				MapVertexBuffer(void** const vb);
	WError				MapIndexBuffer(DWORD** const ib);
	void				UnmapVertexBuffer();
	void				UnmapIndexBuffer();

	bool				Intersect(WVector3 p1, WVector3 p2, unsigned int* triangleIndex, float* u, float* v, WVector3* pt) const;
	void				Scale(float mulFactor);
	void				ScaleX(float mulFactor);
	void				ScaleY(float mulFactor);
	void				ScaleZ(float mulFactor);
	void				ApplyOffset(float x, float y, float z);
	void				ApplyOffset(WVector3 offset);
	void				ApplyRotation(WMatrix mtx);

	WError				Draw(unsigned int num_triangles = -1);

	WVector3			GetMinPoint() const;
	WVector3			GetMaxPoint() const;
	unsigned int		GetNumVertices() const;
	unsigned int		GetNumIndices() const;

	virtual bool		Valid() const;
};

class WGeometryManager : public WManager<WGeometry> {
	friend class WGeometry;

	virtual std::string GetTypeName(void) const;

public:
	WGeometryManager(class Wasabi* const app);
};

