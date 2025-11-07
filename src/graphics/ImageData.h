#ifndef GRAPHICS_IMAGEDATA_H_
#define GRAPHICS_IMAGEDATA_H_

#include "../typedefs.h"
#include <stdint.h>

enum class ImageFormat {
	rgb888,
	rgba8888
};

class ImageData {
	ImageFormat format;
	uint width;
	uint height;
	void* data;
	
public:
	ImageData(ImageFormat format, uint width, uint height, void* data);
	~ImageData();
	
	ImageFormat getFormat() const { return format; }
	uint getWidth() const { return width; }
	uint getHeight() const { return height; }
	void* getData() const { return data; }
};

#endif /* GRAPHICS_IMAGEDATA_H_ */

