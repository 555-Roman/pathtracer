#ifndef PATHTRACER_MODEL_H
#define PATHTRACER_MODEL_H

#include "include.h"

void addModel(uint triangleIndex, uint triangleCount);
void addModel(uint triangleIndex, uint triangleCount, Material material);
void addModel(uint triangleIndex, uint triangleCount, vec3 offset, float scale);
void addModel(uint triangleIndex, uint triangleCount, vec3 offset, float scale, Material material);

#endif //PATHTRACER_MODEL_H
