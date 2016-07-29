#pragma once

#include "Wasabi.h"

class WImage : public WBase {
	virtual std::string GetTypeName() const;

public:
	WImage(Wasabi* const app, unsigned int ID = 0);
	~WImage();

	WError			CretaeFromPixelsArray(
						void*			pixels,
						unsigned int	width,
						unsigned int	height,
						bool			bDynamic = false,
						unsigned int	num_components = 4,
						VkFormat		fmt = VK_FORMAT_UNDEFINED,
						size_t			comp_size = sizeof(float));
	WError			Load(std::string filename, bool bDynamic = false);

	WError			MapPixels(void** const pixels, bool bReadOnly = false);
	void			UnmapPixels();

	VkImageView		GetView() const;
	VkImageLayout	GetViewLayout() const;
	VkFormat		GetFormat() const;
	unsigned int	GetWidth() const;
	unsigned int	GetHeight() const;
	unsigned int	GetNumComponents() const;
	unsigned int	GetComponentSize() const;
	unsigned int	GetPixelSize() const;

	virtual bool	Valid() const;

private:
	VkBuffer		m_stagingBuffer;
	VkDeviceMemory	m_stagingMemory;
	VkImage			m_image;
	VkDeviceMemory	m_deviceMemory;
	VkImageView		m_view;
	VkFormat		m_format;
	bool			m_readOnlyMap;
	unsigned int	m_width, m_height, m_mapSize, m_numComponents, m_componentSize;

	void _DestroyResources();
};

class WImageManager : public WManager<WImage> {
	friend class WImage;

	virtual std::string GetTypeName() const;

	WImage* m_checker_image;
	VkCommandBuffer m_copyCommandBuffer;

	VkResult _BeginCopy();
	VkResult _EndCopy();

public:
	WImageManager(class Wasabi* const app);
	~WImageManager();

	WError Load();

	WImage* GetDefaultImage() const;
};

class WRenderTarget : public WBase {
	virtual std::string GetTypeName() const;

public:
	WRenderTarget(Wasabi* const app, unsigned int ID = 0);
	~WRenderTarget();

	WError			Create(unsigned int width, unsigned int height, WImage* target, bool bDepth = true,
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
	WImage*						m_target;

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

	VkCommandBuffer m_commandBuffer;

	VkResult _BeginSetLayout();
	VkResult _EndSetLayout();

public:
	WRenderTargetManager(class Wasabi* const app);
	~WRenderTargetManager();

	WError Load();
};

