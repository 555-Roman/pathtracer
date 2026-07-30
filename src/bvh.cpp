#include "bvh.h"

void buildBVH(uint triangleIndex, uint triangleCount) {
    vec3 minPos = vec3(+(1.0f / 0.0f));
    vec3 maxPos = vec3(-(1.0f / 0.0f));

    for (uint i = triangleIndex; i < triangleIndex + triangleCount; i++) {
        Triangle t = triangles[i];
        minPos = min(min(min(minPos, t.aPosition), t.bPosition), t.cPosition);
        maxPos = max(max(max(maxPos, t.aPosition), t.bPosition), t.cPosition);
    }

    bvhNodes.push_back({minPos, triangleIndex, maxPos, triangleCount});
}
