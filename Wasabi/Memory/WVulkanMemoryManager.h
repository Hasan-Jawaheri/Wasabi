#pragma once

#include "../Core/WCommon.h"

/** A bitfield specifying the intention for a map operation */
enum W_MAP_FLAGS {
	/** Unspecified */
	W_MAP_UNDEFINED = 0,
	/** Mapping for read */
	W_MAP_READ = 1,
	/** Mapping for write */
	W_MAP_WRITE = 2,
};

inline W_MAP_FLAGS operator | (W_MAP_FLAGS lhs, W_MAP_FLAGS rhs) {
	using T = std::underlying_type_t <W_MAP_FLAGS>;
	return static_cast<W_MAP_FLAGS>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline W_MAP_FLAGS& operator |= (W_MAP_FLAGS& lhs, W_MAP_FLAGS rhs) {
	lhs = lhs | rhs;
	return lhs;
}

/** Specifies where memory is  */
enum W_MEMORY_STORAGE {
	/** Unspecified */
	W_MEMORY_UNDEFINED = 0,
	/** Memory is stored on GPU-only memory. Most efficient, but cannot be read/written on CPU */
	W_MEMORY_DEVICE_LOCAL = 1,
	/** Memory visible to both GPU and CPU, limited memory pool */
	W_MEMORY_HOST_VISIBLE = 2,
	/** Memory is stored on GPU-only memory, but a copy is kept on the heap for CPU reads */
	W_MEMORY_DEVICE_LOCAL_HOST_COPY = 3,
};

/**
 * Wrapper for a Vulkan buffer an its backing memory.
 */
struct WVulkanBuffer {
	/** Vulkan buffer */
	VkBuffer buf;
	/** buf's backing memory */
	VkDeviceMemory mem;

	WVulkanBuffer() : buf(VK_NULL_HANDLE), mem(VK_NULL_HANDLE) {}

	/**
	 * Creates the buffer and its memory and binds the memory to it
	 * @param app         Pointer to a Wasabi instance
	 * @param createInfo  Creation info for the buffer
	 * @param memoryType  Type of the memory used to back the buffer
	 * @return            Vulkan result of the operation
	 */
	VkResult Create(class Wasabi* app, VkBufferCreateInfo createInfo, VkMemoryPropertyFlags memoryType);

	/**
	 * Destroy the buffer and its backing memory
	 * @param app The Vulkan device used to crate the buffer
	 */
	void Destroy(class Wasabi* app);
};

/**
 * Weapper for a Vulkan image and its backing memory and its image view.
 */
struct WVulkanImage {
	/** Vulkan image */
	VkImage img;
	/** img's backing memory */
	VkDeviceMemory mem;
	/** img's view */
	VkImageView view;

	WVulkanImage() : img(VK_NULL_HANDLE), mem(VK_NULL_HANDLE), view(VK_NULL_HANDLE) {}

	/**
	 * Creates the image and its memory and binds the memory to it. Optionally creates a view.
	 * @param app             Pointer to a Wasabi instance
	 * @param createInfo      Creation info for the image
	 * @param memoryType      Type of the memory used to back the image
	 * @param viewCreateInfo  Image view creation info (operational). If provided, the .image
	 *                        member will be automatically set to the created image
	 * @return                Vulkan result of the operation
	 */
	VkResult Create(class Wasabi* app, VkImageCreateInfo createInfo, VkMemoryPropertyFlags memoryType, VkImageViewCreateInfo viewCreateInfo = {});

	/**
	 * Destroy the image and its backing memory and view
	 * @param app The Vulkan device used to crate the buffer
	 */
	void Destroy(class Wasabi* app);
};

class WVulkanMemoryManager {
public:
	WVulkanMemoryManager();
	~WVulkanMemoryManager();

	WError Initialize(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, uint graphicsQueueIndex);

	/**
	 * Retrieves a Vulkan command pool to be used to initialize command buffers.
	 * @return A Vulkan command pool
	 */
	VkCommandPool GetCommandPool() const;

	/**
	 * Retrieves the index of a Vulakn memory type that is compatible with the
	 * requested memory type and properties
	 * @param typeBits   A 32-bit value, in which each bit represents a usable
	 *                   memory type
	 * @param properties The requested memory properties to be found
	 * @param typeIndex  Pointer to an index to be filled
	 */
	void GetMemoryType(uint typeBits, VkFlags properties, uint* typeIndex) const;

	/**
	 * Starts recording commands on the copy command buffer, which can be
	 * acquired using GetCopyCommandBuffer().
	 * @return A Vulkan result, VK_SUCCESS on success
	 */
	VkResult BeginCopyCommandBuffer();

	/**
	 * Ends recording commands on the copy command buffer and submits it to the
	 * graphics queue.
	 * @param waitQueue   Whether or not to wait for the queue to finish copying
	 * @param signalFence A fence to signal when GPU finishes with the submission
	 * @return A Vulkan result, VK_SUCCESS on success
	 */
	VkResult EndCopyCommandBuffer(bool waitQueue, VkFence signalFence = VK_NULL_HANDLE);

	/**
	 * Retrieves the copy command buffer that is used with BeginCopyCommandBuffer()
	 * and EndCopyCommandBuffer().
	 * @return The copy command buffer
	 */
	VkCommandBuffer GetCopyCommandBuffer() const;

private:
	/** The used Vulkan physical device */
	VkPhysicalDevice m_physicalDevice;
	/** The used Vulkan virtual device */
	VkDevice m_device;
	/** The used graphics queue */
	VkQueue m_graphicsQueue;
	/** Vulkan properties of the physical devices */
	VkPhysicalDeviceProperties m_deviceProperties;
	/** Vulkan features of the physical device */
	VkPhysicalDeviceFeatures m_deviceFeatures;
	/** Memory types available on the device */
	VkPhysicalDeviceMemoryProperties m_deviceMemoryProperties;
	/** Command pool created by the engine */
	VkCommandPool m_cmdPool;
	/** A dummy command buffer for general use */
	VkCommandBuffer m_copyCommandBuffer;
};

