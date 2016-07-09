#include "WImage.h"

#pragma warning(disable:4477)
#pragma warning(disable:4838)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_DEFINE
#include "stb/stb.h"
#pragma warning(default:4477)
#pragma warning(default:4838)

std::string WImageManager::GetTypeName() const {
	return "Image";
}

WImageManager::WImageManager(class Wasabi* const app) : WManager<WImage>(app) {
	m_checker_image = nullptr;
}

void WImageManager::Load() {
	m_checker_image = new WImage(m_app);
	int size = 64;
	int comp_size = 4;
	int check_size = 16;
	float* pixels = new float[size * size * comp_size];
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			int col = ((y/ check_size) + ((x/ check_size) % 2)) % 2;
			if (comp_size > 0)
				pixels[(y*size + x) * comp_size + 0] = col;
			if (comp_size > 1)
				pixels[(y*size + x) * comp_size + 1] = col;
			if (comp_size > 2)
				pixels[(y*size + x) * comp_size + 2] = col;
			if (comp_size > 3)
				pixels[(y*size + x) * comp_size + 3] = 1;
		}
	}
	m_checker_image->CretaeFromPixelsArray(pixels, size, size, comp_size);
	delete[] pixels;
}

WImage* WImageManager::GetDefaultImage() const {
	return m_checker_image;
}

WImage::WImage(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_image = VK_NULL_HANDLE;
	m_deviceMemory = VK_NULL_HANDLE;
	m_view = VK_NULL_HANDLE;

	m_app->ImageManager->AddEntity(this);
}
WImage::~WImage() {
	_DestroyResources();

	m_app->ImageManager->RemoveEntity(this);
}

std::string WImage::GetTypeName() const {
	return "Image";
}

bool WImage::Valid() const {
	return m_view != VK_NULL_HANDLE;
}

void WImage::_DestroyResources() {
	VkDevice device = m_app->GetVulkanDevice();
	if (m_image)
		vkDestroyImage(device, m_image, nullptr);
	if (m_deviceMemory)
		vkFreeMemory(device, m_deviceMemory, nullptr);
	if (m_view)
		vkDestroyImageView(device, m_view, nullptr);
	m_image = VK_NULL_HANDLE;
	m_deviceMemory = VK_NULL_HANDLE;
	m_view = VK_NULL_HANDLE;
}

