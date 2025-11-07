#!/usr/bin/env python3
import png
import numpy as np

def create_font_texture(filename, size=256):
    """Создает простую текстуру шрифта с белыми квадратами на черном фоне"""
    # Создаем изображение 256x256 пикселей
    width = height = size
    
    # Создаем массив для изображения (RGBA)
    image = np.zeros((height, width, 4), dtype=np.uint8)
    
    # Заполняем альфа-канал (непрозрачность)
    image[:, :, 3] = 255  # Полная непрозрачность
    
    # Создаем сетку 16x16 для символов
    char_size = size // 16
    
    # Заполняем некоторые символы белыми квадратами для тестирования
    for row in range(16):
        for col in range(16):
            # Делаем каждый второй символ белым для тестирования
            if (row + col) % 2 == 0:
                y_start = row * char_size
                y_end = (row + 1) * char_size
                x_start = col * char_size
                x_end = (col + 1) * char_size
                
                # Белый квадрат
                image[y_start:y_end, x_start:x_end, 0:3] = 255  # R, G, B
    
    # Сохраняем в PNG
    with open(filename, 'wb') as f:
        writer = png.Writer(width, height, alpha=True)
        # Конвертируем в формат, который ожидает png.Writer
        image_flat = image.reshape(height, width * 4)
        writer.write(f, image_flat)
    
    print(f"Создан файл {filename}")

# Создаем 5 файлов шрифта
for i in range(5):
    create_font_texture(f"res/fonts/font_{i}.png")