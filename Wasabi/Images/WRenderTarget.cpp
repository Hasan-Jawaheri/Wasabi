#include "WRenderTarget.h"
#include "WImage.h"
#include "../Cameras/WCamera.h"
#include "../Renderers/WRenderer.h"

WRenderTargetManager::WRenderTargetManager(Wasabi* const app) : WManager<WRenderTarget>(app) {
}

WRenderTargetManager::~WRenderTargetManager() {

}

std::string WRenderTargetManager::GetTypeName(void) const {
	return "RenderTarget";
}

WRenderTarget::WRenderTarget(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_depthStencil.view = VK_NULL_HANDLE;
	m_depthStencil.image = VK_NULL_HANDLE;
	m_depthStencil.mem = VK_NULL_HANDLE;
	m_currentFrameBuffer = 0;
	m_camera = m_app->CameraManager->GetDefaultCamera();

	m_clearColor = { { 0.425f, 0.425f, 0.425f, 1.0f } };
	m_renderCmdBuffer = VK_NULL_HANDLE;
	m_renderPass = VK_NULL_HANDLE;
	m_pipelineCache = VK_NULL_HANDLE;

	m_app->RenderTargetManager->AddEntity(this);
}

WRenderTarget::~WRenderTarget() {
	_DestroyResources();

	m_app->RenderTargetManager->RemoveEntity(this);
}

std::string WRenderTarget::GetTypeName() const {
	return "RenderTarget";
}

bool WRenderTarget::Valid() const {
	return m_frameBuffers.size() > 0;
}

void WRenderTarget::_DestroyResources() {
	VkDevice device = m_app->GetVulkanDevice();

	if (m_renderPass)
		vkDestroyRenderPass(device, m_renderPass, nullptr);
	if (m_pipelineCache)
		vkDestroyPipelineCache(device, m_pipelineCache, nullptr);

	if (m_renderCmdBuffer)
		vkFreeCommandBuffers(device, m_app->GetCommandPool(), 1, &m_renderCmdBuffer);

	m_renderCmdBuffer = VK_NULL_HANDLE;
	m_renderPass = VK_NULL_HANDLE;
	m_pipelineCache = VK_NULL_HANDLE;

	if (m_depthStencil.view)
		vkDestroyImageView(device, m_depthStencil.view, nullptr);
	m_depthStencil.view = VK_NULL_HANDLE;
	if (m_depthStencil.image)
		vkDestroyImage(device, m_depthStencil.image, nullptr);
	m_depthStencil.image = VK_NULL_HANDLE;
	if (m_depthStencil.mem)
		vkFreeMemory(device, m_depthStencil.mem, nullptr);
	m_depthStencil.mem = VK_NULL_HANDLE;

	for (uint32_t i = 0; i < m_frameBuffers.size(); i++)
		vkDestroyFramebuffer(device, m_frameBuffers[i], nullptr);
	m_frameBuffers.clear();
	m_currentFrameBuffer = 0;

	m_depthStencil.view = VK_NULL_HANDLE;
	m_depthStencil.image = VK_NULL_HANDLE;
	m_depthStencil.mem = VK_NULL_HANDLE;

	if (m_target)
		W_SAFE_REMOVEREF(m_target);
}

