#ifndef WIRE_H  // Header guard: checks if this token isn't defined yet
#define WIRE_H

#include <iostream>


class Wire{
    private:
        float resistivity;

    public:
        Wire(float r); 
        float get_resistivity();
};

#endif 
