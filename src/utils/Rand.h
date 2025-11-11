#ifndef UTILS_RAND_H_
#define UTILS_RAND_H_

#include <glm/glm.hpp>
#include <random>
#include <memory>
#include <mutex>

// Обертка для генератора случайных чисел
// Адаптировано из 7 Days To Die Rand
namespace Rand {
    class Rand {
    private:
        std::mt19937 generator;
        std::uniform_real_distribution<float> floatDist;
        std::uniform_int_distribution<int> intDist;
        
        // Для peek (посмотреть следующее значение без изменения состояния)
        mutable std::mt19937 peekGenerator;
        
    public:
        Rand();
        Rand(int seed);
        ~Rand();
        
        // Установить seed
        void SetSeed(int seed);
        
        // Получить случайное float [0.0, 1.0)
        float Float();
        
        // Получить случайное int [min, max)
        int Range(int min, int max);
        
        // Получить случайное int [0, max)
        int Range(int max);
        
        // Получить случайное float [min, max)
        float Range(float min, float max);
        
        // Получить случайный угол [0, 360)
        int Angle();
        
        // Получить случайную точку на единичной окружности
        glm::vec2 RandomOnUnitCircle();
        
        // Посмотреть следующее значение без изменения состояния
        int PeekSample();
        
    private:
        void InitGenerators(int seed);
    };
    
    // Singleton для глобального доступа
    class RandSingleton {
    private:
        static std::unique_ptr<Rand> instance;
        static std::mutex instanceMutex;
        
    public:
        static Rand& Instance();
        static void Cleanup();
    };
    
    // Глобальные функции для удобства
    inline Rand& GetInstance() {
        return RandSingleton::Instance();
    }
    
    inline float Float() {
        return GetInstance().Float();
    }
    
    inline int Range(int min, int max) {
        return GetInstance().Range(min, max);
    }
    
    inline int Range(int max) {
        return GetInstance().Range(max);
    }
    
    inline float Range(float min, float max) {
        return GetInstance().Range(min, max);
    }
    
    inline int Angle() {
        return GetInstance().Angle();
    }
    
    inline glm::vec2 RandomOnUnitCircle() {
        return GetInstance().RandomOnUnitCircle();
    }
}

#endif /* UTILS_RAND_H_ */

