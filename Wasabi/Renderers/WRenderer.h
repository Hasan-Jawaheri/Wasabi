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

/**
 * @ingroup engineclass
 *
 * A WRenderer is a base class that can be derived to implement a certain
 * rendering method (e.g. a forward rendering variant or deferred rendering
 * variant). A renderer can be assigned to the engine the same way components
 * are, during the initialization in Wasabi::SetupComponents().
 */
class WRenderer {
	friend int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
								LPSTR cmdLine, int cmdShow);
	friend class Wasabi;

	/**
	 * Initializes the implementation-independent components of the renderer,
	 * such as the semaphores and the swap chain.
	 * @return Error code, see WError.h
	 */
	WError _Initialize();

	/**
	 * Begin rendering a frame. This should call the implementation-specific
	 * Render(). This function is responsible for semaphore synchronization and
	 * swap chain presentation.
	 */
	void _Render();

	/**
	 * Frees the implementation-independent resources allocated by the renderer.
	 */
	void _Cleanup();

public:
	WRenderer(class Wasabi* const app);

	/**
	 * Initializes the renderer resources. This function should be implemented by
	 * a child class and it should initialize the child-specific resources. For a
	 * renderer bound to Wasabi, this function will be called during Wasabi's
	 * Wasabi::StartEngine() call.
	 * @return Error code, see WError.h
	 */
	virtual WError Initiailize() = 0;

	/**
	 * Renders the scene to a certain render target. This function should be
	 * implemented by a child class and it should perform the child-specific
	 * rendering procedure. For a renderer bound to Wasabi, this function will be
	 * called during Wasabi's Wasabi::StartEngine() call.
	 * @param rt     Render target to render the scene to
	 * @param filter A filter, represented by a bitfield whose bits are any
	 *               combination of W_RENDER_FILTER flags
	 */
	virtual void Render(class WRenderTarget* rt, unsigned int filter = -1) = 0;

	/**
	 * Frees all resources allocated by the renderer. This function should be
	 * implemented by a child class and it should perform the child-specific
	 * cleanup procedure. For a renderer bound to Wasabi, this function will be
	 * called before the application closes.
	 */
	virtual void Cleanup() = 0;

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
	virtual WError Resize(unsigned int width, unsigned int height);

	/**
	 * Returns a pointer to a newly allocated material that performs the default
	 * rendering for this particular renderer. This function should be
	 * implemented by a child class and should ideally return a material that
	 * works with the renderer without any further initialization. The returned
	 * material should be freed (via WMaterial::RemoveReference()) by the caller.
	 * @return Newly allocated material
	 */
	virtual class WMaterial* CreateDefaultMaterial() = 0;

	/**
	 * Retrieves a default Vulkan image sampler used by materials for attaching
	 * textures to the graphics pipeline.
	 * @return A handle of a usable Vulkan image sampler
	 */
	virtual VkSampler GetDefaultSampler() const = 0;

	/**
	 * Retrieves the currently used Vulkan graphics queue.
	 * @return Currently used Vulkan graphics queue
	 */
	VkQueue GetQueue() const;

	/**
	 * Retrieves the default render target, which should be bound to the
	 * backbuffer (via the swapchain) and should render ultimately on-screen.
	 * @return Pointer to the default render target
	 */
	class WRenderTarget* GetDefaultRenderTarget() const;

protected:
	/** Pointer to the Wasabi application */
	class Wasabi* m_app;
	/** Vulkan virtual device */
	VkDevice m_device;
	/** Vulkan graphics queue */
	VkQueue m_queue;
	/** Vulkan swap chain */
	VulkanSwapChain* m_swapChain;
	/** Default render target */
	WRenderTarget* m_renderTarget;

	// Synchronization semaphores
	struct {
		/** Semaphore to synchronize swap chain image presentation */
		VkSemaphore presentComplete;
		/** Semaphore to synchronize command buffer submission and execution */
		VkSemaphore renderComplete;
	} m_semaphores;

	/** Vulkan format of the depth buffer of the swap chain */
	VkFormat m_depthFormat;
	/** Vulkan format of the color buffer(s) of the swap chain */
	VkFormat m_colorFormat;
	/** Current width of the screen (window client) */
	uint32_t m_width
	/** Current height of the screen (window client) */
	uint32_t m_height;
};