WError WImage::CretaeFromPixelsArray(
	void*			pixels,
	unsigned int	width,
	unsigned int	height,
	unsigned int	num_components,
	VkFormat fmt,
	size_t comp_size) {
	VkDevice device = m_app->GetVulkanDevice();
	VkResult err;
	VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs = {};
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	VkBufferCopy copyRegion = {};
	VkSubmitInfo copySubmitInfo = {};
	std::vector<VkBufferImageCopy> bufferCopyRegions;
	VkBufferImageCopy bufferCopyRegion = {};
	VkImageCreateInfo imageCreateInfo = vkTools::initializers::imageCreateInfo();
	VkCommandBuffer copyCommandBuffer;
	VkCommandBufferAllocateInfo cmdBufInfo = {};
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	VkBufferCreateInfo bufferCreateInfo = vkTools::initializers::bufferCreateInfo();
	uint8_t *data;

	VkFormat format = fmt;
	if (fmt == VK_FORMAT_UNDEFINED) {
		switch (num_components) {
		case 1: format = VK_FORMAT_R32_SFLOAT; break;
		case 2: format = VK_FORMAT_R32G32_SFLOAT; break;
		case 3: format = VK_FORMAT_R32G32B32_SFLOAT; break;
		case 4: format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
		default:
			return WError(W_INVALIDPARAM);
		}
	}

	_DestroyResources();

	// Create a host-visible staging buffer that contains the raw image data
	bufferCreateInfo.size = width * height * num_components * comp_size;
	// This buffer is used as a transfer source for the buffer copy
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	err = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer);
	if (err) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	// Get memory requirements for the staging buffer (alignment, memory type bits)
	vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;
	// Get memory type index for a host visible buffer
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);

	err = vkAllocateMemory(device, &memAllocInfo, nullptr, &stagingMemory);
	if (err) {
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}
	err = vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);
	if (err) goto free_buffers;

	// Copy texture data into staging buffer
	err = vkMapMemory(device, stagingMemory, 0, memReqs.size, 0, (void **)&data);
	if (err) goto free_buffers;
	memcpy(data, pixels, bufferCreateInfo.size);
	vkUnmapMemory(device, stagingMemory);

	// Setup buffer copy regions for each mip level
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = width;
	bufferCopyRegion.imageExtent.height = height;
	bufferCopyRegion.imageExtent.depth = 1;
	bufferCopyRegion.bufferOffset = 0;

	bufferCopyRegions.push_back(bufferCopyRegion);

	// Create optimal tiled target image
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageCreateInfo.extent = { width, height, 1 };
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	err = vkCreateImage(device, &imageCreateInfo, nullptr, &m_image);
	if (err) goto free_buffers;

	vkGetImageMemoryRequirements(device, m_image, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;

	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
	err = vkAllocateMemory(device, &memAllocInfo, nullptr, &m_deviceMemory);
	if (err) goto free_buffers;
	err = vkBindImageMemory(device, m_image, m_deviceMemory, 0);
	if (err) goto free_buffers;

	// Buffer copies are done on the queue, so we need a command buffer for them
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufInfo.commandPool = m_app->Renderer->GetCommandPool();
	cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufInfo.commandBufferCount = 1;

	err = vkAllocateCommandBuffers(device, &cmdBufInfo, &copyCommandBuffer);
	if (err) goto free_buffers;

	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.pNext = NULL;

	// Put buffer region copies into command buffer
	// Note that the staging buffer must not be deleted before the copies 
	// have been submitted and executed
	err = vkBeginCommandBuffer(copyCommandBuffer, &cmdBufferBeginInfo);
	if (err) goto free_buffers_and_command;

	// Image barrier for optimal image (target)
	// Optimal image will be used as destination for the copy
	vkTools::setImageLayout(
		copyCommandBuffer,
		m_image,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Copy mip levels from staging buffer
	vkCmdCopyBufferToImage(
		copyCommandBuffer,
		stagingBuffer,
		m_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		bufferCopyRegions.size(),
		bufferCopyRegions.data()
	);

	// Change texture image layout to shader read after all mip levels have been copied
	vkTools::setImageLayout(
		copyCommandBuffer,
		m_image,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	err = vkEndCommandBuffer(copyCommandBuffer);
	if (err) goto free_buffers_and_command;

	// Submit copies to the queue
	copySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	copySubmitInfo.commandBufferCount = 1;
	copySubmitInfo.pCommandBuffers = &copyCommandBuffer;

	err = vkQueueSubmit(m_app->Renderer->GetQueue(), 1, &copySubmitInfo, VK_NULL_HANDLE);
	if (err) goto free_buffers_and_command;
	err = vkQueueWaitIdle(m_app->Renderer->GetQueue());
	if (err) goto free_buffers_and_command;

free_buffers_and_command:
	vkFreeCommandBuffers(device, m_app->Renderer->GetCommandPool(), 1, &copyCommandBuffer);

free_buffers:
	// Clean up staging resources
	vkFreeMemory(device, stagingMemory, nullptr);
	vkDestroyBuffer(device, stagingBuffer, nullptr);

	if (err) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	// Create image view
	// Textures are not directly accessed by the shaders and
	// are abstracted by image views containing additional
	// information and sub resource ranges
	VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
	view.image = VK_NULL_HANDLE;
	view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view.format = format;
	view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.baseMipLevel = 0;
	view.subresourceRange.baseArrayLayer = 0;
	view.subresourceRange.layerCount = 1;
	// Linear tiling usually won't support mip maps
	// Only set mip map count if optimal tiling is used
	view.subresourceRange.levelCount = 1; // mips
	view.image = m_image;
	err = vkCreateImageView(device, &view, nullptr, &m_view);
	if (err) {
		_DestroyResources();
		return WError(W_UNABLETOCREATEIMAGE);
	}

	m_width = width;
	m_height = height;

	return WError(W_SUCCEEDED);
}

WError WImage::Load(std::string filename) {
	// check if file exists
	FILE* fp;
	fopen_s(&fp, filename.c_str(), "r");
	if (fp)
		fclose(fp);
	else
		return WError(W_FILENOTFOUND);

	int n = 4, w, h;
	unsigned char *data;
	if (stbi_info(filename.c_str(), &w, &h, &n) == 0)
		return WError(W_INVALIDFILEFORMAT);
	data = stbi_load(filename.c_str(), &w, &h, &n, 0);
	if (!data)
		return WError(W_INVALIDFILEFORMAT);

	// convert from 8-bit char components to 32-bit float components
	float* pixels = new float[w*h*n];
	for (int i = 0; i < w*h*n; i++) {
		float f = (float)data[i] / (float)(unsigned char)(-1);
		pixels[i] = f;
	}
	free(data);

	WError err = CretaeFromPixelsArray(pixels, w, h, n);
	delete[] pixels;

	return err;
}

VkImageView WImage::GetView() const {
	return m_view;
}

VkImageLayout WImage::GetViewLayout() const {
	return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

unsigned int WImage::GetWidth() {
	return m_width;
}

unsigned int WImage::GetHeight() {
	return m_height;
}

