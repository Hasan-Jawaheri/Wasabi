/** @file WImage.h
 *  @brief Images (textures) implementation
 *
 *  An image in Wasabi is a way to represent textures that could be used for
 *  any purpose. Wasabi images support custom formats and dynamic mapping/
 *  unmapping for dynamically modifing images.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"

/**
 * @ingroup engineclass
 * This class represents an image, or texture, used by Wasabi.
 */
class WImage : public WBase, public WFileAsset {
	/**
	 * Returns "Image" string.
	 * @return Returns "Image" string
	 */
	virtual std::string GetTypeName() const;

public:
	WImage(class Wasabi* const app, unsigned int ID = 0);
	~WImage();

	/**
	 * Creates the image from an array of pixels. The array of pixels need to be
	 * in the same format specified, using the same component size and the same
	 * number of components.
	 *
	 * If fmt is VK_FORMAT_UNDEFINED, then the format will be chosen as follows:
	 * * num_components == 1: VK_FORMAT_R32_SFLOAT
	 * * num_components == 2: VK_FORMAT_R32G32_SFLOAT
	 * * num_components == 3: VK_FORMAT_R32G32B32_SFLOAT
	 * * num_components == 4: VK_FORMAT_R32G32B32A32_SFLOAT
	 * 
	 * Meaning that each pixel in the pixels array will be expected to be
	 * \a num_components consecutive floats.
	 *
	 * Examples:
	 * A 64x64 image with a 4-component floating-point pixel.
	 * @code
	 * WImage* img = new WImage(this);
	 * float* pixels = new float[64*64*4]; // (64x64 image, 4 components each)
	 * for (int y = 0; y < 64; y++) {
	 * 	 for (int x = 0; x < 64; x++) {
	 * 	 	 // Each 4 floats in pixels is one pixel. So pixels[0] to pixels[3] is
	 * 	 	 // the first pixel, pixels[4] to pixels[7] is the second, etc...
	 * 	 	 // So to get the index into the pixels using x and y coordinates, we
	 * 	 	 // use the regular "index = y * width + x" but we multiply it by 4 to
	 * 	 	 // have "index = y * width * 4 + x * 4" so that we account for having
	 * 	 	 // 4 components each pixel
	 * 	 	 pixels[(y*64+x)*4 + 0] = 0.1f; // red component at (x, y)
	 * 	 	 pixels[(y*64+x)*4 + 1] = 0.4f; // green component at (x, y)
	 * 	 	 pixels[(y*64+x)*4 + 2] = 1.0f; // blue component at (x, y)
	 * 	 	 pixels[(y*64+x)*4 + 3] = 1.0f; // alpha component at (x, y)
	 * 	 }
	 * }
	 * img->CreateFromPixelsArray(pixels, 64, 64);
	 * delete[] pixels;
	 * @endcode
	 * Same as the above example, but using WColor for easier access and more
	 * readability.
	 * @code
	 * WImage* img = new WImage(this);
	 * WColor* pixels = new WColor[64*64];
	 * for (int y = 0; y < 64; y++) {
	 * 	 for (int x = 0; x < 64; x++) {
	 * 	 	 pixels[y*64+x] = WColor(0.1f, 0.4f, 1.0f, 1.0f);
	 * 	 }
	 * }
	 * img->CreateFromPixelsArray(pixels, 64, 64);
	 * delete[] pixels;
	 * @endcode
	 * Creating a 32x32, 2 component UNORM format image.
	 * @code
	 * WImage* img = new WImage(this);
	 * char* pixels = new char[32*32*2];
	 * for (int y = 0; y < 32; y++) {
	 * 	 for (int x = 0; x < 32; x++) {
	 * 	 	 pixels[(y*64+x)*2 + 0] = 0;
	 * 	 	 pixels[(y*64+x)*2 + 1] = 255;
	 * 	 }
	 * }
	 * // 2 components, each one is 1 byte (sizeof (char)) and the format is
	 * // VK_FORMAT_R8G8_UNORM (so it is 0-255 in memory and 0.0-1.0 when passed
	 * // to the GPU).
	 * img->CreateFromPixelsArray(pixels, 32, 32, false, 2
	 *                            VK_FORMAT_R8G8_UNORM, 1);
	 * delete[] pixels;
	 * @endcode
	 * 
	 * @param  pixels         A pointer to the memory containing the pixels. If NULL,
	 *                        the image will not have initial data.
	 * @param  width          Width of the image
	 * @param  height         Height of the image
	 * @param  bDynamic       Should be set to true for images that will be
	 *                        modified in the future (via MapPixels() and
	 *                        UnmapPixels()). Setting this to true results in
	 *                        using more memory
	 * @param  num_components Number of components of each pixel in pixels (
	 *                        valid values are 1, 2, 3 and 4)
	 * @param  fmt            If set to VK_FORMAT_UNDEFINED, the image format
	 *                        will be chosen automatically, otherwise fmt will
	 *                        be used
	 * @param  comp_size      The size of each component (in bytes) of a pixel
	 *                        in the pixels array. This should only be used if
	 *                        fmt is not VK_FORMAT_UNDEFINED, otherwise it should
	 *                        be 4 (sizeof (float))
	 * @return                Error code, see WError.h
	 */
	WError CreateFromPixelsArray(void*			pixels,
								 unsigned int	width,
								 unsigned int	height,
								 bool			bDynamic = false,
								 unsigned int	num_components = 4,
								 VkFormat		fmt = VK_FORMAT_UNDEFINED,
								 size_t			comp_size = sizeof(float));

