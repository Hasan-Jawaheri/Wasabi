#include "WImage.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-fpermissive"
#pragma warning(disable:4477)
#pragma warning(disable:4838)
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

#define STB_DEFINE
#include "../stb/stb.h"

#ifdef __GNUC__
#pragma warning(default:4477)
#pragma warning(default:4838)
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif

std::string WImageManager::GetTypeName() const {
	return "Image";
}

WImageManager::WImageManager(class Wasabi* const app) : WManager<WImage>(app) {
	m_checker_image = nullptr;
}

WImageManager::~WImageManager() {
	W_SAFE_REMOVEREF(m_checker_image);
}

WError WImageManager::Load() {
	m_checker_image = new WImage(m_app);
	int size = 256;
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
	WError werr = m_checker_image->CreateFromPixelsArray(pixels, size, size, false, comp_size);
	delete[] pixels;
	if (!werr) {
		W_SAFE_REMOVEREF(m_checker_image);
		return werr;
	}

	return WError(W_SUCCEEDED);
}

WImage* WImageManager::GetDefaultImage() const {
	return m_checker_image;
}

WImage::WImage(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_stagingBuffer = VK_NULL_HANDLE;
	m_stagingMemory = VK_NULL_HANDLE;
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
	if (m_stagingBuffer)
		vkDestroyBuffer(device, m_stagingBuffer, nullptr);
	if (m_stagingMemory)
		vkFreeMemory(device, m_stagingMemory, nullptr);
	if (m_image)
		vkDestroyImage(device, m_image, nullptr);
	if (m_deviceMemory)
		vkFreeMemory(device, m_deviceMemory, nullptr);
	if (m_view)
		vkDestroyImageView(device, m_view, nullptr);
	m_stagingBuffer = VK_NULL_HANDLE;
	m_stagingMemory = VK_NULL_HANDLE;
	m_image = VK_NULL_HANDLE;
	m_deviceMemory = VK_NULL_HANDLE;
	m_view = VK_NULL_HANDLE;
}

