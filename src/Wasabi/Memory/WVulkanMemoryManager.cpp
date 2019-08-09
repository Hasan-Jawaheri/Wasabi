#include "Memory/WVulkanMemoryManager.h"
#include "Core/WCore.h"

enum VULKAN_RESOURCE_TYPE {
	VULKAN_RESOURCE_RENDERPASS = 0,
	VULKAN_RESOURCE_SHADERMODULE = 1,
	VULKAN_RESOURCE_DESCRIPTORSET = 2,
	VULKAN_RESOURCE_PIPELINE = 3,
	VULKAN_RESOURCE_PIPELINECACHE = 4,
	VULKAN_RESOURCE_PIPELINELAYOUT = 5,
	VULKAN_RESOURCE_DESCRIPTORPOOL = 6,
	VULKAN_RESOURCE_FRAMEBUFFER = 7,
	VULKAN_RESOURCE_BUFFER = 8,
	VULKAN_RESOURCE_IMAGE = 9,
	VULKAN_RESOURCE_IMAGEVIEW = 10,
	VULKAN_RESOURCE_DEVICEMEMORY = 11,
	VULKAN_RESOURCE_SAMPLER = 12,
	VULKAN_RESOURCE_COMMANDBUFFER = 13,
	VULKAN_RESOURCE_SEMAPHORE = 14,
	VULKAN_RESOURCE_FENCE = 15,
	VULKAN_RESOURCE_DESCRIPTORSETLAYOUT = 16,
};

VkResult WVulkanBuffer::Create(class Wasabi* app, VkBufferCreateInfo createInfo, VkMemoryPropertyFlags memoryType) {
	VkDevice device = app->GetVulkanDevice();

	VkResult result = vkCreateBuffer(device, &createInfo, nullptr, &buf);
	if (result == VK_SUCCESS) {
		// Get memory requirements
		VkMemoryRequirements memReqs = {};
		vkGetBufferMemoryRequirements(device, buf, &memReqs);

		VkMemoryAllocateInfo memAllocInfo = {};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAllocInfo.allocationSize = memReqs.size;
		// Get memory type index for a host visible buffer
		app->MemoryManager->GetMemoryType(memReqs.memoryTypeBits, memoryType, &memAllocInfo.memoryTypeIndex);

		result = vkAllocateMemory(device, &memAllocInfo, nullptr, &mem);
		if (result == VK_SUCCESS) {
			result = vkBindBufferMemory(device, buf, mem, 0);
		}
	}

	if (result != VK_SUCCESS)
		Destroy(app);

	return result;
}

void WVulkanBuffer::Destroy(class Wasabi* app) {
	app->MemoryManager->ReleaseBuffer(buf, app->GetCurrentBufferingIndex());
	app->MemoryManager->ReleaseDeviceMemory(mem, app->GetCurrentBufferingIndex());
}

VkResult WVulkanImage::Create(class Wasabi* app, VkImageCreateInfo createInfo, VkMemoryPropertyFlags memoryType, VkImageViewCreateInfo viewCreateInfo) {
	VkDevice device = app->GetVulkanDevice();

	VkResult result = vkCreateImage(device, &createInfo, nullptr, &img);
	if (result == VK_SUCCESS) {
		// Get memory requirements
		VkMemoryRequirements memReqs = {};
		vkGetImageMemoryRequirements(device, img, &memReqs);

		VkMemoryAllocateInfo memAllocInfo = {};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAllocInfo.allocationSize = memReqs.size;
		// Get memory type index for a host visible buffer
		app->MemoryManager->GetMemoryType(memReqs.memoryTypeBits, memoryType, &memAllocInfo.memoryTypeIndex);

		result = vkAllocateMemory(device, &memAllocInfo, nullptr, &mem);
		if (result == VK_SUCCESS) {
			result = vkBindImageMemory(device, img, mem, 0);
			if (result == VK_SUCCESS && viewCreateInfo.sType == VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO) {
				viewCreateInfo.image = img;
				result = vkCreateImageView(device, &viewCreateInfo, nullptr, &view);
			}
		}
	}

	if (result != VK_SUCCESS)
		Destroy(app);

	return result;
}

void WVulkanImage::Destroy(class Wasabi* app) {
	app->MemoryManager->ReleaseImage(img, app->GetCurrentBufferingIndex());
	app->MemoryManager->ReleaseDeviceMemory(mem, app->GetCurrentBufferingIndex());
	app->MemoryManager->ReleaseImageView(view, app->GetCurrentBufferingIndex());
}

