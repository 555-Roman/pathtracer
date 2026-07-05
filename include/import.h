#ifndef PATHTRACER_OBJIMPORT_H
#define PATHTRACER_OBJIMPORT_H

#include "include.h"
#include <iostream>
#include <fstream>

#include "assimp/scene.h"

void import(const char* filePath);
void processScene(const aiScene* scene);

#endif //PATHTRACER_OBJIMPORT_H