#pragma once

#include "../Core/Core.h"

class WImage : public WBase {
	virtual std::string GetTypeName() const;

public:
	WImage(class Wasabi* const app, unsigned int ID = 0);
	~WImage();

	WError			CreateFromPixelsArray(
						void*			pixels,
						unsigned int	width,
						unsigned int	height,
						bool			bDynamic = false,
						unsigned int	num_components = 4,
						VkFormat		fmt = VK_FORMAT_UNDEFINED,
						size_t			comp_size = sizeof(float));
	WError			Load(std::string filename, bool bDynamic = false);
	WError			CopyFrom(WImage* const image);

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

public:
	WImageManager(class Wasabi* const app);
	~WImageManager();

	WError Load();

	WImage* GetDefaultImage() const;
};
