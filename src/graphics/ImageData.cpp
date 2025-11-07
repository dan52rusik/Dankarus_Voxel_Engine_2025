#include "ImageData.h"
#include <cstdlib>

ImageData::ImageData(ImageFormat format, uint width, uint height, void* data)
	: format(format), width(width), height(height), data(data) {
}

ImageData::~ImageData() {
	if (data != nullptr) {
		free(data);
	}
}

