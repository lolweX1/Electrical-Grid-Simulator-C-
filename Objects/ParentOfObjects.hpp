#ifndef POO_H 
#define POO_H

#include <string>
#include <QImage>

class ParentOfObjects {
    public:
        ParentOfObjects (std::string src, int width, int height, int x, int y);
        int pos[2];
        int dimen[2];
        QImage *src_of_im;
};

#endif 
