#pragma once

#include "Wasabi.h"
#include "WMath.h"

class WForwardRenderer : public WRenderer {
	VkDevice							m_device;
	VkQueue								m_queue;
	VulkanSwapChain						m_swapChain;
	bool								m_swapchainInitialized;
	std::vector<VkFramebuffer>			m_frameBuffers;

	VkRenderPass						m_renderPass;
	VkPipelineCache						m_pipelineCache;

	VkCommandPool						m_cmdPool; // Command buffer pool
	VkCommandBuffer						m_setupCmdBuffer, m_renderCmdBuffer;

	struct {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} m_depthStencil;

	// Synchronization semaphores
	struct {
		VkSemaphore presentComplete; // Swap chain image presentation
		VkSemaphore renderComplete; // Command buffer submission and execution
	} m_semaphores;

	VkFormat							m_depthFormat, m_colorFormat;
	uint32_t							m_width, m_height;
	bool								m_resizing;
	VkClearColorValue					m_clearColor;
	VkSampler							m_sampler;

	VkResult _BeginSetupCommands();
	VkResult _EndSetupCommands();
	VkResult _SetupSemaphores();
	VkResult _SetupSwapchain();

	class WEffect* m_default_fx;

public:
	WForwardRenderer(Wasabi* app);
	~WForwardRenderer();

	virtual WError		Initiailize();
	virtual WError		Render();
	virtual void		Cleanup();
	virtual WError		Resize(unsigned int width, unsigned int height);

	virtual void		SetClearColor(WColor col);

	virtual class WMaterial*CreateDefaultMaterial();
	virtual VkSampler		GetDefaultSampler() const;

	virtual VkQueue			GetQueue() const;
	virtual VkRenderPass	GetRenderPass() const;
	virtual VkPipelineCache	GetPipelineCache() const;
	virtual VkCommandBuffer	GetCommnadBuffer() const;
	virtual VkCommandPool	GetCommandPool() const;
};
