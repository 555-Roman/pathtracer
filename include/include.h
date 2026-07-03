#ifndef PATHTRACER_INCLUDE_H
#define PATHTRACER_INCLUDE_H

#include <glm/glm.hpp>
#include <glad/glad.h>

using namespace glm;

struct Material {
    vec3 colour;
    float padding0;
    vec3 emissionColour;
    float emissionStrength;
    GLuint64 textureHandle;
    GLuint64 padding1;
};

struct Triangle {
    vec3 a;
    float padding_a;
    vec3 b;
    float padding_b;
    vec3 c;
    float padding_c;
    vec2 uv_a;
    vec2 uv_b;
    vec2 uv_c;
    vec2 uv_padding;
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