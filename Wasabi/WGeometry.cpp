#include "WGeometry.h"

std::string WGeometryManager::GetTypeName(void) const {
	return "Geometry";
}

WGeometryManager::WGeometryManager(class Wasabi* const app) : WManager<WGeometry>(app) {
}

WGeometry::WGeometry(Wasabi* const app, unsigned int ID) : WBase(app) {
	SetID(ID);
	app->GeometryManager->AddEntity(this);
}

WGeometry::~WGeometry() {
	m_app->GeometryManager->RemoveEntity(this);
}

std::string WGeometry::GetTypeName() const {
	return "Geometry";
}

bool WGeometry::Valid() const {
	return true; //TODO: implement this
}

WError WGeometry::CreateCube(float fSize, bool bDynamic) {
	VkDevice device = m_app->GetVulkanDevice();

	struct Vertex {
		float pos[3];
		float col[2];
	};

	// Setup vertices
	std::vector<Vertex> vertexBuffer = {
		{ { 1.0f,  1.0f, 0.0f },{ 1.0f, 0.0f } },
		{ { -1.0f,  1.0f, 0.0f },{ 0.0f, 0.0f } },
		{ { 0.0f, -1.0f, 0.0f },{ 0.5f, 1.0f } }
	};
	int vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);

	m_minPt = WVector3(-1.1f, -1.1f, -0.1f);
	m_maxPt = WVector3(1.1f, 1.1f, 0.1f);

	// Setup indices
	std::vector<uint32_t> indexBuffer = { 0, 1, 2 };
	uint32_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
	m_indices.count = indexBuffer.size();

	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;

	void *data;

	// Static data like vertex and index buffer should be stored on the device memory 
	// for optimal (and fastest) access by the GPU
	//
	// To achieve this we use so-called "staging buffers" :
	// - Create a buffer that's visible to the host (and can be mapped)
	// - Copy the data to this buffer
	// - Create another buffer that's local on the device (VRAM) with the same size
	// - Copy the data from the host to the device using a command buffer

	struct StagingBuffer {
		VkDeviceMemory memory;
		VkBuffer buffer;
	};

	struct {
		StagingBuffer vertices;
		StagingBuffer indices;
	} stagingBuffers;

	// Buffer copies are done on the queue, so we need a command buffer for them
	VkCommandBufferAllocateInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufInfo.commandPool = m_app->Renderer->GetCommandPool();
	cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufInfo.commandBufferCount = 1;

	VkCommandBuffer copyCommandBuffer;
	vkTools::checkResult(vkAllocateCommandBuffers(device, &cmdBufInfo, &copyCommandBuffer));

	// Vertex buffer
	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.size = vertexBufferSize;
	// Buffer is used as the copy source
	vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	// Create a host-visible buffer to copy the vertex data to (staging buffer)
	vkTools::checkResult(vkCreateBuffer(device, &vertexBufferInfo, nullptr, &stagingBuffers.vertices.buffer));
	vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
	vkTools::checkResult(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.vertices.memory));
	// Map and copy
	vkTools::checkResult(vkMapMemory(device, stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, 0, &data));
	memcpy(data, vertexBuffer.data(), vertexBufferSize);
	vkUnmapMemory(device, stagingBuffers.vertices.memory);
	vkTools::checkResult(vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0));

	// Create the destination buffer with device only visibility
	// Buffer will be used as a vertex buffer and is the copy destination
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vkTools::checkResult(vkCreateBuffer(device, &vertexBufferInfo, nullptr, &m_vertices.buf));
	vkGetBufferMemoryRequirements(device, m_vertices.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
	vkTools::checkResult(vkAllocateMemory(device, &memAlloc, nullptr, &m_vertices.mem));
	vkTools::checkResult(vkBindBufferMemory(device, m_vertices.buf, m_vertices.mem, 0));

	// Index buffer
	// todo : comment
	VkBufferCreateInfo indexbufferInfo = {};
	indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	indexbufferInfo.size = indexBufferSize;
	indexbufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	// Copy index data to a buffer visible to the host (staging buffer)
	vkTools::checkResult(vkCreateBuffer(device, &indexbufferInfo, nullptr, &stagingBuffers.indices.buffer));
	vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
	vkTools::checkResult(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.indices.memory));
	vkTools::checkResult(vkMapMemory(device, stagingBuffers.indices.memory, 0, indexBufferSize, 0, &data));
	memcpy(data, indexBuffer.data(), indexBufferSize);
	vkUnmapMemory(device, stagingBuffers.indices.memory);
	vkTools::checkResult(vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0));

	// Create destination buffer with device only visibility
	indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vkTools::checkResult(vkCreateBuffer(device, &indexbufferInfo, nullptr, &m_indices.buf));
	vkGetBufferMemoryRequirements(device, m_indices.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
	vkTools::checkResult(vkAllocateMemory(device, &memAlloc, nullptr, &m_indices.mem));
	vkTools::checkResult(vkBindBufferMemory(device, m_indices.buf, m_indices.mem, 0));
	m_indices.count = indexBuffer.size();

	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.pNext = NULL;

	VkBufferCopy copyRegion = {};

	// Put buffer region copies into command buffer
	// Note that the staging buffer must not be deleted before the copies 
	// have been submitted and executed
	vkTools::checkResult(vkBeginCommandBuffer(copyCommandBuffer, &cmdBufferBeginInfo));

	// Vertex buffer
	copyRegion.size = vertexBufferSize;
	vkCmdCopyBuffer(
		copyCommandBuffer,
		stagingBuffers.vertices.buffer,
		m_vertices.buf,
		1,
		&copyRegion);
	// Index buffer
	copyRegion.size = indexBufferSize;
	vkCmdCopyBuffer(
		copyCommandBuffer,
		stagingBuffers.indices.buffer,
		m_indices.buf,
		1,
		&copyRegion);

	vkTools::checkResult(vkEndCommandBuffer(copyCommandBuffer));

	// Submit copies to the queue
	VkSubmitInfo copySubmitInfo = {};
	copySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	copySubmitInfo.commandBufferCount = 1;
	copySubmitInfo.pCommandBuffers = &copyCommandBuffer;

	vkTools::checkResult(vkQueueSubmit(m_app->Renderer->GetQueue(), 1, &copySubmitInfo, VK_NULL_HANDLE));
	vkTools::checkResult(vkQueueWaitIdle(m_app->Renderer->GetQueue()));

	vkFreeCommandBuffers(device, m_app->Renderer->GetCommandPool(), 1, &copyCommandBuffer);

	// Destroy staging buffers
	vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
	vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
	vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
	vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);

	return WError(W_SUCCEEDED);
}

WError WGeometry::Draw() {
	VkCommandBuffer renderCmdBuffer = m_app->Renderer->GetCommnadBuffer();

	// Bind triangle vertices
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(renderCmdBuffer, 0/*VERTEX_BUFFER_BIND_ID*/, 1, &m_vertices.buf, offsets);

	// Bind triangle indices
	vkCmdBindIndexBuffer(renderCmdBuffer, m_indices.buf, 0, VK_INDEX_TYPE_UINT32);

	// Draw indexed triangle
	vkCmdDrawIndexed(renderCmdBuffer, m_indices.count, 1, 0, 0, 1);

	return WError(W_SUCCEEDED);
}

WVector3 WGeometry::GetMaxPoint() const {
	return m_maxPt;
}

WVector3 WGeometry::GetMinPoint() const {
	return m_minPt;
}
