#include "ParentOfObjects.hpp"
#include "PowerGenerator.hpp"

#include <string>

ParentOfObjects::ParentOfObjects(int x, int y)
    : ParentOfObjects("icons\power_generator.png", 4, 4, x, y)
{
    src_of_im = new QImage(QString::fromStdString(src));
}