WError WRenderTarget::Create(unsigned int width, unsigned int height, WImage* target, bool bDepth, VkFormat depthFormat) {
	if (!target || !target->Valid())
		return WError(W_INVALIDPARAM);

	VkDevice device = m_app->GetVulkanDevice();

	_DestroyResources();

	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vkTools::initializers::commandBufferAllocateInfo(
			m_app->GetCommandPool(),
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1);

	VkResult err = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &m_renderCmdBuffer);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	// create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	err = vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	//
	// Create the render pass
	//
	VkAttachmentDescription attachments[2] = {};

	// Color attachment
	attachments[0].format = target->GetFormat();
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	if (bDepth) {
		// Depth attachment
		attachments[1].format = depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	if (bDepth) {
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

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

	if (!bDepth) {
		subpass.pDepthStencilAttachment = NULL;
	}

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = NULL;
	renderPassInfo.attachmentCount = bDepth ? 2 : 1;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = NULL;

	err = vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	//
	// Create the depth-stencil buffer
	//
	if (bDepth) {
		VkImageCreateInfo image = {};
		image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image.pNext = NULL;
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = depthFormat;
		image.extent = { width, height, 1 };
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
		depthStencilView.format = depthFormat;
		depthStencilView.flags = 0;
		depthStencilView.subresourceRange = {};
		depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthStencilView.subresourceRange.baseMipLevel = 0;
		depthStencilView.subresourceRange.levelCount = 1;
		depthStencilView.subresourceRange.baseArrayLayer = 0;
		depthStencilView.subresourceRange.layerCount = 1;

		VkMemoryRequirements memReqs;

		err = vkCreateImage(device, &image, nullptr, &m_depthStencil.image);
		if (err != VK_SUCCESS) {
			_DestroyResources();
			return WError(W_UNABLETOCREATEIMAGE);
		}
		vkGetImageMemoryRequirements(device, m_depthStencil.image, &memReqs);
		mem_alloc.allocationSize = memReqs.size;
		m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mem_alloc.memoryTypeIndex);
		err = vkAllocateMemory(device, &mem_alloc, nullptr, &m_depthStencil.mem);
		if (err != VK_SUCCESS) {
			_DestroyResources();
			return WError(W_OUTOFMEMORY);
		}

		err = vkBindImageMemory(device, m_depthStencil.image, m_depthStencil.mem, 0);
		if (err != VK_SUCCESS) {
			_DestroyResources();
			return WError(W_OUTOFMEMORY);
		}

		m_app->BeginCommandBuffer();
		vkTools::setImageLayout(
			m_app->GetCommandBuffer(),
			m_depthStencil.image,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		m_app->EndCommandBuffer();

		depthStencilView.image = m_depthStencil.image;
		err = vkCreateImageView(device, &depthStencilView, nullptr, &m_depthStencil.view);
		if (err != VK_SUCCESS) {
			_DestroyResources();
			return WError(W_OUTOFMEMORY);
		}
	}

	//
	// Create the frame buffers
	//
	VkImageView imageViews[2];

	// Depth/Stencil attachment is the same for all frame buffers
	imageViews[0] = target->GetView();
	imageViews[1] = m_depthStencil.view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = m_renderPass;
	frameBufferCreateInfo.attachmentCount = bDepth ? 2 : 1;
	frameBufferCreateInfo.pAttachments = imageViews;
	frameBufferCreateInfo.width = width;
	frameBufferCreateInfo.height = height;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	m_frameBuffers.resize(1);
	err = vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &m_frameBuffers[0]);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_UNABLETOCREATEBUFFER);
	}

	m_width = width;
	m_height = height;
	m_depthFormat = depthFormat;
	m_colorFormat = target->GetFormat();
	m_target = target;
	target->AddReference();

	return WError(W_SUCCEEDED);
}

