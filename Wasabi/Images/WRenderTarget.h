/** @file WRenderTarget.h
 *  @brief Render targets implementation
 *
 *  Render targets are special images that can be used to render a scene to. A
 *  render target enable one to perform render-to-texture operations that are
 *  required for many applications such as post-processing effects.
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
	/**
	 * Returns "RenderTarget" string.
	 * @return Returns "RenderTarget" string
	 */
	virtual std::string GetTypeName() const;

public:
	WRenderTarget(Wasabi* const app, unsigned int ID = 0);
	~WRenderTarget();

	/**
	 * Create a render target backed by a WImage. The format of the render target
	 * will match that of the WImage. The render target will have 1 attachment
	 * for color, and if bDepth == true, will have another attachment for depth.
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
	 * @param  bDepth      true if the render target needs a depth attachment,
	 *                     false otherwise
	 * @param  depthFormat Format for the depth attachment, if bDepth == true
	 * @return             Error code, see WError.h
	 */
	WError Create(unsigned int width, unsigned int height,
				  class WImage* target,
				  bool bDepth = true,
				  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT);

	/**
	 * Create a render target backed by VkImageViews (such as the frame buffer).
	 * This render target will have 1 attachment for color and one attachment for
	 * depth. This function will create as many frame buffers as num_views.
	 * @param  width       Width of the render target
	 * @param  height      Height of the render target
	 * @param  views       An array of VkImageViews to back the render target
	 * @param  num_views   Number of views in views
	 * @param  colorFormat Color format for the render target
	 * @param  depthFormat Depth format for the render target
	 * @return             Error code, see WError.h
	 */
	WError Create(unsigned int width, unsigned int height,
				  VkImageView* views, unsigned int num_views,
				  VkFormat colorFormat, VkFormat depthFormat);

	/**
	 * For the next render between Begin() and End(), use the frame buffer at
	 * index. A render target may have several frame buffers.
	 * @param  index Index of the frame buffer
	 * @return       Error code, see WError.h
	 */
	WError UseFrameBuffer(unsigned int index);

	/**
	 * Begin recording renders on this render target.
	 * @return Error code, see WError.h
	 */
	WError Begin();

	/**
	 * End recording renders on this render target.
	 * @param  bSubmit When set to true, recorded renders will be submitted to
	 *                 the Vulkan graphics queue and thus will be reflected on
	 *                 the backing WImage of VkImageViews
	 * @return         Error code, see WError.h
	 */
	WError End(bool bSubmit = true);

	/**
	 * Sets the color to use when the render target is cleared upon calling
	 * Begin().
	 * @param col Clear color to use
	 */
	void SetClearColor(WColor col);

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
	struct {
		/** Depth image buffer */
		VkImage image;
		/** Memory backing the depth image */
		VkDeviceMemory mem;
		/** Depth image view */
		VkImageView view;
	} m_depthStencil;
	/** List of frame buffers */
	std::vector<VkFramebuffer> m_frameBuffers;
	/** Format of the depth image */
	VkFormat m_depthFormat;
	/** Format of the frame buffer(s) */
	VkFormat m_colorFormat;
	/** WImage backing the frame buffer (if available) */
	class WImage* m_target;
	/** Render pass associated with this render target */
	VkRenderPass m_renderPass;
	/** A pipeline cache */
	VkPipelineCache m_pipelineCache;
	/** The command buffer used for rendering on this render target */
	VkCommandBuffer m_renderCmdBuffer;
	/** Clear color */
	VkClearColorValue m_clearColor;
	/** Width of the render target */
	unsigned int m_width;
	/** Height of the render target */
	unsigned int m_height;
	/** Currently used frame buffer */
	unsigned int m_currentFrameBuffer;
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
};

