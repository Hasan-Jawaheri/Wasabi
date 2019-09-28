#include "Wasabi/Images/WRenderTarget.h"
#include "Wasabi/Images/WImage.h"
#include "Wasabi/Cameras/WCamera.h"
#include "Wasabi/Renderers/WRenderer.h"

WRenderTargetManager::WRenderTargetManager(Wasabi* const app) : WManager<WRenderTarget>(app) {
}

WRenderTargetManager::~WRenderTargetManager() {

}

WRenderTarget* WRenderTargetManager::CreateRenderTarget(uint32_t ID) {
	WRenderTarget* rt = new WRenderTarget(m_app, ID);
	rt->m_haveCommandBuffer = false;
	return rt;
}

WRenderTarget* WRenderTargetManager::CreateImmediateRenderTarget(uint32_t ID) {
	WRenderTarget* rt = new WRenderTarget(m_app, ID);
	rt->m_haveCommandBuffer = true;
	return rt;
}

std::string WRenderTargetManager::GetTypeName(void) const {
	return "RenderTarget";
}

WRenderTarget::WRenderTarget(Wasabi* const app, uint32_t ID) : WBase(app, ID) {
	m_camera = m_app->CameraManager->GetDefaultCamera();
	if (m_camera)
		m_camera->AddReference();

	m_depthTarget = nullptr;
	m_renderCmdBuffer = VK_NULL_HANDLE;
	m_renderPass = VK_NULL_HANDLE;
	m_pipelineCache = VK_NULL_HANDLE;

	m_app->RenderTargetManager->AddEntity(this);
}

WRenderTarget::~WRenderTarget() {
	_DestroyResources();

	m_app->RenderTargetManager->RemoveEntity(this);
}

std::string WRenderTarget::_GetTypeName() {
	return "RenderTarget";
}

std::string WRenderTarget::GetTypeName() const {
	return _GetTypeName();
}

bool WRenderTarget::Valid() const {
	return m_bufferedFrameBuffer.Valid();
}

void WRenderTarget::_DestroyResources() {
	m_app->MemoryManager->ReleaseRenderPass(m_renderPass, m_app->GetCurrentBufferingIndex());
	m_app->MemoryManager->ReleasePipelineCache(m_pipelineCache, m_app->GetCurrentBufferingIndex());
	m_app->MemoryManager->ReleaseCommandBuffer(m_renderCmdBuffer, m_app->GetCurrentBufferingIndex());
	m_bufferedFrameBuffer.Destroy(m_app);

	if (m_depthTarget)
		W_SAFE_REMOVEREF(m_depthTarget);
	for (auto it = m_targets.begin(); it != m_targets.end(); it++)
		W_SAFE_REMOVEREF((*it));
	m_targets.clear();
}

WError WRenderTarget::Create(uint32_t width, uint32_t height, WImage* target, WImage* depth) {
	return Create(width, height, vector<WImage*>({ target }), depth);
}

