#ifndef PATHTRACER_MODEL_H
#define PATHTRACER_MODEL_H

#include "include.h"
#include "objImport.h"

void importModel(const char* filePath);
void importModel(const char* filePath, Material material);
void importModel(const char* filePath, vec3 offset, float scale);
void importModel(const char* filePath, vec3 offset, float scale, Material material);
void addModel(uint triangleIndex, uint triangleCount);
void addModel(uint triangleIndex, uint triangleCount, Material material);
void addModel(uint triangleIndex, uint triangleCount, vec3 offset, float scale);
void addModel(uint triangleIndex, uint triangleCount, vec3 offset, float scale, Material material);

#endif //PATHTRACER_MODEL_H
