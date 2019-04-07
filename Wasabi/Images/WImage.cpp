#include "WImage.h"
#include "../Renderers/WRenderer.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-fpermissive"
#pragma warning(disable:4477)
#pragma warning(disable:4838)
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

#define STB_DEFINE
#include "../stb/stb.h"

#ifdef __GNUC__
#pragma warning(default:4477)
#pragma warning(default:4838)
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif

std::string WImageManager::GetTypeName() const {
	return "Image";
}

WImageManager::WImageManager(class Wasabi* const app) : WManager<WImage>(app) {
	m_checkerImage = nullptr;
}

WImageManager::~WImageManager() {
	W_SAFE_REMOVEREF(m_checkerImage);

	// we need to perform this here because some destructed images will need access to m_dynamicImages
	// which will be destructed by the time WManager::~WManager() destroys the images this way
	for (unsigned int j = 0; j < W_HASHTABLESIZE; j++) {
		for (unsigned int i = 0; i < m_entities[j].size(); i)
			m_entities[j][i]->RemoveReference();
		m_entities[j].clear();
	}
}

WError WImageManager::Load() {
	m_checkerImage = new WImage(m_app);
	int size = 256;
	int comp_size = 4;
	int check_size = 16;
	float* pixels = new float[size * size * comp_size];
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			int col = ((y/ check_size) + ((x/ check_size) % 2)) % 2;
			float fCol = col == 0 ? 0.1f : 1.0f;
			if (comp_size > 0)
				pixels[(y*size + x) * comp_size + 0] = fCol;
			if (comp_size > 1)
				pixels[(y*size + x) * comp_size + 1] = fCol;
			if (comp_size > 2)
				pixels[(y*size + x) * comp_size + 2] = fCol;
			if (comp_size > 3)
				pixels[(y*size + x) * comp_size + 3] = 1;
		}
	}
	WError werr = m_checkerImage->CreateFromPixelsArray(pixels, size, size, VK_FORMAT_R32G32B32A32_SFLOAT);
	delete[] pixels;
	if (!werr) {
		W_SAFE_REMOVEREF(m_checkerImage);
		return werr;
	}

	return WError(W_SUCCEEDED);
}

WImage* WImageManager::GetDefaultImage() const {
	return m_checkerImage;
}

WImage* WImageManager::CreateImage(unsigned int ID) {
	return new WImage(m_app, ID);
}

WImage* WImageManager::CreateImage(void* pixels, uint width, uint height, VkFormat format, W_IMAGE_CREATE_FLAGS flags, unsigned int ID) {
	WImage* img = new WImage(m_app, ID);
	WError err = img->CreateFromPixelsArray(pixels, width, height, format, flags);
	if (!err)
		W_SAFE_REMOVEREF(img);
	return img;
}

WImage* WImageManager::CreateImage(std::string filename, W_IMAGE_CREATE_FLAGS flags, unsigned int ID) {
	WImage* img = new WImage(m_app, ID);
	WError err = img->Load(filename, flags);
	if (!err)
		W_SAFE_REMOVEREF(img);
	return img;
}

WImage* WImageManager::CreateImage(WImage* const image, W_IMAGE_CREATE_FLAGS flags, unsigned int ID) {
	WImage* img = new WImage(m_app, ID);
	WError err = img->CopyFrom(image, flags);
	if (!err)
		W_SAFE_REMOVEREF(img);
	return img;
}

void WImageManager::UpdateDynamicImages(uint bufferIndex) const {
	for (auto it = m_dynamicImages.begin(); it != m_dynamicImages.end(); it++) {
		it->first->_PerformPendingMap(bufferIndex);
	}
}

WImage::WImage(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_app->ImageManager->AddEntity(this);
}
WImage::~WImage() {
	_DestroyResources();

	m_app->ImageManager->RemoveEntity(this);
}

std::string WImage::GetTypeName() const {
	return "Image";
}

bool WImage::Valid() const {
	return m_bufferedImage.Valid();
}

void WImage::_DestroyResources() {
	for (auto bufIt = m_pendingBufferedMaps.begin(); bufIt != m_pendingBufferedMaps.end(); bufIt++) {
		if (*bufIt != nullptr) {
			W_SAFE_FREE(*bufIt);
			break;
		}
	}
	m_pendingBufferedMaps.clear();
	auto it = m_app->ImageManager->m_dynamicImages.find(this);
	if (it != m_app->ImageManager->m_dynamicImages.end())
		m_app->ImageManager->m_dynamicImages.erase(it);

	m_bufferedImage.Destroy(m_app);
}

