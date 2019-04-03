/** @file WRenderTarget.h
 *  @brief Render targets implementation
 *
 *  Render targets are special images that can be used to render a scene to. A
 *  render target enable one to perform render-to-texture operations that are
 *  required for many applications such as post-processing effects.
 *  Rendering to render targets is done by calling WRenderTarget::Begin(), rendering
 *  assets (objects, sprites, etc...) and then calling WRenderTarget::End(). Notice
 *  that after ending, the render target merely creates a "command buffer" filled
 *  with the recorded rendering commands that occurred between Begin() and End(). To
 *  actually perform the rendering stored in the buffer, either call
 *  WRenderTarget::End() passing true (default) or explicitly call
 *  WRenderTarget::Submit() afterwards.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"

/**
 * @ingroup engineclass
 *
 * This class represents a render target, which can be used to render things
 * onto. A render target may be backed by a WImage, in which case any rendering
 * that happens on the render target will be reflected on that image, which can
 * then be used normally.
 */
class WRenderTarget : public WBase {
	friend class WRenderTargetManager;

	/**
	 * Returns "RenderTarget" string.
	 * @return Returns "RenderTarget" string
	 */
	virtual std::string GetTypeName() const;

protected:
	virtual ~WRenderTarget();
	WRenderTarget(Wasabi* const app, unsigned int ID = 0);

public:

	/**
	 * Create a render target with a single color attachment backed by a WImage.
	 * The format of the render target attachment will match that of the WImage.
	 * Iif bDepth == true, the created render target will have an attachment for depth.
	 * This function will cause exactly one frame buffer to be created.
	 *
	 * Examples:
	 * @code
	 * rtImg = new WImage(this);
	 * char* pixels = new char[640 * 480 * 4];
	 * rtImg->CreateFromPixelsArray(pixels, 640, 480, false,
	 *                              4, VK_FORMAT_R8G8B8A8_UNORM, 1);
	 * delete[] pixels;
	 * rt = new WRenderTarget(this);
	 * rt->Create(640, 480, rtImg);
	 * @endcode
	 * 
	 * @param  width       Width of the render target
	 * @param  height      Height of the render target
	 * @param  target      A pointer to a WImage backing the render target
	 * @param  depth       A WImage backing the created depth attachment (can be null)
	 * @return             Error code, see WError.h
	 */
	WError Create(unsigned int width, unsigned int height,
				  class WImage* target,
				  class WImage* depth = nullptr);

	/**
	 * Create a render target with a multiple color attachments, each backed by a WImage.
	 * The format of the render target attachments will match those of the WImage's.
	 * Iif bDepth == true, the created render target will also have an attachment for depth.
	 * This function will cause exactly one frame buffer to be created.
	 *
	 * @param  width       Width of the render target
	 * @param  height      Height of the render target
	 * @param  targets     An array of WImage's backing the created render target attachments
	 * @param  depth       A WImage backing the created depth attachment (can be null)
	 * @return             Error code, see WError.h
	 */
	WError Create(unsigned int width, unsigned int height,
				  vector<class WImage*> targets,
				  class WImage* depth = nullptr);

	/**
	 * Create a render target backed by VkImageViews (such as the frame buffer).
	 * This render target will have 1 attachment for color and one attachment for
	 * depth. This function will create as many frame buffers as num_views.
	 * @param  width       Width of the render target
	 * @param  height      Height of the render target
	 * @param  views       An array of VkImageViews to back the render target
	 * @param  numViews    Number of views in views
	 * @param  colorFormat Color format for the render target
	 * @param  depthFormat Depth format for the render target
	 * @return             Error code, see WError.h
	 */
	WError Create(unsigned int width, unsigned int height,
				  VkImageView* views, unsigned int numViews,
				  VkFormat colorFormat, VkFormat depthFormat);

	/**
	 * Begin recording renders on this render target.
	 * @return Error code, see WError.h
	 */
	WError Begin();

	/**
	 * End recording renders on this render target. If bSubmit is set to false, you
	 * have to call Submit() afterwards to actually perform the rendering.
	 * @param  bSubmit When set to true, recorded renders will be submitted to
	 *                 the Vulkan graphics queue and thus will be reflected on
	 *                 the backing WImage of VkImageViews
	 * @return         Error code, see WError.h
	 */
	WError End(bool bSubmit = true);

	/**
	 * Submit the command queue (generated between a call to Begin() and End()) to be performed
	 * by Vulkan.
	 * @params custom_info  Optional custom VkSubmitInfo structure passed to vkQueueSubmit, this
	 *                      will be filled automatically if you ignore this argument (or pass a
	 *                      VkSubmitInfo with pCommandBuffers = NULL. Normally, you should fill
	 *                      pCommandBuffers with GetCommnadBuffer()
	 * @return              Error code, see WError.h
	 */
	WError Submit(VkSubmitInfo custom_info = {});

