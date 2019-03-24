#include "WRenderer.h"
#include "WRenderStage.h"
#include "../Images/WRenderTarget.h"
#include "../WindowAndInput/WWindowAndInputComponent.h"

WRenderer::WRenderer(Wasabi* const app) : m_app(app) {
	m_renderTarget = NULL;
	m_semaphores.presentComplete = VK_NULL_HANDLE;
	m_semaphores.renderComplete = VK_NULL_HANDLE;
}

void WRenderer::_Cleanup() {
	if (m_semaphores.presentComplete)
		vkDestroySemaphore(m_device, m_semaphores.presentComplete, nullptr);
	if (m_semaphores.renderComplete)
		vkDestroySemaphore(m_device, m_semaphores.renderComplete, nullptr);
	m_semaphores.presentComplete = VK_NULL_HANDLE;
	m_semaphores.renderComplete = VK_NULL_HANDLE;

	SetRenderingStages(std::vector<WRenderStage*>({}));

	W_SAFE_REMOVEREF(m_renderTarget);

	Cleanup();
}

WError WRenderer::LoadDependantResources() {
	return W_SUCCEEDED;
}

WError WRenderer::_Initialize() {
	_Cleanup();

	m_device = m_app->GetVulkanDevice();
	m_queue = m_app->GetVulkanGraphicsQeueue();
	VkPhysicalDevice physDev = m_app->GetVulkanPhysicalDevice();

	// Find a suitable depth format
	m_colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	VkBool32 validDepthFormat = vkTools::getSupportedDepthFormat(physDev, &m_depthFormat);
	if (!m_depthFormat)
		return WError(W_HARDWARENOTSUPPORTED);

	m_swapChain = m_app->GetSwapChain();

	// Create synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkTools::initializers::semaphoreCreateInfo();
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queu
	VkResult err = vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_semaphores.presentComplete);
	if (err)
		return WError(W_OUTOFMEMORY);
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	err = vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_semaphores.renderComplete);
	if (err)
		return WError(W_OUTOFMEMORY);

	//
	// Setup swap chain and render target
	//
	m_renderTarget = new WRenderTarget(m_app);
	Resize(m_app->WindowAndInputComponent->GetWindowWidth(), m_app->WindowAndInputComponent->GetWindowHeight());

	return Initiailize();
}

void WRenderer::_Render() {
	vkDeviceWaitIdle(m_device);

	// Get next image in the swap chain (back/front buffer)
	uint currentBuffer;
	VkResult err = m_swapChain->acquireNextImage(m_semaphores.presentComplete, &currentBuffer);
	if (err)
		return;

	m_renderTarget->UseFrameBuffer(currentBuffer);

	WRenderTarget* currentRT = nullptr;
	for (auto it = m_renderStages.begin(); it != m_renderStages.end(); it++) {
		WRenderStage* stage = *it;
		if (stage->m_stageDescription.target != RENDER_STAGE_TARGET_PREVIOUS) {
			if (currentRT)
				currentRT->End();
			if (stage->m_renderTarget)
				currentRT = stage->m_renderTarget;
			else
				currentRT = m_renderTarget;
			WError status = currentRT->Begin();
			if (!status)
				return;
		}
		WError status = stage->Render(this, currentRT, -1);
		if (!status)
			return;
	}
	if (!currentRT) {
		currentRT = m_renderTarget;
		currentRT->Begin();
	}
	currentRT->End(false);

	// Command buffer to be sumitted to the queue
	VkCommandBuffer cmdBuf = m_renderTarget->GetCommnadBuffer();
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_semaphores.presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_semaphores.renderComplete;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;

	// Submit to queue
	if (m_renderTarget->Submit(submitInfo) == W_SUCCEEDED)
		err = m_swapChain->queuePresent(m_queue, currentBuffer, m_semaphores.renderComplete);

	vkDeviceWaitIdle(m_device);
}

