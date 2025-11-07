#include "Texture.h"
#include "ImageData.h"
#include <GL/glew.h>

Texture::Texture(unsigned int id, int width, int height) : id(id), width(width), height(height) {
}

Texture::Texture(unsigned char* data, int width, int height) : width(width), height(height) {
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::Texture(unsigned char* data, int width, int height, unsigned int format) : width(width), height(height) {
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
		format, GL_UNSIGNED_BYTE, (GLvoid *) data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture() {
	glDeleteTextures(1, &id);
}

void Texture::bind(){
	glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::reload(unsigned char* data){
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture* Texture::from(ImageData* image) {
	if (image == nullptr) {
		return nullptr;
	}
	
	unsigned int format = GL_RGBA;
	if (image->getFormat() == ImageFormat::rgb888) {
		format = GL_RGB;
	}
	
	return new Texture((unsigned char*)image->getData(), 
		image->getWidth(), image->getHeight(), format);
}
