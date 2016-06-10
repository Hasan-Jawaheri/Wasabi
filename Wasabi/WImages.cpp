#include "WImages.h"

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
