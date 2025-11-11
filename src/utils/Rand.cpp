#include "Rand.h"
#include <cmath>

namespace Rand {
    
    Rand::Rand() {
        std::random_device rd;
        InitGenerators(rd());
    }
    
    Rand::Rand(int seed) {
        InitGenerators(seed);
    }
    
    Rand::~Rand() {
    }
    
    void Rand::InitGenerators(int seed) {
        generator.seed(seed);
        peekGenerator.seed(seed);
        floatDist = std::uniform_real_distribution<float>(0.0f, 1.0f);
        intDist = std::uniform_int_distribution<int>(0, std::numeric_limits<int>::max());
    }
    
    void Rand::SetSeed(int seed) {
        InitGenerators(seed);
    }
    
    float Rand::Float() {
        return floatDist(generator);
    }
    
    int Rand::Range(int min, int max) {
        if (min >= max) {
            return min;
        }
        std::uniform_int_distribution<int> dist(min, max - 1);
        return dist(generator);
    }
    
    int Rand::Range(int max) {
        if (max <= 0) {
            return 0;
        }
        std::uniform_int_distribution<int> dist(0, max - 1);
        return dist(generator);
    }
    
    float Rand::Range(float min, float max) {
        if (min >= max) {
            return min;
        }
        std::uniform_real_distribution<float> dist(min, max);
        return dist(generator);
    }
    
    int Rand::Angle() {
        return Range(360);
    }
    
    glm::vec2 Rand::RandomOnUnitCircle() {
        float angle = Float() * 2.0f * static_cast<float>(M_PI);
        return glm::vec2(std::cos(angle), std::sin(angle));
    }
    
    int Rand::PeekSample() {
        // Используем отдельный генератор для peek, чтобы не изменять основной
        std::uniform_int_distribution<int> dist(0, std::numeric_limits<int>::max());
        int value = dist(peekGenerator);
        // Синхронизируем peekGenerator с generator для следующего peek
        peekGenerator.seed(generator());
        return value;
    }
    
    // Singleton implementation
    std::unique_ptr<Rand> RandSingleton::instance = nullptr;
    std::mutex RandSingleton::instanceMutex;
    
    Rand& RandSingleton::Instance() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (!instance) {
            instance = std::make_unique<Rand>();
        }
        return *instance;
    }
    
    void RandSingleton::Cleanup() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        instance.reset();
    }
}

