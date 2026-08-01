#ifndef PATHTRACER_BVH_H
#define PATHTRACER_BVH_H

#include "include.h"

#define MAX_DEPTH 31
#define SAH_PLANES 100
#define SAH_BINS 100
#define TRIANGLE_TEST

#define BVH_SPLIT_METHOD 3

void buildBVH(uint triangleIndex, uint triangleCount);
void updateBounds(uint nodeIdx);
float EvaluateSAH(uint nodeIdx, int axis, float pos);
void subdivide(uint nodeIdx, uint depth = 0);

inline std::vector<vec3> centroids;

#endif //PATHTRACER_BVH_H