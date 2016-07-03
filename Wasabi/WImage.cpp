#include "WImage.h"

#pragma warning(disable:4477)
#pragma warning(disable:4838)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_DEFINE
#include "stb/stb.h"
#pragma warning(default:4477)
#pragma warning(default:4838)

std::string WImageManager::GetTypeName() const {
	return "Image";
}

WImageManager::WImageManager(class Wasabi* const app) : WManager<WImage>(app) {

}

WImage::WImage(Wasabi* const app) : WBase(app) {

}
WImage::~WImage() {

}

std::string WImage::GetTypeName() const {
	return "Image";
}

bool WImage::Valid() const {
	return true;
}

WError WImage::CretaeFromPixelsArray(
	void*			pixels,
	unsigned int	width,
	unsigned int	height,
	unsigned int	num_components,
	unsigned int	mips) {
	return WError(W_SUCCEEDED);
}

WError WImage::Load(std::string filename) {
	// check if file exists
	FILE* fp;
	fopen_s(&fp, filename.c_str(), "r");
	if (fp)
		fclose(fp);
	else
		return WError(W_FILENOTFOUND);

	int n, w, h;
	unsigned char *data;
	if (stbi_info(filename.c_str(), &w, &h, &n) == 0)
		return WError(W_INVALIDFILEFORMAT);
	data = stbi_load(filename.c_str(), &w, &h, &n, 0);
	if (!data)
		return WError(W_INVALIDFILEFORMAT);

	CretaeFromPixelsArray(data, w, h, n, 1);
	free(data);

	return WError(W_SUCCEEDED);
}
