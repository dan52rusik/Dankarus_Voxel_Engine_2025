#include "engine/Engine.h"
#include <iostream>

int main() {
    Engine engine;
    
    if (!engine.initialize()) {
        std::cerr << "Failed to initialize engine" << std::endl;
        return 1;
    }
    
    engine.run();
    engine.shutdown();
    
    return 0;
}

