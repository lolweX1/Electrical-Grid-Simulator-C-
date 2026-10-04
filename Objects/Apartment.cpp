#include "Apartment.hpp"

Apartment::Apartment(int id, int gridX, int gridY)
    : House(id, gridX, gridY, "icons/apartment.png", "Apartment",
            ObjectKind::Apartment, 600.0, 20.0, 25.0)
{
}