WError WImage::CreateFromPixelsArray(void* pixels, uint width, uint height, VkFormat format, W_IMAGE_CREATE_FLAGS flags) {
	_DestroyResources();

	bool isDepth = format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
		format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_S8_UINT ||
		format == VK_FORMAT_D16_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
	VkImageUsageFlags usageFlags = 0;
	if (flags & W_IMAGE_CREATE_TEXTURE) usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (flags & W_IMAGE_CREATE_RENDER_TARGET_ATTACHMENT) usageFlags |= (isDepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	W_MEMORY_STORAGE memory = flags & W_IMAGE_CREATE_DYNAMIC ? W_MEMORY_HOST_VISIBLE : W_MEMORY_DEVICE_LOCAL;
	uint numBuffers = (flags & (W_IMAGE_CREATE_DYNAMIC | W_IMAGE_CREATE_RENDER_TARGET_ATTACHMENT)) ? (uint)m_app->engineParams["bufferingCount"] : 1;
	VkResult result = m_bufferedImage.Create(m_app, numBuffers, width, height, format, pixels, memory, usageFlags);
	if (result != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	if (flags & W_IMAGE_CREATE_DYNAMIC && !(flags & W_IMAGE_CREATE_REWRITE_EVERY_FRAME)) {
		m_pendingBufferedMaps.resize(numBuffers);
		m_app->ImageManager->m_dynamicImages.insert(std::make_pair(this, true));
	}

	m_format = format;

	return WError(W_SUCCEEDED);
}

WError WImage::Load(std::string filename, W_IMAGE_CREATE_FLAGS flags) {
	// check if file exists
	FILE* fp;
	fopen_s(&fp, filename.c_str(), "r");
	if (fp)
		fclose(fp);
	else
		return WError(W_FILENOTFOUND);

	int n = 4, w, h;
	unsigned char *data;
	if (stbi_info(filename.c_str(), &w, &h, &n) == 0)
		return WError(W_INVALIDFILEFORMAT);
	data = stbi_load(filename.c_str(), &w, &h, &n, 0);
	if (!data)
		return WError(W_INVALIDFILEFORMAT);

	// convert from 8-bit char components to 32-bit float components
	float* pixels = new float[w*h*n];
	for (int i = 0; i < w*h*n; i++) {
		float f = (float)data[i] / (float)(unsigned char)(-1);
		pixels[i] = f;
	}
	free(data);

	VkFormat format;
	switch (n) {
	case 1: format = VK_FORMAT_R32_SFLOAT; break;
	case 2: format = VK_FORMAT_R32G32_SFLOAT; break;
	case 3: format = VK_FORMAT_R32G32B32_SFLOAT; break;
	case 4: format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
	}

	WError err = CreateFromPixelsArray(pixels, w, h, format, flags);
	delete[] pixels;

	return err;
}

WError WImage::CopyFrom(WImage* const image, W_IMAGE_CREATE_FLAGS flags) {
	if (!image || !image->Valid())
		return WError(W_INVALIDPARAM);

	void* pixels;
	WError res = image->MapPixels(&pixels, W_MAP_READ);
	if (!res)
		return res;
	res = CreateFromPixelsArray(pixels, image->GetWidth(), image->GetHeight(), image->m_format, flags);
	image->UnmapPixels();

	return res;
}

void WImage::_PerformPendingMap(uint bufferIndex) {
	if (m_pendingBufferedMaps.size() > 0 && m_pendingBufferedMaps[bufferIndex]) {
		void* data = m_pendingBufferedMaps[bufferIndex];
		void* pImagePixels;
		if (m_bufferedImage.Map(m_app, bufferIndex, &pImagePixels, W_MAP_WRITE) == VK_SUCCESS) {
			memcpy(pImagePixels, data, m_bufferedImage.GetMemorySize());
			m_bufferedImage.Unmap(m_app, bufferIndex);
			m_pendingBufferedMaps[bufferIndex] = nullptr;
			uint numRemainingPointers = 0;
			for (auto bufIt = m_pendingBufferedMaps.begin(); bufIt != m_pendingBufferedMaps.end(); bufIt++)
				numRemainingPointers += (*bufIt == nullptr) ? 0 : 1;
			if (numRemainingPointers == 0)
				W_SAFE_FREE(data); // last buffer to erase -> free the memory
		}
	}
}

void WImage::_UpdatePendingMap(void* mappedData, uint bufferIndex, W_MAP_FLAGS mapFlags) {
	if (m_pendingBufferedMaps.size() > 0) {
		if (mapFlags & W_MAP_READ && m_pendingBufferedMaps[bufferIndex]) {
			// the user intends to read and there is a pending write to this image, perform the write
			memcpy(mappedData, m_pendingBufferedMaps[bufferIndex], m_bufferedImage.GetMemorySize());
		}

		// delete all old buffered mapping
		bool bDeleted = false;
		for (auto bufIt = m_pendingBufferedMaps.begin(); bufIt != m_pendingBufferedMaps.end(); bufIt++) {
			if (*bufIt != nullptr && !bDeleted) {
				W_SAFE_FREE(*bufIt);
				bDeleted = true;
			} else if (bDeleted)
				*bufIt = nullptr;
		}
		m_pendingBufferedMaps[bufferIndex] = mappedData;
	}
}

void WImage::_UpdatePendingUnmap(uint bufferIndex) {
	if (m_pendingBufferedMaps.size() > 0) {
		// we stored the mapped pixels in m_pendingBufferedMaps[bufferIndex]
		// create a copy of it and add it as pending for the other 
		void* bufferedMaps = W_SAFE_ALLOC(m_bufferedImage.GetMemorySize());
		memcpy(bufferedMaps, m_pendingBufferedMaps[bufferIndex], m_bufferedImage.GetMemorySize());
		for (uint i = 0; i < m_pendingBufferedMaps.size(); i++) {
			if (i == bufferIndex)
				m_pendingBufferedMaps[i] = nullptr;
			else
				m_pendingBufferedMaps[i] = bufferedMaps;
		}
	}
}

WError WImage::MapPixels(void** const pixels, W_MAP_FLAGS flags) {
	uint bufferIndex = m_app->GetCurrentBufferingIndex();
	VkResult result = m_bufferedImage.Map(m_app, bufferIndex, pixels, flags);
	if (result != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	_UpdatePendingMap(*pixels, bufferIndex, flags);

	return WError(W_SUCCEEDED);
}

void WImage::UnmapPixels() {
	uint bufferIndex = m_app->GetCurrentBufferingIndex();
	_UpdatePendingUnmap(bufferIndex);
	m_bufferedImage.Unmap(m_app, bufferIndex);
}

VkImageView WImage::GetView() const {
	uint bufferIndex = m_app->GetCurrentBufferingIndex();
	return m_bufferedImage.GetView(m_app, bufferIndex);
}

VkImageLayout WImage::GetViewLayout() const {
	return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VkFormat WImage::GetFormat() const {
	return m_format;
}

unsigned int WImage::GetWidth() const {
	return m_bufferedImage.GetWidth();
}

unsigned int WImage::GetHeight() const {
	return m_bufferedImage.GetHeight();
}

unsigned int WImage::GetPixelSize() const {
	return m_bufferedImage.GetMemorySize() / (GetWidth() * GetHeight());
}

WError WImage::SaveToStream(WFile* file, std::ostream& outputStream) {
	if (!Valid())
		return WError(W_NOTVALID);

	uint width = GetWidth();
	uint height = GetHeight();
	outputStream.write((char*)&width, sizeof(width));
	outputStream.write((char*)&height, sizeof(height));
	outputStream.write((char*)&m_format, sizeof(m_format));
	uint dataSize = width * height * GetPixelSize();
	outputStream.write((char*)&dataSize, sizeof(dataSize));

	void* pixels;
	WError err = MapPixels(&pixels, W_MAP_READ);
	if (err) {
		outputStream.write((char*)pixels, dataSize);
		UnmapPixels();
	}

	return err;
}

WError WImage::LoadFromStream(WFile* file, std::istream& inputStream) {
	W_IMAGE_CREATE_FLAGS flags = W_IMAGE_CREATE_TEXTURE;
	uint width, height;
	VkFormat format;

	inputStream.read((char*)&width, sizeof(width));
	inputStream.read((char*)&height, sizeof(height));
	inputStream.read((char*)&format, sizeof(format));
	uint dataSize;
	inputStream.read((char*)&dataSize, sizeof(dataSize));

	void* pixels = W_SAFE_ALLOC(dataSize);
	if (!pixels)
		return WError(W_OUTOFMEMORY);
	inputStream.read((char*)pixels, dataSize);
	WError err = CreateFromPixelsArray(pixels, width, height, format, flags);
	W_SAFE_FREE(pixels);
	return err;
}
