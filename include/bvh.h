#ifndef PATHTRACER_BVH_H
#define PATHTRACER_BVH_H

#include "include.h"

#define MAX_DEPTH 31

void buildBVH(uint triangleIndex, uint triangleCount);
void updateBounds(uint nodeIdx);
void subdivide(uint nodeIdx, uint depth = 0);

inline std::vector<vec3> centroids;

#endif //PATHTRACER_BVH_H