#pragma once

#include "Wasabi.h"

class WImage : public WBase {
	virtual std::string GetTypeName() const;

	VkImage m_image;
	VkDeviceMemory m_deviceMemory;
	VkImageView m_view;
	unsigned int m_width, m_height;

	void _DestroyResources();

public:
	WImage(Wasabi* const app, unsigned int ID = 0);
	~WImage();

	WError			CretaeFromPixelsArray(
						void*			pixels,
						unsigned int	width,
						unsigned int	height,
						unsigned int	num_components = 4,
						VkFormat		fmt = VK_FORMAT_UNDEFINED,
						size_t			comp_size = sizeof(float));
	WError			Load(std::string filename);

	VkImageView		GetView() const;
	VkImageLayout	GetViewLayout() const;
	unsigned int	GetWidth();
	unsigned int	GetHeight();

	virtual bool	Valid() const;
};

class WImageManager : public WManager<WImage> {
	friend class WImage;

	virtual std::string GetTypeName() const;

	WImage* m_checker_image;

public:
	WImageManager(class Wasabi* const app);

	void Load();

	WImage* GetDefaultImage() const;
};

