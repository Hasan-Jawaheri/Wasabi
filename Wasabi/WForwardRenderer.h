#pragma once

#include "Wasabi.h"

class WForwardRenderer : public WRenderer {
	VkDevice							m_device;
	VkQueue								m_queue;
	VkPhysicalDeviceProperties			m_deviceProperties;
	VkPhysicalDeviceFeatures			m_deviceFeatures;
	VkPhysicalDeviceMemoryProperties	m_deviceMemoryProperties;
	VulkanSwapChain						m_swapChain;
	bool								m_swapchainInitialized;

	VkFormat							m_depthFormat;

	// Synchronization semaphores
	struct {
		VkSemaphore presentComplete; // Swap chain image presentation
		VkSemaphore renderComplete; // Command buffer submission and execution
	} m_semaphores;

	VkCommandPool						m_cmdPool; // Command buffer pool
	VkCommandBuffer						m_setupCmdBuffer;

	VkResult BeginSetupCommands();
	VkResult EndSetupCommands();

public:
	WForwardRenderer(Wasabi* app);
	~WForwardRenderer();

	virtual WError		Initiailize();
	virtual WError		Render();
	virtual void		Cleanup();
};
