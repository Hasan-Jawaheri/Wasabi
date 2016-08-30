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
	VkFormat fmt,
	size_t comp_size) {

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

WRenderTargetManager::WRenderTargetManager(Wasabi* const app) : WManager<WRenderTarget>(app) {
}

WRenderTargetManager::~WRenderTargetManager() {

}

std::string WRenderTargetManager::GetTypeName(void) const {
	return "RenderTarget";
}

WRenderTarget::WRenderTarget(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_depthStencil.view = VK_NULL_HANDLE;
	m_depthStencil.image = VK_NULL_HANDLE;
	m_depthStencil.mem = VK_NULL_HANDLE;
	m_currentFrameBuffer = 0;
	m_camera = m_app->CameraManager->GetDefaultCamera();

	m_clearColor = { { 0.425f, 0.425f, 0.425f, 1.0f } };
	m_renderCmdBuffer = VK_NULL_HANDLE;
	m_renderPass = VK_NULL_HANDLE;
	m_pipelineCache = VK_NULL_HANDLE;

	m_app->RenderTargetManager->AddEntity(this);
}

WRenderTarget::~WRenderTarget() {
	_DestroyResources();

	m_app->RenderTargetManager->RemoveEntity(this);
}

std::string WRenderTarget::GetTypeName() const {
	return "RenderTarget";
}

bool WRenderTarget::Valid() const {
	return m_frameBuffers.size() > 0;
}

void WRenderTarget::_DestroyResources() {
	VkDevice device = m_app->GetVulkanDevice();

	if (m_renderPass)
		vkDestroyRenderPass(device, m_renderPass, nullptr);
	if (m_pipelineCache)
		vkDestroyPipelineCache(device, m_pipelineCache, nullptr);

	if (m_renderCmdBuffer)
		vkFreeCommandBuffers(device, m_app->GetCommandPool(), 1, &m_renderCmdBuffer);

	m_renderCmdBuffer = VK_NULL_HANDLE;
	m_renderPass = VK_NULL_HANDLE;
	m_pipelineCache = VK_NULL_HANDLE;

	if (m_depthStencil.view)
		vkDestroyImageView(device, m_depthStencil.view, nullptr);
	m_depthStencil.view = VK_NULL_HANDLE;
	if (m_depthStencil.image)
		vkDestroyImage(device, m_depthStencil.image, nullptr);
	m_depthStencil.image = VK_NULL_HANDLE;
	if (m_depthStencil.mem)
		vkFreeMemory(device, m_depthStencil.mem, nullptr);
	m_depthStencil.mem = VK_NULL_HANDLE;

	for (uint32_t i = 0; i < m_frameBuffers.size(); i++)
		vkDestroyFramebuffer(device, m_frameBuffers[i], nullptr);
	m_frameBuffers.clear();
	m_currentFrameBuffer = 0;

	m_depthStencil.view = VK_NULL_HANDLE;
	m_depthStencil.image = VK_NULL_HANDLE;
	m_depthStencil.mem = VK_NULL_HANDLE;

	if (m_target)
		W_SAFE_REMOVEREF(m_target);
}

WError WRenderTarget::Create(unsigned int width, unsigned int height, WImage* target, bool bDepth, VkFormat depthFormat) {
	if (!target || !target->Valid())
		return WError(W_INVALIDPARAM);

	VkDevice device = m_app->GetVulkanDevice();

	_DestroyResources();

	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vkTools::initializers::commandBufferAllocateInfo(
			m_app->GetCommandPool(),
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1);

	VkResult err = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &m_renderCmdBuffer);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	// create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	err = vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	//
	// Create the render pass
	//
	VkAttachmentDescription attachments[2] = {};

	// Color attachment
	attachments[0].format = target->GetFormat();
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	if (bDepth) {
		// Depth attachment
		attachments[1].format = depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	if (bDepth) {
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = &depthReference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	if (!bDepth) {
		subpass.pDepthStencilAttachment = NULL;
	}

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = NULL;
	renderPassInfo.attachmentCount = bDepth ? 2 : 1;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = NULL;

	err = vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass);
	assert(!err);

	//
	// Create the depth-stencil buffer
	//
	if (bDepth) {
		VkImageCreateInfo image = {};
		image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image.pNext = NULL;
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = depthFormat;
		image.extent = { width, height, 1 };
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image.flags = 0;

		VkMemoryAllocateInfo mem_alloc = {};
		mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		mem_alloc.pNext = NULL;
		mem_alloc.allocationSize = 0;
		mem_alloc.memoryTypeIndex = 0;

		VkImageViewCreateInfo depthStencilView = {};
		depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthStencilView.pNext = NULL;
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = depthFormat;
		depthStencilView.flags = 0;
		depthStencilView.subresourceRange = {};
		depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthStencilView.subresourceRange.baseMipLevel = 0;
		depthStencilView.subresourceRange.levelCount = 1;
		depthStencilView.subresourceRange.baseArrayLayer = 0;
		depthStencilView.subresourceRange.layerCount = 1;

		VkMemoryRequirements memReqs;

		err = vkCreateImage(device, &image, nullptr, &m_depthStencil.image);
		assert(!err);
		vkGetImageMemoryRequirements(device, m_depthStencil.image, &memReqs);
		mem_alloc.allocationSize = memReqs.size;
		m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mem_alloc.memoryTypeIndex);
		err = vkAllocateMemory(device, &mem_alloc, nullptr, &m_depthStencil.mem);
		assert(!err);

		err = vkBindImageMemory(device, m_depthStencil.image, m_depthStencil.mem, 0);
		assert(!err);

		m_app->BeginCommandBuffer();
		vkTools::setImageLayout(
			m_app->GetCommandBuffer(),
			m_depthStencil.image,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		m_app->EndCommandBuffer();

		depthStencilView.image = m_depthStencil.image;
		err = vkCreateImageView(device, &depthStencilView, nullptr, &m_depthStencil.view);
		assert(!err);
	}

	//
	// Create the frame buffers
	//
	VkImageView imageViews[2];

	// Depth/Stencil attachment is the same for all frame buffers
	imageViews[0] = target->GetView();
	imageViews[1] = m_depthStencil.view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = m_renderPass;
	frameBufferCreateInfo.attachmentCount = bDepth ? 2 : 1;
	frameBufferCreateInfo.pAttachments = imageViews;
	frameBufferCreateInfo.width = width;
	frameBufferCreateInfo.height = height;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	m_frameBuffers.resize(1);
	err = vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &m_frameBuffers[0]);
	assert(!err);

	m_width = width;
	m_height = height;
	m_depthFormat = depthFormat;
	m_colorFormat = target->GetFormat();
	m_target = target;
	target->AddReference();

	return WError(W_SUCCEEDED);
}

