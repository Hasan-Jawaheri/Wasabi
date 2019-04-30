#include "WGeometry.h"
#include "../Renderers/WRenderer.h"
#include "../Images/WRenderTarget.h"

const W_VERTEX_DESCRIPTION g_defaultVertexDescriptions[] = {
	W_VERTEX_DESCRIPTION({ // Vertex buffer
		W_ATTRIBUTE_POSITION,
		W_ATTRIBUTE_TANGENT,
		W_ATTRIBUTE_NORMAL,
		W_ATTRIBUTE_UV,
		W_ATTRIBUTE_TEX_INDEX,
	}), W_VERTEX_DESCRIPTION({ // Animation buffer
		W_ATTRIBUTE_BONE_INDEX,
		W_ATTRIBUTE_BONE_WEIGHT,
	})
};

static void ConvertVertices(void* vbFrom, void* vbTo, unsigned int numVerts, W_VERTEX_DESCRIPTION vtxFrom, W_VERTEX_DESCRIPTION vtxTo) {
	size_t vtxSize = vtxTo.GetSize();
	size_t fromVtxSize = vtxFrom.GetSize();
	memset(vbTo, 0, numVerts * vtxTo.GetSize());
	for (int i = 0; i < numVerts; i++) {
		char* fromvtx = (char*)vbFrom + fromVtxSize * i;
		char* myvtx = (char*)vbTo + vtxSize * i;
		for (int j = 0; j < vtxTo.attributes.size(); j++) {
			std::string name = vtxTo.attributes[j].name;
			int off_in_from = vtxFrom.GetOffset(name);
			int index_in_from = vtxFrom.GetIndex(name);
			if (off_in_from >= 0 && vtxTo.attributes[j].numComponents == vtxFrom.attributes[index_in_from].numComponents)
				memcpy(myvtx + vtxTo.GetOffset(name), fromvtx + off_in_from, vtxTo.attributes[j].numComponents * 4);
		}
	}
}

size_t W_VERTEX_DESCRIPTION::GetSize() const {
	if (_size == -1) {
		_size = 0;
		for (int i = 0; i < attributes.size(); i++)
			_size += 4 * attributes[i].numComponents;
	}
	return _size;
}

size_t W_VERTEX_DESCRIPTION::GetOffset(unsigned int attribIndex) const {
	size_t s = 0;
	for (int i = 0; i < attributes.size(); i++) {
		if (i == attribIndex)
			return s;
		s += 4 * attributes[i].numComponents;
	}
	return -1;
}

size_t W_VERTEX_DESCRIPTION::GetOffset(std::string attribName) const {
	size_t s = 0;
	for (int i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == attribName)
			return s;
		s += 4 * attributes[i].numComponents;
	}
	return -1;
}

unsigned int W_VERTEX_DESCRIPTION::GetIndex(std::string attribName) const {
	for (int i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == attribName)
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
	// we need to perform this here because some destructed geometries will need access to m_dynamicGeometries
	// which will be destructed by the time WManager::~WManager() destroys the geometries this way
	for (unsigned int j = 0; j < W_HASHTABLESIZE; j++) {
		for (unsigned int i = 0; i < m_entities[j].size(); i)
			m_entities[j][i]->RemoveReference();
		m_entities[j].clear();
	}
}

void WGeometryManager::UpdateDynamicGeometries(uint bufferIndex) const {
	for (auto it = m_dynamicGeometries.begin(); it != m_dynamicGeometries.end(); it++) {
		it->first->_PerformPendingMaps(bufferIndex);
	}
}

WGeometry::WGeometry(Wasabi* const app, unsigned int ID) : WFileAsset(app, ID) {
	m_mappedVertexBufferForWrite = nullptr;
	app->GeometryManager->AddEntity(this);
}

WGeometry::~WGeometry() {
	_DestroyResources();

	m_app->GeometryManager->RemoveEntity(this);
}

std::string WGeometry::_GetTypeName() {
	return "Geometry";
}

std::string WGeometry::GetTypeName() const {
	return _GetTypeName();
}

bool WGeometry::Valid() const {
	return m_vertices.Valid();
}

