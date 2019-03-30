#include "WGeometry.h"
#include "../Renderers/WRenderer.h"
#include "../Images/WRenderTarget.h"

const W_VERTEX_DESCRIPTION g_defaultVertexDescriptions[] = {
	W_VERTEX_DESCRIPTION({ // Vertex buffer
		W_ATTRIBUTE_POSITION,
		W_ATTRIBUTE_TANGENT,
		W_ATTRIBUTE_NORMAL,
		W_ATTRIBUTE_UV,
	}), W_VERTEX_DESCRIPTION({ // Animation buffer
		W_ATTRIBUTE_BONE_INDEX,
		W_ATTRIBUTE_BONE_WEIGHT,
	})
};

static void ConvertVertices(void* vbFrom, void* vbTo, unsigned int num_verts,
							W_VERTEX_DESCRIPTION vtx_from, W_VERTEX_DESCRIPTION vtx_to) {

	size_t vtx_size = vtx_to.GetSize();
	size_t from_vtx_size = vtx_from.GetSize();
	for (int i = 0; i < num_verts; i++) {
		char* fromvtx = (char*)vbFrom + from_vtx_size * i;
		char* myvtx = (char*)vbTo + vtx_size * i;
		for (int j = 0; j < vtx_to.attributes.size(); j++) {
			std::string name = vtx_to.attributes[j].name;
			int off_in_from = vtx_from.GetOffset(name);
			int index_in_from = vtx_from.GetIndex(name);
			if (off_in_from >= 0 && vtx_to.attributes[j].num_components == vtx_from.attributes[index_in_from].num_components)
				memcpy(myvtx + vtx_to.GetOffset(name), fromvtx + off_in_from, vtx_to.attributes[j].num_components * 4);
		}
	}
}

size_t W_VERTEX_DESCRIPTION::GetSize() const {
	if (_size == -1) {
		_size = 0;
		for (int i = 0; i < attributes.size(); i++)
			_size += 4 * attributes[i].num_components;
	}
	return _size;
}

size_t W_VERTEX_DESCRIPTION::GetOffset(unsigned int attrib_index) const {
	size_t s = 0;
	for (int i = 0; i < attributes.size(); i++) {
		if (i == attrib_index)
			return s;
		s += 4 * attributes[i].num_components;
	}
	return -1;
}

size_t W_VERTEX_DESCRIPTION::GetOffset(std::string attrib_name) const {
	size_t s = 0;
	for (int i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == attrib_name)
			return s;
		s += 4 * attributes[i].num_components;
	}
	return -1;
}

unsigned int W_VERTEX_DESCRIPTION::GetIndex(std::string attrib_name) const {
	for (int i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == attrib_name)
			return i;
	}
	return -1;
}

bool W_VERTEX_DESCRIPTION::isEqualTo(W_VERTEX_DESCRIPTION other) const {
	if (attributes.size() != other.attributes.size())
		return false;
	for (int i = 0; i < attributes.size(); i++)
		if (attributes[i].name != other.attributes[i].name)
			return false;
	return true;
}

std::string WGeometryManager::GetTypeName(void) const {
	return "Geometry";
}

WGeometryManager::WGeometryManager(class Wasabi* const app) : WManager<WGeometry>(app) {
}

WGeometryManager::~WGeometryManager() {
}

WGeometry::WGeometry(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_mappedVertexBufferForWrite = nullptr;
	app->GeometryManager->AddEntity(this);
}

WGeometry::~WGeometry() {
	_DestroyResources();

	m_app->GeometryManager->RemoveEntity(this);
}

std::string WGeometry::GetTypeName() const {
	return "Geometry";
}

bool WGeometry::Valid() const {
	return m_vertices.Valid();
}

W_VERTEX_DESCRIPTION WGeometry::GetVertexDescription(unsigned int layout_index) const {
	if (layout_index >= sizeof(g_defaultVertexDescriptions)/sizeof(W_VERTEX_DESCRIPTION))
		layout_index = 0;
	return g_defaultVertexDescriptions[layout_index];
}

size_t WGeometry::GetVertexDescriptionSize(unsigned int layout_index) const {
	if (layout_index >= sizeof(g_defaultVertexDescriptions) / sizeof(W_VERTEX_DESCRIPTION))
		layout_index = 0;
	return g_defaultVertexDescriptions[layout_index].GetSize();
}

void WGeometry::_DestroyResources() {
	m_vertices.Destroy(m_app);
	m_indices.Destroy(m_app);
	m_animationbuf.Destroy(m_app);
}

void WGeometry::_CalcMinMax(void* vb, unsigned int num_verts) {
	m_minPt = WVector3(FLT_MAX, FLT_MAX, FLT_MAX);
	m_maxPt = WVector3(FLT_MIN, FLT_MIN, FLT_MIN);

	int offset = GetVertexDescription(0).GetOffset("position");
	int vsize = GetVertexDescription(0).GetSize();

	if (offset == -1)
		return; // no position attribute

	for (int i = 0; i < num_verts; i++) {
		WVector3 v;
		memcpy(&v, (char*)vb + vsize * i + offset, sizeof(WVector3));
		m_minPt.x = fmin(m_minPt.x, v.x);
		m_minPt.y = fmin(m_minPt.y, v.y);
		m_minPt.z = fmin(m_minPt.z, v.z);
		m_maxPt.x = fmax(m_maxPt.x, v.x);
		m_maxPt.y = fmax(m_maxPt.y, v.y);
		m_maxPt.z = fmax(m_maxPt.z, v.z);
	}
}

