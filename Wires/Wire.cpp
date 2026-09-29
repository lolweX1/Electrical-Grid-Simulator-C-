#include "Wire.hpp"
#include <iostream>

// constructor
Wire::Wire(float r) : resistivity(r) {
    if (resistivity < 0) {
        std::cout << "Warning: resistivity cannot be negative! Resetting to 0.\n";
        resistivity = 0; 
    }
}

// Method implementation
float Wire::get_resistivity() {
    return resistivity;
};