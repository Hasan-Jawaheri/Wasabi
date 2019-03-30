#include "WBufferedFrameBuffer.h"
#include "../Core/WCore.h"

WBufferedFrameBuffer::WBufferedFrameBuffer() {
}

VkResult WBufferedFrameBuffer::CreateForSwapchain(Wasabi* app, uint numBuffers, uint width, uint height, VkRenderPass renderPass, std::vector<VkImageView> swapchainViews, VkFormat depthFormat) {
	VkResult result = VK_SUCCESS;
	VkDevice device = app->GetVulkanDevice();

	result = m_swapchainDepthBuffer.Create(app, numBuffers, width, height, depthFormat, nullptr, W_MEMORY_DEVICE_LOCAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	if (result != VK_SUCCESS)
		return result;
	m_swapchainDepthBuffer.WaitForFullCreation(app);

	std::vector<VkImageView> attachments(2);
	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.renderPass = renderPass;
	frameBufferCreateInfo.attachmentCount = attachments.size();
	frameBufferCreateInfo.pAttachments = attachments.data();
	frameBufferCreateInfo.width = width;
	frameBufferCreateInfo.height = height;
	frameBufferCreateInfo.layers = 1;

	// Create the frame buffer
	for (uint i = 0; i < numBuffers; i++) {
		attachments[0] = swapchainViews[i];
		attachments[1] = m_swapchainDepthBuffer.GetView(app, i);

		VkFramebuffer frameBuffer;
		result = vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffer);
		if (result != VK_SUCCESS)
			break;
		m_frameBuffers.push_back(frameBuffer);
	}

	if (result != VK_SUCCESS)
		Destroy(app);

	return result;
}

VkResult WBufferedFrameBuffer::Create(Wasabi* app, uint numBuffers, uint width, uint height, VkRenderPass renderPass, std::vector<WBufferedImage2D> colorImages, WBufferedImage2D depthImage) {
	VkResult result = VK_SUCCESS;
	VkDevice device = app->GetVulkanDevice();

	std::vector<VkImageView> attachments(colorImages.size() + (depthImage.Valid() ? 1 : 0));
	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.renderPass = renderPass;
	frameBufferCreateInfo.attachmentCount = attachments.size();
	frameBufferCreateInfo.pAttachments = attachments.data();
	frameBufferCreateInfo.width = width;
	frameBufferCreateInfo.height = height;
	frameBufferCreateInfo.layers = 1;

	// Create the frame buffer
	for (uint i = 0; i < numBuffers; i++) {
		for (uint j = 0; j < colorImages.size(); j++)
			attachments[j] = colorImages[j].GetView(app, i);
		if (depthImage.Valid())
			attachments[attachments.size()-1] = depthImage.GetView(app, i);

		VkFramebuffer frameBuffer;
		result = vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffer);
		if (result != VK_SUCCESS)
			break;
		m_frameBuffers.push_back(frameBuffer);
	}

	if (result != VK_SUCCESS)
		Destroy(app);

	return result;
}

void WBufferedFrameBuffer::Destroy(Wasabi* app) {
	VkDevice device = app->GetVulkanDevice();
	for (auto it = m_frameBuffers.begin(); it != m_frameBuffers.end(); it++)
		vkDestroyFramebuffer(device, *it, nullptr);
	m_frameBuffers.clear();

	m_swapchainDepthBuffer.Destroy(app);
}

VkFramebuffer WBufferedFrameBuffer::GetFrameBuffer(uint bufferIndex) {
	return m_frameBuffers[bufferIndex];
}

bool WBufferedFrameBuffer::Valid() const {
	return m_frameBuffers.size() > 0;
}
