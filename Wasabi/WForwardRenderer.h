#pragma once

#include "Wasabi.h"
#include "WMath.h"

class WForwardRenderer : public WRenderer {
	VkDevice							m_device;
	VkQueue								m_queue;
	VkPhysicalDeviceProperties			m_deviceProperties;
	VkPhysicalDeviceFeatures			m_deviceFeatures;
	VkPhysicalDeviceMemoryProperties	m_deviceMemoryProperties;
	VulkanSwapChain						m_swapChain;
	bool								m_swapchainInitialized;
	std::vector<VkFramebuffer>			m_frameBuffers;

	VkFormat							m_depthFormat, m_colorFormat;
	VkRenderPass						m_renderPass;
	VkPipeline							m_pipeline;
	VkPipelineCache						m_pipelineCache;
	VkDescriptorSetLayout				m_descriptorSetLayout;
	VkDescriptorPool					m_descriptorPool;
	VkDescriptorSet						m_descriptorSet;
	VkPipelineLayout					m_pipelineLayout;
	struct {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} m_depthStencil;
	struct {
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo descriptor;
	}  m_uniformDataVS;

	struct {
		WMatrix projectionMatrix;
		WMatrix modelMatrix;
		WMatrix viewMatrix;
	} m_uboVS;

	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} m_vertices;

	struct {
		int count;
		VkBuffer buf;
		VkDeviceMemory mem;
	} m_indices;

	// Synchronization semaphores
	struct {
		VkSemaphore presentComplete; // Swap chain image presentation
		VkSemaphore renderComplete; // Command buffer submission and execution
	} m_semaphores;

	VkCommandPool						m_cmdPool; // Command buffer pool
	VkCommandBuffer						m_setupCmdBuffer, m_renderCmdBuffer;

	uint32_t							m_width, m_height;
	VkClearColorValue					m_clearColor;

	VkResult _BeginSetupCommands();
	VkResult _EndSetupCommands();
	VkResult _SetupSemaphores();
	VkResult _SetupSwapchain();
	VkResult _CreateTriangle();
	VkResult _CreatePipeline();
	void     _GetMemoryType(uint32_t typeBits, VkFlags properties, uint32_t * typeIndex);

public:
	WForwardRenderer(Wasabi* app);
	~WForwardRenderer();

	virtual WError		Initiailize();
	virtual WError		Render();
	virtual void		Cleanup();

	virtual void		SetClearColor(WColor col);
};
