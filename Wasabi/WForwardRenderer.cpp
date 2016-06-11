#include "WForwardRenderer.h"

static VkCommandBuffer g_CreateCommandBuffer(VkDevice device, VkCommandPool pool) {
	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vkTools::initializers::commandBufferAllocateInfo(
			pool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1);

	VkCommandBuffer buff;
	VkResult err = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &buff);
	if (err != VK_SUCCESS)
		return VK_NULL_HANDLE;

	return buff;
}
static void g_FreeCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer* buf) {
	if (*buf != VK_NULL_HANDLE) {
		vkFreeCommandBuffers(device, pool, 1, buf);
		*buf = VK_NULL_HANDLE;
	}
}

WForwardRenderer::WForwardRenderer(Wasabi* app) : WRenderer(app) {
	m_swapchainInitialized = false;
	m_clearColor = { { 0.425f, 0.425f, 0.425f, 1.0f } };
	m_setupCmdBuffer = m_renderCmdBuffer = VK_NULL_HANDLE;
	m_cmdPool = VK_NULL_HANDLE;
	m_renderPass = VK_NULL_HANDLE;
	m_pipeline = VK_NULL_HANDLE;
	m_pipelineCache = VK_NULL_HANDLE;
	m_descriptorSetLayout = VK_NULL_HANDLE;
	m_pipelineLayout = VK_NULL_HANDLE;
	m_descriptorSet = VK_NULL_HANDLE;
	m_descriptorPool = VK_NULL_HANDLE;
	m_colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
}
WForwardRenderer::~WForwardRenderer() {
}