WError WRenderer::Resize(uint width, uint height) {
	if (m_width == width && m_height == height)
		return W_SUCCEEDED;

	m_width = width;
	m_height = height;

	//
	// Setup the swap chain
	//
	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vkTools::initializers::commandBufferAllocateInfo(
			m_app->GetCommandPool(),
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1);

	VkCommandBuffer cmdBuf;
	VkResult err = vkAllocateCommandBuffers(m_device, &cmdBufAllocateInfo, &cmdBuf);
	if (err)
		return WError(W_OUTOFMEMORY);

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	err = vkBeginCommandBuffer(cmdBuf, &cmdBufInfo);
	if (err) {
		vkFreeCommandBuffers(m_device, m_app->GetCommandPool(), 1, &cmdBuf);
		return WError(W_ERRORUNK);
	}
	m_swapChain->create(cmdBuf, &width, &height);

	err = vkEndCommandBuffer(cmdBuf);
	if (err) {
		vkFreeCommandBuffers(m_device, m_app->GetCommandPool(), 1, &cmdBuf);
		return WError(W_ERRORUNK);
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;

	err = vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
	if (err) {
		vkFreeCommandBuffers(m_device, m_app->GetCommandPool(), 1, &cmdBuf);
		return WError(W_ERRORUNK);
	}

	err = vkQueueWaitIdle(m_queue);
	if (err) {
		vkFreeCommandBuffers(m_device, m_app->GetCommandPool(), 1, &cmdBuf);
		return WError(W_ERRORUNK);
	}

	vkFreeCommandBuffers(m_device, m_app->GetCommandPool(), 1, &cmdBuf);

	vector<VkImageView> views;
	for (int i = 0; i < m_swapChain->buffers.size(); i++)
		views.push_back(m_swapChain->buffers[i].view);
	m_renderTarget->Create(width, height, views.data(), views.size(), m_colorFormat, m_depthFormat);

	vkQueueWaitIdle(m_queue);
	vkDeviceWaitIdle(m_device);

	for (auto it = m_renderStages.begin(); it != m_renderStages.end(); it++) {
		WError err = (*it)->Resize(width, height);
		if (!err)
			return err;
	}

	return WError(W_SUCCEEDED);
}

WError WRenderer::SetRenderingStages(std::vector<WRenderStage*> stages) {
	for (auto it = m_renderStages.begin(); it != m_renderStages.end(); it++)
		(*it)->Cleanup();
	m_renderStages.clear();

	if (stages.size() > 0) {
		if (stages[0]->m_stageDescription.target == RENDER_STAGE_TARGET_PREVIOUS)
			return WError(W_INVALIDPARAM);

		W_RENDER_STAGE_TARGET lastTarget = RENDER_STAGE_TARGET_PREVIOUS;
		for (auto it = stages.begin(); it != stages.end(); it++)
			if ((*it)->m_stageDescription.target != RENDER_STAGE_TARGET_PREVIOUS)
				lastTarget = (*it)->m_stageDescription.target;
		if (lastTarget != RENDER_STAGE_TARGET_BACK_BUFFER)
			return WError(W_INVALIDPARAM);

		uint w = m_app->WindowAndInputComponent->GetWindowWidth();
		uint h = m_app->WindowAndInputComponent->GetWindowHeight();
		for (uint i = 0; i < stages.size(); i++) {
			m_renderStages.push_back(stages[i]);
			WError err = stages[i]->Initialize(m_renderStages, w, h);
			if (!err) {
				SetRenderingStages(std::vector<WRenderStage*>({}));
				return err;
			}
		}
	}

	return WError(W_SUCCEEDED);
}

WRenderStage* WRenderer::GetRenderStage(std::string stageName) const {
	for (auto it = m_renderStages.begin(); it != m_renderStages.end(); it++) {
		if ((*it)->m_stageDescription.name == stageName)
			return *it;
	}
	return nullptr;
}

WRenderTarget* WRenderer::GetRenderTarget(std::string stageName) const {
	if (stageName == "")
		return m_renderTarget;

	WRenderTarget* currentRT = nullptr;
	for (auto it = m_renderStages.begin(); it != m_renderStages.end(); it++) {
		WRenderStage* stage = *it;
		if (stage->m_stageDescription.target != RENDER_STAGE_TARGET_PREVIOUS) {
			if (stage->m_renderTarget)
				currentRT = stage->m_renderTarget;
			else
				currentRT = m_renderTarget;
			if (stage->m_stageDescription.name == stageName)
				return currentRT;
		}
	}

	return currentRT;
}

VkQueue WRenderer::GetQueue() const {
	return m_queue;
}
