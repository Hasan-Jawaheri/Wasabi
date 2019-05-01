/** @file WRenderer.h
 *  @brief Renderer implementation spec
 *
 *  A renderer provides tools and utilities to control the rendering of objects
 *  in Wasabi.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"

/** Specifies which components are to be rendered. The fields can be bitwise
		OR'ed (e.g. RENDER_FILTER_OBJECTS | RENDER_FILTER_PARTICLES) to add
		multiple filters. */
enum W_RENDER_FILTER {
	/** Render objects */
	RENDER_FILTER_OBJECTS = 1,
	/** Render sprites */
	RENDER_FILTER_SPRITES = 2,
	/** Render texts */
	RENDER_FILTER_TEXT = 4,
	/** Render particles */
	RENDER_FILTER_PARTICLES = 8,
	/** Render terrains */
	RENDER_FILTER_TERRAIN = 16,
};

/** Specifies the type of a texture sampler */
enum W_TEXTURE_SAMPLER_TYPE {
	/** Default renderer's sampler */
	TEXTURE_SAMPLER_DEFAULT = 0,
};

/**
 * @ingroup engineclass
 *
 * A WRenderer is a base class that can be derived to implement a certain
 * rendering method (e.g. a forward rendering variant or deferred rendering
 * variant). A renderer can be assigned to the engine the same way components
 * are, during the initialization in Wasabi::SetupComponents().
 */
class WRenderer {
	friend class Wasabi;
	friend int RunWasabi(class Wasabi*);

public:
	WRenderer(class Wasabi* const app);

	/**
	 * Initializes the renderer resources. This function should be implemented by
	 * a child class and it should initialize the child-specific resources. For a
	 * renderer bound to Wasabi, this function will be called during Wasabi's
	 * Wasabi::StartEngine() call.
	 * @return Error code, see WError.h
	 */
	WError Initialize();

	/**
	 * Begin rendering a frame. This function is responsible for semaphore
	 * synchronization and swap chain presentation.
	 */
	void Render();

	/**
	 * Frees all resources allocated by the renderer. This function should be
	 * implemented by a child class and it should perform the child-specific
	 * cleanup procedure. For a renderer bound to Wasabi, this function will be
	 * called before the application closes.
	 */
	void Cleanup();

	/**
	 * Notifies the renderer of a change in screen (window) size. This function
	 * could be implemented by a child class to perform child-specific resizing
	 * procedures. An overridden implementation may (and probably should) call
	 * the WRenderer implementation of resize at some point, which will resize
	 * the swap chain and the default render target attached to it.
	 * @param  width  New screen (window) width
	 * @param  height New screen (window) height
	 * @return        Error code, see WError
	 */
	WError Resize(uint width, uint height);

	/**
	 * Destroys the previously set render stages and assigns the new ones. This
	 * function will call Initialize on all the render stages. Render stages
	 * define the sequence of rendering events that happen when each frame is
	 * rendered.
	 * @param stages  Render stages to use
	 * @return        Error code, see WError
	 */
	WError SetRenderingStages(std::vector<class WRenderStage*> stages);

	/**
	 * Retrieves a set render stage with a given name.
	 * @param stageName  Name of the stage to retrieve
	 * @return           Render stage requested
	 */
	class WRenderStage* GetRenderStage(std::string stageName) const;

	/**
	 * @return The name of the sprites render stage
	 */
	std::string GetSpritesRenderStageName() const;

	/**
	 * @return The name of the text render stage
	 */
	std::string GetTextsRenderStageName() const;

	/**
	 * @return The name of the particles render stage
	 */
	std::string GetParticlesRenderStageName() const;

	/**
	 * @return The name of the render stage to use its render target camera for picking
	 */
	std::string GetPickingRenderStageName() const;

	/**
	 * Retrieves the render target of a given render stage. If stageName is "",
	 * this will return the backbuffer's render target, which is always the last
	 * @param stageName  Name of the stage, using "" will return the backbuffer's
	 *                   render target
	 * @return           Pointer to the render target of the given stage
	 */
	class WRenderTarget* GetRenderTarget(std::string stageName = "") const;

