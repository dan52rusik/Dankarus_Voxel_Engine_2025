#include "Batch2D.h"
#include "Mesh.h"
#include "Texture.h"

#include <GL/glew.h>
#include <cmath>

#define VERTEX_SIZE 8

Batch2D::Batch2D(size_t capacity) : capacity(capacity), offset(0), color(1.0f, 1.0f, 1.0f, 1.0f){
	const vattr attrs[] = {
		{2}, {2}, {4}, {0}
	};
	const int attrsInt[] = {
		attrs[0].size, attrs[1].size, attrs[2].size, attrs[3].size
	};

	buffer = new float[capacity * VERTEX_SIZE];
	mesh = new Mesh(buffer, 0, attrsInt);
	index = 0;

	unsigned char pixels[] = {
			255, 255, 255, 255,
	};
	blank = new Texture(pixels, 1, 1, GL_RGBA);
	_texture = nullptr;
}

Batch2D::~Batch2D(){
	delete blank;
	delete[] buffer;
	delete mesh;
}

void Batch2D::begin(){
	_texture = nullptr;
	blank->bind();
	index = 0;
}

void Batch2D::vertex(float x, float y,
		float u, float v,
		float r, float g, float b, float a) {
	buffer[index++] = x;
	buffer[index++] = y;
	buffer[index++] = u;
	buffer[index++] = v;
	buffer[index++] = r;
	buffer[index++] = g;
	buffer[index++] = b;
	buffer[index++] = a;
}
void Batch2D::vertex(vec2 point,
		vec2 uvpoint,
		float r, float g, float b, float a) {
	buffer[index++] = point.x;
	buffer[index++] = point.y;
	buffer[index++] = uvpoint.x;
	buffer[index++] = uvpoint.y;
	buffer[index++] = r;
	buffer[index++] = g;
	buffer[index++] = b;
	buffer[index++] = a;
}

void Batch2D::texture(Texture* new_texture){
	if (_texture == new_texture)
		return;
	render(GL_TRIANGLES);
	_texture = new_texture;
	if (new_texture == nullptr)
		blank->bind();
	else
		new_texture->bind();
}

void Batch2D::point(float x, float y, float r, float g, float b, float a){
	if (index + 6*VERTEX_SIZE >= capacity)
		render(GL_TRIANGLES);

	vertex(x, y, 0, 0, r,g,b,a);
	render(GL_POINTS);
}

void Batch2D::line(float x1, float y1, float x2, float y2, float r, float g, float b, float a){
	if (index + 6*VERTEX_SIZE >= capacity)
		render(GL_TRIANGLES);

	vertex(x1, y1, 0, 0, r,g,b,a);
	vertex(x2, y2, 1, 1, r,g,b,a);
	render(GL_LINES);
}

void Batch2D::rect(float x, float y, float w, float h){
	const float r = color.r;
	const float g = color.g;
	const float b = color.b;
	const float a = color.a;
	if (index + 6*VERTEX_SIZE >= capacity)
		render(GL_TRIANGLES);

	vertex(x, y, 0, 0, r,g,b,a);
	vertex(x, y+h, 0, 1, r,g,b,a);
	vertex(x+w, y+h, 1, 1, r,g,b,a);

	vertex(x, y, 0, 0, r,g,b,a);
	vertex(x+w, y+h, 1, 1, r,g,b,a);
	vertex(x+w, y, 1, 0, r,g,b,a);
}

