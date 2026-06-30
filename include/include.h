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
    Material material;
};

#endif //PATHTRACER_INCLUDE_H