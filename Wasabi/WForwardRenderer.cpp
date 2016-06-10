#include "WForwardRenderer.h"

WForwardRenderer::WForwardRenderer(Wasabi* app) : WRenderer(app) {
	m_swapchainInitialized = false;
}
WForwardRenderer::~WForwardRenderer() {
}

VkResult WForwardRenderer::BeginSetupCommands() {
	if (m_setupCmdBuffer != VK_NULL_HANDLE) {
		vkFreeCommandBuffers(m_device, m_cmdPool, 1, &m_setupCmdBuffer);
		m_setupCmdBuffer = VK_NULL_HANDLE;
	}

	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vkTools::initializers::commandBufferAllocateInfo(
			m_cmdPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1);

	VkResult err = vkAllocateCommandBuffers(m_device, &cmdBufAllocateInfo, &m_setupCmdBuffer);
	if (err != VK_SUCCESS)
		return err;

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	err = vkBeginCommandBuffer(m_setupCmdBuffer, &cmdBufInfo);
	if (err != VK_SUCCESS)
		return err;

	return err;
}

VkResult WForwardRenderer::EndSetupCommands() {
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

	vkFreeCommandBuffers(m_device, m_cmdPool, 1, &m_setupCmdBuffer);
	m_setupCmdBuffer = VK_NULL_HANDLE;

	return err;
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

	// Create synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkTools::initializers::semaphoreCreateInfo();
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queu
	VkResult err = vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_semaphores.presentComplete);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	err = vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_semaphores.renderComplete);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = m_swapChain.queueNodeIndex;
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	err = vkCreateCommandPool(m_device, &cmdPoolInfo, nullptr, &m_cmdPool);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	//
	// Beginning of setup commands session
	//
	if (BeginSetupCommands() != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	uint32_t width = m_app->WindowComponent->GetWindowWidth();
	uint32_t height = m_app->WindowComponent->GetWindowHeight();
	m_swapChain.create(m_setupCmdBuffer, &width, &height);

	if (EndSetupCommands() != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);
	//
	// ===================================
	//

	return WError(W_SUCCEEDED);
}

WError WForwardRenderer::Render() {
	return WError(W_SUCCEEDED);
}

void WForwardRenderer::Cleanup() {
	if (m_swapchainInitialized)
		m_swapChain.cleanup();
	m_swapchainInitialized = false;
	if (m_semaphores.presentComplete)
		vkDestroySemaphore(m_device, m_semaphores.presentComplete, nullptr);
	if (m_semaphores.renderComplete)
		vkDestroySemaphore(m_device, m_semaphores.renderComplete, nullptr);
}
