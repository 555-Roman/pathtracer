#ifndef PATHTRACER_INCLUDE_H
#define PATHTRACER_INCLUDE_H

#include <glm/glm.hpp>

using namespace glm;

struct Triangle {
    vec3 a;
    float padding_a;
    vec3 b;
    float padding_b;
    vec3 c;
    float padding_c;
};

#endif //PATHTRACER_INCLUDE_H