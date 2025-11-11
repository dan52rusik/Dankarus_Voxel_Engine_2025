#ifndef VOXELS_STAMPMANAGER_H_
#define VOXELS_STAMPMANAGER_H_

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

// Forward declaration
class ChunkManager;

// Система штампов для рисования озер, рек, каньонов на карте
// Адаптировано из 7 Days To Die StampManager
namespace Stamping {
    
    // Константы
    constexpr float ALPHA_CUTOFF = 1e-5f;
    constexpr float ONE_BY_ONE_SCALE = 1.4f;
    
    // Структура для трансформации штампа
    struct StampTransform {
        int x;              // Позиция X
        int y;              // Позиция Y
        float rotation;     // Поворот в градусах
        float scale;        // Масштаб
        
        StampTransform() : x(0), y(0), rotation(0.0f), scale(1.0f) {}
        StampTransform(int x, int y, float rotation = 0.0f, float scale = 1.0f)
            : x(x), y(y), rotation(rotation), scale(scale) {}
    };
    
    // Сырые данные штампа (изображение)
    struct RawStamp {
        std::string name;
        int width;
        int height;
        
        float heightConst;          // Постоянная высота (если нет heightPixels)
        std::vector<float> heightPixels;  // Массив высот (width * height)
        
        float alphaConst;           // Постоянная альфа (если нет alphaPixels)
        std::vector<float> alphaPixels;   // Массив альфа-каналов (width * height)
        
        std::vector<float> waterPixels;  // Массив воды (width * height, опционально)
        
        RawStamp() : width(0), height(0), heightConst(0.0f), alphaConst(1.0f) {}
        
        bool HasWater() const { return !waterPixels.empty(); }
        
        // Сгладить альфа-канал
        void SmoothAlpha(int boxSize);
        
        // Упростить альфа-канал (боксирование)
        void BoxAlpha();
    };
    
    // Штамп с трансформацией
    struct Stamp {
        std::shared_ptr<RawStamp> rawStamp;
        StampTransform transform;
        
        float alpha;                // Прозрачность
        bool additive;              // Аддитивное смешивание
        float scale;                // Масштаб
        bool isCustomColor;         // Использовать кастомный цвет
        glm::vec4 customColor;      // Кастомный цвет (RGBA)
        float biomeAlphaCutoff;     // Порог альфа для биома
        bool isWater;               // Это водный штамп
        std::string name;           // Имя штампа
        
        glm::vec2 areaMin;          // Минимальная граница области
        glm::vec2 areaMax;          // Максимальная граница области
        
        Stamp() : alpha(1.0f), additive(false), scale(1.0f),
                  isCustomColor(false), customColor(1.0f),
                  biomeAlphaCutoff(0.1f), isWater(false) {}
    };
    
    // Группа штампов
    struct StampGroup {
        std::string name;
        std::vector<Stamp> stamps;
        
        StampGroup(const std::string& groupName) : name(groupName) {}
    };
    
    // Менеджер штампов
    class StampManager {
    private:
        ChunkManager* chunkManager;
        std::unordered_map<std::string, std::shared_ptr<RawStamp>> allStamps;
        std::vector<Stamp> tempGetStampList;
        
    public:
        StampManager(ChunkManager* chunkMgr);
        ~StampManager();
        
        // Очистить все штампы
        void ClearStamps();
        
        // Добавить сырой штамп
        void AddRawStamp(const std::string& name, std::shared_ptr<RawStamp> stamp);
        
        // Получить сырой штамп
        std::shared_ptr<RawStamp> GetRawStamp(const std::string& name);
        
        // Нарисовать группу штампов на карте высот и воды
        // dest - массив высот (worldSize * worldSize)
        // waterDest - массив воды (worldSize * worldSize)
        void DrawStampGroup(const StampGroup& group, float* dest, float* waterDest, int worldSize);
        
        // Нарисовать один штамп
        void DrawStamp(float* dest, float* waterDest, const Stamp& stamp, int worldSize);
        
        // Нарисовать водный штамп
        void DrawWaterStamp(const Stamp& stamp, float* dest, int worldSize);
        
        // Создать штамп из сырого штампа
        Stamp CreateStamp(const std::string& rawStampName,
                          const StampTransform& transform,
                          bool isCustomColor = false,
                          const glm::vec4& customColor = glm::vec4(1.0f),
                          float biomeAlphaCutoff = 0.1f,
                          bool isWater = false,
                          const std::string& name = "");
        
    private:
        // Вычислить повернутое значение из исходного массива
        static float CalcRotatedValue(float x1, float y1,
                                      const float* src,
                                      double sine, double cosine,
                                      int width, int height,
                                      bool isWater = false);
        
        // Нарисовать штамп (внутренняя функция)
        static void DrawStampInternal(float* dest, float* waterDest,
                                     const RawStamp& src,
                                     int x, int y,
                                     int destWidth, int destHeight,
                                     double alpha, bool additive,
                                     double scale, bool isCustomColor,
                                     const glm::vec4& customColor,
                                     double biomeCutoff, double angle);
    };
}

#endif /* VOXELS_STAMPMANAGER_H_ */

