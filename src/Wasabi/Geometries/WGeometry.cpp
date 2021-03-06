#include "Wasabi/Geometries/WGeometry.hpp"
#include "Wasabi/Renderers/WRenderer.hpp"
#include "Wasabi/Images/WRenderTarget.hpp"

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

static void ConvertVertices(void* vbFrom, void* vbTo, uint32_t numVerts, W_VERTEX_DESCRIPTION vtxFrom, W_VERTEX_DESCRIPTION vtxTo) {
	size_t vtxSize = vtxTo.GetSize();
	size_t fromVtxSize = vtxFrom.GetSize();
	memset(vbTo, 0, numVerts * vtxTo.GetSize());
	for (uint32_t i = 0; i < numVerts; i++) {
		char* fromvtx = (char*)vbFrom + fromVtxSize * i;
		char* myvtx = (char*)vbTo + vtxSize * i;
		for (uint32_t j = 0; j < vtxTo.attributes.size(); j++) {
			std::string name = vtxTo.attributes[j].name;
			size_t offInFrom = vtxFrom.GetOffset(name);
			uint32_t index_in_from = vtxFrom.GetIndex(name);
			if (offInFrom != std::numeric_limits<size_t>::max() && vtxTo.attributes[j].numComponents == vtxFrom.attributes[index_in_from].numComponents)
				memcpy(myvtx + vtxTo.GetOffset(name), fromvtx + offInFrom, vtxTo.attributes[j].numComponents * 4);
		}
	}
}

size_t W_VERTEX_DESCRIPTION::GetSize() const {
	if (_size == std::numeric_limits<size_t>::max()) {
		_size = 0;
		for (uint32_t i = 0; i < attributes.size(); i++)
			_size += 4 * attributes[i].numComponents;
	}
	return _size;
}

size_t W_VERTEX_DESCRIPTION::GetOffset(uint32_t attribIndex) const {
	size_t s = 0;
	for (uint32_t i = 0; i < attributes.size(); i++) {
		if (i == attribIndex)
			return s;
		s += 4 * attributes[i].numComponents;
	}
	return std::numeric_limits<size_t>::max();
}

size_t W_VERTEX_DESCRIPTION::GetOffset(std::string attribName) const {
	size_t s = 0;
	for (uint32_t i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == attribName)
			return s;
		s += 4 * attributes[i].numComponents;
	}
	return std::numeric_limits<size_t>::max();
}

uint32_t W_VERTEX_DESCRIPTION::GetIndex(std::string attribName) const {
	for (uint32_t i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == attribName)
			return i;
	}
	return std::numeric_limits<uint32_t>::max();
}

bool W_VERTEX_DESCRIPTION::isEqualTo(W_VERTEX_DESCRIPTION other) const {
	if (attributes.size() != other.attributes.size())
		return false;
	for (uint32_t i = 0; i < attributes.size(); i++)
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
	for (uint32_t j = 0; j < W_HASHTABLESIZE; j++) {
		for (uint32_t i = 0; i < m_entities[j].size();)
			m_entities[j][i]->RemoveReference();
		m_entities[j].clear();
	}
}

void WGeometryManager::UpdateDynamicGeometries(uint32_t bufferIndex) const {
	for (auto it = m_dynamicGeometries.begin(); it != m_dynamicGeometries.end(); it++) {
		it->first->_PerformPendingMaps(bufferIndex);
	}
}

