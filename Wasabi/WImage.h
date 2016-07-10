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
	unsigned int	GetWidth();
	unsigned int	GetHeight();

	virtual bool	Valid() const;

private:
	VkBuffer m_stagingBuffer;
	VkDeviceMemory m_stagingMemory;
	VkImage m_image;
	VkDeviceMemory m_deviceMemory;
	VkImageView m_view;
	bool m_readOnlyMap;
	unsigned int m_width, m_height, m_mapSize;

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