WError WForwardRenderer::Initiailize() {
	Cleanup();

	m_device = m_app->GetVulkanDevice();
	m_queue = m_app->GetVulkanGraphicsQeueue();
	VkPhysicalDevice physDev = m_app->GetVulkanPhysicalDevice();

	// Store properties (including limits) and features of the phyiscal device
	vkGetPhysicalDeviceProperties(physDev, &m_deviceProperties);
	vkGetPhysicalDeviceFeatures(physDev, &m_deviceFeatures);
	// Gather physical device memory properties
	vkGetPhysicalDeviceMemoryProperties(physDev, &m_deviceMemoryProperties);

	// Find a suitable depth format
	VkBool32 validDepthFormat = vkTools::getSupportedDepthFormat(physDev, &m_depthFormat);
	if (!m_depthFormat)
		return WError(W_HARDWARENOTSUPPORTED);

	m_swapChain.connect(m_app->GetVulkanInstance(), physDev, m_device);
	if (!m_swapChain.initSurface(
		m_app->WindowComponent->GetPlatformHandle(),
		m_app->WindowComponent->GetWindowHandle()))
		return WError(W_UNABLETOCREATESWAPCHAIN);
	m_swapchainInitialized = true;

	VkResult err = _SetupSemaphores();
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = m_swapChain.queueNodeIndex;
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	err = vkCreateCommandPool(m_device, &cmdPoolInfo, nullptr, &m_cmdPool);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	m_renderCmdBuffer = g_CreateCommandBuffer(m_device, m_cmdPool);
	if (m_renderCmdBuffer == VK_NULL_HANDLE)
		return WError(W_OUTOFMEMORY);

	//
	// Beginning of setup commands session
	//
	if (_BeginSetupCommands() != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	err = _SetupSwapchain();
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	err = _CreateTriangle();
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	err = _CreatePipeline();
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	if (_EndSetupCommands() != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);
	//
	// ===================================
	//

	return WError(W_SUCCEEDED);
}

WError WForwardRenderer::Render() {
	vkDeviceWaitIdle(m_device);

	// Get next image in the swap chain (back/front buffer)
	uint32_t currentBuffer;
	VkResult err = m_swapChain.acquireNextImage(m_semaphores.presentComplete, &currentBuffer);
	assert(!err);

	//submitPostPresentBarrier(m_swapChain.buffers[currentBuffer].image);

	VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

	VkClearValue clearValues[2];
	clearValues[0].color = m_clearColor;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = m_renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_width;
	renderPassBeginInfo.renderArea.extent.height = m_height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	// Set target frame buffer
	renderPassBeginInfo.framebuffer = m_frameBuffers[currentBuffer];

	err = vkResetCommandBuffer(m_renderCmdBuffer, 0);
	assert(!err);

	err = vkBeginCommandBuffer(m_renderCmdBuffer, &cmdBufInfo);
	assert(!err);

	vkCmdBeginRenderPass(m_renderCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = vkTools::initializers::viewport(
		(float)m_width,
		(float)m_height,
		0.0f,
		1.0f);
	vkCmdSetViewport(m_renderCmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = vkTools::initializers::rect2D(
		m_width,
		m_height,
		0,
		0);
	vkCmdSetScissor(m_renderCmdBuffer, 0, 1, &scissor);

	// Bind descriptor sets describing shader binding points
	vkCmdBindDescriptorSets(m_renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, NULL);

	vkCmdBindPipeline(m_renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	/* DRAW HERE */
	// Bind triangle vertices
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(m_renderCmdBuffer, 0/*VERTEX_BUFFER_BIND_ID*/, 1, &m_vertices.buf, offsets);

	// Bind triangle indices
	vkCmdBindIndexBuffer(m_renderCmdBuffer, m_indices.buf, 0, VK_INDEX_TYPE_UINT32);

	// Draw indexed triangle
	vkCmdDrawIndexed(m_renderCmdBuffer, m_indices.count, 1, 0, 0, 1);
	/* ********* */

	vkCmdEndRenderPass(m_renderCmdBuffer);

	err = vkEndCommandBuffer(m_renderCmdBuffer);
	assert(!err);

	// Command buffer to be sumitted to the queue
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_semaphores.presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_semaphores.renderComplete;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_renderCmdBuffer;

	// Submit to queue
	err = vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
	assert(!err);

	//submitPrePresentBarrier(m_swapChain.buffers[currentBuffer].image);

	err = m_swapChain.queuePresent(m_queue, currentBuffer, m_semaphores.renderComplete);
	assert(!err);

	vkDeviceWaitIdle(m_device);

	return WError(W_SUCCEEDED);
}

void WForwardRenderer::Cleanup() {
	if (m_descriptorPool)
		vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
	if (m_descriptorSetLayout)
		vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
	if (m_pipelineLayout)
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
	if (m_renderPass)
		vkDestroyRenderPass(m_device, m_renderPass, nullptr);
	if (m_pipeline)
		vkDestroyPipeline(m_device, m_pipeline, nullptr);
	if (m_pipelineCache)
		vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);

	if (m_semaphores.presentComplete)
		vkDestroySemaphore(m_device, m_semaphores.presentComplete, nullptr);
	if (m_semaphores.renderComplete)
		vkDestroySemaphore(m_device, m_semaphores.renderComplete, nullptr);

	g_FreeCommandBuffer(m_device, m_cmdPool, &m_setupCmdBuffer);
	g_FreeCommandBuffer(m_device, m_cmdPool, &m_renderCmdBuffer);
	if (m_cmdPool)
		vkDestroyCommandPool(m_device, m_cmdPool, nullptr);

	for (uint32_t i = 0; i < m_frameBuffers.size(); i++)
		vkDestroyFramebuffer(m_device, m_frameBuffers[i], nullptr);
	m_frameBuffers.clear();
	if (m_swapchainInitialized)
		m_swapChain.cleanup();
	m_swapchainInitialized = false;
}

void WForwardRenderer::SetClearColor(WColor  col) {
	m_clearColor = { { col.r, col.g, col.b, col.a } };
}

void WForwardRenderer::_GetMemoryType(uint32_t typeBits, VkFlags properties, uint32_t * typeIndex) {
	for (uint32_t i = 0; i < 32; i++) {
		if ((typeBits & 1) == 1) {
			if ((m_deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				*typeIndex = i;
				return;
			}
		}
		typeBits >>= 1;
	}
}

VkResult WForwardRenderer::_BeginSetupCommands() {
	g_FreeCommandBuffer(m_device, m_cmdPool, &m_setupCmdBuffer);

	m_setupCmdBuffer = g_CreateCommandBuffer(m_device, m_cmdPool);
	if (m_setupCmdBuffer == VK_NULL_HANDLE)
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkResult err = vkBeginCommandBuffer(m_setupCmdBuffer, &cmdBufInfo);
	if (err != VK_SUCCESS) {
		g_FreeCommandBuffer(m_device, m_cmdPool, &m_setupCmdBuffer);
		return err;
	}

	return err;
}

VkResult WForwardRenderer::_EndSetupCommands() {
	VkResult err;

	if (m_setupCmdBuffer == VK_NULL_HANDLE)
		return VK_SUCCESS;

	err = vkEndCommandBuffer(m_setupCmdBuffer);
	if (err != VK_SUCCESS)
		return err;

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_setupCmdBuffer;

	err = vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
	if (err != VK_SUCCESS)
		return err;

	err = vkQueueWaitIdle(m_queue);
	if (err != VK_SUCCESS)
		return err;

	g_FreeCommandBuffer(m_device, m_cmdPool, &m_setupCmdBuffer);

	return err;
}

VkResult WForwardRenderer::_SetupSemaphores() {
	// Create synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkTools::initializers::semaphoreCreateInfo();
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queu
	VkResult err = vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_semaphores.presentComplete);
	if (err != VK_SUCCESS)
		return err;
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	err = vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_semaphores.renderComplete);
	if (err != VK_SUCCESS)
		return err;

	return VK_SUCCESS;
}

VkResult WForwardRenderer::_SetupSwapchain() {
	//
	// Setup the swap chain
	//
	m_width = m_app->WindowComponent->GetWindowWidth();
	m_height = m_app->WindowComponent->GetWindowHeight();
	m_swapChain.create(m_setupCmdBuffer, &m_width, &m_height);

	//
	// Create the render pass
	//
	VkAttachmentDescription attachments[2] = {};

	// Color attachment
	attachments[0].format = m_colorFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment
	attachments[1].format = m_depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = &depthReference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = NULL;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = NULL;

	VkResult err;

	err = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
	assert(!err);

	//
	// Create the depth-stencil buffer
	//
	VkImageCreateInfo image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.pNext = NULL;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = m_depthFormat;
	image.extent = { m_width, m_height, 1 };
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image.flags = 0;

	VkMemoryAllocateInfo mem_alloc = {};
	mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc.pNext = NULL;
	mem_alloc.allocationSize = 0;
	mem_alloc.memoryTypeIndex = 0;

	VkImageViewCreateInfo depthStencilView = {};
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.pNext = NULL;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = m_depthFormat;
	depthStencilView.flags = 0;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;

	VkMemoryRequirements memReqs;

	err = vkCreateImage(m_device, &image, nullptr, &m_depthStencil.image);
	assert(!err);
	vkGetImageMemoryRequirements(m_device, m_depthStencil.image, &memReqs);
	mem_alloc.allocationSize = memReqs.size;
	_GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mem_alloc.memoryTypeIndex);
	err = vkAllocateMemory(m_device, &mem_alloc, nullptr, &m_depthStencil.mem);
	assert(!err);

	err = vkBindImageMemory(m_device, m_depthStencil.image, m_depthStencil.mem, 0);
	assert(!err);
	vkTools::setImageLayout(
		m_setupCmdBuffer,
		m_depthStencil.image,
		VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	depthStencilView.image = m_depthStencil.image;
	err = vkCreateImageView(m_device, &depthStencilView, nullptr, &m_depthStencil.view);
	assert(!err);

	//
	// Create the frame buffers
	//
	VkImageView imageViews[2];

	// Depth/Stencil attachment is the same for all frame buffers
	imageViews[1] = m_depthStencil.view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = m_renderPass;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments = imageViews;
	frameBufferCreateInfo.width = m_width;
	frameBufferCreateInfo.height = m_height;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	m_frameBuffers.resize(m_swapChain.imageCount);
	for (uint32_t i = 0; i < m_frameBuffers.size(); i++) {
		imageViews[0] = m_swapChain.buffers[i].view;
		VkResult err = vkCreateFramebuffer(m_device, &frameBufferCreateInfo, nullptr, &m_frameBuffers[i]);
		assert(!err);
	}

	return VK_SUCCESS;
}

VkPipelineShaderStageCreateInfo loadShader(VkDevice device, std::string fileName, VkShaderStageFlagBits stage) {
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.module = vkTools::loadShader(fileName.c_str(), device, stage);
	shaderStage.pName = "main";
	assert(shaderStage.module != NULL);
	//shaderModules.push_back(shaderStage.module); TODO: FREE THIS
	return shaderStage;
}

VkResult WForwardRenderer::_CreateTriangle() {
	struct Vertex {
		float pos[3];
		float col[3];
	};

	// Setup vertices
	std::vector<Vertex> vertexBuffer = {
		{ { 1.0f,  1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },
		{ { -1.0f,  1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
		{ { 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
	};
	int vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);

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
		cmdBufInfo.commandPool = m_cmdPool;
		cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufInfo.commandBufferCount = 1;

		VkCommandBuffer copyCommandBuffer;
		vkTools::checkResult(vkAllocateCommandBuffers(m_device, &cmdBufInfo, &copyCommandBuffer));

		// Vertex buffer
		VkBufferCreateInfo vertexBufferInfo = {};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.size = vertexBufferSize;
		// Buffer is used as the copy source
		vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// Create a host-visible buffer to copy the vertex data to (staging buffer)
		vkTools::checkResult(vkCreateBuffer(m_device, &vertexBufferInfo, nullptr, &stagingBuffers.vertices.buffer));
		vkGetBufferMemoryRequirements(m_device, stagingBuffers.vertices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		_GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
		vkTools::checkResult(vkAllocateMemory(m_device, &memAlloc, nullptr, &stagingBuffers.vertices.memory));
		// Map and copy
		vkTools::checkResult(vkMapMemory(m_device, stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, 0, &data));
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(m_device, stagingBuffers.vertices.memory);
		vkTools::checkResult(vkBindBufferMemory(m_device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0));

		// Create the destination buffer with device only visibility
		// Buffer will be used as a vertex buffer and is the copy destination
		vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vkTools::checkResult(vkCreateBuffer(m_device, &vertexBufferInfo, nullptr, &m_vertices.buf));
		vkGetBufferMemoryRequirements(m_device, m_vertices.buf, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		_GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
		vkTools::checkResult(vkAllocateMemory(m_device, &memAlloc, nullptr, &m_vertices.mem));
		vkTools::checkResult(vkBindBufferMemory(m_device, m_vertices.buf, m_vertices.mem, 0));

		// Index buffer
		// todo : comment
		VkBufferCreateInfo indexbufferInfo = {};
		indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexbufferInfo.size = indexBufferSize;
		indexbufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// Copy index data to a buffer visible to the host (staging buffer)
		vkTools::checkResult(vkCreateBuffer(m_device, &indexbufferInfo, nullptr, &stagingBuffers.indices.buffer));
		vkGetBufferMemoryRequirements(m_device, stagingBuffers.indices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		_GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
		vkTools::checkResult(vkAllocateMemory(m_device, &memAlloc, nullptr, &stagingBuffers.indices.memory));
		vkTools::checkResult(vkMapMemory(m_device, stagingBuffers.indices.memory, 0, indexBufferSize, 0, &data));
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(m_device, stagingBuffers.indices.memory);
		vkTools::checkResult(vkBindBufferMemory(m_device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0));

		// Create destination buffer with device only visibility
		indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vkTools::checkResult(vkCreateBuffer(m_device, &indexbufferInfo, nullptr, &m_indices.buf));
		vkGetBufferMemoryRequirements(m_device, m_indices.buf, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		_GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
		vkTools::checkResult(vkAllocateMemory(m_device, &memAlloc, nullptr, &m_indices.mem));
		vkTools::checkResult(vkBindBufferMemory(m_device, m_indices.buf, m_indices.mem, 0));
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

		vkTools::checkResult(vkQueueSubmit(m_queue, 1, &copySubmitInfo, VK_NULL_HANDLE));
		vkTools::checkResult(vkQueueWaitIdle(m_queue));

		vkFreeCommandBuffers(m_device, m_cmdPool, 1, &copyCommandBuffer);

		// Destroy staging buffers
		vkDestroyBuffer(m_device, stagingBuffers.vertices.buffer, nullptr);
		vkFreeMemory(m_device, stagingBuffers.vertices.memory, nullptr);
		vkDestroyBuffer(m_device, stagingBuffers.indices.buffer, nullptr);
		vkFreeMemory(m_device, stagingBuffers.indices.memory, nullptr);

	// Binding description
	m_vertices.bindingDescriptions.resize(1);
	m_vertices.bindingDescriptions[0].binding = 0; // VERTEX_BUFFER_BIND_ID;
	m_vertices.bindingDescriptions[0].stride = sizeof(Vertex);
	m_vertices.bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Attribute descriptions
	// Describes memory layout and shader attribute locations
	m_vertices.attributeDescriptions.resize(2);
	// Location 0 : Position
	m_vertices.attributeDescriptions[0].binding = 0; // VERTEX_BUFFER_BIND_ID;
	m_vertices.attributeDescriptions[0].location = 0;
	m_vertices.attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertices.attributeDescriptions[0].offset = 0;
	// Location 1 : Color
	m_vertices.attributeDescriptions[1].binding = 0; // VERTEX_BUFFER_BIND_ID;
	m_vertices.attributeDescriptions[1].location = 1;
	m_vertices.attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertices.attributeDescriptions[1].offset = sizeof(float) * 3;

	// Assign to vertex buffer
	m_vertices.inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_vertices.inputState.pNext = NULL;
	m_vertices.inputState.flags = VK_FLAGS_NONE;
	m_vertices.inputState.vertexBindingDescriptionCount = m_vertices.bindingDescriptions.size();
	m_vertices.inputState.pVertexBindingDescriptions = m_vertices.bindingDescriptions.data();
	m_vertices.inputState.vertexAttributeDescriptionCount = m_vertices.attributeDescriptions.size();
	m_vertices.inputState.pVertexAttributeDescriptions = m_vertices.attributeDescriptions.data();

	//
	// Create the uniform buffer
	//
	// Prepare and initialize uniform buffer containing shader uniforms
	// Vertex shader uniform buffer block
	VkBufferCreateInfo bufferInfo = {};
	VkMemoryAllocateInfo uballocInfo = {};
	uballocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	uballocInfo.pNext = NULL;
	uballocInfo.allocationSize = 0;
	uballocInfo.memoryTypeIndex = 0;

	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(m_uboVS);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	// Create a new buffer
	VkResult err = vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_uniformDataVS.buffer);
	assert(!err);
	// Get memory requirements including size, alignment and memory type 
	vkGetBufferMemoryRequirements(m_device, m_uniformDataVS.buffer, &memReqs);
	uballocInfo.allocationSize = memReqs.size;
	// Gets the appropriate memory type for this type of buffer allocation
	// Only memory types that are visible to the host
	_GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &uballocInfo.memoryTypeIndex);
	// Allocate memory for the uniform buffer
	err = vkAllocateMemory(m_device, &uballocInfo, nullptr, &(m_uniformDataVS.memory));
	assert(!err);
	// Bind memory to buffer
	err = vkBindBufferMemory(m_device, m_uniformDataVS.buffer, m_uniformDataVS.memory, 0);
	assert(!err);

	// Store information in the uniform's descriptor
	m_uniformDataVS.descriptor.buffer = m_uniformDataVS.buffer;
	m_uniformDataVS.descriptor.offset = 0;
	m_uniformDataVS.descriptor.range = sizeof(m_uboVS);

	//
	// Update the uniform buffer
	//
	// Update matrices
	m_uboVS.projectionMatrix = (WPerspectiveProjMatrixFOV(60.0f, (float)m_width / (float)m_height, 0.1f, 256.0f));
	m_uboVS.viewMatrix = (WMatrixInverse(WTranslationMatrix(0, 0, -2.5f)));
	m_uboVS.modelMatrix = (WMatrix());

	// Map uniform buffer and update it
	uint8_t *pData;
	err = vkMapMemory(m_device, m_uniformDataVS.memory, 0, sizeof(m_uboVS), 0, (void **)&pData);
	assert(!err);
	memcpy(pData, &m_uboVS, sizeof(m_uboVS));
	vkUnmapMemory(m_device, m_uniformDataVS.memory);
	assert(!err);

	//
	// Create descriptor set layout
	//

	// Setup layout of descriptors used in this example
	// Basically connects the different shader stages to descriptors
	// for binding uniform buffers, image samplers, etc.
	// So every shader binding should map to one descriptor set layout
	// binding

	// Binding 0 : Uniform buffer (Vertex shader)
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBinding.pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
	descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pNext = NULL;
	descriptorLayout.bindingCount = 1;
	descriptorLayout.pBindings = &layoutBinding;

	err = vkCreateDescriptorSetLayout(m_device, &descriptorLayout, NULL, &m_descriptorSetLayout);
	assert(!err);

	// Create the pipeline layout that is used to generate the rendering pipelines that
	// are based on this descriptor set layout
	// In a more complex scenario you would have different pipeline layouts for different
	// descriptor set layouts that could be reused
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext = NULL;
	pPipelineLayoutCreateInfo.setLayoutCount = 1;
	pPipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;

	err = vkCreatePipelineLayout(m_device, &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
	assert(!err);

	//
	// Create descriptor pool
	//
	// We need to tell the API the number of max. requested descriptors per type
	VkDescriptorPoolSize typeCounts[1];
	// This example only uses one descriptor type (uniform buffer) and only
	// requests one descriptor of this type
	typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	typeCounts[0].descriptorCount = 1;
	// For additional types you need to add new entries in the type count list
	// E.g. for two combined image samplers :
	// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// typeCounts[1].descriptorCount = 2;

	// Create the global descriptor pool
	// All descriptors used in this example are allocated from this pool
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = NULL;
	descriptorPoolInfo.poolSizeCount = 1;
	descriptorPoolInfo.pPoolSizes = typeCounts;
	// Set the max. number of sets that can be requested
	// Requesting descriptors beyond maxSets will result in an error
	descriptorPoolInfo.maxSets = 1;

	VkResult vkRes = vkCreateDescriptorPool(m_device, &descriptorPoolInfo, nullptr, &m_descriptorPool);
	assert(!vkRes);

	//
	// Create descriptor set
	//

	// Update descriptor sets determining the shader binding points
	// For every binding point used in a shader there needs to be one
	// descriptor set matching that binding point
	VkWriteDescriptorSet writeDescriptorSet = {};

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_descriptorSetLayout;

	vkRes = vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet);
	assert(!vkRes);

	// Binding 0 : Uniform buffer
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = m_descriptorSet;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet.pBufferInfo = &m_uniformDataVS.descriptor;
	// Binds this uniform buffer to binding point 0
	writeDescriptorSet.dstBinding = 0;

	vkUpdateDescriptorSets(m_device, 1, &writeDescriptorSet, 0, NULL);

	return VK_SUCCESS;
}

VkResult WForwardRenderer::_CreatePipeline() {
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VkResult err = vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
	assert(!err);

	// Create our rendering pipeline used in this example
	// Vulkan uses the concept of rendering pipelines to encapsulate
	// fixed states
	// This replaces OpenGL's huge (and cumbersome) state machine
	// A pipeline is then stored and hashed on the GPU making
	// pipeline changes much faster than having to set dozens of 
	// states
	// In a real world application you'd have dozens of pipelines
	// for every shader set used in a scene
	// Note that there are a few states that are not stored with
	// the pipeline. These are called dynamic states and the 
	// pipeline only stores that they are used with this pipeline,
	// but not their states

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};

	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// The layout used for this pipeline
	pipelineCreateInfo.layout = m_pipelineLayout;
	// Renderpass this pipeline is attached to
	pipelineCreateInfo.renderPass = m_renderPass;

	// Vertex input state
	// Describes the topoloy used with this pipeline
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	// This pipeline renders vertex data as triangle lists
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Rasterization state
	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	// Solid polygon mode
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	// No culling
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.0f;

	// Color blend state
	// Describes blend modes and color masks
	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	// One blend attachment state
	// Blending is not used in this example
	VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
	blendAttachmentState[0].colorWriteMask = 0xf;
	blendAttachmentState[0].blendEnable = VK_FALSE;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = blendAttachmentState;

	// Viewport state
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	// One viewport
	viewportState.viewportCount = 1;
	// One scissor rectangle
	viewportState.scissorCount = 1;

	// Enable dynamic states
	// Describes the dynamic states to be used with this pipeline
	// Dynamic states can be set even after the pipeline has been created
	// So there is no need to create new pipelines just for changing
	// a viewport's dimensions or a scissor box
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	// The dynamic state properties themselves are stored in the command buffer
	std::vector<VkDynamicState> dynamicStateEnables;
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.dynamicStateCount = dynamicStateEnables.size();

	// Depth and stencil state
	// Describes depth and stenctil test and compare ops
	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	// Basic depth compare setup with depth writes and depth test enabled
	// No stencil used 
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
	depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.front = depthStencilState.back;

	// Multi sampling state
	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.pSampleMask = NULL;
	// No multi sampling used in this example
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Load shaders
	// Shaders are loaded from the SPIR-V format, which can be generated from glsl
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
	shaderStages[0] = loadShader(m_device, "shaders/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader(m_device, "shaders/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Assign states
	// Assign pipeline state create information
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pVertexInputState = &m_vertices.inputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.renderPass = m_renderPass;
	pipelineCreateInfo.pDynamicState = &dynamicState;

	// Create rendering pipeline
	err = vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineCreateInfo, nullptr, &m_pipeline);
	assert(!err);

	return VK_SUCCESS;
}
