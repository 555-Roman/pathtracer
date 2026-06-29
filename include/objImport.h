#ifndef PATHTRACER_OBJIMPORT_H
#define PATHTRACER_OBJIMPORT_H

#include "include.h"
#include <iostream>
#include <fstream>

std::vector<Triangle> importTriangles(const char* filePath);

#endif //PATHTRACER_OBJIMPORT_H