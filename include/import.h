#ifndef PATHTRACER_OBJIMPORT_H
#define PATHTRACER_OBJIMPORT_H

#include "include.h"
#include <iostream>
#include <fstream>
#include <filesystem>

#include "assimp/scene.h"
#include "bvh.h"
#include "model.h"

void importAndSend(const char* filePath);
void import(const char* filePath);
void processNode(aiNode* node, const aiScene* scene, const char* filePath);
void processMesh(aiMesh* mesh, const aiScene* scene, const char* filePath);
Material getMeshMaterial(aiMesh* mesh, const aiScene* scene, const char* filePath);
GLuint64 getTextureHandle(aiTextureType textureType, aiMaterial* mat, const char* filePath);

#endif //PATHTRACER_OBJIMPORT_H