#include "WBufferedImage.h"
#include "../Core/WCore.h"

WBufferedImage2D::WBufferedImage2D() {
	m_lastMapFlags = W_MAP_UNDEFINED;
	m_readOnlyMemory = nullptr;
	m_bufferSize = 0;
}

VkResult WBufferedImage2D::Create(Wasabi* app, uint numBuffers, uint width, uint height, VkFormat format, void* texels, W_MEMORY_STORAGE memory, VkImageUsageFlags imageUsage) {
	VkDevice device = app->GetVulkanDevice();
	Destroy(app);

	VkResult result = VK_SUCCESS;

	m_aspect = format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
		format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_S8_UINT ||
		format == VK_FORMAT_D16_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT
		? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

	WVulkanBuffer stagingBuffer;
	for (uint i = 0; i < numBuffers; i++) {
		VkMemoryPropertyFlags imageMemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // device local means only GPU can access it, more efficient
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		if (texels && (memory == W_MEMORY_DEVICE_LOCAL || memory == W_MEMORY_DEVICE_LOCAL_HOST_COPY)) {
			// the image usage is now also a transfer destination
			imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		} else if (memory == W_MEMORY_HOST_VISIBLE) {
			// don't use a staging buffer for dynamic images, instead, just make them
			// host-visible so the CPU can directly access them
			imageMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}

		//
		// Create the image as a destination of a copy, from the staging buffer to this image
		// unless no staging buffer was created, then we don't need to perform any transfer.
		//
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = 1; // TODO: USE m_app->engineParams["numGeneratedMips"]
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.usage = imageUsage;

		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		// imageViewCreateInfo.image = image; <--- will be automatically set
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = format;
		imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		imageViewCreateInfo.subresourceRange.aspectMask = m_aspect;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;

		WVulkanImage image;
		result = image.Create(app, imageCreateInfo, imageMemoryFlags, imageViewCreateInfo);
		if (result != VK_SUCCESS)
			break;
		m_images.push_back(image);

		VkMemoryRequirements memReqs = {};
		vkGetImageMemoryRequirements(device, image.img, &memReqs);
		m_bufferSize = memReqs.size;

		//
		// Create a host-visible staging buffer that contains the raw image data.
		// we will later copy that buffer's contents into the newly created image.
		// If the resource is not dynamic and no initial data is provided, we won't
		// need to perform the staging operation.
		//
		if (texels && (memory == W_MEMORY_DEVICE_LOCAL || memory == W_MEMORY_DEVICE_LOCAL_HOST_COPY)) {
			VkBufferCreateInfo stagingBufferCreateInfo = {};
			stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			stagingBufferCreateInfo.size = m_bufferSize;
			stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // This buffer is used as a transfer source for the buffer copy
			stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			result = stagingBuffer.Create(app, stagingBufferCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			if (result != VK_SUCCESS)
				break;

			void* pStagingMem;
			result = vkMapMemory(device, stagingBuffer.mem, 0, m_bufferSize, 0, &pStagingMem);
			if (result != VK_SUCCESS)
				break;
			memcpy(pStagingMem, texels, m_bufferSize);
			vkUnmapMemory(device, stagingBuffer.mem);
		}

		//
		// Now copy the contents of the staging buffer into the image memory
		// If the image is dynamic, then this staging buffer will persist for
		// as long as the image does. Otherwise, we will need to delete it.
		// But we cannot delete it until the copy operation is complete, so
		// we will assign a fence to the submission of the copy command which
		// will be signalled when the copy is complete, then we will delete
		// the staging buffer later on in a call to GetView()
		//
		if (stagingBuffer.buf) {
			// Setup buffer copy regions for each mip level
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = m_aspect;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width;
			bufferCopyRegion.imageExtent.height = height;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = 0;

			result = app->MemoryManager->BeginCopyCommandBuffer();
			if (result != VK_SUCCESS)
				break;

			VkCommandBuffer copyCmdBuffer = app->MemoryManager->GetCopyCommandBuffer();

			// Image barrier for optimal image (target)
			// Optimal image will be used as destination for the copy
			vkTools::setImageLayout(
				copyCmdBuffer,
				image.img,
				m_aspect,
				imageCreateInfo.initialLayout,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			// Copy mip levels from staging buffer
			vkCmdCopyBufferToImage(
				copyCmdBuffer,
				stagingBuffer.buf,
				image.img,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&bufferCopyRegion
			);

			// Change texture image layout to shader read after all mip levels have been copied
			vkTools::setImageLayout(
				copyCmdBuffer,
				image.img,
				m_aspect,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				imageLayout);

			result = app->MemoryManager->EndCopyCommandBuffer(true);
			if (result != VK_SUCCESS)
				break;

			if (memory == W_MEMORY_DEVICE_LOCAL_HOST_COPY) {
				m_readOnlyMemory = W_SAFE_ALLOC(m_bufferSize);
				memcpy(m_readOnlyMemory, texels, m_bufferSize);
			}
		} else if (texels) {
			// this is a dynamic image with initialization info, map/unmap to initialize
			void* pMemTexels;
			result = Map(app, i, &pMemTexels, W_MAP_WRITE);
			if (result != VK_SUCCESS)
				break;
			memcpy(pMemTexels, texels, m_bufferSize);
			Unmap(app, i);
		}
	}

	if (result != VK_SUCCESS) {
		stagingBuffer.Destroy(app);
		Destroy(app);
	}

	return result;
}

void WBufferedImage2D::Destroy(Wasabi* app) {
	VkDevice device = app->GetVulkanDevice();
	for (auto it = m_images.begin(); it != m_images.end(); it++)
		it->Destroy(app);
	m_images.clear();
	m_bufferSize = 0;
	W_SAFE_FREE(m_readOnlyMemory);
}

/*VkResult WBufferedImage2D::Flush(class Wasabi* app, uint bufferIndex) {
	VkDevice device = app->GetVulkanDevice();
	VkMappedMemoryRange memRange = {};
	memRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memRange.offset = 0;
	memRange.size = m_bufferSize;
	memRange.memory = m_images[bufferIndex].mem;
	VkResult result = vkFlushMappedMemoryRanges(device, 1, &memRange);
	if (result != VK_SUCCESS)
		return result;

	/ *result = app->MemoryManager->BeginCopyCommandBuffer();
	if (result != VK_SUCCESS)
		return result;

	VkCommandBuffer copyCmdBuffer = app->MemoryManager->GetCopyCommandBuffer();

	vkTools::setImageLayout(
		copyCmdBuffer,
		m_images[bufferIndex].img,
		m_aspect,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	return app->MemoryManager->EndCopyCommandBuffer(false);* /
	return result;
}

VkResult WBufferedImage2D::Invalidate(class Wasabi* app, uint bufferIndex) {
	VkDevice device = app->GetVulkanDevice();
	VkMappedMemoryRange memRange = {};
	memRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memRange.offset = 0;
	memRange.size = m_bufferSize;
	memRange.memory = m_images[bufferIndex].mem;
	return vkInvalidateMappedMemoryRanges(device, 1, &memRange);
}*/

VkResult WBufferedImage2D::Map(Wasabi* app, uint bufferIndex, void** texels, W_MAP_FLAGS flags) {
	VkResult result = VK_RESULT_MAX_ENUM;
	if (m_lastMapFlags == W_MAP_UNDEFINED && flags != W_MAP_UNDEFINED) {
		if (m_readOnlyMemory) {
			if (flags != W_MAP_READ)
				return result;
			*texels = m_readOnlyMemory;
			return VK_SUCCESS;
		}

		VkDevice device = app->GetVulkanDevice();
		/*if (flags & W_MAP_READ) {
			result = Invalidate(app, bufferIndex);
			if (result != VK_SUCCESS)
				return result;
		}*/
		result = vkMapMemory(device, m_images[bufferIndex].mem, 0, m_bufferSize, 0, texels);
		if (result == VK_SUCCESS)
			m_lastMapFlags = flags;
	}
	return result;
}

void WBufferedImage2D::Unmap(Wasabi* app, uint bufferIndex) {
	if (m_lastMapFlags != W_MAP_UNDEFINED) {
		if (!m_readOnlyMemory) {
			VkDevice device = app->GetVulkanDevice();
			/*if (m_lastMapFlags & W_MAP_WRITE) {
				Flush(app, bufferIndex);
			}*/

			vkUnmapMemory(device, m_images[bufferIndex].mem);
		}

		m_lastMapFlags = W_MAP_UNDEFINED;
	}
}

VkImageView WBufferedImage2D::GetView(Wasabi* app, uint bufferIndex) const {
	bufferIndex = bufferIndex % m_images.size();
	return m_images[bufferIndex].view;
}

bool WBufferedImage2D::Valid() const {
	return m_bufferSize > 0;
}

size_t WBufferedImage2D::GetMemorySize() const {
	return m_bufferSize;
}
