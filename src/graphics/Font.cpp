#include "Font.h"
#include "Texture.h"
#include "Batch2D.h"
#include "Shader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <iostream>
#include <iomanip>

using namespace glm;

Font::Font(std::vector<Texture*> pages, int lineHeight) : lineHeight_(lineHeight), pages(pages) {
}

Font::~Font(){
	for (Texture* texture : pages)
		delete texture;
}

int Font::lineHeight() const {
	return lineHeight_;
}

bool Font::isPrintableChar(int c) {
	switch (c){
	case ' ':
	case '\t':
	case '\n':
	case '\f':
	case '\r':
		return false;
	default:
		return true;
	}
}

#define RES 16

int Font::calcWidth(std::wstring text) {
	return text.length() * 8;
}

void Font::draw(Batch2D* batch, std::wstring text, int x, int y) {
	draw(batch, text, x, y, STYLE_NONE);
}

void Font::draw(Batch2D* batch, std::wstring text, int x, int y, int style) {
	draw(batch, nullptr, text, x, y, style);
}

void Font::draw(Batch2D* batch, Shader* shader, std::wstring text, int x, int y, int style) {
	const int init_x = x;
	
	// DEBUG: Выводим коды и страницы строки (только один раз для диагностики)
	// Раскомментируйте, если нужно проверить коды символов:
	/*
	std::cout << "[Font] Drawing text: ";
	for (unsigned c : text) {
		if (isPrintableChar(c)) {
			std::cout << std::hex << "U+" << c 
			          << " page=" << ((c >> 8) & 0xFF)
			          << " index=" << (c & 0xFF) << std::dec << " ";
		}
	}
	std::cout << std::endl;
	*/
	
	// Соберём набор страниц, которые реально встречаются в строке
	bool pageUsed[256] = {false}; // достаточно для BMP
	int printableCount = 0;
	for (unsigned c : text) {
		if (isPrintableChar(c)) {
			pageUsed[(c >> 8) & 0xFF] = true;
			printableCount++;
		}
	}
	
	// Убрали лишние логи для производительности
	
	// Проходим по всем страницам, где есть символы
	int pageCount = 0;
	for (int page = 0; page < 256; ++page) {
		if (!pageUsed[page]) continue;
		pageCount++;
		// Убрали лишние логи для производительности
		
		Texture* tex = (page < (int)pages.size() && pages[page] != nullptr) ? pages[page] : pages[0];
		
		// Гарантируем активный текстурный юнит перед биндом текстуры
		glActiveTexture(GL_TEXTURE0);
		if (shader != nullptr) {
			shader->use(); // ВАЖНО: используем шейдер перед каждым batch->begin()
			// ВАЖНО: проекция должна быть установлена ДО вызова font->draw() в Menu.cpp
			// НЕ устанавливаем проекцию здесь - она должна быть установлена в Menu.cpp
			shader->uniform1i("u_texture", 0);   // sampler → слот 0
		}
		batch->begin();
		batch->texture(tex);
		
		x = init_x; // IMPORTANT: каждый прогон возвращаемся к исходному x
		
		// DEBUG: счетчик спрайтов (закомментировано)
		// int spritesThisPage = 0;
		
		for (unsigned c : text) {
			if (!isPrintableChar(c)) {
				x += 8;
				continue;
			}
			
			int charpage = (c >> 8) & 0xFF;
			if (charpage != page) {
				x += 8;
				continue;
			}
			
			// DEBUG: маркеры глифов (закомментировано, так как глифы уже видны)
			/*
			if (shader != nullptr) {
				shader->uniform1i("u_useTex", 0);
			}
			{
				float boxW = RES;
				float boxH = RES;
				batch->texture(nullptr);
				batch->color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				batch->rect((float)x, (float)y, boxW, 1.0f);
				batch->rect((float)x, (float)y, 1.0f, boxH);
				batch->color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				if (shader != nullptr) {
					shader->uniform1i("u_useTex", 1);
				}
			}
			*/
			
			// ВАЖНО: индекс в атласе — младший байт (0-255 для атласа 16x16)
			int glyphIndex = c & 0xFF;
			
			// Тень/обводка
			switch (style) {
				case STYLE_SHADOW:
					batch->sprite(x+1, y+1, RES, RES, 16, glyphIndex, vec4(0.0f, 0.0f, 0.0f, 1.0f));
					break;
				case STYLE_OUTLINE:
					for (int oy = -1; oy <= 1; ++oy) {
						for (int ox = -1; ox <= 1; ++ox) {
							if (ox || oy) {
								batch->sprite(x+ox, y+oy, RES, RES, 16, glyphIndex, vec4(0.0f, 0.0f, 0.0f, 1.0f));
							}
						}
					}
					break;
			}
			
			// Сам глиф (ВАЖНО: индекс в атласе — младший байт)
			batch->sprite(x, y, RES, RES, 16, glyphIndex, batch->color);
			// Убрали лишние логи для производительности
			x += 8;
		}
		
		// DEBUG: Выводим лог для диагностики (раскомментируйте, если нужно)
		// std::cout << "[Font] page=" << page << " sprites=" << spritesThisPage << std::endl;
		
		// ВАЖНО: убеждаемся, что шейдер используется перед render()
		// (проекция должна быть установлена вызывающим кодом)
		// Убрали лишние вызовы glGetUniformLocation для производительности
		batch->render();
	}
}

