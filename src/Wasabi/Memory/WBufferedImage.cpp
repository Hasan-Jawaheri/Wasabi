#include "Wasabi/Memory/WBufferedImage.h"
#include "Wasabi/Core/WCore.h"
namespace {
	extern std::unordered_map<VkFormat, std::pair<int, int>> g_formatSizes;
};

WBufferedImage::WBufferedImage() {
	m_lastMapFlags = W_MAP_UNDEFINED;
	m_readOnlyMemory = nullptr;
	m_bufferSize = 0;
}

VkResult WBufferedImage::Create(Wasabi* app, uint32_t numBuffers, uint32_t width, uint32_t height, uint32_t depth, WBufferedImageProperties properties, void* pixels) {
	VkDevice device = app->GetVulkanDevice();
	Destroy(app);

	VkResult result = VK_SUCCESS;
	VkMemoryPropertyFlags imageMemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // device local means only GPU can access it, more efficient
	properties.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	m_aspect =
		(properties.format == VK_FORMAT_D16_UNORM || properties.format == VK_FORMAT_X8_D24_UNORM_PACK32 || properties.format == VK_FORMAT_D32_SFLOAT)
		? VK_IMAGE_ASPECT_DEPTH_BIT
		: (properties.format == VK_FORMAT_S8_UINT
			? VK_IMAGE_ASPECT_STENCIL_BIT
			: ((properties.format == VK_FORMAT_D16_UNORM_S8_UINT || properties.format == VK_FORMAT_D24_UNORM_S8_UINT || properties.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
				? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)
				: VK_IMAGE_ASPECT_COLOR_BIT));
	m_properties = properties;
	m_width = width;
	m_height = height;
	m_depth = depth;
	std::pair<int, int> pixelSize = g_formatSizes[properties.format];
	m_bufferSize = (pixelSize.second/8) * width * height * depth * properties.arraySize;

	WVulkanBuffer stagingBuffer;
	for (uint32_t i = 0; i < numBuffers; i++) {
		//
		// Create the image as a destination of a copy, from the staging buffer to this image
		// unless no staging buffer was created, then we don't need to perform any transfer.
		//
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = properties.type;
		imageCreateInfo.format = properties.format;
		imageCreateInfo.mipLevels = properties.mipLevels;
		imageCreateInfo.arrayLayers = properties.arraySize;
		imageCreateInfo.samples = properties.sampleCount;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		imageCreateInfo.extent = { width, height, depth };
		imageCreateInfo.usage = m_properties.usage;

		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_1D;
		switch (properties.type) {
		case VK_IMAGE_TYPE_1D: viewType = imageCreateInfo.arrayLayers == 1 ? VK_IMAGE_VIEW_TYPE_1D : VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			break;
		case VK_IMAGE_TYPE_2D: viewType = imageCreateInfo.arrayLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			break;
		case VK_IMAGE_TYPE_3D: viewType = imageCreateInfo.arrayLayers == 1 ? VK_IMAGE_VIEW_TYPE_3D: VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			break;
		default:
			break;
		}

		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		// imageViewCreateInfo.image = image; <--- will be automatically set
		imageViewCreateInfo.viewType = viewType;
		imageViewCreateInfo.format = imageCreateInfo.format;
		imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		imageViewCreateInfo.subresourceRange.aspectMask = m_aspect;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = imageCreateInfo.arrayLayers;
		imageViewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;

		WVulkanImage image;
		result = image.Create(app, imageCreateInfo, imageMemoryFlags, imageViewCreateInfo);
		if (result != VK_SUCCESS)
			break;
		m_images.push_back(image);
		m_layouts.push_back(imageCreateInfo.initialLayout);

		//
		// Create a host-visible staging buffer that contains the raw image data.
		// we will later copy that buffer's contents into the newly created image.
		// In a dynamic image, the staging buffer will be used throughout to map
		// and unmap.
		//
		VkBufferCreateInfo stagingBufferCreateInfo = {};
		stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferCreateInfo.size = m_bufferSize;
		stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // This buffer is used as a transfer source for the buffer copy
		stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		result = stagingBuffer.Create(app, stagingBufferCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		if (result != VK_SUCCESS)
			break;

		if (pixels) {
			void* pStagingMem;
			result = vkMapMemory(device, stagingBuffer.mem, 0, m_bufferSize, 0, &pStagingMem);
			if (result != VK_SUCCESS)
				break;
			memcpy(pStagingMem, pixels, m_bufferSize);
			vkUnmapMemory(device, stagingBuffer.mem);
		}

		//
		// Now copy the contents of the staging buffer into the image memory
		// If the image is dynamic, then this staging buffer will persist for
		// as long as the image does. Otherwise, we will need to delete it.
		//
		result = CopyStagingToImage(app, stagingBuffer, image, m_layouts[m_layouts.size()-1]);
		if (result != VK_SUCCESS)
			break;

		if (properties.memory != W_MEMORY_HOST_VISIBLE)
			stagingBuffer.Destroy(app);
		else
			m_stagingBuffers.push_back(stagingBuffer);

		if (properties.memory == W_MEMORY_DEVICE_LOCAL_HOST_COPY) {
			m_readOnlyMemory = W_SAFE_ALLOC(m_bufferSize);
			memcpy(m_readOnlyMemory, pixels, m_bufferSize);
		}
	}

	if (result != VK_SUCCESS) {
		stagingBuffer.Destroy(app);
		Destroy(app);
	}

	return result;
}

VkResult WBufferedImage::CopyStagingToImage(Wasabi* app, WVulkanBuffer& buffer, WVulkanImage& image, VkImageLayout& initialLayout) {
	VkImageLayout targetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	if (!(m_properties.usage & VK_IMAGE_USAGE_SAMPLED_BIT)) {
		if (m_properties.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		else if (m_properties.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		else
			targetLayout = VK_IMAGE_LAYOUT_GENERAL;
	}

	VkResult result = app->MemoryManager->BeginCopyCommandBuffer();
	if (result == VK_SUCCESS) {
		VkCommandBuffer copyCmdBuffer = app->MemoryManager->GetCopyCommandBuffer();

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = m_aspect;
		subresourceRange.levelCount = m_properties.mipLevels;
		subresourceRange.layerCount = m_properties.arraySize;

		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		vkTools::setImageLayout(
			copyCmdBuffer,
			image.img,
			m_aspect,
			initialLayout == VK_IMAGE_LAYOUT_PREINITIALIZED ? initialLayout : VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange
		);

		// Setup buffer copy regions for each mip level
		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = m_aspect;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = m_properties.arraySize;
		bufferCopyRegion.imageExtent.width = m_width;
		bufferCopyRegion.imageExtent.height = m_height;
		bufferCopyRegion.imageExtent.depth = m_depth;
		bufferCopyRegion.bufferOffset = 0;

		if (m_aspect & VK_IMAGE_ASPECT_DEPTH_BIT && m_aspect & VK_IMAGE_ASPECT_STENCIL_BIT)
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		// Copy mip levels from staging buffer
		vkCmdCopyBufferToImage(
			copyCmdBuffer,
			buffer.buf,
			image.img,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion
		);

		// Change image layout to shader read after all mip levels have been copied
		vkTools::setImageLayout(
			copyCmdBuffer,
			image.img,
			m_aspect,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			targetLayout,
			subresourceRange
		);

		result = app->MemoryManager->EndCopyCommandBuffer(true);

		initialLayout = targetLayout;
	}

	return result;
}

void WBufferedImage::Destroy(Wasabi* app) {
	for (auto it = m_images.begin(); it != m_images.end(); it++)
		it->Destroy(app);
	for (auto it = m_stagingBuffers.begin(); it != m_stagingBuffers.end(); it++)
		it->Destroy(app);
	m_images.clear();
	m_stagingBuffers.clear();
	m_bufferSize = 0;
	W_SAFE_FREE(m_readOnlyMemory);
}

VkResult WBufferedImage::Map(Wasabi* app, uint32_t bufferIndex, void** pixels, W_MAP_FLAGS flags) {
	VkResult result = VK_RESULT_MAX_ENUM;
	if (m_lastMapFlags == W_MAP_UNDEFINED && flags != W_MAP_UNDEFINED) {
		if (m_readOnlyMemory) {
			if (flags != W_MAP_READ)
				return result;
			*pixels = m_readOnlyMemory;
			result = VK_SUCCESS;
		} else if (m_stagingBuffers.size() > 0) {
			VkDevice device = app->GetVulkanDevice();
			result = vkMapMemory(device, m_stagingBuffers[bufferIndex].mem, 0, m_bufferSize, 0, pixels);
			if (result == VK_SUCCESS)
				m_lastMapFlags = flags;
		} else {
			VkDevice device = app->GetVulkanDevice();
			result = vkMapMemory(device, m_images[bufferIndex].mem, 0, m_bufferSize, 0, pixels);
			if (result == VK_SUCCESS)
				m_lastMapFlags = flags;
		}
	}
	return result;
}

void WBufferedImage::Unmap(Wasabi* app, uint32_t bufferIndex) {
	if (m_lastMapFlags != W_MAP_UNDEFINED) {
		if (m_readOnlyMemory) {
		} else if (m_stagingBuffers.size() > 0) {
			VkDevice device = app->GetVulkanDevice();
			vkUnmapMemory(device, m_stagingBuffers[bufferIndex].mem);
			CopyStagingToImage(app, m_stagingBuffers[bufferIndex], m_images[bufferIndex], m_layouts[bufferIndex]);
		} else {
			VkDevice device = app->GetVulkanDevice();
			vkUnmapMemory(device, m_images[bufferIndex].mem);
		}

		m_lastMapFlags = W_MAP_UNDEFINED;
	}
}

VkImageView WBufferedImage::GetView(Wasabi* app, uint32_t bufferIndex) const {
	UNREFERENCED_PARAMETER(app);

	bufferIndex = bufferIndex % m_images.size();
	return m_images[bufferIndex].view;
}

VkImageLayout WBufferedImage::GetLayout(uint32_t bufferIndex) const {
	bufferIndex = bufferIndex % m_images.size();
	return m_layouts[bufferIndex];
}

void WBufferedImage::TransitionLayoutTo(VkCommandBuffer cmdBuf, VkImageLayout newLayout, uint32_t bufferIndex) {
	bufferIndex = bufferIndex % m_images.size();
	vkTools::setImageLayout(
		cmdBuf,
		m_images[bufferIndex].img,
		m_aspect,
		m_layouts[bufferIndex],
		newLayout);
	m_layouts[bufferIndex] = newLayout;
}

bool WBufferedImage::Valid() const {
	return m_bufferSize > 0;
}

size_t WBufferedImage::GetMemorySize() const {
	return m_bufferSize;
}

uint32_t WBufferedImage::GetWidth() const {
	return m_width;
}

uint32_t WBufferedImage::GetHeight() const {
	return m_height;
}

uint32_t WBufferedImage::GetDepth() const {
	return m_depth;
}

uint32_t WBufferedImage::GetArraySize() const {
	return m_properties.arraySize;
}

namespace {
	std::unordered_map<VkFormat, std::pair<int, int>> g_formatSizes = {
		{VK_FORMAT_R4G4_UNORM_PACK8, std::make_pair(2, 8)},
		{VK_FORMAT_R4G4B4A4_UNORM_PACK16, std::make_pair(4, 16)},
		{VK_FORMAT_B4G4R4A4_UNORM_PACK16, std::make_pair(4, 16)},
		{VK_FORMAT_R5G6B5_UNORM_PACK16, std::make_pair(3, 16)},
		{VK_FORMAT_B5G6R5_UNORM_PACK16, std::make_pair(3, 16)},
		{VK_FORMAT_R5G5B5A1_UNORM_PACK16, std::make_pair(4, 16)},
		{VK_FORMAT_B5G5R5A1_UNORM_PACK16, std::make_pair(4, 16)},
		{VK_FORMAT_A1R5G5B5_UNORM_PACK16, std::make_pair(4, 16)},
		{VK_FORMAT_R8_UNORM, std::make_pair(1, 8)},
		{VK_FORMAT_R8_SNORM, std::make_pair(1, 8)},
		{VK_FORMAT_R8_USCALED, std::make_pair(1, 8)},
		{VK_FORMAT_R8_SSCALED, std::make_pair(1, 8)},
		{VK_FORMAT_R8_UINT, std::make_pair(1, 8)},
		{VK_FORMAT_R8_SINT, std::make_pair(1, 8)},
		{VK_FORMAT_R8_SRGB, std::make_pair(1, 8)},
		{VK_FORMAT_R8G8_UNORM, std::make_pair(2, 16)},
		{VK_FORMAT_R8G8_SNORM, std::make_pair(2, 16)},
		{VK_FORMAT_R8G8_USCALED, std::make_pair(2, 16)},
		{VK_FORMAT_R8G8_SSCALED, std::make_pair(2, 16)},
		{VK_FORMAT_R8G8_UINT, std::make_pair(2, 16)},
		{VK_FORMAT_R8G8_SINT, std::make_pair(2, 16)},
		{VK_FORMAT_R8G8_SRGB, std::make_pair(2, 16)},
		{VK_FORMAT_R8G8B8_UNORM, std::make_pair(3, 24)},
		{VK_FORMAT_R8G8B8_SNORM, std::make_pair(3, 24)},
		{VK_FORMAT_R8G8B8_USCALED, std::make_pair(3, 24)},
		{VK_FORMAT_R8G8B8_SSCALED, std::make_pair(3, 24)},
		{VK_FORMAT_R8G8B8_UINT, std::make_pair(3, 24)},
		{VK_FORMAT_R8G8B8_SINT, std::make_pair(3, 24)},
		{VK_FORMAT_R8G8B8_SRGB, std::make_pair(3, 24)},
		{VK_FORMAT_B8G8R8_UNORM, std::make_pair(3, 24)},
		{VK_FORMAT_B8G8R8_SNORM, std::make_pair(3, 24)},
		{VK_FORMAT_B8G8R8_USCALED, std::make_pair(3, 24)},
		{VK_FORMAT_B8G8R8_SSCALED, std::make_pair(3, 24)},
		{VK_FORMAT_B8G8R8_UINT, std::make_pair(3, 24)},
		{VK_FORMAT_B8G8R8_SINT, std::make_pair(3, 24)},
		{VK_FORMAT_B8G8R8_SRGB, std::make_pair(3, 24)},
		{VK_FORMAT_R8G8B8A8_UNORM, std::make_pair(4, 32)},
		{VK_FORMAT_R8G8B8A8_SNORM, std::make_pair(4, 32)},
		{VK_FORMAT_R8G8B8A8_USCALED, std::make_pair(4, 32)},
		{VK_FORMAT_R8G8B8A8_SSCALED, std::make_pair(4, 32)},
		{VK_FORMAT_R8G8B8A8_UINT, std::make_pair(4, 32)},
		{VK_FORMAT_R8G8B8A8_SINT, std::make_pair(4, 32)},
		{VK_FORMAT_R8G8B8A8_SRGB, std::make_pair(4, 32)},
		{VK_FORMAT_B8G8R8A8_UNORM, std::make_pair(4, 32)},
		{VK_FORMAT_B8G8R8A8_SNORM, std::make_pair(4, 32)},
		{VK_FORMAT_B8G8R8A8_USCALED, std::make_pair(4, 32)},
		{VK_FORMAT_B8G8R8A8_SSCALED, std::make_pair(4, 32)},
		{VK_FORMAT_B8G8R8A8_UINT, std::make_pair(4, 32)},
		{VK_FORMAT_B8G8R8A8_SINT, std::make_pair(4, 32)},
		{VK_FORMAT_B8G8R8A8_SRGB, std::make_pair(4, 32)},
		{VK_FORMAT_A8B8G8R8_UNORM_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A8B8G8R8_SNORM_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A8B8G8R8_USCALED_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A8B8G8R8_SSCALED_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A8B8G8R8_UINT_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A8B8G8R8_SINT_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A8B8G8R8_SRGB_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A2R10G10B10_UNORM_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A2R10G10B10_SNORM_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A2R10G10B10_USCALED_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A2R10G10B10_SSCALED_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A2R10G10B10_UINT_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A2R10G10B10_SINT_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A2B10G10R10_UNORM_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A2B10G10R10_SNORM_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A2B10G10R10_USCALED_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A2B10G10R10_SSCALED_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A2B10G10R10_UINT_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_A2B10G10R10_SINT_PACK32, std::make_pair(4, 32)},
		{VK_FORMAT_R16_UNORM, std::make_pair(1, 16)},
		{VK_FORMAT_R16_SNORM, std::make_pair(1, 16)},
		{VK_FORMAT_R16_USCALED, std::make_pair(1, 16)},
		{VK_FORMAT_R16_SSCALED, std::make_pair(1, 16)},
		{VK_FORMAT_R16_UINT, std::make_pair(1, 16)},
		{VK_FORMAT_R16_SINT, std::make_pair(1, 16)},
		{VK_FORMAT_R16_SFLOAT, std::make_pair(1, 16)},
		{VK_FORMAT_R16G16_UNORM, std::make_pair(2, 32)},
		{VK_FORMAT_R16G16_SNORM, std::make_pair(2, 32)},
		{VK_FORMAT_R16G16_USCALED, std::make_pair(2, 32)},
		{VK_FORMAT_R16G16_SSCALED, std::make_pair(2, 32)},
		{VK_FORMAT_R16G16_UINT, std::make_pair(2, 32)},
		{VK_FORMAT_R16G16_SINT, std::make_pair(2, 32)},
		{VK_FORMAT_R16G16_SFLOAT, std::make_pair(2, 32)},
		{VK_FORMAT_R16G16B16_UNORM, std::make_pair(3, 48)},
		{VK_FORMAT_R16G16B16_SNORM, std::make_pair(3, 48)},
		{VK_FORMAT_R16G16B16_USCALED, std::make_pair(3, 48)},
		{VK_FORMAT_R16G16B16_SSCALED, std::make_pair(3, 48)},
		{VK_FORMAT_R16G16B16_UINT, std::make_pair(3, 48)},
		{VK_FORMAT_R16G16B16_SINT, std::make_pair(3, 48)},
		{VK_FORMAT_R16G16B16_SFLOAT, std::make_pair(3, 48)},
		{VK_FORMAT_R16G16B16A16_UNORM, std::make_pair(4, 64)},
		{VK_FORMAT_R16G16B16A16_SNORM, std::make_pair(4, 64)},
		{VK_FORMAT_R16G16B16A16_USCALED, std::make_pair(4, 64)},
		{VK_FORMAT_R16G16B16A16_SSCALED, std::make_pair(4, 64)},
		{VK_FORMAT_R16G16B16A16_UINT, std::make_pair(4, 64)},
		{VK_FORMAT_R16G16B16A16_SINT, std::make_pair(4, 64)},
		{VK_FORMAT_R16G16B16A16_SFLOAT, std::make_pair(4, 64)},
		{VK_FORMAT_R32_UINT, std::make_pair(1, 32)},
		{VK_FORMAT_R32_SINT, std::make_pair(1, 32)},
		{VK_FORMAT_R32_SFLOAT, std::make_pair(1, 32)},
		{VK_FORMAT_R32G32_UINT, std::make_pair(2, 64)},
		{VK_FORMAT_R32G32_SINT, std::make_pair(2, 64)},
		{VK_FORMAT_R32G32_SFLOAT, std::make_pair(2, 64)},
		{VK_FORMAT_R32G32B32_UINT, std::make_pair(3, 96)},
		{VK_FORMAT_R32G32B32_SINT, std::make_pair(3, 96)},
		{VK_FORMAT_R32G32B32_SFLOAT, std::make_pair(3, 96)},
		{VK_FORMAT_R32G32B32A32_UINT, std::make_pair(4, 128)},
		{VK_FORMAT_R32G32B32A32_SINT, std::make_pair(4, 128)},
		{VK_FORMAT_R32G32B32A32_SFLOAT, std::make_pair(4, 128)},
		{VK_FORMAT_R64_UINT, std::make_pair(1, 64)},
		{VK_FORMAT_R64_SINT, std::make_pair(1, 64)},
		{VK_FORMAT_R64_SFLOAT, std::make_pair(1, 64)},
		{VK_FORMAT_R64G64_UINT, std::make_pair(2, 128)},
		{VK_FORMAT_R64G64_SINT, std::make_pair(2, 128)},
		{VK_FORMAT_R64G64_SFLOAT, std::make_pair(2, 128)},
		{VK_FORMAT_R64G64B64_UINT, std::make_pair(3, 192)},
		{VK_FORMAT_R64G64B64_SINT, std::make_pair(3, 192)},
		{VK_FORMAT_R64G64B64_SFLOAT, std::make_pair(3, 192)},
		{VK_FORMAT_R64G64B64A64_UINT, std::make_pair(4, 256)},
		{VK_FORMAT_R64G64B64A64_SINT, std::make_pair(4, 256)},
		{VK_FORMAT_R64G64B64A64_SFLOAT, std::make_pair(4, 256)},
		{VK_FORMAT_B10G11R11_UFLOAT_PACK32, std::make_pair(3, 32)},
		{VK_FORMAT_E5B9G9R9_UFLOAT_PACK32, std::make_pair(3, 32)},
		{VK_FORMAT_D16_UNORM, std::make_pair(1, 16)},
		{VK_FORMAT_X8_D24_UNORM_PACK32, std::make_pair(2, 32)},
		{VK_FORMAT_D32_SFLOAT, std::make_pair(1, 32)},
		{VK_FORMAT_S8_UINT, std::make_pair(1, 8)},
		{VK_FORMAT_D16_UNORM_S8_UINT, std::make_pair(2, 24)},
		{VK_FORMAT_D24_UNORM_S8_UINT, std::make_pair(2, 32)},
		{VK_FORMAT_D32_SFLOAT_S8_UINT, std::make_pair(1, 32)},
		{VK_FORMAT_BC1_RGB_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC1_RGB_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC1_RGBA_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC1_RGBA_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC2_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC2_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC3_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC3_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC4_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC4_SNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC5_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC5_SNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC6H_UFLOAT_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC6H_SFLOAT_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC7_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_BC7_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_EAC_R11_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_EAC_R11_SNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_EAC_R11G11_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_EAC_R11G11_SNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_4x4_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_4x4_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_5x4_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_5x4_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_5x5_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_5x5_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_6x5_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_6x5_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_6x6_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_6x6_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_8x5_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_8x5_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_8x6_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_8x6_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_8x8_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_8x8_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_10x5_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_10x5_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_10x6_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_10x6_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_10x8_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_10x8_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_10x10_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_10x10_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_12x10_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_12x10_SRGB_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_12x12_UNORM_BLOCK, std::make_pair(1, 0)},
		{VK_FORMAT_ASTC_12x12_SRGB_BLOCK, std::make_pair(1, 0)},
	};
};
