#ifndef PATHTRACER_BVH_H
#define PATHTRACER_BVH_H

#include "include.h"

inline int MAX_INNER_NODES = 63;
inline int SAH_PLANES = 100;
inline int SAH_BINS = 100;
inline bool TRIANGLE_TEST = true;
inline bool USE_BOX_CENTROIDS = false;

inline int BVH_SPLIT_METHOD = 3;

inline uint bvhMaxDepth;

inline constexpr float triCost = 1.1;
inline constexpr float traverseCost = 1.0;

void buildBVH(uint triangleIndex, uint triangleCount);
void updateBounds(uint nodeIdx);
float EvaluateSAH(uint nodeIdx, int axis, float pos);
void subdivide(uint nodeIdx, uint depth = 0);

inline std::vector<vec3> centroids;

#endif //PATHTRACER_BVH_H