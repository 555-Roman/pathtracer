#include "objImport.h"

#include <glm/gtc/random.hpp>

void importTriangles(const char* filePath) {
    std::vector<vec3> positions;
    std::vector<uvec3> position_indices;
    std::vector<vec2> uvs;
    std::vector<uvec3> uv_indices;

    std::ifstream objFile(filePath);
    std::string line;
    while (std::getline(objFile, line)) {
        float x, y, z;
        uint posIdx0, posIdx1, posIdx2, posIdx3;
        uint uvIdx0, uvIdx1, uvIdx2, uvIdx3;
        if (sscanf(line.c_str(), "v %f %f %f", &x, &y, &z) == 3) {
            positions.push_back(vec3(x, y, z));
        } else if (sscanf(line.c_str(), "vt %f %f", &x, &y) == 2) {
            uvs.push_back(vec2(x, y));
        } else if (sscanf(line.c_str(), "f %u/%u/%*u %u/%u/%*u %u/%u/%*u %u/%u/%*u", &posIdx0, &uvIdx0, &posIdx1, &uvIdx1, &posIdx2, &uvIdx2, &posIdx3, &uvIdx3) == 8) {
            position_indices.push_back(uvec3(posIdx0, posIdx1, posIdx2));
            position_indices.push_back(uvec3(posIdx0, posIdx2, posIdx3));
            uv_indices.push_back(uvec3(uvIdx0, uvIdx1, uvIdx2));
            uv_indices.push_back(uvec3(uvIdx0, uvIdx2, uvIdx3));
        } else if (sscanf(line.c_str(), "f %u/%u/%*u %u/%u/%*u %u/%u/%*u", &posIdx0, &uvIdx0, &posIdx1, &uvIdx1, &posIdx2, &uvIdx2) == 6) {
            position_indices.push_back(uvec3(posIdx0, posIdx1, posIdx2));
            uv_indices.push_back(uvec3(uvIdx0, uvIdx1, uvIdx2));
        }
    }
    objFile.close();

    for (uint i = 0; i < position_indices.size(); i++) {
        uvec3 triPosIndices = position_indices[i];
        uint posIdx0 = triPosIndices[0]-1;
        uint posIdx1 = triPosIndices[1]-1;
        uint posIdx2 = triPosIndices[2]-1;

        uvec3 triUVIndices = uv_indices[i];
        uint uvIdx0 = triUVIndices[0]-1;
        uint uvIdx1 = triUVIndices[1]-1;
        uint uvIdx2 = triUVIndices[2]-1;

        Triangle tri{};
        tri.a = positions[posIdx0];
        tri.b = positions[posIdx1];
        tri.c = positions[posIdx2];
        tri.uv_a = uvs[uvIdx0];
        tri.uv_b = uvs[uvIdx1];
        tri.uv_c = uvs[uvIdx2];
        triangles.push_back(tri);
    }
}
