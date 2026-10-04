#ifndef APARTMENT_HPP
#define APARTMENT_HPP

#include "House.hpp"

class Apartment : public House {
public:
    Apartment(int id, int gridX, int gridY);
};

#endif
