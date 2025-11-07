#ifndef GRAPHICS_TEXTURE_H_
#define GRAPHICS_TEXTURE_H_

#include <string>

class ImageData;

class Texture {
public:
	unsigned int id;
	int width;
	int height;
	Texture(unsigned int id, int width, int height);
	Texture(unsigned char* data, int width, int height);
	Texture(unsigned char* data, int width, int height, unsigned int format);
	~Texture();

	void bind();
	void reload(unsigned char* data);
	
	static Texture* from(ImageData* image);
};

extern Texture* load_texture(std::string filename);

#endif /* GRAPHICS_TEXTURE_H_ */