void Batch2D::rect(
		float x, float y,
		float w, float h,
		float ox, float oy,
		float angle,
		UVRegion region,
		bool flippedX,
		bool flippedY,
		vec4 tint) {
	if (index + 6*VERTEX_SIZE >= capacity)
		render(GL_TRIANGLES);

    float centerX = w*ox;
    float centerY = h*oy;
    float acenterX = w-centerX;
    float acenterY = h-centerY;

    float _x1 = -centerX;
    float _y1 = -centerY;

    float _x2 = -centerX;
    float _y2 = +acenterY;

    float _x3 = +acenterX;
    float _y3 = +acenterY;

    float _x4 = +acenterX;
    float _y4 = -centerY;

    float x1,y1,x2,y2,x3,y3,x4,y4;

    if (angle != 0) {
        float s = sin(angle);
        float c = cos(angle);

        x1 = c * _x1 - s * _y1;
        y1 = s * _x1 + c * _y1;

        x2 = c * _x2 - s * _y2;
        y2 = s * _x2 + c * _y2;

        x3 = c * _x3 - s * _y3;
        y3 = s * _x3 + c * _y3;

        x4 = c * _x4 - s * _y4;
        y4 = s * _x4 + c * _y4;
    } else {
        x1 = _x1;
        y1 = _y1;
        x2 = _x2;
        y2 = _y2;
        x3 = _x3;
        y3 = _y3;
        x4 = _x4;
        y4 = _y4;
    }

    x1 += x; x2 += x; x3 += x; x4 += x;
    y1 += y; y2 += y; y3 += y; y4 += y;

    float u1 = region.u1;
    float v1 = region.v1;
    float u2 = region.u1;
    float v2 = region.v2;
    float u3 = region.u2;
    float v3 = region.v2;
    float u4 = region.u2;
    float v4 = region.v1;

    if (flippedX) {
        float temp = u1;
        u1 = u3;
        u4 = temp;
        u2 = u3;
        u3 = temp;
    }
    if (flippedY) {
        float temp = v1;
        v1 = v2;
        v4 = v2;
        v2 = temp;
        v3 = temp;
    }

    vertex(x1, y1, u1, v1, tint.r, tint.g, tint.b, tint.a);
    vertex(x2, y2, u2, v2, tint.r, tint.g, tint.b, tint.a);
    vertex(x3, y3, u3, v3, tint.r, tint.g, tint.b, tint.a);

    vertex(x1, y1, u1, v1, tint.r, tint.g, tint.b, tint.a);
    vertex(x3, y3, u3, v3, tint.r, tint.g, tint.b, tint.a);
    vertex(x4, y4, u4, v4, tint.r, tint.g, tint.b, tint.a);
}

void Batch2D::rect(float x, float y, float w, float h,
		float u, float v, float tx, float ty,
		float r, float g, float b, float a){
	if (index + 6*VERTEX_SIZE >= capacity)
		render(GL_TRIANGLES);
	vertex(x, y, u, v+ty, r,g,b,a);
	vertex(x+w, y+h, u+tx, v, r,g,b,a);
	vertex(x, y+h, u, v, r,g,b,a);

	vertex(x, y, u, v+ty, r,g,b,a);
	vertex(x+w, y, u+tx, v+ty, r,g,b,a);
	vertex(x+w, y+h, u+tx, v, r,g,b,a);
}

void Batch2D::sprite(float x, float y, float w, float h, int atlasRes, int index, vec4 tint){
	float scale = 1.0f / (float)atlasRes;
	float u = (index % atlasRes) * scale;
	float v = 1.0f - ((index / atlasRes) * scale) - scale;
	rect(x, y, w, h, u, v, scale, scale, tint.r, tint.g, tint.b, tint.a);
}

void Batch2D::render(unsigned int gl_primitive) {
	if (index == 0)
		return;
	
	// Создаем новый меш с текущими данными
	const vattr attrs[] = {
		{2}, {2}, {4}, {0}
	};
	const int attrsInt[] = {
		attrs[0].size, attrs[1].size, attrs[2].size, attrs[3].size
	};
	delete mesh;
	mesh = new Mesh(buffer, index / VERTEX_SIZE, attrsInt);
	mesh->draw(gl_primitive);
	index = 0;
}

void Batch2D::render() {
	render(GL_TRIANGLES);
}