	/**
	 * Sets the color to use when the render target is cleared upon calling
	 * Begin().
	 * @param col   Clear color to use
	 * @param index Attachment index (as supplied to Create()). The depth target attachment
	 *              index is the last one (so if this render target has 1 color attachment and
	 *              one depth attachment, they will have indices 0 and 1 respectively) and
	 *              the clear value of the depth attachment will only use the red component of
	 *              the supplied WColor (the stencil will be green component)
	 */
	void SetClearColor(WColor col, unsigned int index = 0);

	/**
	 * Sets the camera that will be used when things are rendered using this
	 * render target.
	 * @param cam Camera to use
	 */
	void SetCamera(class WCamera* cam);

	/**
	 * Retrieves the render pass associated with this render target.
	 * @return Handle of the render pass for this render target
	 */
	VkRenderPass GetRenderPass() const;

	/**
	 * Retrieves the pipeline cache associated with this render target.
	 * @return Handle of the pipeline cache for this render target
	 */
	VkPipelineCache GetPipelineCache() const;

	/**
	 * Retrieves the command buffer onto which rendering should occur to happen
	 * on this render target.
	 * @return Handle of the rendering command buffer for this render target
	 */
	VkCommandBuffer GetCommnadBuffer() const;

	/**
	 * Retrieves the number of color output attachments.
	 * @return The number of color output attachments
	 */
	int GetNumColorOutputs() const;

	/**
	 * Checks whether or not the render target has a depth attachment.
	 * @return True if the render target has a depth attachment, false otherwise
	 */
	bool HasDepthOutput() const;

	/**
	 * Retrieve the camera for this render target.
	 * @return Pointer to the camera of this render target
	 */
	class WCamera* GetCamera() const;

	/**
	 * Checks the validity of this render target. A render target is valid when
	 * it has at least one frame buffer.
	 * @return true if the render target is valid, false otherwise
	 */
	virtual bool Valid() const;

private:
	/** Buffered frame buffer for the render target */
	WBufferedFrameBuffer m_bufferedFrameBuffer;
	/** WImage backing the frame buffer depth attachment (if available) */
	WImage* m_depthTarget;
	/** WImage's backing the frame buffer attachments (if available) */
	vector<class WImage*> m_targets;
	/** Render pass associated with this render target */
	VkRenderPass m_renderPass;
	/** A pipeline cache */
	VkPipelineCache m_pipelineCache;
	/** Whether or not this render target has an independent command buffer */
	bool m_haveCommandBuffer;
	/** The command buffer used for rendering on this render target */
	VkCommandBuffer m_renderCmdBuffer;
	/** Clear values to be used by Vulkan */
	vector<VkClearValue> m_clearValues;
	/** Width of the render target */
	unsigned int m_width;
	/** Height of the render target */
	unsigned int m_height;
	/** The camera of this render target */
	class WCamera* m_camera;

	/**
	 * Free all the resources allocated by this render target.
	 */
	void _DestroyResources();
};

/**
 * @ingroup engineclass
 * Manager class for WRenderTarget.
 */
class WRenderTargetManager : public WManager<WRenderTarget> {
	friend class WRenderTarget;

	/**
	 * Returns "RenderTarget" string.
	 * @return Returns "RenderTarget" string
	 */
	virtual std::string GetTypeName() const;

public:
	WRenderTargetManager(class Wasabi* const app);
	~WRenderTargetManager();

	/**
	 * Allocates and initializes a "regular" render target. Regular render
	 * targets must only be rendered to (i.e. calling WRenderTarget::Begin()
	 * and WRenderTarget::End()) within a WRenderStage.
	 */
	WRenderTarget* CreateRenderTarget(unsigned int ID = 0);

	/**
	 * Allocates and initializes an "immediate" render target. An immediate
	 * render target is able to perform rendering to a texture at any point
	 * in the program and not strictly in a render stage. Immediate render
	 * targets are significantly slower than regular render taregts (refer
	 * to WRenderTargetManager::CreateRenderTarget) so consider using a
	 * regular render target whenever possible, and using an immediate render
	 * target only when necessary.
	 * Immediate render targets create their own command buffers and have to
	 * block on GPU operations before WRenderTarget::Begin() and after
	 * WRenderTarget::End().
	 */
	WRenderTarget* CreateImmediateRenderTarget(unsigned int ID = 0);
};