	/**
	 * Retrieves a render target output image from a currently set render stage.
	 * @param imageName  Name of the image in one of the render stages
	 * @return           Pointer to the render target image with the given name
	 */
	class WImage* GetRenderTargetImage(std::string imageName) const;

	/**
	 * Retrieves the swap chain.
	 */
	VulkanSwapChain* GetSwapchain() const;

	/**
	 * Retrieves a Vulkan image sampler of a given type
	 * @param type  Type of the requested sampler
	 * @return      A handle of a usable Vulkan image sampler
	 */
	VkSampler GetTextureSampler(W_TEXTURE_SAMPLER_TYPE type = TEXTURE_SAMPLER_DEFAULT) const;

	/**
	 * Retrieves the primary command buffer used in the current frame (should be
	 * called within a WRenderStage's render).
	 * @return  Current primary command buffer for the frame
	 */
	VkCommandBuffer GetCurrentPrimaryCommandBuffer() const;

	/**
	 * Retrieves the current buffering index. The current buffering index is the
	 * index of buffers that should be used for the current frame (safe to
	 * assume GPU access to it is done). All memory access to those buffers have
	 * to use a memory barriers to ensure no races happen.
	 */
	uint GetCurrentBufferingIndex() const;

	/**
	 * Retrieves the currently used Vulkan graphics queue.
	 * @return Currently used Vulkan graphics queue
	 */
	VkQueue GetQueue() const;

private:
	/** Pointer to the Wasabi application */
	class Wasabi* m_app;
	/** Vulkan virtual device */
	VkDevice m_device;
	/** Vulkan graphics queue */
	VkQueue m_queue;
	/** Vulkan swap chain */
	VulkanSwapChain* m_swapChain;
	/** Default Vulkan sampler */
	VkSampler m_sampler;
	/** Currently set rendering stages */
	std::vector<class WRenderStage*> m_renderStages;
	/** Currently set rendering stages, stored in an unordered map for quick access */
	std::unordered_map<std::string, class WRenderStage*> m_renderStageMap;
	/** Name of the currently set render stage that renders to the back buffer */
	std::string m_backbufferRenderStageName;
	/** Name of the currently set render stage for sprites */
	std::string m_spritesRenderStageName;
	/** Name of the currently set render stage for texts */
	std::string m_textsRenderStageName;
	/** Name of the currently set render stage for particles */
	std::string m_particlesRenderStageName;
	/** Name of the currently set render stage for picking objects */
	std::string m_pickingRenderStageName;

	// Synchronization semaphores
	struct PerBufferResources {
		/** Primary command buffer for rendering, one per buffer. */
		std::vector<VkCommandBuffer> primaryCommandBuffers;
		/** Semaphores to synchronize swap chain image presentation, one per buffer.
		    Ensures that the image is displayed before we start submitting new commands
		    to the queue */
		std::vector<VkSemaphore> presentComplete;
		/** Semaphores to synchronize command buffer submission and execution, one
		    per buffer. Ensures that the image is not presented until all commands have
			been sumbitted and executed */
		std::vector<VkSemaphore> renderComplete;
		/** Memory fences, one per buffer, to ensure that the next frame which uses
		    that buffer will wait until the memory fence has been signalled (which
			means that previous frame which used this buffer index is done with the
			memory and it is safe to write to it) */
		std::vector<VkFence> memoryFences;
		/** Index currently used, this is not the same as the framebuffer returned
		    by VkAcquireNExtImageKHR, it is independent and round-robin'd */
		uint curIndex;

		VkResult Create(class Wasabi* app, uint numBuffers);
		void Destroy(class Wasabi* app);
	} m_perBufferResources;

	/** Current width of the screen (window client) */
	uint m_width;
	/** Current height of the screen (window client) */
	uint m_height;
};

