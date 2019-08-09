#include "Renderers/WRenderer.h"
#include "Renderers/WRenderStage.h"
#include "Images/WRenderTarget.h"
#include "Images/WImage.h"
#include "Geometries/WGeometry.h"
#include "WindowAndInput/WWindowAndInputComponent.h"

WRenderer::WRenderer(Wasabi* const app) : m_app(app) {
	m_queue = VK_NULL_HANDLE;
	m_sampler = VK_NULL_HANDLE;
}

void WRenderer::Cleanup() {
	m_app->MemoryManager->ReleaseSampler(m_sampler, m_app->GetCurrentBufferingIndex());
	if (m_queue)
		vkQueueWaitIdle(m_queue);
	m_perBufferResources.Destroy(m_app);
	SetRenderingStages(std::vector<WRenderStage*>({}));
}

VkResult WRenderer::PerBufferResources::Create(Wasabi* app, uint numBuffers) {
	Destroy(app);
	VkDevice device = app->GetVulkanDevice();
	VkResult err = VK_SUCCESS;

	curIndex = 0;

	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vkTools::initializers::commandBufferAllocateInfo(
			app->MemoryManager->GetCommandPool(),
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			numBuffers);

	primaryCommandBuffers.resize(numBuffers);
	err = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, primaryCommandBuffers.data());
	if (err)
		return err;

	VkSemaphoreCreateInfo semaphoreCreateInfo = vkTools::initializers::semaphoreCreateInfo();
	VkFenceCreateInfo fenceCreateInfo = vkTools::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphore sem = VK_NULL_HANDLE;
	VkFence fence = VK_NULL_HANDLE;
	for (uint i = 0; i < numBuffers; i++) {
		err = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &sem);
		if (err)
			break;
		presentComplete.push_back(sem);

		err = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &sem);
		if (err)
			break;
		renderComplete.push_back(sem);

		err = vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);
		if (err)
			break;
		memoryFences.push_back(fence);
	}
	return err;
}

void WRenderer::PerBufferResources::Destroy(Wasabi* app) {
	VkDevice device = app->GetVulkanDevice();
	for (auto it = primaryCommandBuffers.begin(); it != primaryCommandBuffers.end(); it++)
		app->MemoryManager->ReleaseCommandBuffer(*it, app->GetCurrentBufferingIndex());
	primaryCommandBuffers.clear();
	for (auto it = presentComplete.begin(); it != presentComplete.end(); it++)
		app->MemoryManager->ReleaseSemaphore(*it, app->GetCurrentBufferingIndex());
	presentComplete.clear();
	for (auto it = renderComplete.begin(); it != renderComplete.end(); it++)
		app->MemoryManager->ReleaseSemaphore(*it, app->GetCurrentBufferingIndex());
	renderComplete.clear();
	for (auto it = memoryFences.begin(); it != memoryFences.end(); it++)
		app->MemoryManager->ReleaseFence(*it, app->GetCurrentBufferingIndex());
	memoryFences.clear();
}

WError WRenderer::Initialize() {
	Cleanup();

	m_device = m_app->GetVulkanDevice();
	m_queue = m_app->GetVulkanGraphicsQeueue();
	m_swapChain = m_app->GetSwapChain();

	//
	// Create the texture sampler
	//
	VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	// Max level-of-detail should match mip level count
	sampler.maxLod = 1;
	// Enable anisotropic filtering
	sampler.maxAnisotropy = 8;
	sampler.anisotropyEnable = VK_TRUE;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VkResult err = vkCreateSampler(m_device, &sampler, nullptr, &m_sampler);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	//
	// Setup swap chain and render target
	//
	return Resize(m_app->WindowAndInputComponent->GetWindowWidth(), m_app->WindowAndInputComponent->GetWindowHeight());
}