WError WImage::CreateFromPixelsArray(
	void*			pixels,
	unsigned int	width,
	unsigned int	height,
	bool			bDynamic,
	unsigned int	num_components,
	VkFormat		fmt,
	size_t			comp_size) {

	VkDevice device = m_app->GetVulkanDevice();
	VkResult err;
	VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs = {};
	VkBufferImageCopy bufferCopyRegion = {};
	VkImageCreateInfo imageCreateInfo = vkTools::initializers::imageCreateInfo();
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
		comp_size = 4;
	}

	_DestroyResources();

	// Create a host-visible staging buffer that contains the raw image data
	bufferCreateInfo.size = width * height * num_components * comp_size;
	// This buffer is used as a transfer source for the buffer copy
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	err = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &m_stagingBuffer);
	if (err) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	// Get memory requirements for the staging buffer (alignment, memory type bits)
	vkGetBufferMemoryRequirements(device, m_stagingBuffer, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;
	// Get memory type index for a host visible buffer
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);

	err = vkAllocateMemory(device, &memAllocInfo, nullptr, &m_stagingMemory);
	if (err) {
		vkDestroyBuffer(device, m_stagingBuffer, nullptr);
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}
	err = vkBindBufferMemory(device, m_stagingBuffer, m_stagingMemory, 0);
	if (err) goto free_buffers;

	// Copy texture data into staging buffer
	if (pixels) {
		err = vkMapMemory(device, m_stagingMemory, 0, memReqs.size, 0, (void **)&data);
		if (err) goto free_buffers;
		memcpy(data, pixels, bufferCreateInfo.size);
		vkUnmapMemory(device, m_stagingMemory);
	}

	// Create optimal tiled target image
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.mipLevels = 1; // TODO: USE m_app->engineParams["numGeneratedMips"]
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

	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						 &memAllocInfo.memoryTypeIndex);
	err = vkAllocateMemory(device, &memAllocInfo, nullptr, &m_deviceMemory);
	if (err) goto free_buffers;
	err = vkBindImageMemory(device, m_image, m_deviceMemory, 0);
	if (err) goto free_buffers;

	// Setup buffer copy regions for each mip level
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = width;
	bufferCopyRegion.imageExtent.height = height;
	bufferCopyRegion.imageExtent.depth = 1;
	bufferCopyRegion.bufferOffset = 0;

	err = m_app->BeginCommandBuffer();
	if (err) goto free_buffers;

	// Image barrier for optimal image (target)
	// Optimal image will be used as destination for the copy
	vkTools::setImageLayout(
		m_app->GetCommandBuffer(),
		m_image,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Copy mip levels from staging buffer
	vkCmdCopyBufferToImage(
		m_app->GetCommandBuffer(),
		m_stagingBuffer,
		m_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&bufferCopyRegion
	);

	// Change texture image layout to shader read after all mip levels have been copied
	vkTools::setImageLayout(
		m_app->GetCommandBuffer(),
		m_image,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	err = m_app->EndCommandBuffer();
	if (err) goto free_buffers;

free_buffers:
	// Clean up staging resources
	if (err || !bDynamic) {
		vkFreeMemory(device, m_stagingMemory, nullptr);
		vkDestroyBuffer(device, m_stagingBuffer, nullptr);
		m_stagingMemory = VK_NULL_HANDLE;
		m_stagingBuffer = VK_NULL_HANDLE;
	}

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
	m_numComponents = num_components;
	m_componentSize = comp_size;
	m_mapSize = bufferCreateInfo.size;
	m_format = format;

	return WError(W_SUCCEEDED);
}

WError WImage::Load(std::string filename, bool bDynamic) {
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

	WError err = CreateFromPixelsArray(pixels, w, h, bDynamic, n);
	delete[] pixels;

	return err;
}

WError WImage::CopyFrom(WImage* const image) {
	if (!image || !image->Valid() || !image->m_stagingBuffer)
		return WError(W_INVALIDPARAM);

	void* pixels;
	WError res = image->MapPixels(&pixels, true);
	if (!res)
		return res;
	res = CreateFromPixelsArray(pixels, image->m_width, image->m_height, true,
								image->m_numComponents, image->m_format, image->m_componentSize);
	image->UnmapPixels();

	return res;
}

WError WImage::MapPixels(void** const pixels, bool bReadOnly) {
	if (!Valid() || !m_stagingMemory)
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();

	m_readOnlyMap = bReadOnly;

	VkResult err = vkMapMemory(device, m_stagingMemory, 0, m_mapSize, 0, pixels);
	if (err)
		return WError(W_NOTVALID);

	return WError(W_SUCCEEDED);
}

void WImage::UnmapPixels() {
	VkDevice device = m_app->GetVulkanDevice();
	vkUnmapMemory(device, m_stagingMemory);

	if (!m_readOnlyMap) {
		VkResult err = m_app->BeginCommandBuffer();
		if (err)
			return;

		VkBufferImageCopy bufferCopyRegion = {};
		// Setup buffer copy regions for each mip level
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = m_width;
		bufferCopyRegion.imageExtent.height = m_height;
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = 0;

		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		vkTools::setImageLayout(
			m_app->GetCommandBuffer(),
			m_image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Copy mip levels from staging buffer
		vkCmdCopyBufferToImage(
			m_app->GetCommandBuffer(),
			m_stagingBuffer,
			m_image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion
		);

		// Change texture image layout to shader read after all mip levels have been copied
		vkTools::setImageLayout(
			m_app->GetCommandBuffer(),
			m_image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		err = m_app->EndCommandBuffer();
	}
}

VkImageView WImage::GetView() const {
	return m_view;
}

VkImageLayout WImage::GetViewLayout() const {
	return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VkFormat WImage::GetFormat() const {
	return m_format;
}

unsigned int WImage::GetWidth() const {
	return m_width;
}

unsigned int WImage::GetHeight() const {
	return m_height;
}

unsigned int WImage::GetNumComponents() const {
	return m_numComponents;
}

unsigned int WImage::GetComponentSize() const {
	return m_componentSize;
}

unsigned int WImage::GetPixelSize() const {
	return m_numComponents * m_componentSize;

}
