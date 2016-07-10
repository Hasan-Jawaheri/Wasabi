#include "WGeometry.h"

size_t W_VERTEX_DESCRIPTION::GetSize() const {
	size_t s = 0;
	for (int i = 0; i < attributes.size(); i++)
		s += 4 * attributes[i].size;
	return s;
}

size_t W_VERTEX_DESCRIPTION::GetOffset(unsigned int attrib_index) const {
	size_t s = 0;
	for (int i = 0; i < attributes.size(); i++) {
		if (i == attrib_index)
			return s;
		s += 4 * attributes[i].size;
	}
	return -1;
}

size_t W_VERTEX_DESCRIPTION::GetOffset(std::string attrib_name) const {
	size_t s = 0;
	for (int i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == attrib_name)
			return s;
		s += 4 * attributes[i].size;
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

std::string WGeometryManager::GetTypeName(void) const {
	return "Geometry";
}

WGeometryManager::WGeometryManager(class Wasabi* const app) : WManager<WGeometry>(app) {
	m_copyCommandBuffer = VK_NULL_HANDLE;
}
WGeometryManager::~WGeometryManager() {
	VkDevice device = m_app->GetVulkanDevice();
	if (m_copyCommandBuffer)
		vkFreeCommandBuffers(device, m_app->GetCommandPool(), 1, &m_copyCommandBuffer);
	m_copyCommandBuffer = VK_NULL_HANDLE;
}

WError WGeometryManager::Load() {
	VkDevice device = m_app->GetVulkanDevice();
	VkCommandBufferAllocateInfo cmdBufInfo = {};

	// Buffer copies are done on the queue, so we need a command buffer for them
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufInfo.commandPool = m_app->GetCommandPool();
	cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufInfo.commandBufferCount = 1;

	VkResult err = vkAllocateCommandBuffers(device, &cmdBufInfo, &m_copyCommandBuffer);
	if (err)
		return WError(W_OUTOFMEMORY);

	return WError(W_SUCCEEDED);
}

VkResult WGeometryManager::_BeginCopy() {
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.pNext = NULL;

	VkResult err = vkResetCommandBuffer(m_copyCommandBuffer, 0);
	if (err)
		return err;

	// Put buffer region copies into command buffer
	// Note that the staging buffer must not be deleted before the copies 
	// have been submitted and executed
	return vkBeginCommandBuffer(m_copyCommandBuffer, &cmdBufferBeginInfo);
}

VkResult WGeometryManager::_EndCopy() {
	VkSubmitInfo copySubmitInfo = {};
	VkResult err = vkEndCommandBuffer(m_copyCommandBuffer);
	if (err)
		return err;

	// Submit copies to the queue
	copySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	copySubmitInfo.commandBufferCount = 1;
	copySubmitInfo.pCommandBuffers = &m_copyCommandBuffer;

	err = vkQueueSubmit(m_app->Renderer->GetQueue(), 1, &copySubmitInfo, VK_NULL_HANDLE);
	if (err)
		return err;
	err = vkQueueWaitIdle(m_app->Renderer->GetQueue());
	if (err)
		return err;

	return VK_SUCCESS;
}

WGeometry::WGeometry(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	ZeroMemory(&m_vertices, sizeof(m_vertices));
	ZeroMemory(&m_indices, sizeof(m_indices));
	m_dynamic = false;
	
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
	return m_indices.buffer.buf && m_vertices.buffer.buf;
}

void WGeometry::_DestroyResources() {
	VkDevice device = m_app->GetVulkanDevice();
	W_BUFFER* buffers[] = {&m_vertices.buffer, &m_vertices.staging, &m_indices.buffer, &m_indices.staging};
	for (int i = 0; i < sizeof(buffers) / sizeof(W_BUFFER*); i++) {
		if (buffers[i]->buf)
			vkDestroyBuffer(device, buffers[i]->buf, nullptr);
		buffers[i]->buf = VK_NULL_HANDLE;
		if (buffers[i]->mem)
			vkFreeMemory(device, buffers[i]->mem, nullptr);
		buffers[i]->mem = VK_NULL_HANDLE;
	}
}

void WGeometry::_CalcMinMax(void* vb, unsigned int num_verts) {
	m_minPt = WVector3(FLT_MAX, FLT_MAX, FLT_MAX);
	m_maxPt = WVector3(FLT_MIN, FLT_MIN, FLT_MIN);

	int offset = GetVertexDescription().GetOffset("position");
	int vsize = GetVertexDescription().GetSize();

	if (offset == -1)
		return; // no position attribute

	for (int i = 0; i < num_verts; i++) {
		WVector3 v;
		memcpy(&v, (char*)vb + vsize * i + offset, sizeof(WVector3));
		m_minPt.x = min(m_minPt.x, v.x);
		m_minPt.y = min(m_minPt.y, v.y);
		m_minPt.z = min(m_minPt.z, v.z);
		m_maxPt.x = max(m_maxPt.x, v.x);
		m_maxPt.y = max(m_maxPt.y, v.y);
		m_maxPt.z = max(m_maxPt.z, v.z);
	}
}

void WGeometry::_CalcNormals(void* vb, unsigned int num_verts, void* ib, unsigned int num_indices) {
	if (num_indices % 3 != 0)
		return; // must be a triangle list

	int offset = GetVertexDescription().GetOffset("position");
	int normal_offset = GetVertexDescription().GetOffset("position");
	int vsize = GetVertexDescription().GetSize();

	int num_tris = num_indices / 3;
	for (int i = 0; i < num_tris; i++) {
		DWORD ind[3];
		memcpy(ind, (char*)ib + (i*3) * sizeof(DWORD), 3 * sizeof(DWORD));
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
	int norm_offset = GetVertexDescription().GetOffset("normal");
	int tang_offset = GetVertexDescription().GetOffset("tangent");
	int vsize = GetVertexDescription().GetSize();

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

WError WGeometry::CreateFromData(void* vb, unsigned int num_verts, void* ib, unsigned int num_indices,
									bool bDynamic, bool bCalcNormals, bool bCalcTangents) {
	VkDevice device = m_app->GetVulkanDevice();
	VkMemoryAllocateInfo memAlloc = {};
	VkMemoryRequirements memReqs;
	VkBufferCreateInfo vertexBufferInfo = {};
	VkBufferCreateInfo indexbufferInfo = {};
	VkBufferCopy copyRegion = {};
	void *data;
	VkResult err;

	if (num_verts <= 0 || num_indices <= 0 || !vb || !ib)
		return WError(W_INVALIDPARAM);

	int vertexBufferSize = num_verts * GetVertexDescription().GetSize();
	uint32_t indexBufferSize = num_indices * sizeof(uint32_t);

	_DestroyResources();

	if (bCalcNormals && GetVertexDescription().GetIndex("normal") >= 0)
		_CalcNormals(vb, num_verts, ib, num_indices);
	if (bCalcTangents && GetVertexDescription().GetIndex("tangent") >= 0)
		_CalcTangents(vb, num_verts);

	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	// Vertex buffer
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.size = vertexBufferSize;
	// Buffer is used as the copy source
	vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	// Create a host-visible buffer to copy the vertex data to (staging buffer)
	err = vkCreateBuffer(device, &vertexBufferInfo, nullptr, &m_vertices.staging.buf);
	if (err)
		goto destroy_resources;
	vkGetBufferMemoryRequirements(device, m_vertices.staging.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &memAlloc, nullptr, &m_vertices.staging.mem);
	if (err) {
		vkDestroyBuffer(device, m_vertices.staging.buf, nullptr);
		goto destroy_resources;
	}

	// Index buffer
	indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	indexbufferInfo.size = indexBufferSize;
	indexbufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	// Copy index data to a buffer visible to the host (staging buffer)
	err = vkCreateBuffer(device, &indexbufferInfo, nullptr, &m_indices.staging.buf);
	if (err) {
		vkDestroyBuffer(device, m_vertices.staging.buf, nullptr);
		vkFreeMemory(device, m_vertices.staging.mem, nullptr);
		goto destroy_resources;
	}

	vkGetBufferMemoryRequirements(device, m_indices.staging.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &memAlloc, nullptr, &m_indices.staging.mem);
	if (err) {
		vkDestroyBuffer(device, m_vertices.staging.buf, nullptr);
		vkFreeMemory(device, m_vertices.staging.mem, nullptr);
		vkDestroyBuffer(device, m_indices.staging.buf, nullptr);
		goto destroy_resources;
	}

	// Map and copy VB
	err = vkMapMemory(device, m_vertices.staging.mem, 0, memAlloc.allocationSize, 0, &data);
	if (err)
		goto destroy_staging;
	memcpy(data, vb, vertexBufferSize);
	vkUnmapMemory(device, m_vertices.staging.buf);
	err = vkBindBufferMemory(device, m_vertices.staging.buf, m_vertices.staging.mem, 0);
	if (err)
		goto destroy_staging;

	// Create the destination buffer with device only visibility
	// Buffer will be used as a vertex buffer
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	err = vkCreateBuffer(device, &vertexBufferInfo, nullptr, &m_vertices.buffer.buf);
	if (err)
		goto destroy_staging;
	vkGetBufferMemoryRequirements(device, m_vertices.buffer.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits,
		bDynamic ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&memAlloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &memAlloc, nullptr, &m_vertices.buffer.mem);
	if (err)
		goto destroy_staging;
	err = vkBindBufferMemory(device, m_vertices.buffer.buf, m_vertices.buffer.mem, 0);
	if (err)
		goto destroy_staging;

	// Map and copy IB
	err = vkMapMemory(device, m_indices.staging.mem, 0, indexBufferSize, 0, &data);
	if (err)
		goto destroy_staging;
	memcpy(data, ib, indexBufferSize);
	vkUnmapMemory(device, m_indices.staging.mem);
	err = vkBindBufferMemory(device, m_indices.staging.buf, m_indices.staging.mem, 0);
	if (err)
		goto destroy_staging;

	// Create destination buffer with device only visibility
	// Buffer will be used as an index buffer
	indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	err = vkCreateBuffer(device, &indexbufferInfo, nullptr, &m_indices.buffer.buf);
	if (err)
		goto destroy_staging;
	vkGetBufferMemoryRequirements(device, m_indices.buffer.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits,
		bDynamic ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&memAlloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &memAlloc, nullptr, &m_indices.buffer.mem);
	if (err)
		goto destroy_staging;
	err = vkBindBufferMemory(device, m_indices.buffer.buf, m_indices.buffer.mem, 0);
	if (err)
		goto destroy_staging;

	err = m_app->GeometryManager->_BeginCopy();
	if (err)
		goto destroy_staging;

	// Vertex buffer
	copyRegion.size = vertexBufferSize;
	vkCmdCopyBuffer(
		m_app->GeometryManager->m_copyCommandBuffer,
		m_vertices.staging.buf,
		m_vertices.buffer.buf,
		1,
		&copyRegion);
	// Index buffer
	copyRegion.size = indexBufferSize;
	vkCmdCopyBuffer(
		m_app->GeometryManager->m_copyCommandBuffer,
		m_indices.staging.buf,
		m_indices.buffer.buf,
		1,
		&copyRegion);

	err = m_app->GeometryManager->_EndCopy();
	if (err)
		goto destroy_staging;

	// Destroy staging buffers
destroy_staging:
	// remove staging buffers if there's an error, geometry is immutable or if its dynamic
	m_immutable = m_app->engineParams["geometryImmutable"];
	if (err || m_immutable || bDynamic) {
		vkDestroyBuffer(device, m_vertices.staging.buf, nullptr);
		vkFreeMemory(device, m_vertices.staging.mem, nullptr);
		vkDestroyBuffer(device, m_indices.staging.buf, nullptr);
		vkFreeMemory(device, m_indices.staging.mem, nullptr);
		m_vertices.staging.buf = m_indices.staging.buf = VK_NULL_HANDLE;
		m_vertices.staging.mem = m_indices.staging.mem = VK_NULL_HANDLE;
	}

destroy_resources:
	if (err) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	m_indices.count = num_indices;
	m_vertices.count = num_verts;
	m_dynamic = bDynamic;
	_CalcMinMax(vb, num_verts);

	return WError(W_SUCCEEDED);
}

WError WGeometry::CreateCube(float fSize, bool bDynamic) {
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
		vertices[i].pos *= fSize;

	//allocate 36 DWORDs for the indices of the cube
	vector<DWORD> indices (36);

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

	return CreateFromData(vertices.data(), vertices.size(), indices.data(), indices.size(), bDynamic);
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

	//allocate 36 DWORDs for the indices of the cube
	vector<DWORD> indices(36);

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

	return CreateFromData(vertices.data(), vertices.size(), indices.data(), indices.size(), bDynamic);
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
	UINT curVert = 0;

	//fill the plain vertex data
	for (UINT z = vpc - 1; z >= 0 && z != UINT_MAX; z--) {
		for (UINT x = 0; x < vpr; x++) {
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
	vector<DWORD> indices(num_indices);

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

	return CreateFromData(vertices.data(), vertices.size(), indices.data(), indices.size(), bDynamic);
}

WError WGeometry::CreateSphere(float Radius, unsigned int VRes, unsigned int URes, bool bDynamic) {
	const UINT NumVertexRings = VRes - 2;
	unsigned int num_vertices = NumVertexRings * URes + 2;
	const UINT NumTriangleRings = VRes - 1;
	const UINT NumTriangles = (NumTriangleRings + 1) * URes * 2;
	unsigned int num_indices = NumTriangles * 3;

	// Calculate all of the vertex positions
	vector<WDefaultVertex> vertices (num_vertices);
	int currVert = 0;

	// First vertex will be at the top pole
	vertices[currVert++] = WDefaultVertex(0.0f, Radius, 0, 1, 0, 0, 0, 1, 0, 0, 0);

	// Add in the vertical rings of vertices
	for (UINT v = 1; v <= NumVertexRings; ++v) {
		for (UINT u = 0; u < URes; ++u) {
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
	vector<DWORD> indices(num_indices);
	UINT curIndex = 0;

	// Top ring first
	for (UINT u = 0; u < URes; ++u) {
		const UINT currentU = u;
		const UINT nextU = (u + 1) % URes;
		indices[curIndex++] = 0;
		indices[curIndex++] = u + 1;
		indices[curIndex++] = nextU + 1;
	}

	// Now the middle rings
	for (UINT v = 1; v < VRes - 2; ++v) {
		const UINT top = 1 + ((v - 1) * URes);
		const UINT bottom = top + URes;
		for (UINT u = 0; u < URes; ++u) {
			const UINT currentU = u;
			const UINT nextU = (u + 1) % URes;
			const UINT currTop = top + currentU;
			const UINT nextTop = top + nextU;
			const UINT currBottom = bottom + currentU;
			const UINT nextBottom = bottom + nextU;

			indices[curIndex++] = currTop;
			indices[curIndex++] = currBottom;
			indices[curIndex++] = nextBottom;

			indices[curIndex++] = nextBottom;
			indices[curIndex++] = nextTop;
			indices[curIndex++] = currTop;
		}
	}

	// Now the bottom ring
	const UINT top = 1 + ((NumVertexRings - 1) * URes);
	const UINT bottom = num_vertices - 1;
	for (UINT u = 0; u < URes; ++u) {
		const UINT currentU = u;
		const UINT nextU = (u + 1) % URes;
		const UINT currTop = top + currentU;
		const UINT nextTop = top + nextU;

		indices[curIndex++] = currTop;
		indices[curIndex++] = bottom;
		indices[curIndex++] = nextTop;
	}

	return CreateFromData(vertices.data(), vertices.size(), indices.data(), indices.size(), bDynamic);
}

WError WGeometry::CreateCone(float fRadius, float fHeight, unsigned int hsegs, unsigned int csegs, bool bDynamic) {
	//3 indices * number of triangles (top and bottom triangles + side triangles)
	unsigned int num_indices = 3 * (csegs + (hsegs - 1)*csegs * 2);
	//bottom with the circle vertices plus [csegs] vertices for every [hsegs] + one extra seg for uvs
	unsigned int num_vertices = 1 + csegs + hsegs * (csegs + 1);

	//allocate vertices
	vector<WDefaultVertex> vertices (num_vertices);

	UINT pos = 0; //current vertex index
	float deltaAngle = (W_PI*2.0f) / csegs;
	float fCurAngle = 0.0f;

	//a circle of vertices for every height segment
	for (UINT i = 0; i < hsegs; i++) {
		fCurAngle = 0.0f;
		for (UINT n = 0; n <= csegs; n++) {
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
	for (UINT n = 0; n < csegs; n++) {
		float x = sinf(fCurAngle);
		float z = cosf(fCurAngle);
		vertices[pos++] = WDefaultVertex(x, 0.0f, z, 0.0f, 1.0f, 0.0f, 0, -1.0f, 0, 0.0f, 0.0f);

		fCurAngle += deltaAngle;
	}
	vertices[pos++] = WDefaultVertex(0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);

	//scale and shift vertices
	for (UINT i = 0; i < num_vertices; i++) {
		vertices[i].pos.y -= 0.5f;
		vertices[i].pos.x *= fRadius;
		vertices[i].pos.y *= fHeight;
		vertices[i].pos.z *= fRadius;
	}

	//allocate DWORDs for the indices
	vector<DWORD> indices(num_indices);
	pos = 0;

	//middle triangles
	UINT ringBaseIndex = 0;
	for (UINT i = 0; i < hsegs - 1; i++) {
		for (UINT n = 0; n < csegs; n++) {
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
	for (UINT i = 0; i < csegs; i++) {
		indices[pos++] = num_vertices - 1;
		indices[pos++] = num_vertices - 1 - csegs + i + 1;
		indices[pos++] = num_vertices - 1 - csegs + i;
		if (indices[pos - 2] == num_vertices - 1)
			indices[pos - 2] = num_vertices - 1 - csegs;
	}

	return CreateFromData(vertices.data(), vertices.size(), indices.data(), indices.size(), bDynamic);
}

WError WGeometry::CreateCylinder(float fRadius, float fHeight, unsigned int hsegs, unsigned int csegs, bool bDynamic) {
	//3 indices * number of triangles (top and bottom triangles + side triangles)
	unsigned int num_indices = 3 * (csegs * 2 + (hsegs - 1)*csegs * 2);
	//top and bottom with their circles vertices plus [csegs] vertices for every [hsegs] + one extra seg for uvs
	unsigned int num_vertices = 2 + csegs * 2 + hsegs * (csegs + 1);

	//allocate vertices
	vector<WDefaultVertex> vertices (num_vertices);

	UINT pos = 0; //current vertex index
	float deltaAngle = (W_PI*2.0f) / csegs;
	float fCurAngle = 0.0f;
	//first, top vertex and it's circle
	vertices[pos++] = WDefaultVertex(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	fCurAngle = 0.0f;
	for (UINT n = 0; n < csegs; n++) {
		float x = sinf(fCurAngle);
		float z = cosf(fCurAngle);
		vertices[pos++] = WDefaultVertex(x, 1.0f, z, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);

		fCurAngle += deltaAngle;
	}

	//a circle of vertices for every height segment
	for (UINT i = 0; i < hsegs; i++) {
		fCurAngle = 0.0f;
		for (UINT n = 0; n <= csegs; n++) {
			float x = sinf(fCurAngle);
			float z = cosf(fCurAngle);
			float y = 1.0f - ((float)i / (float)(hsegs - 1));
			vertices[pos++] = WDefaultVertex(x, y, z, 0.0f, 1.0f, 0.0f, x, 0, z, fCurAngle / (W_PI*2.0f), y - 1.0f);

			fCurAngle += deltaAngle;
		}
	}

	//bottom vertex and it's circle
	fCurAngle = 0.0f;
	for (UINT n = 0; n < csegs; n++) {
		float x = sinf(fCurAngle);
		float z = cosf(fCurAngle);
		vertices[pos++] = WDefaultVertex(x, 0.0f, z, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);

		fCurAngle += deltaAngle;
	}
	vertices[pos++] = WDefaultVertex(0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);

	//scale and shift vertices
	for (UINT i = 0; i < num_vertices; i++) {
		vertices[i].pos.y -= 0.5f;
		vertices[i].pos.x *= fRadius;
		vertices[i].pos.y *= fHeight;
		vertices[i].pos.z *= fRadius;
	}

	//allocate DWORDs for the indices
	vector<DWORD> indices(num_indices);
	pos = 0;

	//top triangles
	for (UINT i = 0; i < csegs; i++) {
		indices[pos++] = 0;
		indices[pos++] = i + 1;
		indices[pos++] = i + 2;
		if (indices[pos - 1] == csegs + 1)
			indices[pos - 1] = 1;
	}

	//middle triangles
	UINT ringBaseIndex = csegs + 1;
	for (UINT i = 0; i < hsegs - 1; i++) {
		for (UINT n = 0; n < csegs; n++) {
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
	for (UINT i = 0; i < csegs; i++) {
		indices[pos++] = num_vertices - 1;
		indices[pos++] = num_vertices - 1 - csegs + i + 1;
		indices[pos++] = num_vertices - 1 - csegs + i;
		if (indices[pos - 2] == num_vertices - 1)
			indices[pos - 2] = num_vertices - 1 - csegs;
	}

	return CreateFromData(vertices.data(), vertices.size(), indices.data(), indices.size(), bDynamic);
}

WError WGeometry::CopyFrom(WGeometry* const from, bool bDynamic) {
	if (!from->Valid() || (from->m_immutable && !from->m_dynamic))
		return WError(W_INVALIDPARAM);

	unsigned int num_verts = from->GetNumVertices();
	unsigned int num_indices = from->GetNumIndices();
	W_VERTEX_DESCRIPTION my_desc = GetVertexDescription();
	W_VERTEX_DESCRIPTION from_desc = from->GetVertexDescription();
	size_t vtx_size = my_desc.GetSize();
	size_t from_vtx_size = from_desc.GetSize();
	void *vb, *fromvb, *fromib;

	vb = malloc(num_verts * vtx_size);
	if (!vb)
		return WError(W_OUTOFMEMORY);

	WError err = from->MapIndexBuffer((DWORD**)&fromib);
	if (err) {
		free(vb);
		return WError(W_INVALIDPARAM);
	}
	err = from->MapVertexBuffer(&fromvb);
	if (err) {
		from->UnmapIndexBuffer();
		free(vb);
		return WError(W_INVALIDPARAM);
	}

	for (int i = 0; i < num_verts; i++) {
		char* fromvtx = (char*)fromvb + from_vtx_size * i;
		char* myvtx = (char*)vb + vtx_size * i;
		for (int j = 0; j < my_desc.attributes.size(); j++) {
			std::string name = my_desc.attributes[j].name;
			int off_in_from = from_desc.GetOffset(name);
			int index_in_from = from_desc.GetIndex(name);
			if (off_in_from >= 0 && my_desc.attributes[j].size == from_desc.attributes[index_in_from].size)
				memcpy(myvtx + my_desc.GetOffset(name), fromvtx + off_in_from, my_desc.attributes[j].size);
		}
	}

	bool bCalcTangents = false, bCalcNormals = false;
	if (my_desc.GetOffset("normal") >= 0 && from_desc.GetOffset("normal") == -1)
		bCalcNormals = true;
	if (my_desc.GetOffset("tangent") >= 0 && from_desc.GetOffset("tangent") == -1)
		bCalcTangents = true;

	from->UnmapVertexBuffer();
	err = CreateFromData(vb, num_verts, fromib, num_indices, bDynamic, bCalcNormals, bCalcTangents);
	free(vb);
	from->UnmapIndexBuffer();
	return err;
}

WError WGeometry::MapVertexBuffer(void** const vb, bool bReadOnly) {
	if (!Valid() || (m_immutable && !m_dynamic))
		return WError(W_NOTVALID);

	m_vertices.readOnlyMap = bReadOnly;

	W_BUFFER* b = m_dynamic ? &m_vertices.buffer : &m_vertices.staging;
	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription().GetSize();

	VkResult err = vkMapMemory(device, b->mem, 0, m_vertices.count * vtx_size, 0, vb);
	if (err)
		return WError(W_NOTVALID);

	return WError(W_SUCCEEDED);
}

WError WGeometry::MapIndexBuffer(DWORD** const ib, bool bReadOnly) {
	if (!Valid() || (m_immutable && !m_dynamic))
		return WError(W_NOTVALID);

	m_indices.readOnlyMap = bReadOnly;

	W_BUFFER* b = m_dynamic ? &m_indices.buffer : &m_indices.staging;
	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription().GetSize();

	VkResult err = vkMapMemory(device, b->mem, 0, m_indices.count * sizeof(DWORD), 0, (void**)ib);
	if (err)
		return WError(W_NOTVALID);

	return WError(W_SUCCEEDED);
}

void WGeometry::UnmapVertexBuffer() {
	VkDevice device = m_app->GetVulkanDevice();
	if (m_dynamic) {
		vkUnmapMemory(device, m_vertices.buffer.mem);
	} else {
		vkUnmapMemory(device, m_vertices.staging.mem);
		if (!m_vertices.readOnlyMap) {
			VkBufferCopy copyRegion = {};

			VkResult err = m_app->GeometryManager->_BeginCopy();
			if (err)
				return;

			// Vertex buffer
			copyRegion.size = m_vertices.count * GetVertexDescription().GetSize();
			vkCmdCopyBuffer(
				m_app->GeometryManager->m_copyCommandBuffer,
				m_vertices.staging.buf,
				m_vertices.buffer.buf,
				1,
				&copyRegion);

			m_app->GeometryManager->_EndCopy();
		}
	}
}

void WGeometry::UnmapIndexBuffer() {
	VkDevice device = m_app->GetVulkanDevice();
	if (m_dynamic) {
		vkUnmapMemory(device, m_indices.buffer.mem);
	} else {
		vkUnmapMemory(device, m_indices.staging.mem);
		if (!m_indices.readOnlyMap) {
			VkBufferCopy copyRegion = {};

			VkResult err = m_app->GeometryManager->_BeginCopy();
			if (err)
				return;

			// Index buffer
			copyRegion.size = m_indices.count * sizeof(DWORD);
			vkCmdCopyBuffer(
				m_app->GeometryManager->m_copyCommandBuffer,
				m_indices.staging.buf,
				m_indices.buffer.buf,
				1,
				&copyRegion);

			m_app->GeometryManager->_EndCopy();
		}
	}
}

bool WGeometry::Intersect(WVector3 p1, WVector3 p2, unsigned int* triangleIndex, float* u, float* v, WVector3* pt) const {
	// TODO: fill this
	return false;
}

WError WGeometry::Scale(float mulFactor) {
	if (!Valid() || (m_immutable && !m_dynamic))
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription().GetSize();
	int offset = GetVertexDescription().GetOffset("position");

	void* data;
	WError err = MapVertexBuffer(&data);
	if (!err)
		return err;

	for (int i = 0; i < m_vertices.count; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtx_size * i + offset, sizeof(WVector3));
		v *= mulFactor;
		memcpy((char*)data + vtx_size * i + offset, &v, sizeof(WVector3));
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ScaleX(float mulFactor) {
	if (!Valid() || (m_immutable && !m_dynamic))
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription().GetSize();
	int offset = GetVertexDescription().GetOffset("position");

	void* data;
	WError err = MapVertexBuffer(&data);
	if (!err)
		return err;

	for (int i = 0; i < m_vertices.count; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtx_size * i + offset, sizeof(WVector3));
		v.x *= mulFactor;
		memcpy((char*)data + vtx_size * i + offset, &v, sizeof(WVector3));
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ScaleY(float mulFactor) {
	if (!Valid() || (m_immutable && !m_dynamic))
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription().GetSize();
	int offset = GetVertexDescription().GetOffset("position");

	void* data;
	WError err = MapVertexBuffer(&data);
	if (!err)
		return err;

	for (int i = 0; i < m_vertices.count; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtx_size * i + offset, sizeof(WVector3));
		v.y *= mulFactor;
		memcpy((char*)data + vtx_size * i + offset, &v, sizeof(WVector3));
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ScaleZ(float mulFactor) {
	if (!Valid() || (m_immutable && !m_dynamic))
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription().GetSize();
	int offset = GetVertexDescription().GetOffset("position");

	void* data;
	WError err = MapVertexBuffer(&data);
	if (!err)
		return err;

	for (int i = 0; i < m_vertices.count; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtx_size * i + offset, sizeof(WVector3));
		v.z *= mulFactor;
		memcpy((char*)data + vtx_size * i + offset, &v, sizeof(WVector3));
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ApplyOffset(float x, float y, float z) {
	return ApplyOffset(WVector3(x, y, z));
}

WError WGeometry::ApplyOffset(WVector3 _offset) {
	if (!Valid() || (m_immutable && !m_dynamic))
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription().GetSize();
	int offset = GetVertexDescription().GetOffset("position");

	void* data;
	WError err = MapVertexBuffer(&data);
	if (!err)
		return err;

	for (int i = 0; i < m_vertices.count; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtx_size * i + offset, sizeof(WVector3));
		v += _offset;
		memcpy((char*)data + vtx_size * i + offset, &v, sizeof(WVector3));
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::ApplyRotation(WMatrix mtx) {
	if (!Valid() || (m_immutable && !m_dynamic))
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	size_t vtx_size = GetVertexDescription().GetSize();
	int offset = GetVertexDescription().GetOffset("position");

	void* data;
	WError err = MapVertexBuffer(&data);
	if (!err)
		return err;

	for (int i = 0; i < m_vertices.count; i++) {
		WVector3 v;
		memcpy(&v, (char*)data + vtx_size * i + offset, sizeof(WVector3));
		v = WVec3TransformCoord(v, mtx);
		memcpy((char*)data + vtx_size * i + offset, &v, sizeof(WVector3));
	}

	UnmapVertexBuffer();
	return WError(W_SUCCEEDED);
}

WError WGeometry::Draw(unsigned int num_triangles) {
	VkCommandBuffer renderCmdBuffer = m_app->Renderer->GetCommnadBuffer();

	if (num_triangles == -1 || num_triangles * 3 > m_indices.count)
		num_triangles = m_indices.count / 3;

	// Bind triangle vertices
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(renderCmdBuffer, 0/*VERTEX_BUFFER_BIND_ID*/, 1, &m_vertices.buffer.buf, offsets);

	// Bind triangle indices
	vkCmdBindIndexBuffer(renderCmdBuffer, m_indices.buffer.buf, 0, VK_INDEX_TYPE_UINT32);

	// Draw indexed triangle
	vkCmdDrawIndexed(renderCmdBuffer, num_triangles * 3, 1, 0, 0, 1);

	return WError(W_SUCCEEDED);
}

WVector3 WGeometry::GetMaxPoint() const {
	return m_maxPt;
}

WVector3 WGeometry::GetMinPoint() const {
	return m_minPt;
}

unsigned int WGeometry::GetNumVertices() const {
	return m_vertices.count;
}

unsigned int WGeometry::GetNumIndices() const {
	return m_indices.count;
}