void WRenderer::Render() {
	// wait for the fence to be signalled (by vkQueueSubmit of the last frame that used this buffer index (m_perBufferResources.curIndex))
	VkResult err = vkWaitForFences(m_device, 1, &m_perBufferResources.memoryFences[m_perBufferResources.curIndex], VK_TRUE, -1);
	if (err == VK_SUCCESS) {
		err = vkResetFences(m_device, 1, &m_perBufferResources.memoryFences[m_perBufferResources.curIndex]);
	}
	if (err != VK_SUCCESS)
		return; // fence is not ready yet or can't be reset

	// allow the memory manager to free any resources pending on this frame, now that the fence is signalled
	m_app->MemoryManager->ReleaseFrameResources(m_perBufferResources.curIndex);

	m_app->ImageManager->UpdateDynamicImages(m_perBufferResources.curIndex);
	m_app->GeometryManager->UpdateDynamicGeometries(m_perBufferResources.curIndex);

	err = vkResetCommandBuffer(m_perBufferResources.primaryCommandBuffers[m_perBufferResources.curIndex], 0);
	if (err)
		return;

	VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();
	err = vkBeginCommandBuffer(m_perBufferResources.primaryCommandBuffers[m_perBufferResources.curIndex], &cmdBufInfo);
	if (err)
		return;

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.layerCount = 1;
	subresourceRange.levelCount = 1;
	VkImageMemoryBarrier presentImageBarrier = {};
	presentImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	presentImageBarrier.image = m_swapChain->buffers[m_perBufferResources.curIndex].image;
	presentImageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	presentImageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	presentImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	presentImageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	presentImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	presentImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	presentImageBarrier.subresourceRange = subresourceRange;

	vkCmdPipelineBarrier(
		m_perBufferResources.primaryCommandBuffers[m_perBufferResources.curIndex],
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &presentImageBarrier
	);

	WRenderTarget* currentRT = nullptr;
	for (auto it = m_renderStages.begin(); it != m_renderStages.end(); it++) {
		WRenderStage* stage = *it;
		if (stage->m_stageDescription.target != RENDER_STAGE_TARGET_PREVIOUS) {
			if (currentRT)
				currentRT->End();
			currentRT = stage->m_renderTarget;
			WError status = currentRT->Begin();
			if (!status)
				return;
		}
		WError status = stage->Render(this, currentRT, -1);
		if (!status)
			return;
	}
	currentRT->End();

	presentImageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	presentImageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	vkCmdPipelineBarrier(
		m_perBufferResources.primaryCommandBuffers[m_perBufferResources.curIndex],
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &presentImageBarrier
	);

	err = vkEndCommandBuffer(m_perBufferResources.primaryCommandBuffers[m_perBufferResources.curIndex]);
	if (err)
		return;

	// Get next image in the swap chain (back/front buffer)
	uint currentSwapchainIndex;
	err = m_swapChain->acquireNextImage(m_perBufferResources.presentComplete[m_perBufferResources.curIndex], &currentSwapchainIndex);
	if (err)
		return;

	// Command buffer to be sumitted to the queue
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_perBufferResources.presentComplete[m_perBufferResources.curIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_perBufferResources.renderComplete[m_perBufferResources.curIndex];
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_perBufferResources.primaryCommandBuffers[m_perBufferResources.curIndex];

	// Submit to queue
	if (vkQueueSubmit(m_queue, 1, &submitInfo, m_perBufferResources.memoryFences[m_perBufferResources.curIndex]) == VK_SUCCESS)
		err = m_swapChain->queuePresent(m_queue, currentSwapchainIndex, m_perBufferResources.renderComplete[m_perBufferResources.curIndex]);

	// increment the current semaphores index (round-robin) for the next frame
	m_perBufferResources.curIndex = (m_perBufferResources.curIndex + 1) % m_perBufferResources.presentComplete.size();
}

WError WRenderer::Resize(uint width, uint height) {
	if (m_width == width && m_height == height)
		return W_SUCCEEDED;

	m_width = width;
	m_height = height;

	//
	// Setup the swap chain
	// Allocate a command buffer and record the creation of the swap chain
	// Then submit that command buffer and wait on the queue to finish
	//
	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vkTools::initializers::commandBufferAllocateInfo(
			m_app->MemoryManager->GetCommandPool(),
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1);

	VkCommandBuffer cmdBuf;
	VkResult err = vkAllocateCommandBuffers(m_device, &cmdBufAllocateInfo, &cmdBuf);
	if (err)
		return WError(W_OUTOFMEMORY);

	vkDeviceWaitIdle(m_device);

	// begin command buffer
	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	err = vkBeginCommandBuffer(cmdBuf, &cmdBufInfo);
	if (!err) {
		// record swapchain creation commands
		m_swapChain->create(cmdBuf, &m_width, &m_height, (uint)m_app->engineParams["bufferingCount"]);

		// end command buffer
		err = vkEndCommandBuffer(cmdBuf);
		if (!err) {
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmdBuf;

			// submit command buffer and wait for the queue to finish executing it
			err = vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(m_queue);
		}
	}
	m_app->MemoryManager->ReleaseAllResources(m_swapChain->imageCount); // reset the buffering count and release all resources
	m_app->MemoryManager->ReleaseCommandBuffer(cmdBuf, 0);
	if (err)
		return WError(W_ERRORUNK);

	vkDeviceWaitIdle(m_device);

	// remake our semaphores
	if (m_perBufferResources.Create(m_app, m_swapChain->imageCount))
		return WError(W_ERRORUNK);

	for (auto it = m_renderStages.begin(); it != m_renderStages.end(); it++) {
		WError err = (*it)->Resize(m_width, m_height);
		vkDeviceWaitIdle(m_device);
		if (!err)
			return err;
	}

	return WError(W_SUCCEEDED);
}

WError WRenderer::SetRenderingStages(std::vector<WRenderStage*> stages) {
	for (auto it = m_renderStages.begin(); it != m_renderStages.end(); it++)
		(*it)->Cleanup();
	m_renderStages.clear();
	m_renderStageMap.clear();

	m_spritesRenderStageName = "";
	m_textsRenderStageName = "";
	m_pickingRenderStageName = "";
	m_backbufferRenderStageName = "";
	m_particlesRenderStageName = "";

	if (stages.size() > 0) {
		// make sure the input has a sprites rendering stage, texts rendering stage and a picking stage
		uint allFlags = 0;
		for (auto it = stages.begin(); it != stages.end(); it++)
			allFlags |= (*it)->m_stageDescription.flags;

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
			m_renderStageMap.insert(std::make_pair(stages[i]->m_stageDescription.name, stages[i]));
			if (stages[i]->m_stageDescription.flags & RENDER_STAGE_FLAG_SPRITES_RENDER_STAGE)
				m_spritesRenderStageName = stages[i]->m_stageDescription.name;
			if (stages[i]->m_stageDescription.flags & RENDER_STAGE_FLAG_TEXTS_RENDER_STAGE)
				m_textsRenderStageName = stages[i]->m_stageDescription.name;
			if (stages[i]->m_stageDescription.flags & RENDER_STAGE_FLAG_PICKING_RENDER_STAGE)
				m_pickingRenderStageName = stages[i]->m_stageDescription.name;
			if (stages[i]->m_stageDescription.target == RENDER_STAGE_TARGET_BACK_BUFFER)
				m_backbufferRenderStageName = stages[i]->m_stageDescription.name;
			if (stages[i]->m_stageDescription.target == RENDER_STAGE_FLAG_PARTICLES_RENDER_STAGE)
				m_particlesRenderStageName = stages[i]->m_stageDescription.name;

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
	auto it = m_renderStageMap.find(stageName);
	if (it != m_renderStageMap.end())
		return it->second;
	return nullptr;
}

WRenderTarget* WRenderer::GetRenderTarget(std::string stageName) const {
	if (stageName == "")
		stageName = m_backbufferRenderStageName;

	auto it = m_renderStageMap.find(stageName);
	if (it != m_renderStageMap.end())
		return it->second->m_renderTarget;
	return nullptr;
}

std::string WRenderer::GetSpritesRenderStageName() const {
	return m_spritesRenderStageName;
}

std::string WRenderer::GetTextsRenderStageName() const {
	return m_textsRenderStageName;
}

std::string WRenderer::GetParticlesRenderStageName() const {
	return m_particlesRenderStageName;
}

std::string WRenderer::GetPickingRenderStageName() const {
	return m_pickingRenderStageName;
}

WImage* WRenderer::GetRenderTargetImage(std::string imageName) const {
	for (auto it = m_renderStages.begin(); it != m_renderStages.end(); it++) {
		if ((*it)->m_stageDescription.depthOutput.name == imageName)
			return (*it)->m_depthOutput;
		for (uint i = 0; i < (*it)->m_stageDescription.colorOutputs.size(); i++)
			if ((*it)->m_stageDescription.colorOutputs[i].name == imageName)
				return (*it)->m_colorOutputs[i];
	}

	return nullptr;
}

VulkanSwapChain* WRenderer::GetSwapchain() const {
	return m_swapChain;
}

VkCommandBuffer WRenderer::GetCurrentPrimaryCommandBuffer() const {
	return m_perBufferResources.primaryCommandBuffers[m_perBufferResources.curIndex];
}

uint WRenderer::GetCurrentBufferingIndex() const {
	return m_perBufferResources.curIndex;
}

VkQueue WRenderer::GetQueue() const {
	return m_queue;
}

VkSampler WRenderer::GetTextureSampler(W_TEXTURE_SAMPLER_TYPE type) const {
	return m_sampler;
}

