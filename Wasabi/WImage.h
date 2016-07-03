#pragma once

#include "Wasabi.h"

class WImage : public WBase {
	virtual std::string GetTypeName() const;

	VkImage m_image;
	VkImageLayout m_imageLayout;
	VkDeviceMemory m_deviceMemory;
	VkImageView m_view;
	unsigned int m_width, m_height;
	unsigned int m_mipLevels;

public:
	WImage(Wasabi* const app);
	~WImage();

	WError CretaeFromPixelsArray(
		void*			pixels,
		unsigned int	width,
		unsigned int	height,
		unsigned int	num_components = 4,
		unsigned int	mips = 1);
	WError			Load(std::string filename);

	virtual bool	Valid() const;
};

class WImageManager : public WManager<WImage> {
	friend class WImage;

	virtual std::string GetTypeName() const;

public:
	WImageManager(class Wasabi* const app);
};