	/**
	 * Loads an image from a file. The image format can be any of the formats
	 * supported by the stb library (includes .png, .jpg, .tga, .bmp).
	 * @param  filename Name of the file to load
	 * @param  bDynamic Should be set to true for images that will be modified
	 *                  in the future (via MapPixels() and UnmapPixels()).
	 *                  Setting this to true results in using more memory
	 * @return          Error code, see WError.h
	 */
	WError Load(std::string filename, bool bDynamic = false);

	/**
	 * Copy another WImage. Only images created with bDynamic == true can be
	 * copied.
	 * @param  image Pointer to the image to copy from, which should be dynamic
	 * @return       Error code, see WError.h
	 */
	WError CopyFrom(WImage* const image);

	/**
	 * Maps the pixels of the image for reading or writing. Only images created
	 * with bDynamic == true can be mapped.
	 *
	 * Examples:
	 * @code
	 * WColor* pixels; // Assuming the image is 4 components float-point format
	 * img->MapPixels((void**)&pixels);
	 * // Change the pixel at index 10 to full red
	 * pixels[10] = WColor(1.0f, 0.0f, 0.0f, 1.0f);
	 * img->UnmapPixels();
	 * @endcode
	 * 
	 * @param  pixels    The address of a pointer to have it point to the mapped
	 *                   memory of the pixels
	 * @param  bReadOnly Set to true if you intend to only read from the pixels
	 *                   array, false if you want to modify the pixels
	 * @return           Error code, see WError.h
	 */
	WError MapPixels(void** const pixels, bool bReadOnly = false);

	/**
	 * Unmaps pixels from a previous MapPixels() call. If MapPixels was called
	 * with bReadOnly set to false, this will apply the changes to the image.
	 */
	void UnmapPixels();

	/**
	 * Retrieves the Vulkan image view object for this image.
	 * @return The image view
	 */
	VkImageView GetView() const;

	/**
	 * Retrieves the Vulkan image layout for the image view.
	 * @return The image view layout
	 */
	VkImageLayout GetViewLayout() const;

	/**
	 * Retrieves the Vulkan format used for this image.
	 * @return The format of the image
	 */
	VkFormat GetFormat() const;

	/**
	 * Retrieves the width of the image.
	 * @return Width of the image, in pixels
	 */
	unsigned int GetWidth() const;

	/**
	 * Retrieves the height of the image.
	 * @return Height of the image, in pixels
	 */
	unsigned int GetHeight() const;

	/**
	 * Retrieves the number of components in each pixels of this image.
	 * @return Number of components per pixel
	 */
	unsigned int GetNumComponents() const;

	/**
	 * Retrieves the size of each component in a pixel.
	 * @return Size of a component, in bytes
	 */
	unsigned int GetComponentSize() const;

	/**
	 * Retrieves the size of a pixel in this image.
	 * @return Size of a pixel, in bytes
	 */
	unsigned int GetPixelSize() const;

	/**
	 * Returns true if the image is valid. The image is valid if it has a usable
	 * Vulkan image view.
	 * @return true if the image is valid, false otherwise
	 */
	virtual bool Valid() const;

	virtual WError SaveToStream(WFile* file, std::ostream& outputStream);
	virtual WError LoadFromStream(WFile* file, std::istream& inputStream);

private:
	/** A staging buffer used to create the image and map/unmap (if dynamic) */
	VkBuffer m_stagingBuffer;
	/** Memory backing the staging buffer */
	VkDeviceMemory m_stagingMemory;
	/** Vulkan image handle */
	VkImage m_image;
	/** Memory backing the Vulkan image */
	VkDeviceMemory m_deviceMemory;
	/** The Vulkan image view handle */
	VkImageView m_view;
	/** The Vulkan format */
	VkFormat m_format;
	/** true if the last MapPixels() was read-only */
	bool m_readOnlyMap;
	/** Width of the image, in pixels */
	unsigned int m_width;
	/** Height of the image, in pixels */
	unsigned int m_height;
	/** Size of the entire image memory buffer, in bytes, for mapping */
	unsigned int m_mapSize;
	/** Number of pixel components */
	unsigned int m_numComponents;
	/** Size of each pixel component, in bytes */
	unsigned int m_componentSize;

	/**
	 * Cleanup all image resources (including all Vulkan-related resources)
	 */
	void _DestroyResources();
};

/**
 * @ingroup engineclass
 * Manager class for WCamera.
 */
class WImageManager : public WManager<WImage> {
	friend class WImage;

	/**
	 * Returns "Image" string.
	 * @return Returns "Image" string
	 */
	virtual std::string GetTypeName() const;

	/** This is the default image, which is a checker board */
	WImage* m_checker_image;

public:
	WImageManager(class Wasabi* const app);
	~WImageManager();

	/**
	 * Loads the manager and creates the default checker image.
	 * @return Error code, see WError.h
	 */
	WError Load();

	/**
	 * Retrieves the default (checkers) image.
	 * @return A pointer to the default image
	 */
	WImage* GetDefaultImage() const;
};