WError WRenderTarget::Create(unsigned int width, unsigned int height, VkImageView* views,
							 unsigned int num_views, VkFormat colorFormat, VkFormat depthFormat) {
	VkDevice device = m_app->GetVulkanDevice();

	_DestroyResources();

	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vkTools::initializers::commandBufferAllocateInfo(
			m_app->GetCommandPool(),
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1);

	VkResult err = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &m_renderCmdBuffer);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	// create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	err = vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	//
	// Create the render pass
	//
	VkAttachmentDescription attachments[2] = {};

	// Color attachment
	attachments[0].format = colorFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment
	attachments[1].format = depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = &depthReference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = NULL;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = NULL;

	err = vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass);
	assert(!err);

	//
	// Create the depth-stencil buffer
	//
	VkImageCreateInfo image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.pNext = NULL;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = depthFormat;
	image.extent = { width, height, 1 };
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image.flags = 0;

	VkMemoryAllocateInfo mem_alloc = {};
	mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc.pNext = NULL;
	mem_alloc.allocationSize = 0;
	mem_alloc.memoryTypeIndex = 0;

	VkImageViewCreateInfo depthStencilView = {};
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.pNext = NULL;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = depthFormat;
	depthStencilView.flags = 0;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;

	VkMemoryRequirements memReqs;

	err = vkCreateImage(device, &image, nullptr, &m_depthStencil.image);
	assert(!err);
	vkGetImageMemoryRequirements(device, m_depthStencil.image, &memReqs);
	mem_alloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mem_alloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &mem_alloc, nullptr, &m_depthStencil.mem);
	assert(!err);

	err = vkBindImageMemory(device, m_depthStencil.image, m_depthStencil.mem, 0);
	assert(!err);

	m_app->BeginCommandBuffer();
	vkTools::setImageLayout(
		m_app->GetCommandBuffer(),
		m_depthStencil.image,
		VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	m_app->EndCommandBuffer();

	depthStencilView.image = m_depthStencil.image;
	err = vkCreateImageView(device, &depthStencilView, nullptr, &m_depthStencil.view);
	assert(!err);

	//
	// Create the frame buffers
	//
	VkImageView imageViews[2];

	// Depth/Stencil attachment is the same for all frame buffers
	imageViews[1] = m_depthStencil.view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = m_renderPass;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments = imageViews;
	frameBufferCreateInfo.width = width;
	frameBufferCreateInfo.height = height;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	m_frameBuffers.resize(num_views);
	for (uint32_t i = 0; i < m_frameBuffers.size(); i++) {
		imageViews[0] = views[i];
		VkResult err = vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &m_frameBuffers[i]);
		assert(!err);
	}

	m_width = width;
	m_height = height;
	m_depthFormat = depthFormat;
	m_colorFormat = colorFormat;

	return WError(W_SUCCEEDED);
}

