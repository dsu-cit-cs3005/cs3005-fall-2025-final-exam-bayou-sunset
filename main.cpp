#include "Arena.h"
#include <iostream>

int main() {
    Arena arena;
    
    std::cout << "===========================================\n";
    std::cout << "         R O B O T W A R Z\n";
    std::cout << "===========================================\n";
    
    arena.initialize("arena.config");
    arena.run();
    
    return 0;
}
