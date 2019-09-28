#pragma once

#include "Wasabi/Core/WCommon.h"
#include "Wasabi/Memory/WBufferedImage.h"

class WBufferedFrameBuffer {
public:
	WBufferedFrameBuffer();

	VkResult CreateForSwapchain(class Wasabi* app, uint32_t numBuffers, uint32_t width, uint32_t height, VkRenderPass renderPass, std::vector<VkImageView> swapchainViews, VkFormat depthFormat);
	VkResult Create(class Wasabi* app, uint32_t numBuffers, uint32_t width, uint32_t height, VkRenderPass renderPass, std::vector<WBufferedImage> colorImages, WBufferedImage depthImage = WBufferedImage());
	void Destroy(class Wasabi* app);

	VkFramebuffer GetFrameBuffer(uint32_t bufferIndex);

	bool Valid() const;

private:
	std::vector<VkFramebuffer> m_frameBuffers;
	WBufferedImage m_swapchainDepthBuffer;
};
