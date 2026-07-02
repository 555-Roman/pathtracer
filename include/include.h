#ifndef PATHTRACER_INCLUDE_H
#define PATHTRACER_INCLUDE_H

#include <glm/glm.hpp>

using namespace glm;

struct Material {
    vec3 colour;
    float padding;
    vec3 emissionColour;
    float emissionStrength;
};

struct Triangle {
    vec3 a;
    float padding_a;
    vec3 b;
    float padding_b;
    vec3 c;
    float padding_c;
};

struct Model {
    uint triangleIndex;
    uint triangleCount;
    uint padding[2];
    vec3 offset;
    float scale;
    Material material;
};

inline std::vector<Triangle> triangles;
inline std::vector<Model> models;

#endif //PATHTRACER_INCLUDE_H