WVulkanMemoryManager::WVulkanMemoryManager() {
	m_physicalDevice = VK_NULL_HANDLE;
	m_device = VK_NULL_HANDLE;
	m_graphicsQueue = VK_NULL_HANDLE;
	m_deviceFeatures = {};
	m_deviceMemoryProperties = {};
	m_deviceProperties = {};

	m_copyCommandBuffer = VK_NULL_HANDLE;
	m_cmdPool = VK_NULL_HANDLE;
}

WVulkanMemoryManager::~WVulkanMemoryManager() {
	ReleaseCommandBuffer(m_copyCommandBuffer, 0);
	ReleaseAllResources();

	if (m_cmdPool)
		vkDestroyCommandPool(m_device, m_cmdPool, nullptr);
	m_cmdPool = VK_NULL_HANDLE;
}

WError WVulkanMemoryManager::Initialize(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, uint graphicsQueueIndex) {
	m_physicalDevice = physicalDevice;
	m_device = device;
	m_graphicsQueue = queue;

	// Store properties (including limits) and features of the phyiscal device
	vkGetPhysicalDeviceProperties(m_physicalDevice, &m_deviceProperties);
	vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_deviceFeatures);
	// Gather physical device memory properties
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_deviceMemoryProperties);

	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = graphicsQueueIndex;
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VkResult err = vkCreateCommandPool(m_device, &cmdPoolInfo, nullptr, &m_cmdPool);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	VkCommandBufferAllocateInfo cmdBufInfo = {};
	// Buffer copies are done on the queue, so we need a command buffer for them
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufInfo.commandPool = m_cmdPool;
	cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufInfo.commandBufferCount = 1;

	err = vkAllocateCommandBuffers(m_device, &cmdBufInfo, &m_copyCommandBuffer);
	if (err)
		return WError(W_OUTOFMEMORY);

	return WError(W_SUCCEEDED);
}

void WVulkanMemoryManager::GetMemoryType(uint typeBits, VkFlags properties, uint * typeIndex) const {
	for (uint i = 0; i < 32; i++) {
		if ((typeBits & 1) == 1) {
			if ((m_deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				*typeIndex = i;
				return;
			}
		}
		typeBits >>= 1;
	}
}

VkCommandPool WVulkanMemoryManager::GetCommandPool() const {
	return m_cmdPool;
}

VkResult WVulkanMemoryManager::BeginCopyCommandBuffer() {
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.pNext = NULL;

	VkResult err = vkResetCommandBuffer(m_copyCommandBuffer, 0);
	if (err)
		return err;

	// Put buffer region copies into command buffer
	// Note that the staging buffer must not be deleted before the copies
	// have been submitted and executed
	return vkBeginCommandBuffer(m_copyCommandBuffer, &cmdBufferBeginInfo);
}

VkResult WVulkanMemoryManager::EndCopyCommandBuffer(bool waitQueue, VkFence signalFence) {
	VkSubmitInfo copySubmitInfo = {};
	VkResult err = vkEndCommandBuffer(m_copyCommandBuffer);
	if (err)
		return err;

	// Submit copies to the queue
	copySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	copySubmitInfo.commandBufferCount = 1;
	copySubmitInfo.pCommandBuffers = &m_copyCommandBuffer;

	err = vkQueueSubmit(m_graphicsQueue, 1, &copySubmitInfo, signalFence);
	if (err != VK_SUCCESS)
		return err;
	
	if (waitQueue)
		err = vkQueueWaitIdle(m_graphicsQueue);

	return err;
}

VkCommandBuffer WVulkanMemoryManager::GetCopyCommandBuffer() const {
	return m_copyCommandBuffer;
}

void WVulkanMemoryManager::ReleaseAllResources(uint setBufferingCount) {
	for (auto it = m_resourcesToBeFreed.begin(); it != m_resourcesToBeFreed.end(); it++) {
		for (auto it2 = it->begin(); it2 != it->end(); it2++) {
			_ReleaseResource(it2->type, it2->resource, it2->aux);
		}
		it->clear();
	}
	if (setBufferingCount != -1)
		m_resourcesToBeFreed.resize(setBufferingCount * 2);
}

void WVulkanMemoryManager::ReleaseFrameResources(uint bufferIndex) {
	for (auto it = m_resourcesToBeFreed[bufferIndex].begin(); it != m_resourcesToBeFreed[bufferIndex].end(); it++)
		_ReleaseResource(it->type, it->resource, it->aux);
	m_resourcesToBeFreed[bufferIndex].clear();
	std::swap(m_resourcesToBeFreed[bufferIndex], m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex]);
}

