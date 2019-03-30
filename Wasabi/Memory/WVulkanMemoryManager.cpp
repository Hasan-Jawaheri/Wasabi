#include "WVulkanMemoryManager.h"
#include "../Core/WCore.h"

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
	VkDevice device = app->GetVulkanDevice();
	if (buf)
		vkDestroyBuffer(device, buf, nullptr);
	if (mem)
		vkFreeMemory(device, mem, nullptr);
	buf = VK_NULL_HANDLE;
	mem = VK_NULL_HANDLE;
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
	VkDevice device = app->GetVulkanDevice();
	if (view)
		vkDestroyImageView(device, view, nullptr);
	if (img)
		vkDestroyImage(device, img, nullptr);
	if (mem)
		vkFreeMemory(device, mem, nullptr);
	img = VK_NULL_HANDLE;
	mem = VK_NULL_HANDLE;
	view = VK_NULL_HANDLE;
}

WVulkanMemoryManager::WVulkanMemoryManager() {
	m_physicalDevice = VK_NULL_HANDLE;
	m_device = VK_NULL_HANDLE;
	m_graphicsQueue = VK_NULL_HANDLE;

	m_copyCommandBuffer = VK_NULL_HANDLE;
	m_cmdPool = VK_NULL_HANDLE;
}

WVulkanMemoryManager::~WVulkanMemoryManager() {
	if (m_copyCommandBuffer)
		vkFreeCommandBuffers(m_device, m_cmdPool, 1, &m_copyCommandBuffer);
	m_copyCommandBuffer = VK_NULL_HANDLE;

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

VkResult WVulkanMemoryManager::EndCopyCommandBuffer(VkFence signalFence) {
	VkSubmitInfo copySubmitInfo = {};
	VkResult err = vkEndCommandBuffer(m_copyCommandBuffer);
	if (err)
		return err;

	// Submit copies to the queue
	copySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	copySubmitInfo.commandBufferCount = 1;
	copySubmitInfo.pCommandBuffers = &m_copyCommandBuffer;

	err = vkQueueSubmit(m_graphicsQueue, 1, &copySubmitInfo, signalFence);
	if (err)
		return err;
	err = vkQueueWaitIdle(m_graphicsQueue);
	if (err)
		return err;

	return VK_SUCCESS;
}

VkCommandBuffer WVulkanMemoryManager::GetCopyCommandBuffer() const {
	return m_copyCommandBuffer;
}