W_VERTEX_DESCRIPTION WGeometry::GetVertexDescription(unsigned int layoutIndex) const {
	if (layoutIndex >= sizeof(g_defaultVertexDescriptions)/sizeof(W_VERTEX_DESCRIPTION))
		layoutIndex = 0;
	return g_defaultVertexDescriptions[layoutIndex];
}

size_t WGeometry::GetVertexDescriptionSize(unsigned int layoutIndex) const {
	if (layoutIndex >= sizeof(g_defaultVertexDescriptions) / sizeof(W_VERTEX_DESCRIPTION))
		layoutIndex = 0;
	return g_defaultVertexDescriptions[layoutIndex].GetSize();
}

void WGeometry::_DestroyResources() {
	for (auto it = m_pendingBufferedMaps.begin(); it != m_pendingBufferedMaps.end(); it++) {
		WBufferedBuffer* buffer = it->first;
		for (auto bufIt = it->second.begin(); bufIt != it->second.end(); bufIt++) {
			if (*bufIt != nullptr) {
				W_SAFE_FREE(*bufIt);
				break;
			}
		}
		it->second.clear();
	}
	m_pendingBufferedMaps.clear();
	auto it = m_app->GeometryManager->m_dynamicGeometries.find(this);
	if (it != m_app->GeometryManager->m_dynamicGeometries.end())
		m_app->GeometryManager->m_dynamicGeometries.erase(it);

	m_vertices.Destroy(m_app);
	m_indices.Destroy(m_app);
	m_animationbuf.Destroy(m_app);
}

