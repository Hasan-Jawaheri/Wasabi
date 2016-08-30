#pragma once

#include "Wasabi.h"
#include "WBase.h"
#include "WMath.h"

#define W_ATTRIBUTE_POSITION	W_VERTEX_ATTRIBUTE(std::string("position"), 3)
#define W_ATTRIBUTE_TANGENT		W_VERTEX_ATTRIBUTE(std::string("tangent"), 3)
#define W_ATTRIBUTE_NORMAL		W_VERTEX_ATTRIBUTE(std::string("normal"), 3)
#define W_ATTRIBUTE_UV			W_VERTEX_ATTRIBUTE(std::string("uv"), 2)
#define W_ATTRIBUTE_BONE_INDEX	W_VERTEX_ATTRIBUTE(std::string("bone_index"), 4)
#define W_ATTRIBUTE_BONE_WEIGHT	W_VERTEX_ATTRIBUTE(std::string("bone_weight"), 4)

struct W_VERTEX_ATTRIBUTE {
	W_VERTEX_ATTRIBUTE() : size(0) {}
	W_VERTEX_ATTRIBUTE(std::string n, unsigned char s) : name(n), size(s) {}

	std::string name;
	unsigned char size;
};

struct W_VERTEX_DESCRIPTION {
	W_VERTEX_DESCRIPTION(std::vector<W_VERTEX_ATTRIBUTE> attribs = {}) : attributes(attribs) {}

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
	WVector3 tang;
	WVector3 norm;
	WVector2 texC;
};

class WGeometry : public WBase {
	virtual std::string GetTypeName() const;

public:
	WGeometry(Wasabi* const app, unsigned int ID = 0);
	~WGeometry();

	virtual unsigned int GetVertexBufferCount() const {
		return 2; // one vertex buffer, one animation buffer
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(unsigned int layout_index = 0) const {
		if (layout_index >= 2)
			layout_index = 0;
		vector<W_VERTEX_DESCRIPTION> VD =
		{   W_VERTEX_DESCRIPTION({ // VB
			W_ATTRIBUTE_POSITION,
			W_ATTRIBUTE_TANGENT,
			W_ATTRIBUTE_NORMAL,
			W_ATTRIBUTE_UV,
		}), W_VERTEX_DESCRIPTION({ // AB
			W_ATTRIBUTE_BONE_INDEX,
			W_ATTRIBUTE_BONE_WEIGHT,
		})};
		return VD[layout_index];
	}

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

	WError				CreateAnimationData(void* animBuf);

	WError				LoadFromWGM(std::string filename, bool bDynamic = false);
	WError				SaveToWGM(std::string filename);
	WError				LoadFromHXM(std::string filename, bool bDynamic = false);

	WError				MapVertexBuffer(void** const vb, bool bReadOnly = false);
	WError				MapIndexBuffer(DWORD** const ib, bool bReadOnly = false);
	WError				MapAnimationBuffer(void** const ab, bool bReadOnly = false);
	void				UnmapVertexBuffer();
	void				UnmapIndexBuffer();
	void				UnmapAnimationBuffer();

	WError				Scale(float mulFactor);
	WError				ScaleX(float mulFactor);
	WError				ScaleY(float mulFactor);
	WError				ScaleZ(float mulFactor);
	WError				ApplyOffset(float x, float y, float z);
	WError				ApplyOffset(WVector3 offset);
	WError				ApplyRotation(WMatrix mtx);

	bool				Intersect(WVector3 p1, WVector3 p2,
								  WVector3* pt = nullptr, WVector2* uv = nullptr,
								  unsigned int* triangleIndex = nullptr);

	WError				Draw(class WRenderTarget* rt, unsigned int num_triangles = -1, unsigned int num_instances = 1);

	WVector3			GetMinPoint() const;
	WVector3			GetMaxPoint() const;
	unsigned int		GetNumVertices() const;
	unsigned int		GetNumIndices() const;
	bool				IsRigged() const;

	virtual bool		Valid() const;

private:
	struct {
		int count;
		W_BUFFER buffer, staging;
		bool readOnlyMap;
	} m_vertices;

	struct {
		int count;
		W_BUFFER buffer, staging;
		bool readOnlyMap;
	} m_indices;

	struct {
		int count;
		W_BUFFER buffer, staging;
		bool readOnlyMap;
	} m_animationbuf;

	WVector3		m_maxPt;
	WVector3		m_minPt;
	bool			m_dynamic;
	bool			m_immutable;

	void _DestroyResources();
	void _CalcMinMax(void* vb, unsigned int num_vert);
	void _CalcNormals(void* vb, unsigned int num_verts, void* ib, unsigned int num_indices);
	void _CalcTangents(void* vb, unsigned int num_verts);
};

class WGeometryManager : public WManager<WGeometry> {
	virtual std::string GetTypeName(void) const;

public:
	WGeometryManager(class Wasabi* const app);
	~WGeometryManager();
};

