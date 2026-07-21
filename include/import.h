#ifndef PATHTRACER_OBJIMPORT_H
#define PATHTRACER_OBJIMPORT_H

#include "include.h"
#include <iostream>
#include <fstream>
#include <filesystem>

#include "assimp/scene.h"

void import(const char* filePath);
void processScene(const aiScene* scene, const char* filePath);
void processMesh(aiMesh* mesh, const aiScene* scene, const char* filePath);
Material getMeshMaterial(aiMesh* mesh, const aiScene* scene, const char* filePath);

#endif //PATHTRACER_OBJIMPORT_H