void WGeometry::_CalcNormals(void* vb, unsigned int num_verts, void* ib, unsigned int num_indices) {
	if (num_indices % 3 != 0)
		return; // must be a triangle list

	int offset = GetVertexDescription(0).GetOffset("position");
	int normal_offset = GetVertexDescription(0).GetOffset("normal");
	int vsize = GetVertexDescription(0).GetSize();

	if (offset == -1 || normal_offset == -1)
		return; // no position/normal attributes

	int num_tris = num_indices / 3;
	for (int i = 0; i < num_tris; i++) {
		uint ind[3];
		memcpy(ind, (char*)ib + (i*3) * sizeof(uint), 3 * sizeof(uint));
		WVector3 p[3];
		memcpy(&p[0], (char*)vb + vsize * (ind[0]) + offset, sizeof(WVector3));
		memcpy(&p[1], (char*)vb + vsize * (ind[1]) + offset, sizeof(WVector3));
		memcpy(&p[2], (char*)vb + vsize * (ind[2]) + offset, sizeof(WVector3));
		WVector3 norm = WVec3Normalize(WVec3Cross(p[1] - p[0], p[2] - p[0]));
		memcpy((char*)vb + vsize * (ind[0]) + normal_offset, &norm, sizeof(WVector3));
		memcpy((char*)vb + vsize * (ind[1]) + normal_offset, &norm, sizeof(WVector3));
		memcpy((char*)vb + vsize * (ind[2]) + normal_offset, &norm, sizeof(WVector3));
	}
}

void WGeometry::_CalcTangents(void* vb, unsigned int num_verts) {
	int norm_offset = GetVertexDescription(0).GetOffset("normal");
	int tang_offset = GetVertexDescription(0).GetOffset("tangent");
	int vsize = GetVertexDescription(0).GetSize();

	if (norm_offset == -1 || tang_offset == -1)
		return; // no position attribute

	for (unsigned int i = 0; i < num_verts; i++) {
		WVector3 norm;
		memcpy(&norm, (char*)vb + vsize * i + norm_offset, sizeof(WVector3));

		WVector3 forw(0, 0, 1);
		WVector3 up(0, 1, 0);
		WVector3 c1 = WVec3Cross(norm, forw);
		WVector3 c2 = WVec3Cross(norm, up);
		WVector3 tang;

		if (WVec3LengthSq(c1) > WVec3LengthSq(c2))
			tang = c1;
		else
			tang = c2;

		tang = WVec3Normalize(tang);
		memcpy((char*)vb + vsize * i + tang_offset, &tang, sizeof(WVector3));
	}
}