void WGeometry::_CalcMinMax(void* vb, unsigned int numVerts) {
	m_minPt = WVector3(FLT_MAX, FLT_MAX, FLT_MAX);
	m_maxPt = WVector3(FLT_MIN, FLT_MIN, FLT_MIN);

	int offset = GetVertexDescription(0).GetOffset("position");
	int vsize = GetVertexDescription(0).GetSize();

	if (offset == -1)
		return; // no position attribute

	for (int i = 0; i < numVerts; i++) {
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

void WGeometry::_CalcNormals(void* vb, unsigned int numVerts, void* ib, unsigned int numIndices) {
	if (numIndices % 3 != 0)
		return; // must be a triangle list

	int offset = GetVertexDescription(0).GetOffset("position");
	int normalOffset = GetVertexDescription(0).GetOffset("normal");
	int vsize = GetVertexDescription(0).GetSize();

	if (offset == -1 || normalOffset == -1)
		return; // no position/normal attributes

	int numTris = numIndices / 3;
	for (int i = 0; i < numTris; i++) {
		uint ind[3];
		memcpy(ind, (char*)ib + (i*3) * sizeof(uint), 3 * sizeof(uint));
		WVector3 p[3];
		memcpy(&p[0], (char*)vb + vsize * (ind[0]) + offset, sizeof(WVector3));
		memcpy(&p[1], (char*)vb + vsize * (ind[1]) + offset, sizeof(WVector3));
		memcpy(&p[2], (char*)vb + vsize * (ind[2]) + offset, sizeof(WVector3));
		WVector3 norm = WVec3Normalize(WVec3Cross(p[1] - p[0], p[2] - p[0]));
		memcpy((char*)vb + vsize * (ind[0]) + normalOffset, &norm, sizeof(WVector3));
		memcpy((char*)vb + vsize * (ind[1]) + normalOffset, &norm, sizeof(WVector3));
		memcpy((char*)vb + vsize * (ind[2]) + normalOffset, &norm, sizeof(WVector3));
	}
}

void WGeometry::_CalcTangents(void* vb, unsigned int numVerts) {
	int norm_offset = GetVertexDescription(0).GetOffset("normal");
	int tang_offset = GetVertexDescription(0).GetOffset("tangent");
	int vsize = GetVertexDescription(0).GetSize();

	if (norm_offset == -1 || tang_offset == -1)
		return; // no position attribute

	for (unsigned int i = 0; i < numVerts; i++) {
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

WError WGeometry::CreateFromData(void* vb, unsigned int numVerts, void* ib, unsigned int numIndices, W_GEOMETRY_CREATE_FLAGS flags) {
	if (numVerts <= 0 || (numIndices > 0 && !ib))
		return WError(W_INVALIDPARAM);

	size_t vertexBufferSize = numVerts * GetVertexDescription(0).GetSize();
	size_t indexBufferSize = numIndices * sizeof(uint);

	_DestroyResources();

	if ((flags & W_GEOMETRY_CREATE_CALCULATE_NORMALS) && vb && ib && GetVertexDescription(0).GetIndex("normal") >= 0)
		_CalcNormals(vb, numVerts, ib, numIndices);
	if ((flags & W_GEOMETRY_CREATE_CALCULATE_TANGENTS) && vb && GetVertexDescription(0).GetIndex("tangent") >= 0)
		_CalcTangents(vb, numVerts);


	uint numBuffersVB = (flags & W_GEOMETRY_CREATE_VB_DYNAMIC) ? (uint)m_app->engineParams["bufferingCount"] : 1;
	uint numBuffersIB = (flags & W_GEOMETRY_CREATE_IB_DYNAMIC) ? (uint)m_app->engineParams["bufferingCount"] : 1;

	W_MEMORY_STORAGE memory = (flags & W_GEOMETRY_CREATE_VB_DYNAMIC) ? W_MEMORY_HOST_VISIBLE : W_MEMORY_DEVICE_LOCAL_HOST_COPY;
	VkResult result = m_vertices.Create(m_app, numBuffersVB, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vb, memory);
	if (result == VK_SUCCESS && indexBufferSize > 0) {
		memory = (flags & W_GEOMETRY_CREATE_AB_DYNAMIC) ? W_MEMORY_HOST_VISIBLE : W_MEMORY_DEVICE_LOCAL_HOST_COPY;
		result = m_indices.Create(m_app, numBuffersIB, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, ib, memory);
	}

	if (result != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	if ((flags & W_GEOMETRY_CREATE_IB_DYNAMIC && !(flags & W_GEOMETRY_CREATE_IB_REWRITE_EVERY_FRAME)) ||
		(flags & W_GEOMETRY_CREATE_VB_DYNAMIC && !(flags & W_GEOMETRY_CREATE_VB_REWRITE_EVERY_FRAME))) {
		m_app->GeometryManager->m_dynamicGeometries.insert(std::make_pair(this, true));
		if (flags & W_GEOMETRY_CREATE_VB_DYNAMIC && !(flags & W_GEOMETRY_CREATE_VB_REWRITE_EVERY_FRAME)) {
			m_pendingBufferedMaps.insert(std::make_pair(&m_vertices, std::vector<void*>(numBuffersVB)));
			memset(m_pendingBufferedMaps[&m_vertices].data(), 0, sizeof(void*) * numBuffersVB);
		}
		if (flags & W_GEOMETRY_CREATE_IB_DYNAMIC && !(flags & W_GEOMETRY_CREATE_IB_REWRITE_EVERY_FRAME)) {
			m_pendingBufferedMaps.insert(std::make_pair(&m_indices, std::vector<void*>(numBuffersIB)));
			memset(m_pendingBufferedMaps[&m_indices].data(), 0, sizeof(void*) * numBuffersIB);
		}
	}

	m_numVertices = numVerts;
	m_numIndices = numIndices;
	if (vb)
		_CalcMinMax(vb, numVerts);

	return WError(W_SUCCEEDED);
}

WError WGeometry::CreateFromDefaultVerticesData(vector<WDefaultVertex>& default_vertices, vector<uint>& indices, W_GEOMETRY_CREATE_FLAGS flags) {
	return CreateFromData(default_vertices.data(), default_vertices.size(), indices.data(), indices.size(), flags);
}

WError WGeometry::CreateAnimationData(void* ab, W_GEOMETRY_CREATE_FLAGS flags) {
	if (!Valid() || GetVertexBufferCount() < 2)
		return WError(W_NOTVALID);
	if (!ab)
		return WError(W_INVALIDPARAM);

	int animBufferSize = m_numVertices * GetVertexDescription(1).GetSize();
	uint numBuffers = (flags & W_GEOMETRY_CREATE_AB_DYNAMIC) ? (uint)m_app->engineParams["bufferingCount"] : 1;
	W_MEMORY_STORAGE memory = (flags & W_GEOMETRY_CREATE_AB_DYNAMIC) ? W_MEMORY_HOST_VISIBLE : W_MEMORY_DEVICE_LOCAL_HOST_COPY;
	VkResult result = m_animationbuf.Create(m_app, numBuffers, animBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, ab, memory);
	if (result != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	if (flags & W_GEOMETRY_CREATE_AB_DYNAMIC && !(flags & W_GEOMETRY_CREATE_AB_REWRITE_EVERY_FRAME)) {
		auto it = m_app->GeometryManager->m_dynamicGeometries.find(this);
		if (it == m_app->GeometryManager->m_dynamicGeometries.end())
			m_app->GeometryManager->m_dynamicGeometries.insert(std::make_pair(this, true));
		m_pendingBufferedMaps.insert(std::make_pair(&m_animationbuf, std::vector<void*>(numBuffers)));
		memset(m_pendingBufferedMaps[&m_animationbuf].data(), 0, sizeof(void*) * numBuffers);
	}

	return WError(W_SUCCEEDED);
}

WError WGeometry::CreateCube(float size, W_GEOMETRY_CREATE_FLAGS flags) {
	return CreateBox(WVector3(size, size, size), flags);
}

WError WGeometry::CreateBox(WVector3 dimensions, W_GEOMETRY_CREATE_FLAGS flags) {
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

	return CreateFromDefaultVerticesData(vertices, indices, flags);
}

WError WGeometry::CreatePlain(float size, int xsegs, int zsegs, W_GEOMETRY_CREATE_FLAGS flags) {
	unsigned int numIndices = (((xsegs + 1)*(zsegs + 1)) * 2) * 3;
	unsigned int numVertices = (xsegs + 2)*(zsegs + 2);

	//allocate the plain vertices
	vector<WDefaultVertex> vertices(numVertices);

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
			vertices[curVert].pos *= size;
			_u += uI; //u starts from 0 increasing to 1
			curVert++;
		}
		_v += vI; //v starts from 0 increasing to 1
		_u = 0;
	}


	//allocate the index buffer
	vector<uint> indices(numIndices);

	//fill the indices data
	int numt = 0;
	int offset = 0;
	for (int t = 0; t < (numIndices / 3) / 2; t++) {
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

	return CreateFromDefaultVerticesData(vertices, indices, flags);
}

WError WGeometry::CreateSphere(float Radius, unsigned int VRes, unsigned int URes, W_GEOMETRY_CREATE_FLAGS flags) {
	if (VRes < 3 || URes < 2)
		return WError(W_INVALIDPARAM);
	const uint NumVertexRings = VRes - 2;
	unsigned int numVertices = NumVertexRings * URes + 2;
	const uint NumTriangleRings = VRes - 1;
	const uint NumTriangles = (NumTriangleRings + 1) * URes * 2;
	unsigned int numIndices = NumTriangles * 3;

	// Calculate all of the vertex positions
	vector<WDefaultVertex> vertices (numVertices);
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
	vector<uint> indices(numIndices);
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
	const uint bottom = numVertices - 1;
	for (uint u = 0; u < URes; ++u) {
		const uint currentU = u;
		const uint nextU = (u + 1) % URes;
		const uint currTop = top + currentU;
		const uint nextTop = top + nextU;

		indices[curIndex++] = currTop;
		indices[curIndex++] = bottom;
		indices[curIndex++] = nextTop;
	}

	return CreateFromDefaultVerticesData(vertices, indices, flags);
}

WError WGeometry::CreateCone(float fRadius, float fHeight, unsigned int hsegs, unsigned int csegs, W_GEOMETRY_CREATE_FLAGS flags) {
	hsegs += 2;
	if (csegs < 3 || hsegs < 2)
		return WError(W_INVALIDPARAM);
	//3 indices * number of triangles (top and bottom triangles + side triangles)
	unsigned int numIndices = 3 * (csegs + (hsegs - 1)*csegs * 2);
	//bottom with the circle vertices plus [csegs] vertices for every [hsegs] + one extra seg for uvs
	unsigned int numVertices = 1 + csegs + hsegs * (csegs + 1);

	//allocate vertices
	vector<WDefaultVertex> vertices (numVertices);

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
	for (uint i = 0; i < numVertices; i++) {
		vertices[i].pos.y -= 0.5f;
		vertices[i].pos.x *= fRadius;
		vertices[i].pos.y *= fHeight;
		vertices[i].pos.z *= fRadius;
	}

	//allocate uints for the indices
	vector<uint> indices(numIndices);
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
		indices[pos++] = numVertices - 1;
		indices[pos++] = numVertices - 1 - csegs + i + 1;
		indices[pos++] = numVertices - 1 - csegs + i;
		if (indices[pos - 2] == numVertices - 1)
			indices[pos - 2] = numVertices - 1 - csegs;
	}

	return CreateFromDefaultVerticesData(vertices, indices, flags);
}

WError WGeometry::CreateCylinder(float fRadius, float fHeight, unsigned int hsegs, unsigned int csegs, W_GEOMETRY_CREATE_FLAGS flags) {
	hsegs -= 2;
	if (csegs < 3 || hsegs < 2)
		return WError(W_INVALIDPARAM);
	//3 indices * number of triangles (top and bottom triangles + side triangles)
	unsigned int numIndices = 3 * (csegs * 2 + (hsegs - 1)*csegs * 2);
	//top and bottom with their circles vertices plus [csegs] vertices for every [hsegs] + one extra seg for uvs
	unsigned int numVertices = 2 + csegs * 2 + hsegs * (csegs + 1);

	//allocate vertices
	vector<WDefaultVertex> vertices (numVertices);

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
	for (uint i = 0; i < numVertices; i++) {
		vertices[i].pos.y -= 0.5f;
		vertices[i].pos.x *= fRadius;
		vertices[i].pos.y *= fHeight;
		vertices[i].pos.z *= fRadius;
	}

	//allocate uints for the indices
	vector<uint> indices(numIndices);
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
		indices[pos++] = numVertices - 1;
		indices[pos++] = numVertices - 1 - csegs + i + 1;
		indices[pos++] = numVertices - 1 - csegs + i;
		if (indices[pos - 2] == numVertices - 1)
			indices[pos - 2] = numVertices - 1 - csegs;
	}

	return CreateFromDefaultVerticesData(vertices, indices, flags);
}

WError WGeometry::CopyFrom(WGeometry* const from, W_GEOMETRY_CREATE_FLAGS flags) {
	if (!from->Valid())
		return WError(W_INVALIDPARAM);

	unsigned int numVerts = from->GetNumVertices();
	unsigned int numIndices = from->GetNumIndices();
	W_VERTEX_DESCRIPTION my_desc = GetVertexDescription(0);
	W_VERTEX_DESCRIPTION from_desc = from->GetVertexDescription(0);
	void *vb, *fromvb, *fromib;

	if (!my_desc.isEqualTo(from_desc)) {
		vb = W_SAFE_ALLOC(numVerts *my_desc.GetSize());
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
		ConvertVertices(fromvb, vb, numVerts, from_desc, my_desc);

	if (my_desc.GetOffset("normal") >= 0 && from_desc.GetOffset("normal") == -1)
		flags |= W_GEOMETRY_CREATE_CALCULATE_NORMALS;
	if (my_desc.GetOffset("tangent") >= 0 && from_desc.GetOffset("tangent") == -1)
		flags |= W_GEOMETRY_CREATE_CALCULATE_TANGENTS;

	from->UnmapVertexBuffer();
	ret = CreateFromData(vb, numVerts, fromib, numIndices, flags);
	from->UnmapIndexBuffer();
	if (!my_desc.isEqualTo(from_desc))
		W_SAFE_FREE(vb);

	if (ret && from->m_animationbuf.Valid()) {
		void* fromab;
		ret = from->MapAnimationBuffer(&fromab, W_MAP_READ);
		if (ret) {
			ret = CreateAnimationData(fromab, flags);
			from->UnmapAnimationBuffer();
		}
	}

	return ret;
}

WError WGeometry::LoadFromHXM(std::string filename, W_GEOMETRY_CREATE_FLAGS flags) {
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

	WError ret = CreateFromData(newverts, numV, ind, numI, flags);

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

void WGeometry::_UpdatePendingMap(WBufferedBuffer* buffer, void* mappedData, uint bufferIndex, W_MAP_FLAGS mapFlags) {
	auto it = m_pendingBufferedMaps.find(buffer);
	if (it != m_pendingBufferedMaps.end()) {
		if (mapFlags & W_MAP_READ && it->second[bufferIndex]) {
			// the user intends to read and there is a pending write to this buffer, perform the write
			memcpy(mappedData, it->second[bufferIndex], buffer->GetMemorySize());
		}

		// delete all old buffered mapping
		bool bDeleted = false;
		for (auto dataIt = it->second.begin(); dataIt != it->second.end(); dataIt++) {
			if (*dataIt != nullptr && !bDeleted) {
				W_SAFE_FREE(*dataIt);
				bDeleted = true;
			} else if (bDeleted)
				*dataIt = nullptr;
		}
		it->second[bufferIndex] = mappedData;
	}
}

void WGeometry::_UpdatePendingUnmap(WBufferedBuffer* buffer, uint bufferIndex) {
	auto it = m_pendingBufferedMaps.find(buffer);
	if (it != m_pendingBufferedMaps.end()) {
		void* bufferedMaps = W_SAFE_ALLOC(buffer->GetMemorySize());
		memcpy(bufferedMaps, it->second[bufferIndex], buffer->GetMemorySize());
		for (uint i = 0; i < it->second.size(); i++) {
			if (i == bufferIndex)
				it->second[i] = nullptr;
			else
				it->second[i] = bufferedMaps;
		}
	}
}

void WGeometry::_PerformPendingMaps(uint bufferIndex) {
	for (auto buf = m_pendingBufferedMaps.begin(); buf != m_pendingBufferedMaps.end(); buf++) {
		WBufferedBuffer* buffer = buf->first;
		if (buf->second.size() > 0 && buf->second[bufferIndex]) {
			void* data = buf->second[bufferIndex];
			void* pMappedData;
			if (buffer->Map(m_app, bufferIndex, &pMappedData, W_MAP_WRITE) == VK_SUCCESS) {
				memcpy(pMappedData, data, buffer->GetMemorySize());
				buffer->Unmap(m_app, bufferIndex);
				buf->second[bufferIndex] = nullptr;
				uint numRemainingPointers = 0;
				for (auto bufIt = buf->second.begin(); bufIt != buf->second.end(); bufIt++)
					numRemainingPointers += (*bufIt == nullptr) ? 0 : 1;
				if (numRemainingPointers == 0)
					W_SAFE_FREE(data); // last buffer to erase -> free the memory
			}
		}
	}
}

WError WGeometry::MapVertexBuffer(void** const vb, W_MAP_FLAGS mapFlags) {
	uint bufferIndex = m_app->GetCurrentBufferingIndex();
	VkResult result = m_vertices.Map(m_app, bufferIndex, vb, mapFlags);
	if (result != VK_SUCCESS)
		return WError(W_NOTVALID);

	if (mapFlags & W_MAP_WRITE)
		m_mappedVertexBufferForWrite = *vb;

	_UpdatePendingMap(&m_vertices, *vb, bufferIndex, mapFlags);

	return WError(W_SUCCEEDED);
}

WError WGeometry::MapIndexBuffer(uint** const ib, W_MAP_FLAGS mapFlags) {
	uint bufferIndex = m_app->GetCurrentBufferingIndex();
	VkResult result = m_indices.Map(m_app, bufferIndex, (void**)ib, mapFlags);
	if (result != VK_SUCCESS)
		return WError(W_NOTVALID);

	_UpdatePendingMap(&m_indices, *ib, bufferIndex, mapFlags);

	return WError(W_SUCCEEDED);
}

WError WGeometry::MapAnimationBuffer(void** const ab, W_MAP_FLAGS mapFlags) {
	uint bufferIndex = m_app->GetCurrentBufferingIndex();
	VkResult result = m_animationbuf.Map(m_app, bufferIndex, ab, mapFlags);
	if (result != VK_SUCCESS)
		return WError(W_NOTVALID);

	_UpdatePendingMap(&m_animationbuf, *ab, bufferIndex, mapFlags);

	return WError(W_SUCCEEDED);
}

void WGeometry::UnmapVertexBuffer(bool recalculateBoundingBox) {
	if (m_mappedVertexBufferForWrite) {
		if (recalculateBoundingBox)
			_CalcMinMax(m_mappedVertexBufferForWrite, m_numVertices);
		m_mappedVertexBufferForWrite = nullptr;
	}

	uint bufferIndex = m_app->GetCurrentBufferingIndex();
	_UpdatePendingUnmap(&m_vertices, bufferIndex);
	m_vertices.Unmap(m_app, bufferIndex);
}

void WGeometry::UnmapIndexBuffer() {
	uint bufferIndex = m_app->GetCurrentBufferingIndex();
	_UpdatePendingUnmap(&m_indices, bufferIndex);
	m_indices.Unmap(m_app, bufferIndex);
}

void WGeometry::UnmapAnimationBuffer() {
	uint bufferIndex = m_app->GetCurrentBufferingIndex();
	_UpdatePendingUnmap(&m_animationbuf, bufferIndex);
	m_animationbuf.Unmap(m_app, bufferIndex);
}

WError WGeometry::Scale(float mulFactor) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtxSize = GetVertexDescription(0).GetSize();
	int offset = GetVertexDescription(0).GetOffset("position");
	if (offset == -1)
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (int i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtxSize * i + offset, size);
		v *= mulFactor;
		memcpy((char*)data + vtxSize * i + offset, &v, size);
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ScaleX(float mulFactor) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtxSize = GetVertexDescription(0).GetSize();
	int offset = GetVertexDescription(0).GetOffset("position");
	if (offset == -1)
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (int i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtxSize * i + offset, size);
		v.x *= mulFactor;
		memcpy((char*)data + vtxSize * i + offset, &v, size);
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ScaleY(float mulFactor) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtxSize = GetVertexDescription(0).GetSize();
	int offset = GetVertexDescription(0).GetOffset("position");
	if (offset == -1)
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (int i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtxSize * i + offset, size);
		v.y *= mulFactor;
		memcpy((char*)data + vtxSize * i + offset, &v, size);
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ScaleZ(float mulFactor) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtxSize = GetVertexDescription(0).GetSize();
	int offset = GetVertexDescription(0).GetOffset("position");
	if (offset == -1)
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (int i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtxSize * i + offset, size);
		v.z *= mulFactor;
		memcpy((char*)data + vtxSize * i + offset, &v, size);
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
	size_t vtxSize = GetVertexDescription(0).GetSize();
	int offset = GetVertexDescription(0).GetOffset("position");
	if (offset == -1)
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (int i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtxSize * i + offset, size);
		v += _offset;
		memcpy((char*)data + vtxSize * i + offset, &v, size);
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ApplyTransformation(WMatrix mtx) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtxSize = GetVertexDescription(0).GetSize();
	int offset = GetVertexDescription(0).GetOffset("position");
	if (offset == -1)
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (int i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtxSize * i + offset, size);
		v = WVec3TransformCoord(v, mtx);
		memcpy((char*)data + vtxSize * i + offset, &v, size);
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
	unsigned int vtxSize = GetVertexDescription(0).GetSize();
	unsigned int uv_offset = GetVertexDescription(0).GetOffset("uv");
	unsigned int uv_size = -1;

	if (pos_offset == -1)
		return false;
	if (GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents < 3)
		return false;
	if (uv_offset != -1)
		uv_size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("uv")].numComponents * 4;
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
		memcpy(&v0, &((char*)vb)[ib[i * 3 + 0] * vtxSize + pos_offset], sizeof(WVector3));
		memcpy(&v1, &((char*)vb)[ib[i * 3 + 1] * vtxSize + pos_offset], sizeof(WVector3));
		memcpy(&v2, &((char*)vb)[ib[i * 3 + 2] * vtxSize + pos_offset], sizeof(WVector3));
		if (uv_offset != -1) {
			memcpy(&uv0, &((char*)vb)[ib[i * 3 + 0] * vtxSize + uv_offset], sizeof(WVector2));
			memcpy(&uv1, &((char*)vb)[ib[i * 3 + 1] * vtxSize + uv_offset], sizeof(WVector2));
			memcpy(&uv2, &((char*)vb)[ib[i * 3 + 2] * vtxSize + uv_offset], sizeof(WVector2));
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

WError WGeometry::Draw(WRenderTarget* rt, unsigned int numIndices, unsigned int numInstances, bool bind_animation) {
	VkCommandBuffer renderCmdBuffer = rt->GetCommnadBuffer();
	if (!renderCmdBuffer)
		return WError(W_NORENDERTARGET);

	// Bind triangle vertices
	VkDeviceSize offsets[] = { 0, 0 };
	uint bufferIndex = m_app->GetCurrentBufferingIndex();
	VkBuffer bindings[] = { m_vertices.GetBuffer(m_app, bufferIndex), VK_NULL_HANDLE };
	if (m_animationbuf.Valid())
		bindings[1] = m_animationbuf.GetBuffer(m_app, bufferIndex);
	else
		bindings[1] = bindings[0];
	vkCmdBindVertexBuffers(renderCmdBuffer, 0, bind_animation ? 2 : 1, bindings, offsets);

	if (m_indices.Valid()) {
		if (numIndices == -1 || numIndices > m_numIndices)
			numIndices = m_numIndices;
		// Bind triangle indices & draw the indexed triangle
		vkCmdBindIndexBuffer(renderCmdBuffer, m_indices.GetBuffer(m_app, bufferIndex), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(renderCmdBuffer, numIndices, numInstances, 0, 0, 0);
	} else {
		if (numIndices == -1 || numIndices > m_numVertices)
			numIndices = m_numVertices;
		// render the vertices without indices
		vkCmdDraw(renderCmdBuffer, numIndices, numInstances, 0, 0);
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

	unsigned int numVbs = m_animationbuf.Valid() ? 2 : 1;
	outputStream.write((char*)&numVbs, sizeof(unsigned int));
	for (int d = 0; d < numVbs; d++) {
		W_VERTEX_DESCRIPTION my_desc = GetVertexDescription(d);
		unsigned int numAttributes = my_desc.attributes.size();
		outputStream.write((char*)&numAttributes, sizeof(unsigned int));
		for (int i = 0; i < numAttributes; i++) {
			outputStream.write((char*)&my_desc.attributes[i].numComponents, sizeof(unsigned char));
			unsigned int namesize = my_desc.attributes[i].name.length();
			outputStream.write((char*)&namesize, sizeof(unsigned int));
			outputStream.write(my_desc.attributes[i].name.c_str(), namesize);
		}
	}

	outputStream.write((char*)&m_numVertices, sizeof(unsigned int));
	outputStream.write((char*)&m_numIndices, sizeof(unsigned int));
	outputStream.write((char*)vb, m_vertices.GetMemorySize());
	outputStream.write((char*)ib, m_indices.GetMemorySize());
	if (ab && numVbs > 1) {
		outputStream.write((char*)ab, m_animationbuf.GetMemorySize());
	}

	UnmapVertexBuffer();
	UnmapIndexBuffer();
	if (m_animationbuf.Valid())
		UnmapAnimationBuffer();

	return WError(W_SUCCEEDED);
}

std::vector<void*> WGeometry::LoadArgs(W_GEOMETRY_CREATE_FLAGS flags) {
	return std::vector<void*>({
		(void*)flags
	});
}

WError WGeometry::LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args) {
	if (args.size() != 1)
		return WError(W_INVALIDPARAM);
	W_GEOMETRY_CREATE_FLAGS flags = (W_GEOMETRY_CREATE_FLAGS)(int)(args[0]);

	vector<W_VERTEX_DESCRIPTION> from_descs;
	char temp[256];
	unsigned int numVbs;
	inputStream.read((char*)&numVbs, sizeof(unsigned int));
	if (numVbs == 0)
		return WError(W_INVALIDFILEFORMAT);

	from_descs.resize(numVbs);
	for (int d = 0; d < numVbs; d++) {
		W_VERTEX_DESCRIPTION desc;
		unsigned int numAttributes;
		inputStream.read((char*)&numAttributes, sizeof(unsigned int));
		desc.attributes.resize(numAttributes);
		for (int i = 0; i < numAttributes; i++) {
			inputStream.read((char*)&desc.attributes[i].numComponents, sizeof(unsigned char));
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

		ret = CreateFromData(convertedVB, numV, ib, numI, flags);

		W_SAFE_FREE(convertedVB);
	}
	W_SAFE_FREE(ib);

	if (numVbs > 1 && ret && GetVertexBufferCount() > 1 && from_descs[1].GetSize() == GetVertexDescription(1).GetSize()) {
		void *ab;
		ab = W_SAFE_ALLOC(numV * from_descs[1].GetSize());
		inputStream.read((char*)ab, numV * from_descs[1].GetSize());
		ret = CreateAnimationData(ab, flags);
		W_SAFE_FREE(ab);
	}

	return ret;
}
