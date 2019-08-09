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

#include "Core/WCore.h"

enum W_IMAGE_CREATE_FLAGS {
	W_IMAGE_CREATE_TEXTURE = 1,
	W_IMAGE_CREATE_DYNAMIC = 2,
	W_IMAGE_CREATE_REWRITE_EVERY_FRAME = 4,
	W_IMAGE_CREATE_RENDER_TARGET_ATTACHMENT = 8,
};

inline W_IMAGE_CREATE_FLAGS operator | (W_IMAGE_CREATE_FLAGS lhs, W_IMAGE_CREATE_FLAGS rhs) {
	using T = std::underlying_type_t <W_IMAGE_CREATE_FLAGS>;
	return static_cast<W_IMAGE_CREATE_FLAGS>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline W_IMAGE_CREATE_FLAGS operator & (W_IMAGE_CREATE_FLAGS lhs, W_IMAGE_CREATE_FLAGS rhs) {
	using T = std::underlying_type_t <W_IMAGE_CREATE_FLAGS>;
	return static_cast<W_IMAGE_CREATE_FLAGS>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline W_IMAGE_CREATE_FLAGS& operator |= (W_IMAGE_CREATE_FLAGS& lhs, W_IMAGE_CREATE_FLAGS rhs) {
	lhs = lhs | rhs;
	return lhs;
}

inline W_IMAGE_CREATE_FLAGS& operator &= (W_IMAGE_CREATE_FLAGS& lhs, W_IMAGE_CREATE_FLAGS rhs) {
	lhs = lhs & rhs;
	return lhs;
}

/**
 * @ingroup engineclass
 * This class represents an image, or texture, used by Wasabi.
 */
class WImage : public WFileAsset {
	friend class WRenderTarget;
	friend class WImageManager;
	friend class WFileAsset;

protected:
	virtual ~WImage();

public:
	/**
	 * Returns "Image" string.
	 * @return Returns "Image" string
	 */
	virtual std::string GetTypeName() const;
	static std::string _GetTypeName();

	WImage(class Wasabi* const app, unsigned int ID = 0);

	/**
	 * Creates the image from an array of pixels. The array of pixels need to be
	 * in the same format specified, using the same component size and the same
	 * number of components.
	 *
	 * Examples:
	 * A 64x64 image with a 4-component floating-point pixel (R32G32B32A32).
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
	 * img->CreateFromPixelsArray(pixels, 64, 64, VK_FORMAT_R32G32B32A32_SFLOAT);
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
	 * img->CreateFromPixelsArray(pixels, 64, 64, VK_FORMAT_R32G32B32A32_SFLOAT);
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
	 * // 2 components, each one is 1 byte and the format is
	 * // VK_FORMAT_R8G8_UNORM (so it is 0-255 in memory and 0.0-1.0 when passed
	 * // to the GPU).
	 * img->CreateFromPixelsArray(pixels, 32, 32, VK_FORMAT_R8G8_UNORM);
	 * delete[] pixels;
	 * @endcode
	 * 
	 * @param  pixels         A pointer to the memory containing the pixels. If NULL,
	 *                        the image will not have initial data.
	 * @param  width          Width of the image
	 * @param  height         Height of the image
	 * @param  format         Image format
	 * @param  flags          Image creation flags, see W_IMAGE_CREATE_FLAGS
	 * @return                Error code, see WError.h
	 */
	WError CreateFromPixelsArray(
		void*					pixels,
		uint					width,
		uint					height,
		VkFormat				format,
		W_IMAGE_CREATE_FLAGS	flags = W_IMAGE_CREATE_TEXTURE
	);

	/**
	 * See CreateFromPixelsArray()
	 * @param depth     Depth of the image
	 * @param arraySize Can be used to crteate an array of images
	 */
	WError CreateFromPixelsArray(
		void*					pixels,
		uint					width,
		uint					height,
		uint					depth,
		VkFormat				format,
		uint					arraySize = 1,
		W_IMAGE_CREATE_FLAGS	flags = W_IMAGE_CREATE_TEXTURE
	);

	/**
	 * Loads an image from a file. The image format can be any of the formats
	 * supported by the stb library (includes .png, .jpg, .tga, .bmp).
	 * @param  filename Name of the file to load
	 * @param  flags    Image creation flags, see W_IMAGE_CREATE_FLAGS
	 * @return          Error code, see WError.h
	 */
	WError Load(std::string filename, W_IMAGE_CREATE_FLAGS flags = W_IMAGE_CREATE_TEXTURE);

	/**
	 * Copy another WImage. Only images created with bDynamic == true can be
	 * copied.
	 * @param  image  Pointer to the (dynamic) image to copy from
	 * @param  flags  Image creation flags, see W_IMAGE_CREATE_FLAGS
	 * @return        Error code, see WError.h
	 */
	WError CopyFrom(WImage* const image, W_IMAGE_CREATE_FLAGS flags = W_IMAGE_CREATE_TEXTURE);

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
	 * @param  flags     Map flags (bitwise OR'd), specifying read/write intention
	 * @return           Error code, see WError.h
	 */
	WError MapPixels(void** const pixels, W_MAP_FLAGS flags);

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
	 * Transitions the layout of the currently buffered image to the specified layout
	 * @param cmdBuf     Command buffer to perform the transition in
	 * @param newLayout  New Vulkan layout for the underlying image
	 */
	void TransitionLayoutTo(VkCommandBuffer cmdBuf, VkImageLayout newLayout);

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
	 * Retrieves the depth of the image.
	 * @return Depth of the image, in pixels
	 */
	unsigned int GetDepth() const;

	/**
	 * Retrieves the size of the array of images (if used, default is 1).
	 * @return Size of the image array
	 */
	unsigned int GetArraySize() const;

	/**
	 * Returns true if the image is valid. The image is valid if it has a usable
	 * Vulkan image view.
	 * @return true if the image is valid, false otherwise
	 */
	virtual bool Valid() const;

	/**
	 * Retrieves the size of a pixel in this image.
	 * @return Size of a pixel, in bytes
	 */
	unsigned int GetPixelSize() const;

	static std::vector<void*> LoadArgs(W_IMAGE_CREATE_FLAGS flags = W_IMAGE_CREATE_TEXTURE);
	virtual WError SaveToStream(WFile* file, std::ostream& outputStream);
	virtual WError LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args);

private:
	/** Buffered image resource */
	WBufferedImage m_bufferedImage;
	/** The Vulkan format */
	VkFormat m_format;
	/** An array of buffered maps to perform, one per buffered image */
	std::vector<void*> m_pendingBufferedMaps;

	/**
	 * Cleanup all image resources (including all Vulkan-related resources)
	 */
	void _DestroyResources();

	/**
	 * Performs the pending map for the given buffer index
	 */
	void _PerformPendingMap(uint bufferIndex);

	/**
	 * Performs an update necessary to the buffered image at a given index after it gets mapped
	 */
	void _UpdatePendingMap(void* mappedData, uint bufferIndex, W_MAP_FLAGS mapFlags);

	/**
	 * Performs an update necessary to the buffered image at a given index before it gets unmapped
	 */
	void _UpdatePendingUnmap(uint bufferIndex);
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

	/** A container of the dynamic images that need to be (possibly) updated
	    per-frame for buffered mapping/unmapping */
	std::unordered_map<WImage*, bool> m_dynamicImages;

	/** This is the default image, which is a checker board */
	WImage* m_checkerImage;

public:
	WImageManager(class Wasabi* const app);
	~WImageManager();

	/**
	 * Loads the manager and creates the default checker image.
	 * @return Error code, see WError.h
	 */
	WError Load();

	/**
	 * Allocates an image.
	 */
	WImage* CreateImage(unsigned int ID = 0);

	/**
	 * Allocates and builds a new image from supplied pixels. See
	 * WImage::CreateFromPixelsArray().
	 */
	WImage* CreateImage(void* pixels, uint width, uint height, VkFormat format, W_IMAGE_CREATE_FLAGS flags = W_IMAGE_CREATE_TEXTURE, unsigned int ID = 0);

	/**
	 * Allocates and builds a new image from a file. See
	 * WImage::Load().
	 */
	WImage* CreateImage(std::string filename, W_IMAGE_CREATE_FLAGS flags = W_IMAGE_CREATE_TEXTURE, unsigned int ID = 0);

	/**
	 * Allocates and builds a new image that is a copy of another. See
	 * WImage::CopyFrom().
	 */
	WImage* CreateImage(WImage* const image, W_IMAGE_CREATE_FLAGS flags = W_IMAGE_CREATE_TEXTURE, unsigned int ID = 0);

	/**
	 * Retrieves the default (checkers) image.
	 * @return A pointer to the default image
	 */
	WImage* GetDefaultImage() const;

	/**
	 * Makes sure all Map call results are propagated to the buffered images at
	 * the given buffer index.
	 */
	void UpdateDynamicImages(uint bufferIndex) const;
};