WError WGeometry::CreateFromData(void* vb, unsigned int numVerts, void* ib, unsigned int numIndices,
									bool bDynamic, bool bCalcNormals, bool bCalcTangents) {
	if (numVerts <= 0 || !vb || (numIndices > 0 && !ib))
		return WError(W_INVALIDPARAM);

	size_t vertexBufferSize = numVerts * GetVertexDescription(0).GetSize();
	size_t indexBufferSize = numIndices * sizeof(uint);

	_DestroyResources();

	if (bCalcNormals && vb && ib && GetVertexDescription(0).GetIndex("normal") >= 0)
		_CalcNormals(vb, numVerts, ib, numIndices);
	if (bCalcTangents && vb && GetVertexDescription(0).GetIndex("tangent") >= 0)
		_CalcTangents(vb, numVerts);


	uint numBuffers = bDynamic ? (uint)m_app->engineParams["bufferingCount"] : 1;
	W_MEMORY_STORAGE memory = bDynamic ? W_MEMORY_HOST_VISIBLE : W_MEMORY_DEVICE_LOCAL_HOST_COPY;
	VkResult result = m_vertices.Create(m_app, numBuffers, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vb, memory);
	if (result == VK_SUCCESS && indexBufferSize > 0) {
		result = m_indices.Create(m_app, numBuffers, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, ib, memory);
	}

	if (result != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	m_numVertices = numVerts;
	m_numIndices = numIndices;
	_CalcMinMax(vb, numVerts);

	return WError(W_SUCCEEDED);
}

WError WGeometry::CreateFromDefaultVerticesData(vector<WDefaultVertex>& default_vertices, vector<uint>& indices, bool bDynamic) {
	return CreateFromData(default_vertices.data(), default_vertices.size(), indices.data(), indices.size(), bDynamic);
}

WError WGeometry::CreateAnimationData(void* ab) {
	if (!Valid() || GetVertexBufferCount() < 2)
		return WError(W_NOTVALID);
	if (!ab)
		return WError(W_INVALIDPARAM);

	int animBufferSize = m_numVertices * GetVertexDescription(1).GetSize();
	VkResult result = m_vertices.Create(m_app, 1, animBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, ab);
	if (result != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	return WError(W_SUCCEEDED);
}

WError WGeometry::CreateCube(float fSize, bool bDynamic) {
	return CreateBox(WVector3(fSize, fSize, fSize), bDynamic);
}

WError WGeometry::CreateBox(WVector3 dimensions, bool bDynamic) {
	vector<WDefaultVertex> vertices(24);

	//fill the vertex buffer data
	vertices[0] = WDefaultVertex(-0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vertices[1] = WDefaultVertex(-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	vertices[2] = WDefaultVertex(0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	vertices[3] = WDefaultVertex(0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	vertices[4] = WDefaultVertex(-0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	vertices[5] = WDefaultVertex(0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	vertices[6] = WDefaultVertex(0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	vertices[7] = WDefaultVertex(-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

	vertices[8] = WDefaultVertex(-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	vertices[9] = WDefaultVertex(-0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	vertices[10] = WDefaultVertex(0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
	vertices[11] = WDefaultVertex(0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);

	vertices[12] = WDefaultVertex(-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);
	vertices[13] = WDefaultVertex(0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
	vertices[14] = WDefaultVertex(0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
	vertices[15] = WDefaultVertex(-0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);

	vertices[16] = WDefaultVertex(-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	vertices[17] = WDefaultVertex(-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	vertices[18] = WDefaultVertex(-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	vertices[19] = WDefaultVertex(-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	vertices[20] = WDefaultVertex(0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	vertices[21] = WDefaultVertex(0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	vertices[22] = WDefaultVertex(0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	vertices[23] = WDefaultVertex(0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	//scale the vertices
	for (int i = 0; i < 24; i++)
		vertices[i].pos *= dimensions;

	//allocate 36 uints for the indices of the cube
	vector<uint> indices(36);

	//fill cube's index data
	indices[0] = 0; indices[1] = 1; indices[2] = 2;
	indices[3] = 0; indices[4] = 2; indices[5] = 3;
	indices[6] = 4; indices[7] = 5; indices[8] = 6;
	indices[9] = 4; indices[10] = 6; indices[11] = 7;
	indices[12] = 8; indices[13] = 9; indices[14] = 10;
	indices[15] = 8; indices[16] = 10; indices[17] = 11;
	indices[18] = 12; indices[19] = 13; indices[20] = 14;
	indices[21] = 12; indices[22] = 14; indices[23] = 15;
	indices[24] = 16; indices[25] = 17; indices[26] = 18;
	indices[27] = 16; indices[28] = 18; indices[29] = 19;
	indices[30] = 20; indices[31] = 21; indices[32] = 22;
	indices[33] = 20; indices[34] = 22; indices[35] = 23;

	return CreateFromDefaultVerticesData(vertices, indices, bDynamic);
}

WError WGeometry::CreatePlain(float fSize, int xsegs, int zsegs, bool bDynamic) {
	unsigned int num_indices = (((xsegs + 1)*(zsegs + 1)) * 2) * 3;
	unsigned int num_vertices = (xsegs + 2)*(zsegs + 2);

	//allocate the plain vertices
	vector<WDefaultVertex> vertices(num_vertices);

	int vpc = zsegs + 2, vpr = xsegs + 2;
	float uI = 1.0f / (float)(vpc - 1);
	float vI = 1.0f / (float)(vpr - 1);

	float _u = 0.0f, _v = 0.0f;
	uint curVert = 0;

	//fill the plain vertex data
	for (uint z = vpc - 1; z >= 0 && z != UINT_MAX; z--) {
		for (uint x = 0; x < vpr; x++) {
			vertices[curVert] =
				WDefaultVertex((float)x / (vpr - 1), 0.0f, (float)z / (vpc - 1), 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, _u, _v);
			vertices[curVert].pos.x -= 0.5f;
			vertices[curVert].pos.z -= 0.5f;
			vertices[curVert].pos *= fSize;
			_u += uI; //u starts from 0 increasing to 1
			curVert++;
		}
		_v += vI; //v starts from 0 increasing to 1
		_u = 0;
	}


	//allocate the index buffer
	vector<uint> indices(num_indices);

	//fill the indices data
	int numt = 0;
	int offset = 0;
	for (int t = 0; t < (num_indices / 3) / 2; t++) {
		indices[t * 6 + 2] = 1 + offset;
		indices[t * 6 + 1] = 0 + offset;
		indices[t * 6 + 0] = vpr + offset;

		indices[t * 6 + 5] = vpr + offset + 1;
		indices[t * 6 + 4] = 1 + offset;
		indices[t * 6 + 3] = vpr + offset;

		numt++;
		offset++;

		if (numt == vpr - 1) {
			offset++;
			numt = 0;
		}
	}

	return CreateFromDefaultVerticesData(vertices, indices, bDynamic);
}

WError WGeometry::CreateSphere(float Radius, unsigned int VRes, unsigned int URes, bool bDynamic) {
	if (VRes < 3 || URes < 2)
		return WError(W_INVALIDPARAM);
	const uint NumVertexRings = VRes - 2;
	unsigned int num_vertices = NumVertexRings * URes + 2;
	const uint NumTriangleRings = VRes - 1;
	const uint NumTriangles = (NumTriangleRings + 1) * URes * 2;
	unsigned int num_indices = NumTriangles * 3;

	// Calculate all of the vertex positions
	vector<WDefaultVertex> vertices (num_vertices);
	int currVert = 0;

	// First vertex will be at the top pole
	vertices[currVert++] = WDefaultVertex(0.0f, Radius, 0, 1, 0, 0, 0, 1, 0, 0, 0);

	// Add in the vertical rings of vertices
	for (uint v = 1; v <= NumVertexRings; ++v) {
		for (uint u = 0; u < URes; ++u) {
			float uAngle = u / (float)(URes)* W_PI * 2.0f;
			float vAngle = v / (float)(VRes - 1) * W_PI;

			float x = sinf(vAngle) * cosf(uAngle) * Radius;
			float y = cosf(vAngle) * Radius;
			float z = -sinf(vAngle) * sinf(uAngle) * Radius;
			WVector3 norm = WVector3(x, y, z) / Radius;
			WVector3 tang = WVec3Cross(norm, -norm);

			vertices[currVert++] =
				WDefaultVertex(x, y, z, tang.x, tang.y, tang.z, norm.x, norm.y, norm.z, uAngle / (W_PI*2.0f), vAngle / W_PI);
		}
	}

	// Last vertex is the bottom pole
	vertices[currVert++] = WDefaultVertex(0.0f, -Radius, 0, -1, 0, 0, 0, -1, 0, 0, 1);

	// Now we'll add the triangles
	vector<uint> indices(num_indices);
	uint curIndex = 0;

	// Top ring first
	for (uint u = 0; u < URes; ++u) {
		const uint currentU = u;
		const uint nextU = (u + 1) % URes;
		indices[curIndex++] = 0;
		indices[curIndex++] = u + 1;
		indices[curIndex++] = nextU + 1;
	}

	// Now the middle rings
	for (uint v = 1; v < VRes - 2; ++v) {
		const uint top = 1 + ((v - 1) * URes);
		const uint bottom = top + URes;
		for (uint u = 0; u < URes; ++u) {
			const uint currentU = u;
			const uint nextU = (u + 1) % URes;
			const uint currTop = top + currentU;
			const uint nextTop = top + nextU;
			const uint currBottom = bottom + currentU;
			const uint nextBottom = bottom + nextU;

			indices[curIndex++] = currTop;
			indices[curIndex++] = currBottom;
			indices[curIndex++] = nextBottom;

			indices[curIndex++] = nextBottom;
			indices[curIndex++] = nextTop;
			indices[curIndex++] = currTop;
		}
	}

	// Now the bottom ring
	const uint top = 1 + ((NumVertexRings - 1) * URes);
	const uint bottom = num_vertices - 1;
	for (uint u = 0; u < URes; ++u) {
		const uint currentU = u;
		const uint nextU = (u + 1) % URes;
		const uint currTop = top + currentU;
		const uint nextTop = top + nextU;

		indices[curIndex++] = currTop;
		indices[curIndex++] = bottom;
		indices[curIndex++] = nextTop;
	}

	return CreateFromDefaultVerticesData(vertices, indices, bDynamic);
}

WError WGeometry::CreateCone(float fRadius, float fHeight, unsigned int hsegs, unsigned int csegs, bool bDynamic) {
	hsegs += 2;
	if (csegs < 3 || hsegs < 2)
		return WError(W_INVALIDPARAM);
	//3 indices * number of triangles (top and bottom triangles + side triangles)
	unsigned int num_indices = 3 * (csegs + (hsegs - 1)*csegs * 2);
	//bottom with the circle vertices plus [csegs] vertices for every [hsegs] + one extra seg for uvs
	unsigned int num_vertices = 1 + csegs + hsegs * (csegs + 1);

	//allocate vertices
	vector<WDefaultVertex> vertices (num_vertices);

	uint pos = 0; //current vertex index
	float deltaAngle = (W_PI*2.0f) / csegs;
	float fCurAngle = 0.0f;

	//a circle of vertices for every height segment
	for (uint i = 0; i < hsegs; i++) {
		fCurAngle = 0.0f;
		for (uint n = 0; n <= csegs; n++) {
			float y = 1.0f - ((float)i / (float)(hsegs - 1));
			float x = sinf(fCurAngle) * (1.0f - y);
			float z = cosf(fCurAngle) * (1.0f - y);
			WVector3 norm = WVec3Normalize(WVector3(sinf(fCurAngle), (2.0f*fRadius) / fHeight, cosf(fCurAngle)));
			vertices[pos++] =
				WDefaultVertex(x, y, z, 0.0f, 1.0f, 0.0f, norm.x, norm.y, norm.z, fCurAngle / (W_PI*2.0f), y - 1.0f);

			fCurAngle += deltaAngle;
		}
	}

	//bottom vertex and it's circle
	fCurAngle = 0.0f;
	for (uint n = 0; n < csegs; n++) {
		float x = sinf(fCurAngle);
		float z = cosf(fCurAngle);
		vertices[pos++] = WDefaultVertex(x, 0.0f, z, 0.0f, 1.0f, 0.0f, 0, -1.0f, 0, 0.0f, 0.0f);

		fCurAngle += deltaAngle;
	}
	vertices[pos++] = WDefaultVertex(0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);

	//scale and shift vertices
	for (uint i = 0; i < num_vertices; i++) {
		vertices[i].pos.y -= 0.5f;
		vertices[i].pos.x *= fRadius;
		vertices[i].pos.y *= fHeight;
		vertices[i].pos.z *= fRadius;
	}

	//allocate uints for the indices
	vector<uint> indices(num_indices);
	pos = 0;

	//middle triangles
	uint ringBaseIndex = 0;
	for (uint i = 0; i < hsegs - 1; i++) {
		for (uint n = 0; n < csegs; n++) {
			indices[pos++] = ringBaseIndex + 1;
			indices[pos++] = ringBaseIndex;
			indices[pos++] = ringBaseIndex + csegs + 2; //actual vertices csegs are csegs+1

			indices[pos++] = ringBaseIndex + csegs + 2;
			indices[pos++] = ringBaseIndex;
			indices[pos++] = ringBaseIndex + csegs + 1; //actual vertices csegs are csegs+1

			ringBaseIndex++;
		}
		ringBaseIndex++; //skip the extra cseg
	}

	//bottom triangles
	for (uint i = 0; i < csegs; i++) {
		indices[pos++] = num_vertices - 1;
		indices[pos++] = num_vertices - 1 - csegs + i + 1;
		indices[pos++] = num_vertices - 1 - csegs + i;
		if (indices[pos - 2] == num_vertices - 1)
			indices[pos - 2] = num_vertices - 1 - csegs;
	}

	return CreateFromDefaultVerticesData(vertices, indices, bDynamic);
}

WError WGeometry::CreateCylinder(float fRadius, float fHeight, unsigned int hsegs, unsigned int csegs, bool bDynamic) {
	hsegs -= 2;
	if (csegs < 3 || hsegs < 2)
		return WError(W_INVALIDPARAM);
	//3 indices * number of triangles (top and bottom triangles + side triangles)
	unsigned int num_indices = 3 * (csegs * 2 + (hsegs - 1)*csegs * 2);
	//top and bottom with their circles vertices plus [csegs] vertices for every [hsegs] + one extra seg for uvs
	unsigned int num_vertices = 2 + csegs * 2 + hsegs * (csegs + 1);

	//allocate vertices
	vector<WDefaultVertex> vertices (num_vertices);

	uint pos = 0; //current vertex index
	float deltaAngle = (W_PI*2.0f) / csegs;
	float fCurAngle = 0.0f;
	//first, top vertex and it's circle
	vertices[pos++] = WDefaultVertex(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	fCurAngle = 0.0f;
	for (uint n = 0; n < csegs; n++) {
		float x = sinf(fCurAngle);
		float z = cosf(fCurAngle);
		vertices[pos++] = WDefaultVertex(x, 1.0f, z, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);

		fCurAngle += deltaAngle;
	}

	//a circle of vertices for every height segment
	for (uint i = 0; i < hsegs; i++) {
		fCurAngle = 0.0f;
		for (uint n = 0; n <= csegs; n++) {
			float x = sinf(fCurAngle);
			float z = cosf(fCurAngle);
			float y = 1.0f - ((float)i / (float)(hsegs - 1));
			vertices[pos++] = WDefaultVertex(x, y, z, 0.0f, 1.0f, 0.0f, x, 0, z, fCurAngle / (W_PI*2.0f), y - 1.0f);

			fCurAngle += deltaAngle;
		}
	}

	//bottom vertex and it's circle
	fCurAngle = 0.0f;
	for (uint n = 0; n < csegs; n++) {
		float x = sinf(fCurAngle);
		float z = cosf(fCurAngle);
		vertices[pos++] = WDefaultVertex(x, 0.0f, z, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);

		fCurAngle += deltaAngle;
	}
	vertices[pos++] = WDefaultVertex(0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);

	//scale and shift vertices
	for (uint i = 0; i < num_vertices; i++) {
		vertices[i].pos.y -= 0.5f;
		vertices[i].pos.x *= fRadius;
		vertices[i].pos.y *= fHeight;
		vertices[i].pos.z *= fRadius;
	}

	//allocate uints for the indices
	vector<uint> indices(num_indices);
	pos = 0;

	//top triangles
	for (uint i = 0; i < csegs; i++) {
		indices[pos++] = 0;
		indices[pos++] = i + 1;
		indices[pos++] = i + 2;
		if (indices[pos - 1] == csegs + 1)
			indices[pos - 1] = 1;
	}

	//middle triangles
	uint ringBaseIndex = csegs + 1;
	for (uint i = 0; i < hsegs - 1; i++) {
		for (uint n = 0; n < csegs; n++) {
			indices[pos++] = ringBaseIndex + 1;
			indices[pos++] = ringBaseIndex;
			indices[pos++] = ringBaseIndex + csegs + 2; //actual vertices csegs are csegs+1

			indices[pos++] = ringBaseIndex + csegs + 2;
			indices[pos++] = ringBaseIndex;
			indices[pos++] = ringBaseIndex + csegs + 1; //actual vertices csegs are csegs+1

			ringBaseIndex++;
		}
		ringBaseIndex++; //skip the extra cseg
	}

	//bottom triangles
	for (uint i = 0; i < csegs; i++) {
		indices[pos++] = num_vertices - 1;
		indices[pos++] = num_vertices - 1 - csegs + i + 1;
		indices[pos++] = num_vertices - 1 - csegs + i;
		if (indices[pos - 2] == num_vertices - 1)
			indices[pos - 2] = num_vertices - 1 - csegs;
	}

	return CreateFromDefaultVerticesData(vertices, indices, bDynamic);
}

WError WGeometry::CopyFrom(WGeometry* const from, bool bDynamic) {
	if (!from->Valid())
		return WError(W_INVALIDPARAM);

	unsigned int num_verts = from->GetNumVertices();
	unsigned int num_indices = from->GetNumIndices();
	W_VERTEX_DESCRIPTION my_desc = GetVertexDescription(0);
	W_VERTEX_DESCRIPTION from_desc = from->GetVertexDescription(0);
	void *vb, *fromvb, *fromib;

	if (!my_desc.isEqualTo(from_desc)) {
		vb = W_SAFE_ALLOC(num_verts *my_desc.GetSize());
		if (!vb)
			return WError(W_OUTOFMEMORY);
	}

	WError ret = from->MapIndexBuffer((uint**)&fromib, W_MAP_READ);
	if (!ret) {
		W_SAFE_FREE(vb);
		return ret;
	}
	ret = from->MapVertexBuffer(&fromvb, W_MAP_READ);
	if (!ret) {
		from->UnmapIndexBuffer();
		W_SAFE_FREE(vb);
		return ret;
	}

	if (!vb)
		vb = fromvb;
	else
		ConvertVertices(fromvb, vb, num_verts, from_desc, my_desc);

	bool bCalcTangents = false, bCalcNormals = false;
	if (my_desc.GetOffset("normal") >= 0 && from_desc.GetOffset("normal") == -1)
		bCalcNormals = true;
	if (my_desc.GetOffset("tangent") >= 0 && from_desc.GetOffset("tangent") == -1)
		bCalcTangents = true;

	from->UnmapVertexBuffer();
	ret = CreateFromData(vb, num_verts, fromib, num_indices, bDynamic, bCalcNormals, bCalcTangents);
	from->UnmapIndexBuffer();
	if (!my_desc.isEqualTo(from_desc))
		W_SAFE_FREE(vb);

	if (ret && from->m_animationbuf.Valid()) {
		void* fromab;
		ret = from->MapAnimationBuffer(&fromab, W_MAP_READ);
		if (ret) {
			ret = CreateAnimationData(fromab);
			from->UnmapAnimationBuffer();
		}
	}

	return ret;
}

WError WGeometry::LoadFromHXM(std::string filename, bool bDynamic) {
	W_VERTEX_DESCRIPTION hx_vtx_desc = W_VERTEX_DESCRIPTION({
		W_ATTRIBUTE_POSITION,
		W_ATTRIBUTE_TANGENT,
		W_ATTRIBUTE_NORMAL,
		W_ATTRIBUTE_UV,
	});

	//open the file for reading
	std::fstream file;
	file.open(filename, ios::in | ios::binary);
	if (!file.is_open())
		return WError(W_FILENOTFOUND);

	uint numV = 0, numI = 0, structSize = 0, filesize = 0;
	char usedBuffers = 1, topology;

	file.seekg(0, ios::end);
	filesize = file.tellg();
	file.seekg(0, ios::beg);

	if (filesize < 11) {
		file.close();
		return WError(W_INVALIDFILEFORMAT);
	}

	//read the header ([topology(1)][vertex size(1)][vbSize(4)][ibSize(4)][[usedBuffers(1)]])
	file.read((char*)&topology, 1); //topology is saved in D3D format
	file.read((char*)&structSize, 1);
	file.read((char*)&numV, 4);
	file.read((char*)&numI, 4);

	if (filesize >= 11 + 44 * numV + sizeof(uint) * numI)
		file.read(&usedBuffers, 1);

	if (topology != 4 /*D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST*/ ||
		structSize != 44 || // wrong vertex format, cannot read it (3x position, 3x tangent, 3x normal, 2x uv)
		!(usedBuffers & 1) || numI == 0 || numV == 0 || // no index/vertex buffers
		(filesize != 11 + 44 * numV + sizeof(uint) * numI && filesize != 10 + 44 * numV + sizeof(uint) * numI
		 && filesize != 11 + 44 * numV + sizeof(uint) * numI + (4*4+4*4) * numV)) {
		file.close();
		return WError(W_INVALIDFILEFORMAT);
	}

	//allocate temporary vertex/index buffers
	char* v = new char[numV * structSize];
	uint* ind = new uint[numI];
	char* a = nullptr;

	if (!v || !ind) {
		W_SAFE_DELETE_ARRAY(v);
		W_SAFE_DELETE_ARRAY(ind);
		return WError(W_OUTOFMEMORY);
	}

	//read data
	file.read((char*)v, numV * structSize);
	file.read((char*)ind, numI * sizeof(uint));
	if (usedBuffers & 2) { // animation data
		a = new char[numV * (4 * 4 + 4 * 4)];
		file.read((char*)a, numV * (4 * 4 + 4 * 4));
	}
	file.close();

	W_VERTEX_DESCRIPTION my_desc = GetVertexDescription(0);
	void* newverts;
	if (my_desc.isEqualTo(hx_vtx_desc))
		newverts = v;
	else
		newverts = (char*)W_SAFE_ALLOC(numV * my_desc.GetSize());
	if (!newverts) {
		W_SAFE_DELETE_ARRAY(v);
		W_SAFE_DELETE_ARRAY(ind);
		return WError(W_OUTOFMEMORY);
	}

	if (!my_desc.isEqualTo(hx_vtx_desc)) {
		ConvertVertices((void*)v, newverts, numV, hx_vtx_desc, my_desc);
		W_SAFE_DELETE_ARRAY(v);
	}

	WError ret = CreateFromData(newverts, numV, ind, numI, bDynamic);

	//de-allocate the temporary buffers
	W_SAFE_FREE(newverts);
	W_SAFE_DELETE_ARRAY(ind);

	if (a) {
		if (GetVertexBufferCount() > 1 || GetVertexDescription(1).GetSize() == (4 * 4 + 4 * 4) && ret) {
			ret = CreateAnimationData((void*)a);
		}
		W_SAFE_DELETE_ARRAY(a);
	}

	return ret;
}

WError WGeometry::MapVertexBuffer(void** const vb, W_MAP_FLAGS mapFlags) {
	uint bufferIndex = m_app->Renderer->GetCurrentBufferingIndex();
	VkResult result = m_vertices.Map(m_app, bufferIndex, vb, mapFlags);
	if (result != VK_SUCCESS)
		return WError(W_NOTVALID);
	if (mapFlags & W_MAP_WRITE)
		m_mappedVertexBufferForWrite = *vb;
	return WError(W_SUCCEEDED);
}

WError WGeometry::MapIndexBuffer(uint** const ib, W_MAP_FLAGS mapFlags) {
	uint bufferIndex = m_app->Renderer->GetCurrentBufferingIndex();
	VkResult result = m_indices.Map(m_app, bufferIndex, (void**)ib, mapFlags);
	if (result != VK_SUCCESS)
		return WError(W_NOTVALID);
	return WError(W_SUCCEEDED);
}

WError WGeometry::MapAnimationBuffer(void** const ab, W_MAP_FLAGS mapFlags) {
	uint bufferIndex = m_app->Renderer->GetCurrentBufferingIndex();
	VkResult result = m_animationbuf.Map(m_app, bufferIndex, ab, mapFlags);
	if (result != VK_SUCCESS)
		return WError(W_NOTVALID);
	return WError(W_SUCCEEDED);
}

void WGeometry::UnmapVertexBuffer() {
	if (m_mappedVertexBufferForWrite) {
		_CalcMinMax(m_mappedVertexBufferForWrite, m_numVertices);
		m_mappedVertexBufferForWrite = nullptr;
	}

	uint bufferIndex = m_app->Renderer->GetCurrentBufferingIndex();
	m_vertices.Unmap(m_app, bufferIndex);
}

void WGeometry::UnmapIndexBuffer() {
	uint bufferIndex = m_app->Renderer->GetCurrentBufferingIndex();
	m_indices.Unmap(m_app, bufferIndex);
}

void WGeometry::UnmapAnimationBuffer() {
	uint bufferIndex = m_app->Renderer->GetCurrentBufferingIndex();
	m_animationbuf.Unmap(m_app, bufferIndex);
}

WError WGeometry::Scale(float mulFactor) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription(0).GetSize();
	int offset = GetVertexDescription(0).GetOffset("position");
	if (offset == -1)
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].num_components * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (int i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtx_size * i + offset, size);
		v *= mulFactor;
		memcpy((char*)data + vtx_size * i + offset, &v, size);
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ScaleX(float mulFactor) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription(0).GetSize();
	int offset = GetVertexDescription(0).GetOffset("position");
	if (offset == -1)
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].num_components * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (int i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtx_size * i + offset, size);
		v.x *= mulFactor;
		memcpy((char*)data + vtx_size * i + offset, &v, size);
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ScaleY(float mulFactor) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription(0).GetSize();
	int offset = GetVertexDescription(0).GetOffset("position");
	if (offset == -1)
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].num_components * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (int i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtx_size * i + offset, size);
		v.y *= mulFactor;
		memcpy((char*)data + vtx_size * i + offset, &v, size);
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ScaleZ(float mulFactor) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription(0).GetSize();
	int offset = GetVertexDescription(0).GetOffset("position");
	if (offset == -1)
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].num_components * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (int i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtx_size * i + offset, size);
		v.z *= mulFactor;
		memcpy((char*)data + vtx_size * i + offset, &v, size);
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ApplyOffset(float x, float y, float z) {
	return ApplyOffset(WVector3(x, y, z));
}

WError WGeometry::ApplyOffset(WVector3 _offset) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription(0).GetSize();
	int offset = GetVertexDescription(0).GetOffset("position");
	if (offset == -1)
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].num_components * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (int i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtx_size * i + offset, size);
		v += _offset;
		memcpy((char*)data + vtx_size * i + offset, &v, size);
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ApplyTransformation(WMatrix mtx) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription(0).GetSize();
	int offset = GetVertexDescription(0).GetOffset("position");
	if (offset == -1)
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].num_components * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (int i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtx_size * i + offset, size);
		v = WVec3TransformCoord(v, mtx);
		memcpy((char*)data + vtx_size * i + offset, &v, size);
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

bool WGeometry::Intersect(WVector3 p1, WVector3 p2, WVector3* pt, WVector2* uv, unsigned int* triangleIndex) {
	if (!Valid())
		return false;

	/*
		Check all triangles, what is intersected will
		be inserted into the vector intersection, then
		the closest triangle to p1 will be returned
	*/

	unsigned int pos_offset = GetVertexDescription(0).GetOffset("position");
	unsigned int vtx_size = GetVertexDescription(0).GetSize();
	unsigned int uv_offset = GetVertexDescription(0).GetOffset("uv");
	unsigned int uv_size = -1;

	if (pos_offset == -1)
		return false;
	if (GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].num_components < 3)
		return false;
	if (uv_offset != -1)
		uv_size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("uv")].num_components * 4;
	if (uv_size < 8) // if we don't have at least 2 components, ignore UVs
		uv_offset = -1;

	struct IntersectionInfo {
		unsigned int index;
		float u, v;
		WVector3 point;
	};
	vector<IntersectionInfo> intersection;

	void *vb;
	uint* ib;
	WError err = MapVertexBuffer(&vb, W_MAP_READ);
	if (!err)
		return false;
	err = MapIndexBuffer(&ib, W_MAP_READ);
	if (!err) {
		UnmapVertexBuffer();
		return false;
	}

	for (uint i = 0; i < m_numIndices / 3; i++) {
		WVector3 v0;
		WVector3 v1;
		WVector3 v2;
		WVector2 uv0 (0, 0);
		WVector2 uv1 (1, 0);
		WVector2 uv2 (0, 1);
		memcpy(&v0, &((char*)vb)[ib[i * 3 + 0] * vtx_size + pos_offset], sizeof(WVector3));
		memcpy(&v1, &((char*)vb)[ib[i * 3 + 1] * vtx_size + pos_offset], sizeof(WVector3));
		memcpy(&v2, &((char*)vb)[ib[i * 3 + 2] * vtx_size + pos_offset], sizeof(WVector3));
		if (uv_offset != -1) {
			memcpy(&uv0, &((char*)vb)[ib[i * 3 + 0] * vtx_size + uv_offset], sizeof(WVector2));
			memcpy(&uv1, &((char*)vb)[ib[i * 3 + 1] * vtx_size + uv_offset], sizeof(WVector2));
			memcpy(&uv2, &((char*)vb)[ib[i * 3 + 2] * vtx_size + uv_offset], sizeof(WVector2));
		}

		WVector3 e1, e2, h, s, q;
		float a, f, u, v;

		e1 = v1 - v0; //vector ( e1, v1, v0 );
		e2 = v2 - v0; //vector ( e2, v2, v0 );
		h = WVec3Cross(p2, e2);
		a = WVec3Dot(e1, h);

		if (a > -0.00001 && a < 0.00001)
			continue; //no intersection

		f = 1 / a;
		s = p1 - v0; //vector ( s, p, v0 );
		u = f * (WVec3Dot(s, h));

		if (u < 0.0 || u > 1.0)
			continue; //no intersection

		q = WVec3Cross(s, e1);
		v = f * WVec3Dot(p2, q);
		if (v < 0.0 || u + v > 1.0)
			continue; //no intersection
					  // at this stage we can compute t to find out where
					  // the intersection point is on the line
		float t = f * WVec3Dot(e2, q);
		if (t > 0.00001) // ray intersection
		{
			//add this triangle to the intersection vector
			IntersectionInfo ii;
			ii.index = i;
			ii.u = uv0.x * (1-u) + uv1.x * u;
			ii.v = uv0.y * (1-v) + uv2.y * v;
			ii.point = v0 + u*(v1 - v0) + v*(v2 - v0);

			intersection.push_back(ii);
			continue;
		} else // this means that there is a line intersection
			   // but not a ray intersection
			continue;
	}

	UnmapVertexBuffer();
	UnmapIndexBuffer();

	//no intersection
	if (!intersection.size())
		return false;

	//find the closest triangle to p1 and return it's info
	unsigned int chosen = 0;
	float ld = WVec3LengthSq(p1 - intersection[0].point);
	for (unsigned int i = 1; i < intersection.size(); i++) {
		float fDist = WVec3LengthSq(p1 - intersection[i].point);
		if (fDist < ld) {
			ld = fDist;
			chosen = i;
		}
	}

	if (uv) *uv = WVector2(intersection[chosen].u,  intersection[chosen].v);
	if (pt) *pt = intersection[chosen].point;
	if (triangleIndex) *triangleIndex = intersection[chosen].index;

	return true;
}

WError WGeometry::Draw(WRenderTarget* rt, unsigned int num_indices, unsigned int num_instances, bool bind_animation) {
	VkCommandBuffer renderCmdBuffer = rt->GetCommnadBuffer();
	if (!renderCmdBuffer)
		return WError(W_NORENDERTARGET);

	// Bind triangle vertices
	VkDeviceSize offsets[] = { 0, 0 };
	uint bufferIndex = m_app->Renderer->GetCurrentBufferingIndex();
	VkBuffer bindings[] = { m_vertices.GetBuffer(m_app, bufferIndex), VK_NULL_HANDLE };
	if (m_animationbuf.Valid())
		bindings[1] = m_animationbuf.GetBuffer(m_app, bufferIndex);
	else
		bindings[1] = bindings[0];
	vkCmdBindVertexBuffers(renderCmdBuffer, 0, bind_animation ? 2 : 1, bindings, offsets);

	if (m_indices.Valid()) {
		if (num_indices == -1 || num_indices > m_numIndices)
			num_indices = m_numIndices;
		// Bind triangle indices & draw the indexed triangle
		vkCmdBindIndexBuffer(renderCmdBuffer, m_indices.GetBuffer(m_app, bufferIndex), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(renderCmdBuffer, num_indices, num_instances, 0, 0, 0);
	} else {
		if (num_indices == -1 || num_indices > m_numVertices)
			num_indices = m_numVertices;
		// render the vertices without indices
		vkCmdDraw(renderCmdBuffer, num_indices, num_instances, 0, 0);
	}


	return WError(W_SUCCEEDED);
}

WVector3 WGeometry::GetMaxPoint() const {
	return m_maxPt;
}

WVector3 WGeometry::GetMinPoint() const {
	return m_minPt;
}

unsigned int WGeometry::GetNumVertices() const {
	return m_numVertices;
}

unsigned int WGeometry::GetNumIndices() const {
	return m_numIndices;
}

bool WGeometry::IsRigged() const {
	return m_animationbuf.Valid();
}

WError WGeometry::SaveToStream(WFile* file, std::ostream& outputStream) {
	if (!Valid())
		return WError(W_NOTVALID);

	void *vb, *ib, *ab = nullptr;
	WError ret = MapVertexBuffer(&vb, W_MAP_READ);
	if (!ret)
		return ret;

	ret = MapIndexBuffer((uint**)&ib, W_MAP_READ);
	if (!ret) {
		UnmapVertexBuffer();
		return ret;
	}

	if (m_animationbuf.Valid()) {
		ret = MapAnimationBuffer(&ab, W_MAP_READ);
		if (!ret) {
			UnmapVertexBuffer();
			UnmapIndexBuffer();
			return ret;
		}
	}

	unsigned int num_vbs = m_animationbuf.Valid() ? 2 : 1;
	outputStream.write((char*)&num_vbs, sizeof(unsigned int));
	for (int d = 0; d < num_vbs; d++) {
		W_VERTEX_DESCRIPTION my_desc = GetVertexDescription(d);
		unsigned int num_attributes = my_desc.attributes.size();
		outputStream.write((char*)&num_attributes, sizeof(unsigned int));
		for (int i = 0; i < num_attributes; i++) {
			outputStream.write((char*)&my_desc.attributes[i].num_components, sizeof(unsigned char));
			unsigned int namesize = my_desc.attributes[i].name.length();
			outputStream.write((char*)&namesize, sizeof(unsigned int));
			outputStream.write(my_desc.attributes[i].name.c_str(), namesize);
		}
	}

	outputStream.write((char*)&m_numVertices, sizeof(unsigned int));
	outputStream.write((char*)&m_numIndices, sizeof(unsigned int));
	outputStream.write((char*)vb, m_vertices.GetMemorySize());
	outputStream.write((char*)ib, m_indices.GetMemorySize());
	if (ab && num_vbs > 1) {
		outputStream.write((char*)ab, m_animationbuf.GetMemorySize());
	}

	UnmapVertexBuffer();
	UnmapIndexBuffer();
	if (m_animationbuf.Valid())
		UnmapAnimationBuffer();

	return WError(W_SUCCEEDED);
}

WError WGeometry::LoadFromStream(WFile* file, std::istream& inputStream) {
	bool bDynamic = false;
	vector<W_VERTEX_DESCRIPTION> from_descs;
	char temp[256];
	unsigned int num_vbs;
	inputStream.read((char*)&num_vbs, sizeof(unsigned int));
	if (num_vbs == 0)
		return WError(W_INVALIDFILEFORMAT);

	from_descs.resize(num_vbs);
	for (int d = 0; d < num_vbs; d++) {
		W_VERTEX_DESCRIPTION desc;
		unsigned int num_attributes;
		inputStream.read((char*)&num_attributes, sizeof(unsigned int));
		desc.attributes.resize(num_attributes);
		for (int i = 0; i < num_attributes; i++) {
			inputStream.read((char*)&desc.attributes[i].num_components, sizeof(unsigned char));
			unsigned int namesize = desc.attributes[i].name.length();
			if (namesize > 255)
				namesize = 255;
			inputStream.read((char*)&namesize, sizeof(unsigned int));
			inputStream.read(temp, namesize);
			temp[namesize] = '\0';
			desc.attributes[i].name = temp;
		}
		from_descs[d] = desc;
	}

	unsigned int numV, numI;
	inputStream.read((char*)&numV, sizeof(unsigned int));
	inputStream.read((char*)&numI, sizeof(unsigned int));

	void *vb, *ib;
	vb = W_SAFE_ALLOC(numV * from_descs[0].GetSize());
	ib = W_SAFE_ALLOC(numI * sizeof(uint));
	if (!ib || !vb) {
		W_SAFE_FREE(vb);
		W_SAFE_FREE(ib);
		return WError(W_OUTOFMEMORY);
	}

	inputStream.read((char*)vb, numV * from_descs[0].GetSize());
	inputStream.read((char*)ib, numI * sizeof(uint));

	WError ret;
	W_VERTEX_DESCRIPTION my_desc = GetVertexDescription(0);
	void* convertedVB;
	if (my_desc.isEqualTo(from_descs[0]))
		convertedVB = vb;
	else
		convertedVB = W_SAFE_ALLOC(numV * my_desc.GetSize());

	if (!convertedVB) {
		ret = WError(W_OUTOFMEMORY);
		W_SAFE_FREE(vb);
	} else {
		if (!my_desc.isEqualTo(from_descs[0])) {
			ConvertVertices(vb, convertedVB, numV, from_descs[0], my_desc);
			W_SAFE_FREE(vb);
		}

		bool bCalcTangents = false, bCalcNormals = false;
		if (my_desc.GetOffset("normal") >= 0 && from_descs[0].GetOffset("normal") == -1)
			bCalcNormals = true;
		if (my_desc.GetOffset("tangent") >= 0 && from_descs[0].GetOffset("tangent") == -1)
			bCalcTangents = true;

		ret = CreateFromData(convertedVB, numV, ib, numI, bDynamic, bCalcNormals, bCalcTangents);

		W_SAFE_FREE(convertedVB);
	}
	W_SAFE_FREE(ib);

	if (num_vbs > 1 && ret && GetVertexBufferCount() > 1 && from_descs[1].GetSize() == GetVertexDescription(1).GetSize()) {
		void *ab;
		ab = W_SAFE_ALLOC(numV * from_descs[1].GetSize());
		inputStream.read((char*)ab, numV * from_descs[1].GetSize());
		ret = CreateAnimationData(ab);
		W_SAFE_FREE(ab);
	}

	return ret;
}