WGeometry::WGeometry(Wasabi* const app, uint32_t ID) : WFileAsset(app, ID) {
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

void WGeometry::SetID(uint32_t newID) {
	m_app->GeometryManager->RemoveEntity(this);
	m_ID = newID;
	m_app->GeometryManager->AddEntity(this);
}

void WGeometry::SetName(std::string newName) {
	m_name = newName;
	m_app->GeometryManager->OnEntityNameChanged(this, newName);
}

bool WGeometry::Valid() const {
	return m_vertices.Valid();
}

W_VERTEX_DESCRIPTION WGeometry::GetVertexDescription(uint32_t layoutIndex) const {
	if (layoutIndex >= sizeof(g_defaultVertexDescriptions)/sizeof(W_VERTEX_DESCRIPTION))
		layoutIndex = 0;
	return g_defaultVertexDescriptions[layoutIndex];
}

size_t WGeometry::GetVertexDescriptionSize(uint32_t layoutIndex) const {
	if (layoutIndex >= sizeof(g_defaultVertexDescriptions) / sizeof(W_VERTEX_DESCRIPTION))
		layoutIndex = 0;
	return g_defaultVertexDescriptions[layoutIndex].GetSize();
}

void WGeometry::_DestroyResources() {
	for (auto it = m_pendingBufferedMaps.begin(); it != m_pendingBufferedMaps.end(); it++) {
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

void WGeometry::_CalcMinMax(void* vb, uint32_t numVerts) {
	m_minPt = WVector3(FLT_MAX, FLT_MAX, FLT_MAX);
	m_maxPt = WVector3(FLT_MIN, FLT_MIN, FLT_MIN);

	size_t offset = GetVertexDescription(0).GetOffset("position");
	size_t vsize = GetVertexDescription(0).GetSize();

	if (offset == std::numeric_limits<size_t>::max())
		return; // no position attribute

	for (uint32_t i = 0; i < numVerts; i++) {
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

void WGeometry::_CalcNormals(void* vb, uint32_t numVerts, void* ib, uint32_t numIndices) {
	UNREFERENCED_PARAMETER(numVerts);

	if (numIndices % 3 != 0)
		return; // must be a triangle list

	size_t offset = GetVertexDescription(0).GetOffset("position");
	size_t normalOffset = GetVertexDescription(0).GetOffset("normal");
	size_t vsize = GetVertexDescription(0).GetSize();

	if (offset == std::numeric_limits<size_t>::max() || normalOffset == std::numeric_limits<size_t>::max())
		return; // no position/normal attributes

	uint32_t numTris = numIndices / 3;
	for (uint32_t i = 0; i < numTris; i++) {
		uint32_t ind[3];
		memcpy(ind, (char*)ib + (i*3) * sizeof(uint32_t), 3 * sizeof(uint32_t));
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

void WGeometry::_CalcTangents(void* vb, uint32_t numVerts) {
	size_t norm_offset = GetVertexDescription(0).GetOffset("normal");
	size_t tang_offset = GetVertexDescription(0).GetOffset("tangent");
	size_t vsize = GetVertexDescription(0).GetSize();

	if (norm_offset == std::numeric_limits<size_t>::max() || tang_offset == std::numeric_limits<size_t>::max())
		return; // no position attribute

	for (uint32_t i = 0; i < numVerts; i++) {
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

WError WGeometry::CreateFromData(void* vb, uint32_t numVerts, void* ib, uint32_t numIndices, W_GEOMETRY_CREATE_FLAGS flags) {
	if (numVerts <= 0 || (numIndices > 0 && !ib))
		return WError(W_INVALIDPARAM);

	size_t vertexBufferSize = numVerts * GetVertexDescription(0).GetSize();
	size_t indexBufferSize = numIndices * sizeof(uint32_t);

	_DestroyResources();

	if ((flags & W_GEOMETRY_CREATE_CALCULATE_NORMALS) && vb && ib && GetVertexDescription(0).GetIndex("normal") >= 0)
		_CalcNormals(vb, numVerts, ib, numIndices);
	if ((flags & W_GEOMETRY_CREATE_CALCULATE_TANGENTS) && vb && GetVertexDescription(0).GetIndex("tangent") >= 0)
		_CalcTangents(vb, numVerts);


	uint32_t numBuffersVB = (flags & W_GEOMETRY_CREATE_VB_DYNAMIC) ? m_app->GetEngineParam<uint32_t>("bufferingCount") : 1;
	uint32_t numBuffersIB = (flags & W_GEOMETRY_CREATE_IB_DYNAMIC) ? m_app->GetEngineParam<uint32_t>("bufferingCount") : 1;

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

WError WGeometry::CreateFromDefaultVerticesData(vector<WDefaultVertex>& default_vertices, vector<uint32_t>& indices, W_GEOMETRY_CREATE_FLAGS flags) {
	return CreateFromData(default_vertices.data(), (uint32_t)default_vertices.size(), indices.data(), (uint32_t)indices.size(), flags);
}

WError WGeometry::CreateAnimationData(void* ab, W_GEOMETRY_CREATE_FLAGS flags) {
	if (!Valid() || GetVertexBufferCount() < 2)
		return WError(W_NOTVALID);
	if (!ab)
		return WError(W_INVALIDPARAM);

	size_t animBufferSize = m_numVertices * GetVertexDescription(1).GetSize();
	uint32_t numBuffers = (flags & W_GEOMETRY_CREATE_AB_DYNAMIC) ? m_app->GetEngineParam<uint32_t>("bufferingCount") : 1;
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
	vector<uint32_t> indices(36);

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
	return CreateRectanglePlain(size, size, xsegs, zsegs, flags);
}

WError WGeometry::CreateRectanglePlain(float sizeX, float sizeZ, int xsegs, int zsegs, W_GEOMETRY_CREATE_FLAGS flags) {
	uint32_t numIndices = (((xsegs + 1) * (zsegs + 1)) * 2) * 3;
	uint32_t numVertices = (xsegs + 2) * (zsegs + 2);

	//allocate the plain vertices
	vector<WDefaultVertex> vertices(numVertices);

	uint32_t vpc = zsegs + 2, vpr = xsegs + 2;
	float uI = 1.0f / (float)(vpc - 1);
	float vI = 1.0f / (float)(vpr - 1);

	float _u = 0.0f, _v = 0.0f;
	uint32_t curVert = 0;

	//fill the plain vertex data
	for (uint32_t z = vpc - 1; z >= 0 && z != UINT_MAX; z--) {
		for (uint32_t x = 0; x < vpr; x++) {
			vertices[curVert] =
				WDefaultVertex((float)x / (vpr - 1), 0.0f, (float)z / (vpc - 1), 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, _u, _v);
			vertices[curVert].pos.x -= 0.5f;
			vertices[curVert].pos.z -= 0.5f;
			vertices[curVert].pos *= WVector3(sizeX, 1.0f, sizeZ);
			_u += uI; //u starts from 0 increasing to 1
			curVert++;
		}
		_v += vI; //v starts from 0 increasing to 1
		_u = 0;
	}


	//allocate the index buffer
	vector<uint32_t> indices(numIndices);

	//fill the indices data
	uint32_t numt = 0;
	uint32_t offset = 0;
	for (uint32_t t = 0; t < (numIndices / 3) / 2; t++) {
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

WError WGeometry::CreateSphere(float Radius, uint32_t VRes, uint32_t URes, W_GEOMETRY_CREATE_FLAGS flags) {
	if (VRes < 3 || URes < 2)
		return WError(W_INVALIDPARAM);
	const uint32_t NumVertexRings = VRes - 2;
	uint32_t numVertices = NumVertexRings * URes + 2;
	const uint32_t NumTriangleRings = VRes - 1;
	const uint32_t NumTriangles = (NumTriangleRings + 1) * URes * 2;
	uint32_t numIndices = NumTriangles * 3;

	// Calculate all of the vertex positions
	vector<WDefaultVertex> vertices (numVertices);
	int currVert = 0;

	// First vertex will be at the top pole
	vertices[currVert++] = WDefaultVertex(0.0f, Radius, 0, 1, 0, 0, 0, 1, 0, 0, 0);

	// Add in the vertical rings of vertices
	for (uint32_t v = 1; v <= NumVertexRings; ++v) {
		for (uint32_t u = 0; u < URes; ++u) {
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
	vector<uint32_t> indices(numIndices);
	uint32_t curIndex = 0;

	// Top ring first
	for (uint32_t u = 0; u < URes; ++u) {
		uint32_t nextU = (u + 1) % URes;
		indices[curIndex++] = 0;
		indices[curIndex++] = u + 1;
		indices[curIndex++] = nextU + 1;
	}

	// Now the middle rings
	for (uint32_t v = 1; v < VRes - 2; ++v) {
		uint32_t top = 1 + ((v - 1) * URes);
		uint32_t bottom = top + URes;
		for (uint32_t u = 0; u < URes; ++u) {
			uint32_t currentU = u;
			uint32_t nextU = (u + 1) % URes;
			uint32_t currTop = top + currentU;
			uint32_t nextTop = top + nextU;
			uint32_t currBottom = bottom + currentU;
			uint32_t nextBottom = bottom + nextU;

			indices[curIndex++] = currTop;
			indices[curIndex++] = currBottom;
			indices[curIndex++] = nextBottom;

			indices[curIndex++] = nextBottom;
			indices[curIndex++] = nextTop;
			indices[curIndex++] = currTop;
		}
	}

	// Now the bottom ring
	const uint32_t top = 1 + ((NumVertexRings - 1) * URes);
	const uint32_t bottom = numVertices - 1;
	for (uint32_t u = 0; u < URes; ++u) {
		const uint32_t currentU = u;
		const uint32_t nextU = (u + 1) % URes;
		const uint32_t currTop = top + currentU;
		const uint32_t nextTop = top + nextU;

		indices[curIndex++] = currTop;
		indices[curIndex++] = bottom;
		indices[curIndex++] = nextTop;
	}

	return CreateFromDefaultVerticesData(vertices, indices, flags);
}

WError WGeometry::CreateCone(float fRadius, float fHeight, uint32_t hsegs, uint32_t csegs, W_GEOMETRY_CREATE_FLAGS flags) {
	hsegs += 2;
	if (csegs < 3 || hsegs < 2)
		return WError(W_INVALIDPARAM);
	//3 indices * number of triangles (top and bottom triangles + side triangles)
	uint32_t numIndices = 3 * (csegs + (hsegs - 1)*csegs * 2);
	//bottom with the circle vertices plus [csegs] vertices for every [hsegs] + one extra seg for uvs
	uint32_t numVertices = 1 + csegs + hsegs * (csegs + 1);

	//allocate vertices
	vector<WDefaultVertex> vertices (numVertices);

	uint32_t pos = 0; //current vertex index
	float deltaAngle = (W_PI*2.0f) / csegs;
	float fCurAngle = 0.0f;

	//a circle of vertices for every height segment
	for (uint32_t i = 0; i < hsegs; i++) {
		fCurAngle = 0.0f;
		for (uint32_t n = 0; n <= csegs; n++) {
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
	for (uint32_t n = 0; n < csegs; n++) {
		float x = sinf(fCurAngle);
		float z = cosf(fCurAngle);
		vertices[pos++] = WDefaultVertex(x, 0.0f, z, 0.0f, 1.0f, 0.0f, 0, -1.0f, 0, 0.0f, 0.0f);

		fCurAngle += deltaAngle;
	}
	vertices[pos++] = WDefaultVertex(0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);

	//scale and shift vertices
	for (uint32_t i = 0; i < numVertices; i++) {
		vertices[i].pos.y -= 0.5f;
		vertices[i].pos.x *= fRadius;
		vertices[i].pos.y *= fHeight;
		vertices[i].pos.z *= fRadius;
	}

	//allocate uints for the indices
	vector<uint32_t> indices(numIndices);
	pos = 0;

	//middle triangles
	uint32_t ringBaseIndex = 0;
	for (uint32_t i = 0; i < hsegs - 1; i++) {
		for (uint32_t n = 0; n < csegs; n++) {
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
	for (uint32_t i = 0; i < csegs; i++) {
		indices[pos++] = numVertices - 1;
		indices[pos++] = numVertices - 1 - csegs + i + 1;
		indices[pos++] = numVertices - 1 - csegs + i;
		if (indices[pos - 2] == numVertices - 1)
			indices[pos - 2] = numVertices - 1 - csegs;
	}

	return CreateFromDefaultVerticesData(vertices, indices, flags);
}

WError WGeometry::CreateCylinder(float fRadius, float fHeight, uint32_t hsegs, uint32_t csegs, W_GEOMETRY_CREATE_FLAGS flags) {
	hsegs -= 2;
	if (csegs < 3 || hsegs < 2)
		return WError(W_INVALIDPARAM);
	//3 indices * number of triangles (top and bottom triangles + side triangles)
	uint32_t numIndices = 3 * (csegs * 2 + (hsegs - 1)*csegs * 2);
	//top and bottom with their circles vertices plus [csegs] vertices for every [hsegs] + one extra seg for uvs
	uint32_t numVertices = 2 + csegs * 2 + hsegs * (csegs + 1);

	//allocate vertices
	vector<WDefaultVertex> vertices (numVertices);

	uint32_t pos = 0; //current vertex index
	float deltaAngle = (W_PI*2.0f) / csegs;
	float fCurAngle = 0.0f;
	//first, top vertex and it's circle
	vertices[pos++] = WDefaultVertex(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	fCurAngle = 0.0f;
	for (uint32_t n = 0; n < csegs; n++) {
		float x = sinf(fCurAngle);
		float z = cosf(fCurAngle);
		vertices[pos++] = WDefaultVertex(x, 1.0f, z, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);

		fCurAngle += deltaAngle;
	}

	//a circle of vertices for every height segment
	for (uint32_t i = 0; i < hsegs; i++) {
		fCurAngle = 0.0f;
		for (uint32_t n = 0; n <= csegs; n++) {
			float x = sinf(fCurAngle);
			float z = cosf(fCurAngle);
			float y = 1.0f - ((float)i / (float)(hsegs - 1));
			vertices[pos++] = WDefaultVertex(x, y, z, 0.0f, 1.0f, 0.0f, x, 0, z, fCurAngle / (W_PI*2.0f), y - 1.0f);

			fCurAngle += deltaAngle;
		}
	}

	//bottom vertex and it's circle
	fCurAngle = 0.0f;
	for (uint32_t n = 0; n < csegs; n++) {
		float x = sinf(fCurAngle);
		float z = cosf(fCurAngle);
		vertices[pos++] = WDefaultVertex(x, 0.0f, z, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);

		fCurAngle += deltaAngle;
	}
	vertices[pos++] = WDefaultVertex(0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);

	//scale and shift vertices
	for (uint32_t i = 0; i < numVertices; i++) {
		vertices[i].pos.y -= 0.5f;
		vertices[i].pos.x *= fRadius;
		vertices[i].pos.y *= fHeight;
		vertices[i].pos.z *= fRadius;
	}

	//allocate uints for the indices
	vector<uint32_t> indices(numIndices);
	pos = 0;

	//top triangles
	for (uint32_t i = 0; i < csegs; i++) {
		indices[pos++] = 0;
		indices[pos++] = i + 1;
		indices[pos++] = i + 2;
		if (indices[pos - 1] == csegs + 1)
			indices[pos - 1] = 1;
	}

	//middle triangles
	uint32_t ringBaseIndex = csegs + 1;
	for (uint32_t i = 0; i < hsegs - 1; i++) {
		for (uint32_t n = 0; n < csegs; n++) {
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
	for (uint32_t i = 0; i < csegs; i++) {
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

	uint32_t numVerts = from->GetNumVertices();
	uint32_t numIndices = from->GetNumIndices();
	W_VERTEX_DESCRIPTION my_desc = GetVertexDescription(0);
	W_VERTEX_DESCRIPTION from_desc = from->GetVertexDescription(0);
	void* vb = nullptr;
	void* fromvb = nullptr;
	void* fromib = nullptr;

	if (!my_desc.isEqualTo(from_desc)) {
		vb = W_SAFE_ALLOC(numVerts *my_desc.GetSize());
		if (!vb)
			return WError(W_OUTOFMEMORY);
	}

	WError ret = from->MapIndexBuffer(&fromib, W_MAP_READ);
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

	if (my_desc.GetOffset("normal") >= 0 && from_desc.GetOffset("normal") == std::numeric_limits<size_t>::max())
		flags |= W_GEOMETRY_CREATE_CALCULATE_NORMALS;
	if (my_desc.GetOffset("tangent") >= 0 && from_desc.GetOffset("tangent") == std::numeric_limits<size_t>::max())
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

	uint32_t numV = 0, numI = 0, structSize = 0, filesize = 0;
	char usedBuffers = 1, topology;

	file.seekg(0, ios::end);
	filesize = (uint32_t)file.tellg();
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

	if (filesize >= 11 + 44 * numV + sizeof(uint32_t) * numI)
		file.read(&usedBuffers, 1);

	if (topology != 4 /*D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST*/ ||
		structSize != 44 || // wrong vertex format, cannot read it (3x position, 3x tangent, 3x normal, 2x uv)
		!(usedBuffers & 1) || numI == 0 || numV == 0 || // no index/vertex buffers
		(filesize != 11 + 44 * numV + sizeof(uint32_t) * numI && filesize != 10 + 44 * numV + sizeof(uint32_t) * numI
		 && filesize != 11 + 44 * numV + sizeof(uint32_t) * numI + (4*4+4*4) * numV)) {
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
	file.read((char*)ind, numI * sizeof(uint32_t));
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
		if (ret && GetVertexBufferCount() > 1 && GetVertexDescription(1).GetSize() == sizeof(WDefaultVertex_Animation)) {
			ret = CreateAnimationData((void*)a);
		}
		W_SAFE_DELETE_ARRAY(a);
	}

	return ret;
}

void WGeometry::_UpdatePendingMap(WBufferedBuffer* buffer, void* mappedData, uint32_t bufferIndex, W_MAP_FLAGS mapFlags) {
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

void WGeometry::_UpdatePendingUnmap(WBufferedBuffer* buffer, uint32_t bufferIndex) {
	auto it = m_pendingBufferedMaps.find(buffer);
	if (it != m_pendingBufferedMaps.end()) {
		void* bufferedMaps = W_SAFE_ALLOC(buffer->GetMemorySize());
		memcpy(bufferedMaps, it->second[bufferIndex], buffer->GetMemorySize());
		for (uint32_t i = 0; i < it->second.size(); i++) {
			if (i == bufferIndex)
				it->second[i] = nullptr;
			else
				it->second[i] = bufferedMaps;
		}
	}
}

void WGeometry::_PerformPendingMaps(uint32_t bufferIndex) {
	for (auto buf = m_pendingBufferedMaps.begin(); buf != m_pendingBufferedMaps.end(); buf++) {
		WBufferedBuffer* buffer = buf->first;
		if (buf->second.size() > 0 && buf->second[bufferIndex]) {
			void* data = buf->second[bufferIndex];
			void* pMappedData;
			if (buffer->Map(m_app, bufferIndex, &pMappedData, W_MAP_WRITE) == VK_SUCCESS) {
				memcpy(pMappedData, data, buffer->GetMemorySize());
				buffer->Unmap(m_app, bufferIndex);
				buf->second[bufferIndex] = nullptr;
				uint32_t numRemainingPointers = 0;
				for (auto bufIt = buf->second.begin(); bufIt != buf->second.end(); bufIt++)
					numRemainingPointers += (*bufIt == nullptr) ? 0 : 1;
				if (numRemainingPointers == 0)
					W_SAFE_FREE(data); // last buffer to erase -> free the memory
			}
		}
	}
}

WError WGeometry::MapVertexBuffer(void** const vb, W_MAP_FLAGS mapFlags) {
	uint32_t bufferIndex = m_app->GetCurrentBufferingIndex();
	VkResult result = m_vertices.Map(m_app, bufferIndex, vb, mapFlags);
	if (result != VK_SUCCESS)
		return WError(W_NOTVALID);

	if (mapFlags & W_MAP_WRITE)
		m_mappedVertexBufferForWrite = *vb;

	_UpdatePendingMap(&m_vertices, *vb, bufferIndex, mapFlags);

	return WError(W_SUCCEEDED);
}

WError WGeometry::MapIndexBuffer(void** const ib, W_MAP_FLAGS mapFlags) {
	uint32_t bufferIndex = m_app->GetCurrentBufferingIndex();
	VkResult result = m_indices.Map(m_app, bufferIndex, (void**)ib, mapFlags);
	if (result != VK_SUCCESS)
		return WError(W_NOTVALID);

	_UpdatePendingMap(&m_indices, *ib, bufferIndex, mapFlags);

	return WError(W_SUCCEEDED);
}

WError WGeometry::MapAnimationBuffer(void** const ab, W_MAP_FLAGS mapFlags) {
	uint32_t bufferIndex = m_app->GetCurrentBufferingIndex();
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

	uint32_t bufferIndex = m_app->GetCurrentBufferingIndex();
	_UpdatePendingUnmap(&m_vertices, bufferIndex);
	m_vertices.Unmap(m_app, bufferIndex);
}

void WGeometry::UnmapIndexBuffer() {
	uint32_t bufferIndex = m_app->GetCurrentBufferingIndex();
	_UpdatePendingUnmap(&m_indices, bufferIndex);
	m_indices.Unmap(m_app, bufferIndex);
}

void WGeometry::UnmapAnimationBuffer() {
	uint32_t bufferIndex = m_app->GetCurrentBufferingIndex();
	_UpdatePendingUnmap(&m_animationbuf, bufferIndex);
	m_animationbuf.Unmap(m_app, bufferIndex);
}

WError WGeometry::Scale(float mulFactor) {
	if (!Valid())
		return WError(W_NOTVALID);

	size_t vtxSize = GetVertexDescription(0).GetSize();
	size_t offset = GetVertexDescription(0).GetOffset("position");
	if (offset == std::numeric_limits<size_t>::max())
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (uint32_t i = 0; i < m_numVertices; i++) {
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

	size_t vtxSize = GetVertexDescription(0).GetSize();
	size_t offset = GetVertexDescription(0).GetOffset("position");
	if (offset == std::numeric_limits<size_t>::max())
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (uint32_t i = 0; i < m_numVertices; i++) {
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

	size_t vtxSize = GetVertexDescription(0).GetSize();
	size_t offset = GetVertexDescription(0).GetOffset("position");
	if (offset == std::numeric_limits<size_t>::max())
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (uint32_t i = 0; i < m_numVertices; i++) {
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

	size_t vtxSize = GetVertexDescription(0).GetSize();
	size_t offset = GetVertexDescription(0).GetOffset("position");
	if (offset == std::numeric_limits<size_t>::max())
		return W_ERROR(W_NOTVALID);
	int size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (uint32_t i = 0; i < m_numVertices; i++) {
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

	size_t vtxSize = GetVertexDescription(0).GetSize();
	size_t offset = GetVertexDescription(0).GetOffset("position");
	if (offset == std::numeric_limits<size_t>::max())
		return W_ERROR(W_NOTVALID);
	size_t size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (uint32_t i = 0; i < m_numVertices; i++) {
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

	size_t vtxSize = GetVertexDescription(0).GetSize();
	size_t offset = GetVertexDescription(0).GetOffset("position");
	if (offset == std::numeric_limits<size_t>::max())
		return W_ERROR(W_NOTVALID);
	size_t size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents * 4;

	void* data;
	WError err = MapVertexBuffer(&data, W_MAP_WRITE | W_MAP_READ);
	if (!err)
		return err;

	for (uint32_t i = 0; i < m_numVertices; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtxSize * i + offset, size);
		v = WVec3TransformCoord(v, mtx);
		memcpy((char*)data + vtxSize * i + offset, &v, size);
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

bool WGeometry::Intersect(WVector3 p1, WVector3 p2, WVector3* pt, WVector2* uv, uint32_t* triangleIndex) {
	if (!Valid())
		return false;

	/*
		Check all triangles, what is intersected will
		be inserted into the vector intersection, then
		the closest triangle to p1 will be returned
	*/

	uint32_t pos_offset = (uint32_t)GetVertexDescription(0).GetOffset("position");
	uint32_t vtxSize = (uint32_t)GetVertexDescription(0).GetSize();
	uint32_t uv_offset = (uint32_t)GetVertexDescription(0).GetOffset("uv");
	uint32_t uv_size = std::numeric_limits<uint32_t>::max();

	if (pos_offset == std::numeric_limits<uint32_t>::max())
		return false;
	if (GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("position")].numComponents < 3)
		return false;
	if (uv_offset != std::numeric_limits<uint32_t>::max())
		uv_size = GetVertexDescription(0).attributes[GetVertexDescription(0).GetIndex("uv")].numComponents * 4;
	if (uv_size < 8) // if we don't have at least 2 components, ignore UVs
		uv_offset = std::numeric_limits<uint32_t>::max();

	struct IntersectionInfo {
		uint32_t index;
		float u, v;
		WVector3 point;
	};
	vector<IntersectionInfo> intersection;

	void *vb;
	uint* ib;
	WError err = MapVertexBuffer(&vb, W_MAP_READ);
	if (!err)
		return false;
	err = MapIndexBuffer((void**)&ib, W_MAP_READ);
	if (!err) {
		UnmapVertexBuffer();
		return false;
	}

	for (uint32_t i = 0; i < m_numIndices / 3; i++) {
		WVector3 v0;
		WVector3 v1;
		WVector3 v2;
		WVector2 uv0 (0, 0);
		WVector2 uv1 (1, 0);
		WVector2 uv2 (0, 1);
		memcpy(&v0, &((char*)vb)[ib[i * 3 + 0] * vtxSize + pos_offset], sizeof(WVector3));
		memcpy(&v1, &((char*)vb)[ib[i * 3 + 1] * vtxSize + pos_offset], sizeof(WVector3));
		memcpy(&v2, &((char*)vb)[ib[i * 3 + 2] * vtxSize + pos_offset], sizeof(WVector3));
		if (uv_offset != std::numeric_limits<uint32_t>::max()) {
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
	uint32_t chosen = 0;
	float ld = WVec3LengthSq(p1 - intersection[0].point);
	for (uint32_t i = 1; i < intersection.size(); i++) {
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

WError WGeometry::Draw(WRenderTarget* rt, uint32_t numIndices, uint32_t numInstances, bool bind_animation) {
	VkCommandBuffer renderCmdBuffer = rt->GetCommnadBuffer();
	if (!renderCmdBuffer)
		return WError(W_NORENDERTARGET);

	// Bind triangle vertices
	VkDeviceSize offsets[] = { 0, 0 };
	uint32_t bufferIndex = m_app->GetCurrentBufferingIndex();
	VkBuffer bindings[] = { m_vertices.GetBuffer(m_app, bufferIndex), VK_NULL_HANDLE };
	if (bind_animation && m_animationbuf.Valid())
		bindings[1] = m_animationbuf.GetBuffer(m_app, bufferIndex);
	vkCmdBindVertexBuffers(renderCmdBuffer, 0, bindings[1] == VK_NULL_HANDLE ? 1 : 2, bindings, offsets);

	if (m_indices.Valid()) {
		if (numIndices == std::numeric_limits<uint32_t>::max() || numIndices > m_numIndices)
			numIndices = m_numIndices;
		// Bind triangle indices & draw the indexed triangle
		vkCmdBindIndexBuffer(renderCmdBuffer, m_indices.GetBuffer(m_app, bufferIndex), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(renderCmdBuffer, numIndices, numInstances, 0, 0, 0);
	} else {
		if (numIndices == std::numeric_limits<uint32_t>::max() || numIndices > m_numVertices)
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

uint32_t WGeometry::GetNumVertices() const {
	return m_numVertices;
}

uint32_t WGeometry::GetNumIndices() const {
	return m_numIndices;
}

bool WGeometry::IsRigged() const {
	return m_animationbuf.Valid();
}

WError WGeometry::SaveToStream(WFile* file, std::ostream& outputStream) {
	UNREFERENCED_PARAMETER(file);

	if (!Valid())
		return WError(W_NOTVALID);

	void *vb, *ib, *ab = nullptr;
	WError ret = MapVertexBuffer(&vb, W_MAP_READ);
	if (!ret)
		return ret;

	ret = MapIndexBuffer(&ib, W_MAP_READ);
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

	uint32_t numVbs = m_animationbuf.Valid() ? 2 : 1;
	outputStream.write((char*)&numVbs, sizeof(uint32_t));
	for (uint32_t d = 0; d < numVbs; d++) {
		W_VERTEX_DESCRIPTION my_desc = GetVertexDescription(d);
		uint32_t numAttributes = (uint32_t)my_desc.attributes.size();
		outputStream.write((char*)&numAttributes, sizeof(uint32_t));
		for (uint32_t i = 0; i < numAttributes; i++) {
			outputStream.write((char*)&my_desc.attributes[i].numComponents, sizeof(unsigned char));
			uint32_t namesize = (uint32_t)my_desc.attributes[i].name.length();
			outputStream.write((char*)&namesize, sizeof(uint32_t));
			outputStream.write(my_desc.attributes[i].name.c_str(), namesize);
		}
	}

	outputStream.write((char*)&m_numVertices, sizeof(uint32_t));
	outputStream.write((char*)&m_numIndices, sizeof(uint32_t));
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

WError WGeometry::LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args, std::string nameSuffix) {
	UNREFERENCED_PARAMETER(file);

	if (args.size() != 1)
		return WError(W_INVALIDPARAM);
	W_GEOMETRY_CREATE_FLAGS flags = static_cast<W_GEOMETRY_CREATE_FLAGS>(reinterpret_cast<size_t>(args[0]));

	vector<W_VERTEX_DESCRIPTION> from_descs;
	char temp[256];
	uint32_t numVbs;
	inputStream.read((char*)&numVbs, sizeof(uint32_t));
	if (numVbs == 0)
		return WError(W_INVALIDFILEFORMAT);

	from_descs.resize(numVbs);
	for (uint32_t d = 0; d < numVbs; d++) {
		W_VERTEX_DESCRIPTION desc;
		uint32_t numAttributes;
		inputStream.read((char*)&numAttributes, sizeof(uint32_t));
		desc.attributes.resize(numAttributes);
		for (uint32_t i = 0; i < numAttributes; i++) {
			inputStream.read((char*)&desc.attributes[i].numComponents, sizeof(unsigned char));
			uint32_t namesize = (uint32_t)desc.attributes[i].name.length();
			if (namesize > 255)
				namesize = 255;
			inputStream.read((char*)&namesize, sizeof(uint32_t));
			inputStream.read(temp, namesize);
			temp[namesize] = '\0';
			desc.attributes[i].name = temp;
		}
		from_descs[d] = desc;
	}

	uint32_t numV, numI;
	inputStream.read((char*)&numV, sizeof(uint32_t));
	inputStream.read((char*)&numI, sizeof(uint32_t));

	void *vb, *ib;
	vb = W_SAFE_ALLOC(numV * from_descs[0].GetSize());
	ib = W_SAFE_ALLOC(numI * sizeof(uint32_t));
	if (!ib || !vb) {
		W_SAFE_FREE(vb);
		W_SAFE_FREE(ib);
		return WError(W_OUTOFMEMORY);
	}

	inputStream.read((char*)vb, numV * from_descs[0].GetSize());
	inputStream.read((char*)ib, numI * sizeof(uint32_t));

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
