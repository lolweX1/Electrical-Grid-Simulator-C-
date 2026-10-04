#ifndef PGEN_H 
#define PGEN_H

#include "ParentOfObjects.hpp"
#include "PowerGenerator.hpp"

class PowerGenerator : public ParentOfObjects {
    public:
        PowerGenerator (int x, int y);
};

#endif 