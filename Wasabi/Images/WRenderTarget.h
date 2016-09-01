#pragma once

#include "../Core/Core.h"

class WRenderTarget : public WBase {
	virtual std::string GetTypeName() const;

public:
	WRenderTarget(Wasabi* const app, unsigned int ID = 0);
	~WRenderTarget();

	WError			Create(unsigned int width, unsigned int height, class WImage* target, bool bDepth = true,
						   VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT);
	WError			Create(unsigned int width, unsigned int height, VkImageView* views,
						   unsigned int num_views, VkFormat colorFormat, VkFormat depthFormat);

	WError			UseFrameBuffer(unsigned int index);
	WError			Begin();
	WError			End(bool bSubmit = true);

	void			SetClearColor(WColor col);
	void			SetCamera(class WCamera* cam);

	VkRenderPass	GetRenderPass() const;
	VkPipelineCache	GetPipelineCache() const;
	VkCommandBuffer	GetCommnadBuffer() const;
	class WCamera*	GetCamera() const;

	virtual bool	Valid() const;

private:
	struct {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} m_depthStencil;
	std::vector<VkFramebuffer>	m_frameBuffers;
	VkFormat					m_depthFormat, m_colorFormat;
	class WImage*				m_target;

	VkRenderPass				m_renderPass;
	VkPipelineCache				m_pipelineCache;
	VkCommandBuffer				m_renderCmdBuffer;
	VkClearColorValue			m_clearColor;
	unsigned int				m_width, m_height, m_currentFrameBuffer;
	class WCamera*				m_camera;

	void _DestroyResources();
};

class WRenderTargetManager : public WManager<WRenderTarget> {
	friend class WRenderTarget;

	virtual std::string GetTypeName() const;

public:
	WRenderTargetManager(class Wasabi* const app);
	~WRenderTargetManager();
};