WError WRenderTarget::Create(unsigned int width, unsigned int height, VkImageView* views,
							 unsigned int num_views, VkFormat colorFormat, VkFormat depthFormat) {
	VkDevice device = m_app->GetVulkanDevice();

	_DestroyResources();

	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vkTools::initializers::commandBufferAllocateInfo(
			m_app->GetCommandPool(),
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1);

	VkResult err = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &m_renderCmdBuffer);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	// create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	err = vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	//
	// Create the render pass
	//
	VkAttachmentDescription attachments[2] = {};

	// Color attachment
	attachments[0].format = colorFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment
	attachments[1].format = depthFormat;
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

	err = vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	//
	// Create the depth-stencil buffer
	//
	VkImageCreateInfo image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.pNext = NULL;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = depthFormat;
	image.extent = { width, height, 1 };
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
	depthStencilView.format = depthFormat;
	depthStencilView.flags = 0;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;

	VkMemoryRequirements memReqs;

	err = vkCreateImage(device, &image, nullptr, &m_depthStencil.image);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_UNABLETOCREATEIMAGE);
	}
	vkGetImageMemoryRequirements(device, m_depthStencil.image, &memReqs);
	mem_alloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mem_alloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &mem_alloc, nullptr, &m_depthStencil.mem);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	err = vkBindImageMemory(device, m_depthStencil.image, m_depthStencil.mem, 0);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	m_app->BeginCommandBuffer();
	vkTools::setImageLayout(
		m_app->GetCommandBuffer(),
		m_depthStencil.image,
		VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	m_app->EndCommandBuffer();

	depthStencilView.image = m_depthStencil.image;
	err = vkCreateImageView(device, &depthStencilView, nullptr, &m_depthStencil.view);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

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
	frameBufferCreateInfo.width = width;
	frameBufferCreateInfo.height = height;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	m_frameBuffers.resize(num_views);
	for (uint32_t i = 0; i < m_frameBuffers.size(); i++) {
		imageViews[0] = views[i];
		VkResult err = vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &m_frameBuffers[i]);
		if (err != VK_SUCCESS) {
			while (m_frameBuffers.size() > i)
				m_frameBuffers.pop_back();
			_DestroyResources();
			return WError(W_OUTOFMEMORY);
		}
	}

	m_width = width;
	m_height = height;
	m_depthFormat = depthFormat;
	m_colorFormat = colorFormat;

	return WError(W_SUCCEEDED);
}

WError WRenderTarget::UseFrameBuffer(unsigned int index) {
	if (index >= m_frameBuffers.size())
		return WError(W_INVALIDPARAM);

	m_currentFrameBuffer = index;

	return WError(W_SUCCEEDED);
}

WError WRenderTarget::Begin() {
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
	renderPassBeginInfo.clearValueCount = m_depthStencil.image ? 2 : 1;
	renderPassBeginInfo.pClearValues = clearValues;

	// Set target frame buffer
	renderPassBeginInfo.framebuffer = m_frameBuffers[m_currentFrameBuffer];

	// Bind descriptor sets describing shader binding points
	VkResult err = vkResetCommandBuffer(m_renderCmdBuffer, 0);
	if (err)
		return WError(W_ERRORUNK);

	err = vkBeginCommandBuffer(m_renderCmdBuffer, &cmdBufInfo);
	if (err)
		return WError(W_ERRORUNK);

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

	m_camera->Render(m_width, m_height);

	return WError(W_SUCCEEDED);
}

WError WRenderTarget::End(bool bSubmit) {
	vkCmdEndRenderPass(m_renderCmdBuffer);

	VkResult err = vkEndCommandBuffer(m_renderCmdBuffer);
	if (err)
		return WError(W_ERRORUNK);

	if (bSubmit) {
		// Command buffer to be sumitted to the queue
		VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
		submitInfo.pWaitDstStageMask = &submitPipelineStages;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_renderCmdBuffer;

		// Submit to queue
		err = vkQueueSubmit(m_app->Renderer->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		if (err)
			return WError(W_ERRORUNK);
	}

	return WError(W_SUCCEEDED);
}

void WRenderTarget::SetClearColor(WColor  col) {
	m_clearColor = { { col.r, col.g, col.b, col.a } };
}

void WRenderTarget::SetCamera(WCamera* cam) {
	if (m_camera)
		m_camera->RemoveReference();
	m_camera = cam;
	if (cam)
		cam->AddReference();
}

VkRenderPass WRenderTarget::GetRenderPass() const {
	return m_renderPass;
}

VkPipelineCache WRenderTarget::GetPipelineCache() const {
	return m_pipelineCache;
}

VkCommandBuffer WRenderTarget::GetCommnadBuffer() const {
	return m_renderCmdBuffer;
}

WCamera* WRenderTarget::GetCamera() const {
	return m_camera;
}
