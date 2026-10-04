#include "ParentOfObjects.hpp"

#include <QImage>
#include <string>

ParentOfObjects::ParentOfObjects(std::string src, int width, int height, int x, int y)
    : pos{x, y}, dimen{width, height}
{
    src_of_im = new QImage(QString::fromStdString(src));
}