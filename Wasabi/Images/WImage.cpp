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
	m_checker_image = nullptr;
}

WImageManager::~WImageManager() {
	W_SAFE_REMOVEREF(m_checker_image);
}

WError WImageManager::Load() {
	m_checker_image = new WImage(m_app);
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
	WError werr = m_checker_image->CreateFromPixelsArray(pixels, size, size, VK_FORMAT_R32G32B32A32_SFLOAT);
	delete[] pixels;
	if (!werr) {
		W_SAFE_REMOVEREF(m_checker_image);
		return werr;
	}

	return WError(W_SUCCEEDED);
}

WImage* WImageManager::GetDefaultImage() const {
	return m_checker_image;
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
	m_bufferedImage.Destroy(m_app);
}

WError WImage::CreateFromPixelsArray(void* texels, uint width, uint height, VkFormat format, bool isRenderTargetAttachment, bool isDynamic) {
	_DestroyResources();

	bool isDepth = format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
		format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_S8_UINT ||
		format == VK_FORMAT_D16_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
	VkImageUsageFlags usageFlags = (isRenderTargetAttachment ? (isDepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) : 0) | VK_IMAGE_USAGE_SAMPLED_BIT;
	W_MEMORY_STORAGE memory = isDynamic ? W_MEMORY_HOST_VISIBLE : W_MEMORY_DEVICE_LOCAL;
	uint numBuffers = (uint)m_app->engineParams["bufferingCount"];
	m_bufferedImage.Create(m_app, isDynamic || isRenderTargetAttachment ? numBuffers : 1, width, height, format, texels, memory, usageFlags);

	m_width = width;
	m_height = height;
	m_format = format;

	return WError(W_SUCCEEDED);
}

WError WImage::Load(std::string filename, bool bDynamic) {
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

	WError err = CreateFromPixelsArray(pixels, w, h, format, bDynamic);
	delete[] pixels;

	return err;
}

WError WImage::CopyFrom(WImage* const image, bool isRenderTargetAttachment, bool isDynamic) {
	if (!image || !image->Valid())
		return WError(W_INVALIDPARAM);

	void* pixels;
	WError res = image->MapPixels(&pixels, W_MAP_READ);
	if (!res)
		return res;
	res = CreateFromPixelsArray(pixels, image->m_width, image->m_height, image->m_format, isRenderTargetAttachment, isDynamic);
	image->UnmapPixels();

	return res;
}

WError WImage::MapPixels(void** const pixels, W_MAP_FLAGS flags) {
	uint bufferIndex = m_app->Renderer->GetCurrentBufferingIndex();
	VkResult result = m_bufferedImage.Map(m_app, bufferIndex, pixels, flags);
	if (result != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);
	return WError(W_SUCCEEDED);
}

void WImage::UnmapPixels() {
	uint bufferIndex = m_app->Renderer->GetCurrentBufferingIndex();
	m_bufferedImage.Unmap(m_app, bufferIndex);
}

VkImageView WImage::GetView() const {
	uint bufferIndex = m_app->Renderer->GetCurrentBufferingIndex();
	return m_bufferedImage.GetView(m_app, bufferIndex);
}

VkImageLayout WImage::GetViewLayout() const {
	return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VkFormat WImage::GetFormat() const {
	return m_format;
}

unsigned int WImage::GetWidth() const {
	return m_width;
}

unsigned int WImage::GetHeight() const {
	return m_height;
}

unsigned int WImage::GetPixelSize() const {
	return m_bufferedImage.GetMemorySize() / (m_width * m_height);
}

WError WImage::SaveToStream(WFile* file, std::ostream& outputStream) {
	if (!Valid())
		return WError(W_NOTVALID);

	outputStream.write((char*)&m_width, sizeof(m_width));
	outputStream.write((char*)&m_height, sizeof(m_height));
	outputStream.write((char*)&m_format, sizeof(m_format));
	uint dataSize = m_width * m_height * GetPixelSize();
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
	bool bDynamic = false;
	unsigned int width, height;
	VkFormat format;

	inputStream.read((char*)&width, sizeof(m_width));
	inputStream.read((char*)&height, sizeof(m_height));
	inputStream.read((char*)&format, sizeof(m_format));
	uint dataSize;
	inputStream.read((char*)&dataSize, sizeof(dataSize));

	void* pixels = W_SAFE_ALLOC(dataSize);
	if (!pixels)
		return WError(W_OUTOFMEMORY);
	inputStream.read((char*)pixels, dataSize);
	WError err = CreateFromPixelsArray(pixels, width, height, format, false, bDynamic);
	W_SAFE_FREE(pixels);
	return err;
}
