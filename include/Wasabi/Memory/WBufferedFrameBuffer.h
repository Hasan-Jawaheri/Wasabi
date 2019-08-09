#pragma once

#include "Core/WCommon.h"
#include "Memory/WBufferedImage.h"

class WBufferedFrameBuffer {
public:
	WBufferedFrameBuffer();

	VkResult CreateForSwapchain(class Wasabi* app, uint numBuffers, uint width, uint height, VkRenderPass renderPass, std::vector<VkImageView> swapchainViews, VkFormat depthFormat);
	VkResult Create(class Wasabi* app, uint numBuffers, uint width, uint height, VkRenderPass renderPass, std::vector<WBufferedImage> colorImages, WBufferedImage depthImage = WBufferedImage());
	void Destroy(class Wasabi* app);

	VkFramebuffer GetFrameBuffer(uint bufferIndex);

	bool Valid() const;

private:
	std::vector<VkFramebuffer> m_frameBuffers;
	WBufferedImage m_swapchainDepthBuffer;
};