WError WRenderTarget::UseFrameBuffer(unsigned int index) {
	if (index >= m_frameBuffers.size())
		return WError(W_INVALIDPARAM);

	m_currentFrameBuffer = index;

	return WError(W_SUCCEEDED);
}

WError WRenderTarget::Begin() {
	VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

	VkClearValue clearValues[2];
	clearValues[0].color = m_clearColor;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = m_renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_width;
	renderPassBeginInfo.renderArea.extent.height = m_height;
	renderPassBeginInfo.clearValueCount = m_depthStencil.image ? 2 : 1;
	renderPassBeginInfo.pClearValues = clearValues;

	// Set target frame buffer
	renderPassBeginInfo.framebuffer = m_frameBuffers[m_currentFrameBuffer];

	// Bind descriptor sets describing shader binding points
	VkResult err = vkResetCommandBuffer(m_renderCmdBuffer, 0);
	if (err)
		return WError(W_ERRORUNK);

	err = vkBeginCommandBuffer(m_renderCmdBuffer, &cmdBufInfo);
	if (err)
		return WError(W_ERRORUNK);

	vkCmdBeginRenderPass(m_renderCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = vkTools::initializers::viewport(
		(float)m_width,
		(float)m_height,
		0.0f,
		1.0f);
	vkCmdSetViewport(m_renderCmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = vkTools::initializers::rect2D(
		m_width,
		m_height,
		0,
		0);
	vkCmdSetScissor(m_renderCmdBuffer, 0, 1, &scissor);

	m_camera->Render(m_width, m_height);

	return WError(W_SUCCEEDED);
}

WError WRenderTarget::End(bool bSubmit) {
	vkCmdEndRenderPass(m_renderCmdBuffer);

	VkResult err = vkEndCommandBuffer(m_renderCmdBuffer);
	if (err)
		return WError(W_ERRORUNK);

	if (bSubmit) {
		// Command buffer to be sumitted to the queue
		VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
		submitInfo.pWaitDstStageMask = &submitPipelineStages;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_renderCmdBuffer;

		// Submit to queue
		err = vkQueueSubmit(m_app->Renderer->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		if (err)
			return WError(W_ERRORUNK);
	}

	return WError(W_SUCCEEDED);
}

void WRenderTarget::SetClearColor(WColor  col) {
	m_clearColor = { { col.r, col.g, col.b, col.a } };
}

void WRenderTarget::SetCamera(WCamera* cam) {
	m_camera = cam;
}

VkRenderPass WRenderTarget::GetRenderPass() const {
	return m_renderPass;
}

VkPipelineCache WRenderTarget::GetPipelineCache() const {
	return m_pipelineCache;
}

VkCommandBuffer WRenderTarget::GetCommnadBuffer() const {
	return m_renderCmdBuffer;
}

WCamera* WRenderTarget::GetCamera() const {
	return m_camera;
}