WError WRenderTarget::Create(uint32_t width, uint32_t height, vector<class WImage*> targets, WImage* depth) {
	if ((!targets.size() && (!depth || !depth->Valid())) || (depth && !depth->Valid()))
		return WError(W_INVALIDPARAM);
	for (auto it = targets.begin(); it != targets.end(); it++)
		if (!(*it) || !(*it)->Valid())
			return WError(W_INVALIDPARAM);

	VkDevice device = m_app->GetVulkanDevice();
	VkResult err = VK_SUCCESS;

	_DestroyResources();

	if (m_haveCommandBuffer) {
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vkTools::initializers::commandBufferAllocateInfo(
				m_app->MemoryManager->GetCommandPool(),
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);

		err = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &m_renderCmdBuffer);
		if (err != VK_SUCCESS) {
			_DestroyResources();
			return WError(W_OUTOFMEMORY);
		}
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
	vector<VkAttachmentDescription> attachmentDescs;
	VkAttachmentDescription attachment = {};
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Color attachments
	for (auto it = targets.begin(); it != targets.end(); it++) {
		attachment.format = (*it)->GetFormat();
		attachmentDescs.push_back(attachment);
	}

	if (depth) {
		// Depth attachment
		attachment.format = depth->GetFormat();
		attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachmentDescs.push_back(attachment);
	}

	vector<VkAttachmentReference> colorReferences;
	for (auto it = targets.begin(); it != targets.end(); it++) {
		VkAttachmentReference attachmentRef = {};
		attachmentRef.attachment = (uint32_t)colorReferences.size();
		attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorReferences.push_back(attachmentRef);
	}

	VkAttachmentReference depthReference = {};
	if (depth) {
		depthReference.attachment = (uint32_t)targets.size();
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = (uint32_t)colorReferences.size();
	subpass.pColorAttachments = colorReferences.data();
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = depth ? &depthReference : NULL;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = NULL;
	renderPassInfo.attachmentCount = (uint32_t)attachmentDescs.size();
	renderPassInfo.pAttachments = attachmentDescs.data();
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
	// Create the frame buffers
	//
	std::vector<WBufferedImage> colorImages;
	WBufferedImage depthImage;
	for (auto it = targets.begin(); it != targets.end(); it++)
		colorImages.push_back((*it)->m_bufferedImage);
	if (depth)
		depthImage = depth->m_bufferedImage;
	uint32_t numBuffers = m_app->GetEngineParam<uint32_t>("bufferingCount");
	m_bufferedFrameBuffer.Create(m_app, numBuffers, width, height, m_renderPass, colorImages, depthImage);


	m_width = width;
	m_height = height;
	m_depthTarget = depth;

	if (depth)
		depth->AddReference();
	for (auto it = targets.begin(); it != targets.end(); it++) {
		m_targets.push_back(*it);
		(*it)->AddReference();
	}

	m_clearValues.resize(colorImages.size() + (depth ? 1 : 0));
	for (uint32_t i = 0; i < colorImages.size(); i++) {
		SetClearColor(WColor(0.425f, 0.425f, 0.425f), i);
	}
	if (depth)
		SetClearColor(WColor(1, 0, 0), (uint32_t)colorImages.size());

	return WError(W_SUCCEEDED);
}

WError WRenderTarget::Create(uint32_t width, uint32_t height, VkImageView* views,
							 uint32_t numViews, VkFormat colorFormat, VkFormat depthFormat) {
	_DestroyResources();

	VkDevice device = m_app->GetVulkanDevice();
	VkResult err = VK_SUCCESS;

	if (m_haveCommandBuffer) {
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vkTools::initializers::commandBufferAllocateInfo(
				m_app->MemoryManager->GetCommandPool(),
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);

		err = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &m_renderCmdBuffer);
		if (err != VK_SUCCESS) {
			_DestroyResources();
			return WError(W_OUTOFMEMORY);
		}
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
	renderPassInfo.attachmentCount = sizeof(attachments)/sizeof(VkAttachmentDescription);
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

	std::vector<VkImageView> swapchainViewsVector(numViews);
	for (uint32_t i = 0; i < numViews; i++)
		swapchainViewsVector[i] = views[i];
	uint32_t numBuffers = m_app->GetEngineParam<uint32_t>("bufferingCount");
	m_bufferedFrameBuffer.CreateForSwapchain(m_app, numBuffers, width, height, m_renderPass, swapchainViewsVector, depthFormat);

	m_width = width;
	m_height = height;

	m_clearValues.resize(2);
	SetClearColor(WColor(0.425f, 0.425f, 0.425f, 0.0f), 0);
	SetClearColor(WColor(1.0f, 0.0f, 0, 0.0f), 1);

	return WError(W_SUCCEEDED);
}

WError WRenderTarget::Begin() {
	if (m_renderCmdBuffer != VK_NULL_HANDLE) {
		VkResult err = vkResetCommandBuffer(m_renderCmdBuffer, 0);
		if (err)
			return WError(W_ERRORUNK);

		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();
		err = vkBeginCommandBuffer(m_renderCmdBuffer, &cmdBufInfo);
		if (err)
			return WError(W_ERRORUNK);
	}

	for (auto imgTarget : m_targets) {
		if (imgTarget->GetViewLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			imgTarget->TransitionLayoutTo(GetCommnadBuffer(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}
	if (m_depthTarget && m_depthTarget->GetViewLayout() != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		m_depthTarget->TransitionLayoutTo(GetCommnadBuffer(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = m_renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_width;
	renderPassBeginInfo.renderArea.extent.height = m_height;
	renderPassBeginInfo.clearValueCount = (uint32_t)m_clearValues.size();
	renderPassBeginInfo.pClearValues = m_clearValues.data();

	// Set target frame buffer
	uint32_t bufferIndex = m_app->GetCurrentBufferingIndex();
	renderPassBeginInfo.framebuffer = m_bufferedFrameBuffer.GetFrameBuffer(bufferIndex);

	vkCmdBeginRenderPass(GetCommnadBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = vkTools::initializers::viewport(
		(float)m_width,
		(float)m_height,
		0.0f,
		1.0f);
	vkCmdSetViewport(GetCommnadBuffer(), 0, 1, &viewport);

	VkRect2D scissor = vkTools::initializers::rect2D(
		m_width,
		m_height,
		0,
		0);
	vkCmdSetScissor(GetCommnadBuffer(), 0, 1, &scissor);

	m_camera->Render(m_width, m_height);

	return WError(W_SUCCEEDED);
}

WError WRenderTarget::End(bool bSubmit) {
	vkCmdEndRenderPass(GetCommnadBuffer());

	for (auto imgTarget : m_targets) {
		imgTarget->TransitionLayoutTo(GetCommnadBuffer(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	if (m_depthTarget)
		m_depthTarget->TransitionLayoutTo(GetCommnadBuffer(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	if (m_renderCmdBuffer != VK_NULL_HANDLE) {
		VkResult err = vkEndCommandBuffer(m_renderCmdBuffer);
		if (err)
			return WError(W_ERRORUNK);

		if (bSubmit)
			return Submit();
	}

	return WError(W_SUCCEEDED);
}

WError WRenderTarget::Submit(VkSubmitInfo custom_info) {
	if (!custom_info.pCommandBuffers) {
		if (m_renderCmdBuffer == VK_NULL_HANDLE)
			return WError(W_NOTVALID);

		VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		memset(&custom_info, 0, sizeof(VkSubmitInfo));
		custom_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		custom_info.pWaitDstStageMask = &submitPipelineStages;
		custom_info.waitSemaphoreCount = 0;
		custom_info.pWaitSemaphores = nullptr;
		custom_info.signalSemaphoreCount = 0;
		custom_info.pSignalSemaphores = nullptr;
		custom_info.commandBufferCount = 1;
		custom_info.pCommandBuffers = &m_renderCmdBuffer;
	}

	// Submit to queue
	VkResult err = vkQueueSubmit(m_app->Renderer->GetQueue(), 1, &custom_info, VK_NULL_HANDLE);
	if (err)
		return WError(W_ERRORUNK);

	return WError(W_SUCCEEDED);
}

void WRenderTarget::SetClearColor(WColor col, uint32_t index) {
	if (index >= m_clearValues.size())
		return;

	if (HasDepthOutput() && index == m_clearValues.size() - 1)
		m_clearValues[index].depthStencil = { col.r, (uint32_t)col.g };
	else
		m_clearValues[index].color = { { col.r, col.g, col.b, col.a } };
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
	if (m_renderCmdBuffer != VK_NULL_HANDLE)
		return m_renderCmdBuffer;
	return m_app->Renderer->GetCurrentPrimaryCommandBuffer();
}

int WRenderTarget::GetNumColorOutputs() const {
	return !Valid() ? 0 : (int)(m_targets.size() == 0 ? 1 : m_targets.size());
}

bool WRenderTarget::HasDepthOutput() const {
	return m_depthTarget != nullptr || (Valid() && m_targets.size() == 0);
}

WCamera* WRenderTarget::GetCamera() const {
	return m_camera;
}
