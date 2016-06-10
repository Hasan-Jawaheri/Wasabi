#include "WForwardRenderer.h"

WForwardRenderer::WForwardRenderer(Wasabi* app) : WRenderer(app) {

}
WForwardRenderer::~WForwardRenderer() {

}

WError WForwardRenderer::Initiailize() {
	/*// Get the graphics queue
	vkGetDeviceQueue(m_vkDevice, graphicsQueueIndex, 0, &queue);

	// Find a suitable depth format
	VkBool32 validDepthFormat = vkTools::getSupportedDepthFormat(m_vkPhysDev, &depthFormat);
	assert(validDepthFormat);

	swapChain.connect(m_vkInstance, m_vkPhysDev, m_vkDevice);

	// Create synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkTools::initializers::semaphoreCreateInfo();
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queu
	err = vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &semaphores.presentComplete);
	assert(!err);
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	err = vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &semaphores.renderComplete);
	assert(!err);

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime
	// Command buffer submission info is set by each example
	submitInfo = vkTools::initializers::submitInfo();
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &semaphores.presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &semaphores.renderComplete;*/

	return WError(W_SUCCEEDED);
}

WError WForwardRenderer::Render() {
	return WError(W_SUCCEEDED);
}

void WForwardRenderer::Cleanup() {

}
