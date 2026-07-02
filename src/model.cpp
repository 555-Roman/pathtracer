#include "model.h"

void importModel(const char* filePath, vec3 offset, float scale, Material material) {
    Model model{};
    model.triangleIndex = triangles.size();
    importTriangles(filePath);
    model.triangleCount = triangles.size() - model.triangleIndex;
    model.offset = offset;
    model.scale = scale;
    model.material = material;
    models.push_back(model);
}

void importModel(const char* filePath, vec3 offset, float scale) {
    importModel(filePath, offset, scale, {vec3(1.0), 0.0, vec3(0.0), 0.0});
}

void importModel(const char* filePath, Material material) {
    importModel(filePath, vec3(0.0), 1.0, material);
}

void importModel(const char* filePath) {
    importModel(filePath, vec3(0.0), 1.0, {vec3(1.0), 0.0, vec3(0.0), 0.0});
}

void addModel(uint triangleIndex, uint triangleCount, vec3 offset, float scale, Material material) {
    Model model{};
    model.triangleIndex = triangleIndex;
    model.triangleCount = triangleCount;
    model.offset = offset;
    model.scale = scale;
    model.material = material;
    models.push_back(model);
}

void addModel(uint triangleIndex, uint triangleCount, vec3 offset, float scale) {
    addModel(triangleIndex, triangleCount, offset, scale, {vec3(1.0), 0.0, vec3(0.0), 0.0});
}

void addModel(uint triangleIndex, uint triangleCount, Material material) {
    addModel(triangleIndex, triangleCount, vec3(0.0), 1.0, material);
}

void addModel(uint triangleIndex, uint triangleCount) {
    addModel(triangleIndex, triangleCount, vec3(0.0), 1.0, {vec3(1.0), 0.0, vec3(0.0), 0.0});
}
