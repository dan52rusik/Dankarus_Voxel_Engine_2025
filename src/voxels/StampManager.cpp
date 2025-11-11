#include "StampManager.h"
#include "../voxels/ChunkManager.h"
#include <algorithm>
#include <cmath>
#include <limits>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Stamping {
    
    StampManager::StampManager(ChunkManager* chunkMgr)
        : chunkManager(chunkMgr) {
    }
    
    StampManager::~StampManager() {
        ClearStamps();
    }
    
    void StampManager::ClearStamps() {
        allStamps.clear();
        tempGetStampList.clear();
    }
    
    void StampManager::AddRawStamp(const std::string& name, std::shared_ptr<RawStamp> stamp) {
        allStamps[name] = stamp;
    }
    
    std::shared_ptr<RawStamp> StampManager::GetRawStamp(const std::string& name) {
        auto it = allStamps.find(name);
        if (it != allStamps.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    Stamp StampManager::CreateStamp(const std::string& rawStampName,
                                     const StampTransform& transform,
                                     bool isCustomColor,
                                     const glm::vec4& customColor,
                                     float biomeAlphaCutoff,
                                     bool isWater,
                                     const std::string& name) {
        Stamp stamp;
        stamp.rawStamp = GetRawStamp(rawStampName);
        if (!stamp.rawStamp) {
            return stamp; // Пустой штамп, если не найден
        }
        
        stamp.transform = transform;
        stamp.scale = transform.scale;
        stamp.isCustomColor = isCustomColor;
        stamp.customColor = customColor;
        stamp.biomeAlphaCutoff = biomeAlphaCutoff;
        stamp.isWater = isWater;
        stamp.name = name.empty() ? rawStampName : name;
        stamp.alpha = 1.0f;
        stamp.additive = false;
        
        // Вычисляем область штампа
        int rotation = static_cast<int>(transform.rotation);
        int scaledWidth = static_cast<int>(stamp.rawStamp->width * stamp.scale * ONE_BY_ONE_SCALE);
        int scaledHeight = static_cast<int>(stamp.rawStamp->height * stamp.scale * ONE_BY_ONE_SCALE);
        
        // Упрощенное вычисление границ (без поворота для начала)
        stamp.areaMin = glm::vec2(
            static_cast<float>(transform.x - scaledWidth / 2),
            static_cast<float>(transform.y - scaledHeight / 2)
        );
        stamp.areaMax = glm::vec2(
            static_cast<float>(transform.x + scaledWidth / 2),
            static_cast<float>(transform.y + scaledHeight / 2)
        );
        
        return stamp;
    }
    
    void StampManager::DrawStampGroup(const StampGroup& group, float* dest, float* waterDest, int worldSize) {
        for (const auto& stamp : group.stamps) {
            if (stamp.rawStamp) {
                DrawStamp(dest, waterDest, stamp, worldSize);
            }
        }
    }
    
    void StampManager::DrawStamp(float* dest, float* waterDest, const Stamp& stamp, int worldSize) {
        if (!stamp.rawStamp || !dest || !waterDest) {
            return;
        }
        
        DrawStampInternal(dest, waterDest,
                         *stamp.rawStamp,
                         stamp.transform.x, stamp.transform.y,
                         worldSize, worldSize,
                         static_cast<double>(stamp.alpha),
                         stamp.additive,
                         static_cast<double>(stamp.scale),
                         stamp.isCustomColor,
                         stamp.customColor,
                         static_cast<double>(stamp.biomeAlphaCutoff),
                         static_cast<double>(stamp.transform.rotation));
    }
    
    void StampManager::DrawWaterStamp(const Stamp& stamp, float* dest, int worldSize) {
        if (!stamp.rawStamp || !dest) {
            return;
        }
        
        // Упрощенная версия для воды
        DrawStampInternal(dest, nullptr,
                         *stamp.rawStamp,
                         stamp.transform.x, stamp.transform.y,
                         worldSize, worldSize,
                         static_cast<double>(stamp.alpha),
                         false,
                         static_cast<double>(stamp.scale),
                         false,
                         glm::vec4(1.0f),
                         static_cast<double>(stamp.biomeAlphaCutoff),
                         static_cast<double>(stamp.transform.rotation));
    }
    
    float StampManager::CalcRotatedValue(float x1, float y1,
                                         const float* src,
                                         double sine, double cosine,
                                         int width, int height,
                                         bool isWater) {
        int halfWidth = width / 2;
        int halfHeight = height / 2;
        
        double rotatedX = cosine * (x1 - halfWidth) + sine * (y1 - halfHeight) + halfWidth;
        int x = static_cast<int>(rotatedX);
        if (x < 0 || x >= width) {
            return 0.0f;
        }
        
        double rotatedY = -sine * (x1 - halfWidth) + cosine * (y1 - halfHeight) + halfHeight;
        int y = static_cast<int>(rotatedY);
        if (y < 0 || y >= height) {
            return 0.0f;
        }
        
        int index = x + y * width;
        float value = src[index];
        
        // Для воды: если есть хотя бы одно ненулевое значение, возвращаем его
        if (isWater && value > 0.0f) {
            return value;
        }
        
        // Билинейная интерполяция
        float valueRight = value;
        float valueUp = value;
        float valueUpRight = value;
        
        if (x + 1 < width) {
            valueRight = src[index + 1];
        }
        if (y + 1 < height) {
            valueUp = src[index + width];
        }
        if (x + 1 < width && y + 1 < height) {
            valueUpRight = src[index + width + 1];
        }
        
        if (isWater && (value > 0.0f || valueUp > 0.0f || valueRight > 0.0f || valueUpRight > 0.0f)) {
            return value;
        }
        
        double fracX = rotatedX - static_cast<double>(x);
        double fracY = rotatedY - static_cast<double>(y);
        
        double lerp1 = value + (valueRight - value) * fracX;
        double lerp2 = valueUp + (valueUpRight - valueUp) * fracX;
        return static_cast<float>(lerp1 + (lerp2 - lerp1) * fracY);
    }
    
    void StampManager::DrawStampInternal(float* dest, float* waterDest,
                                        const RawStamp& src,
                                        int x, int y,
                                        int destWidth, int destHeight,
                                        double alpha, bool additive,
                                        double scale, bool isCustomColor,
                                        const glm::vec4& customColor,
                                        double biomeCutoff, double angle) {
        // Центрируем штамп
        x -= static_cast<int>(static_cast<double>(src.width) * scale) / 2;
        y -= static_cast<int>(static_cast<double>(src.height) * scale) / 2;
        
        // Вычисляем синус и косинус поворота
        double angleRad = angle * M_PI / 180.0;
        double sine = std::sin(angleRad);
        double cosine = std::cos(angleRad);
        
        // Вычисляем размеры повернутого штампа
        int diagonal = static_cast<int>(std::sqrt(src.width * src.width + src.height * src.height));
        int padding = (diagonal - src.width) / 2;
        int scaledWidth = static_cast<int>(src.width * scale + padding);
        int scaledHeight = static_cast<int>(src.height * scale + padding);
        
        // Определяем границы для обработки
        int startX = -padding;
        int endX = scaledWidth;
        int startY = -padding;
        int endY = scaledHeight;
        
        // Обрезаем по границам целевого изображения
        if (x + startX < 0) {
            startX = -x;
        }
        if (x + endX >= destWidth) {
            endX = destWidth - x;
        }
        if (y + startY < 0) {
            startY = -y;
        }
        if (y + endY >= destHeight) {
            endY = destHeight - y;
        }
        
        // Рисуем штамп
        for (int dy = startY; dy < endY; dy++) {
            int destY = y + dy;
            if (destY < 0 || destY >= destHeight) continue;
            
            double srcY = static_cast<double>(dy) / scale;
            
            for (int dx = startX; dx < endX; dx++) {
                int destX = x + dx;
                if (destX < 0 || destX >= destWidth) continue;
                
                double srcX = static_cast<double>(dx) / scale;
                
                // Получаем альфа-канал
                double alphaValue = static_cast<double>(src.alphaConst);
                if (!src.alphaPixels.empty()) {
                    alphaValue = CalcRotatedValue(static_cast<float>(srcX), static_cast<float>(srcY),
                                                  src.alphaPixels.data(), sine, cosine,
                                                  src.width, src.height, false);
                }
                
                if (alphaValue < ALPHA_CUTOFF) {
                    continue;
                }
                
                int destIndex = destX + destY * destWidth;
                
                if (isCustomColor) {
                    if (alphaValue > biomeCutoff) {
                        if (dest) {
                            dest[destIndex] = customColor.r;
                        }
                        if (waterDest) {
                            waterDest[destIndex] = customColor.b;
                        }
                    }
                } else {
                    // Получаем высоту
                    double heightValue = static_cast<double>(src.heightConst);
                    if (!src.heightPixels.empty()) {
                        heightValue = CalcRotatedValue(static_cast<float>(srcX), static_cast<float>(srcY),
                                                       src.heightPixels.data(), sine, cosine,
                                                       src.width, src.height, false);
                    }
                    
                    // Получаем воду
                    double waterValue = heightValue;
                    if (!src.waterPixels.empty()) {
                        waterValue = CalcRotatedValue(static_cast<float>(srcX), static_cast<float>(srcY),
                                                      src.waterPixels.data(), sine, cosine,
                                                      src.width, src.height, true);
                    }
                    
                    double alphaBlend = alphaValue * alpha;
                    
                    // Смешиваем высоту
                    if (dest) {
                        double currentHeight = static_cast<double>(dest[destIndex]);
                        double newHeight = additive ? currentHeight + heightValue * alphaBlend
                                                    : currentHeight + (heightValue - currentHeight) * alphaBlend;
                        dest[destIndex] = static_cast<float>(newHeight);
                    }
                    
                    // Смешиваем воду
                    if (waterDest) {
                        double currentWater = static_cast<double>(waterDest[destIndex]);
                        double newWater = currentWater + (waterValue - currentWater) * alphaBlend;
                        waterDest[destIndex] = static_cast<float>(newWater);
                    }
                }
            }
        }
    }
    
    void RawStamp::SmoothAlpha(int boxSize) {
        if (alphaPixels.empty()) {
            return;
        }
        
        std::vector<float> smoothed(alphaPixels.size());
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                double sum = 0.0;
                int count = 0;
                
                for (int dy = -1; dy < boxSize; dy++) {
                    int sy = y + dy;
                    if (sy >= 0 && sy < height) {
                        for (int dx = -1; dx < boxSize; dx++) {
                            int sx = x + dx;
                            if (sx >= 0 && sx < width) {
                                sum += alphaPixels[sx + sy * width];
                                count++;
                            }
                        }
                    }
                }
                
                smoothed[x + y * width] = static_cast<float>(sum / static_cast<double>(count));
            }
        }
        
        alphaPixels = smoothed;
    }
    
    void RawStamp::BoxAlpha() {
        if (alphaPixels.empty()) {
            return;
        }
        
        for (int y = 0; y < height; y += 4) {
            for (int x = 0; x < width; x += 4) {
                int index = x + y * width;
                double sum = 0.0;
                
                for (int dy = 0; dy < 4 && y + dy < height; dy++) {
                    for (int dx = 0; dx < 4 && x + dx < width; dx++) {
                        sum += alphaPixels[index + dx + dy * width];
                    }
                }
                
                double avg = sum / 16.0;
                
                for (int dy = 0; dy < 4 && y + dy < height; dy++) {
                    for (int dx = 0; dx < 4 && x + dx < width; dx++) {
                        alphaPixels[index + dx + dy * width] = static_cast<float>(avg);
                    }
                }
            }
        }
    }
}

