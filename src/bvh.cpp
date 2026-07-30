#include "bvh.h"

void buildBVH(uint triangleIndex, uint triangleCount) {
    for (uint i = triangleIndex; i < triangleIndex + triangleCount; i++) {
        Triangle t = triangles[i];
        centroids.push_back((t.aPosition + t.bPosition + t.cPosition) * 0.3333333f);
    }

    // assign all triangles to root node
    uint rootNodeIdx = bvhNodes.size();
    bvhNodes.push_back({vec3(0.0), triangleIndex, vec3(0.0), triangleCount});
    updateBounds(rootNodeIdx);

    // subdivide recursively
    subdivide(rootNodeIdx);
}

void updateBounds(uint nodeIdx) {
    BVHNode node = bvhNodes[nodeIdx];
    node.minBound = vec3(+(1.0f / 0.0f));
    node.maxBound = vec3(-(1.0f / 0.0f));
    for (uint i = node.index; i < node.index + node.triangleCount; i++) {
        Triangle t = triangles[i];
        node.minBound = min(min(min(node.minBound, t.aPosition), t.bPosition), t.cPosition);
        node.maxBound = max(max(max(node.maxBound, t.aPosition), t.bPosition), t.cPosition);
    }
    bvhNodes[nodeIdx] = node;
}

void subdivide(uint nodeIdx, uint depth) {
    // terminate recursion
    BVHNode node = bvhNodes[nodeIdx];
    if (node.triangleCount == 1 || depth >= MAX_DEPTH) return;

    // determine split axis and position
    vec3 extent = node.maxBound - node.minBound;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    float splitPos = node.minBound[axis] + extent[axis] * 0.5f;

    // in-place partition
    int i = node.index;
    int j = i + node.triangleCount - 1;
    while (i <= j) {
        if (centroids[i][axis] < splitPos) {
            i++;
        } else {
            Triangle tA = triangles[i];
            vec3     cA = centroids[i];
            Triangle tB = triangles[j];
            vec3     cB = centroids[j];

            triangles[i] = tB;
            centroids[i] = cB;
            triangles[j] = tA;
            centroids[j] = cA;

            j--;
        }
    }

    // abort split if one of the sides is empty
    uint leftCount = i - node.index;
    if (leftCount == 0 || leftCount == node.triangleCount) return;

    // create child nodes
    uint leftChildIdx = bvhNodes.size();
    bvhNodes.push_back({vec3(0.0), node.index, vec3(0.0), leftCount});
    updateBounds(leftChildIdx);

    uint rightChildIdx = bvhNodes.size();
    bvhNodes.push_back({vec3(0.0), i, vec3(0.0), node.triangleCount - leftCount});
    updateBounds(rightChildIdx);

    node.index = leftChildIdx;
    node.triangleCount = 0;
    bvhNodes[nodeIdx] = node;

    // recurse
    subdivide(leftChildIdx, depth + 1);
    subdivide(rightChildIdx, depth + 1);
}
