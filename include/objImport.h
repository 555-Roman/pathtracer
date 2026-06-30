#ifndef PATHTRACER_OBJIMPORT_H
#define PATHTRACER_OBJIMPORT_H

#include "include.h"
#include <iostream>
#include <fstream>

std::vector<Triangle> importTriangles(const char* filePath);
std::vector<Triangle> importTriangles(const char* filePath, vec3 offset, float scale);
std::vector<Triangle> importTriangles(const char* filePath, vec3 offset, float scale, Material material);

#endif //PATHTRACER_OBJIMPORT_H