#include "objImport.h"

#include <glm/gtc/random.hpp>

void importTriangles(const char* filePath) {
    std::vector<vec3> vertices;
    std::vector<uvec3> indices;

    std::ifstream objFile(filePath);
    std::string line;
    while (std::getline(objFile, line)) {
        float x, y, z;
        uint idx0, idx1, idx2, idx3;
        if (sscanf(line.c_str(), "v %f %f %f", &x, &y, &z) == 3) {
            vertices.push_back(vec3(x, y, z));
        } else if (sscanf(line.c_str(), "f %u/%*u/%*u %u/%*u/%*u %u/%*u/%*u %u/%*u/%*u", &idx0, &idx1, &idx2, &idx3) == 4) {
            indices.push_back(uvec3(idx0, idx1, idx2));
            indices.push_back(uvec3(idx0, idx2, idx3));
        } else if (sscanf(line.c_str(), "f %u/%*u/%*u %u/%*u/%*u %u/%*u/%*u", &idx0, &idx1, &idx2) == 3) {
            indices.push_back(uvec3(idx0, idx1, idx2));
        }
    }
    objFile.close();

    for (uint i = 0; i < indices.size(); i++) {
        uvec3 triIndices = indices[i];
        uint idx0 = triIndices[0]-1;
        uint idx1 = triIndices[1]-1;
        uint idx2 = triIndices[2]-1;

        Triangle tri{};
        tri.a = vertices[idx0];
        tri.b = vertices[idx1];
        tri.c = vertices[idx2];
        triangles.push_back(tri);
    }
}