void WVulkanMemoryManager::_ReleaseResource(int type, void* resource, void* aux) {
	VkDescriptorSet ds = (VkDescriptorSet)resource;
	VkCommandBuffer cmdBuf = (VkCommandBuffer)resource;

	switch (type) {
	case VULKAN_RESOURCE_RENDERPASS:
		vkDestroyRenderPass(m_device, (VkRenderPass)resource, nullptr);
		break;
	case VULKAN_RESOURCE_SHADERMODULE:
		vkDestroyShaderModule(m_device, (VkShaderModule)resource, nullptr);
		break;
	case VULKAN_RESOURCE_DESCRIPTORSET:
		vkFreeDescriptorSets(m_device, (VkDescriptorPool)aux, 1, &ds);
		break;
	case VULKAN_RESOURCE_DESCRIPTORSETLAYOUT:
		vkDestroyDescriptorSetLayout(m_device, (VkDescriptorSetLayout)resource, nullptr);
		break;
	case VULKAN_RESOURCE_PIPELINE:
		vkDestroyPipeline(m_device, (VkPipeline)resource, nullptr);
		break;
	case VULKAN_RESOURCE_PIPELINECACHE:
		vkDestroyPipelineCache(m_device, (VkPipelineCache)resource, nullptr);
		break;
	case VULKAN_RESOURCE_PIPELINELAYOUT:
		vkDestroyPipelineLayout(m_device, (VkPipelineLayout)resource, nullptr);
		break;
	case VULKAN_RESOURCE_DESCRIPTORPOOL:
		vkDestroyDescriptorPool(m_device, (VkDescriptorPool)resource, nullptr);
		break;
	case VULKAN_RESOURCE_FRAMEBUFFER:
		vkDestroyFramebuffer(m_device, (VkFramebuffer)resource, nullptr);
		break;
	case VULKAN_RESOURCE_BUFFER:
		vkDestroyBuffer(m_device, (VkBuffer)resource, nullptr);
		break;
	case VULKAN_RESOURCE_IMAGE:
		vkDestroyImage(m_device, (VkImage)resource, nullptr);
		break;
	case VULKAN_RESOURCE_IMAGEVIEW:
		vkDestroyImageView(m_device, (VkImageView)resource, nullptr);
		break;
	case VULKAN_RESOURCE_DEVICEMEMORY:
		vkFreeMemory(m_device, (VkDeviceMemory)resource, nullptr);
		break;
	case VULKAN_RESOURCE_SAMPLER:
		vkDestroySampler(m_device, (VkSampler)resource, nullptr);
		break;
	case VULKAN_RESOURCE_COMMANDBUFFER:
		vkFreeCommandBuffers(m_device, m_cmdPool, 1, &cmdBuf);
		break;
	case VULKAN_RESOURCE_SEMAPHORE:
		vkDestroySemaphore(m_device, (VkSemaphore)resource, nullptr);
		break;
	case VULKAN_RESOURCE_FENCE:
		vkDestroyFence(m_device, (VkFence)resource, nullptr);
		break;
	}
}

void WVulkanMemoryManager::ReleaseRenderPass(VkRenderPass& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_RENDERPASS, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseShaderModule(VkShaderModule& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_SHADERMODULE, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseDescriptorSet(VkDescriptorSet& obj, VkDescriptorPool& descriptorPool, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_DESCRIPTORSET, (void*)obj, (void*)descriptorPool });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseDescriptorSetLayout(VkDescriptorSetLayout& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_DESCRIPTORSETLAYOUT, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleasePipeline(VkPipeline& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_PIPELINE, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleasePipelineCache(VkPipelineCache& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_PIPELINECACHE, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleasePipelineLayout(VkPipelineLayout& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_PIPELINELAYOUT, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseDescriptorPool(VkDescriptorPool& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_DESCRIPTORPOOL, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseFramebuffer(VkFramebuffer& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_FRAMEBUFFER, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseBuffer(VkBuffer& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_BUFFER, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseImage(VkImage& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_IMAGE, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseImageView(VkImageView& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_IMAGEVIEW, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseDeviceMemory(VkDeviceMemory& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_DEVICEMEMORY, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseSampler(VkSampler& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_SAMPLER, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseCommandBuffer(VkCommandBuffer& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_COMMANDBUFFER, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseSemaphore(VkSemaphore& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_SEMAPHORE, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

void WVulkanMemoryManager::ReleaseFence(VkFence& obj, uint bufferIndex) {
	if (obj)
		m_resourcesToBeFreed[m_resourcesToBeFreed.size() / 2 + bufferIndex].push_back({ VULKAN_RESOURCE_FENCE, (void*)obj, nullptr });
	obj = VK_NULL_HANDLE;
}

