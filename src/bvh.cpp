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

float CalculateNodeCost(BVHNode& node) {
    vec3 e = node.maxBound - node.minBound; // extent of the node
    float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
    return node.triangleCount * surfaceArea;
}

float EvaluateSAH(BVHNode node, int axis, float pos) {
    // determine triangle counts and bounds for this split candidate
    BVHNode leftBox = {vec3(+(1.0 / 0.0)), 0, vec3(-(1.0 / 0.0)), 0};
    BVHNode rightBox = {vec3(+(1.0 / 0.0)), 0, vec3(-(1.0 / 0.0)), 0};
    int leftCount = 0, rightCount = 0;
    for (uint i = node.index; i < node.index + node.triangleCount; i++) {
        Triangle t = triangles[i];
        if (centroids[i][axis] < pos) {
            leftCount++;
            leftBox.minBound = min(min(min(leftBox.minBound, t.aPosition), t.bPosition), t.cPosition);
            leftBox.maxBound = max(max(max(leftBox.maxBound, t.aPosition), t.bPosition), t.cPosition);
        } else {
            rightCount++;
            rightBox.minBound = min(min(min(rightBox.minBound, t.aPosition), t.bPosition), t.cPosition);
            rightBox.maxBound = max(max(max(rightBox.maxBound, t.aPosition), t.bPosition), t.cPosition);
        }
    }

    vec3 e = leftBox.maxBound - leftBox.minBound;
    float leftArea = e.x * e.y + e.y * e.z + e.z * e.x;
    e = rightBox.maxBound - rightBox.minBound;
    float rightArea = e.x * e.y + e.y * e.z + e.z * e.x;

    float cost = leftCount * leftArea + rightCount * rightArea;
    return cost > 0 ? cost : 1.0 / 0.0;
}

float FindBestSplitPlane(BVHNode& node, int& axis, float& splitPos) {
    float bestCost = 1.0 / 0.0;
    for (int a = 0; a < 3; a++) {
#if BVH_SPLIT_METHOD == 2
#ifdef TRIANGLE_TEST
        if (node.triangleCount < SAH_PLANES) {
            for (uint i = 0; i < node.triangleCount; i++) {
                float candidatePos = centroids[node.index + i][a];
                float cost = EvaluateSAH(node, a, candidatePos);
                if (cost < bestCost)
                    splitPos = candidatePos, axis = a, bestCost = cost;
            }
            continue;
        }
#endif

        float boundsMin = 1.0 / 0.0, boundsMax = -1.0 / 0.0;
        for (int i = 0; i < node.triangleCount; i++) {
            vec3 centroid = centroids[node.index + i];
            boundsMin = min( boundsMin, centroid[a] );
            boundsMax = max( boundsMax, centroid[a] );
        }
        if (boundsMin == boundsMax) continue;

        float scale = (boundsMax - boundsMin) / SAH_PLANES;
        for (uint i = 1; i < SAH_PLANES; i++) {
            float candidatePos = boundsMin + i * scale;
            float cost = EvaluateSAH( node, a, candidatePos );
            if (cost < bestCost)
                splitPos = candidatePos, axis = a, bestCost = cost;
        }
#elif BVH_SPLIT_METHOD == 3
#ifdef TRIANGLE_TEST
        if (node.triangleCount < SAH_BINS) {
            for (uint i = 0; i < node.triangleCount; i++) {
                float candidatePos = centroids[node.index + i][a];
                float cost = EvaluateSAH(node, a, candidatePos);
                if (cost < bestCost)
                    splitPos = candidatePos, axis = a, bestCost = cost;
            }
            continue;
        }
#endif

        float boundsMin = 1.0 / 0.0, boundsMax = -1.0 / 0.0;
        for (int i = 0; i < node.triangleCount; i++) {
            vec3 centroid = centroids[node.index + i];
            boundsMin = min( boundsMin, centroid[a] );
            boundsMax = max( boundsMax, centroid[a] );
        }
        if (boundsMin == boundsMax) continue;

        // populate the bins
        BVHNode bins[SAH_BINS];
        for (int i = 0; i < SAH_BINS; i++) {
            bins[i].triangleCount = 0;
            bins[i].minBound = vec3(+(1.0f / 0.0f));
            bins[i].maxBound = vec3(-(1.0f / 0.0f));
        }
        float scale = SAH_BINS / (boundsMax - boundsMin);
        for (uint i = 0; i < node.triangleCount; i++) {
            Triangle t = triangles[node.index + i];
            int binIdx = min(SAH_BINS - 1, (int)((centroids[node.index + i][a] - boundsMin) * scale));
            bins[binIdx].triangleCount++;
            bins[binIdx].minBound = min(min(min(bins[binIdx].minBound, t.aPosition), t.bPosition), t.cPosition);
            bins[binIdx].maxBound = max(max(max(bins[binIdx].maxBound, t.aPosition), t.bPosition), t.cPosition);
        }

        // gather data for the 7 planes between the 8 bins
        float leftArea[SAH_BINS - 1], rightArea[SAH_BINS - 1];
        int leftCount[SAH_BINS - 1], rightCount[SAH_BINS - 1];
        BVHNode leftBox = {vec3(+(1.0 / 0.0)), 0, vec3(-(1.0/0.0)), 0};
        BVHNode rightBox = {vec3(+(1.0 / 0.0)), 0, vec3(-(1.0/0.0)), 0};
        int leftSum = 0, rightSum = 0;
        for (int i = 0; i < SAH_BINS - 1; i++) {
            leftSum += bins[i].triangleCount;
            leftCount[i] = leftSum;
            leftBox.minBound = min(leftBox.minBound, bins[i].minBound);
            leftBox.maxBound = max(leftBox.maxBound, bins[i].maxBound);
            vec3 e = leftBox.maxBound - leftBox.minBound;
            leftArea[i] = e.x * e.y + e.y * e.z + e.z * e.x;

            rightSum += bins[SAH_BINS - 1 - i].triangleCount;
            rightCount[SAH_BINS - 2 - i] = rightSum;
            rightBox.minBound = min(rightBox.minBound, bins[SAH_BINS - 2 - i].minBound);
            rightBox.maxBound = max(rightBox.maxBound, bins[SAH_BINS - 2 - i].maxBound);
            e = rightBox.maxBound - rightBox.minBound;
            rightArea[SAH_BINS - 2 - i] = e.x * e.y + e.y * e.z + e.z * e.x;
        }

        // calculate SAH cost for the SAH_BINS - 1 planes
        scale = (boundsMax - boundsMin) / SAH_BINS;
        for (int i = 0; i < SAH_BINS - 1; i++) {
            float planeCost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
            if (planeCost < bestCost)
                axis = a, splitPos = boundsMin + scale * (i + 1), bestCost = planeCost;
        }
#endif
    }
    return bestCost;
}

void subdivide(uint nodeIdx, uint depth) {
    // terminate recursion
    BVHNode node = bvhNodes[nodeIdx];
    if (node.triangleCount == 1 || depth >= MAX_DEPTH) return;

#if BVH_SPLIT_METHOD == 0
    // determine split axis and position
    vec3 extent = node.maxBound - node.minBound;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    float splitPos = node.minBound[axis] + extent[axis] * 0.5f;
#elif BVH_SPLIT_METHOD == 1
    // determine split axis using SAH
    int bestAxis = -1;
    float bestPos = 0, bestCost = 1e30f;
    for (int axis = 0; axis < 3; axis++) for (uint i = 0; i < node.triangleCount; i++) {
        float candidatePos = centroids[node.index + i][axis];
        float cost = EvaluateSAH(node, axis, candidatePos);
        if (cost < bestCost)
            bestPos = candidatePos, bestAxis = axis, bestCost = cost;
    }
    int axis = bestAxis;
    float splitPos = bestPos;

    vec3 e = node.maxBound - node.minBound; // extent of parent
    float parentArea = e.x * e.y + e.y * e.z + e.z * e.x;
    float parentCost = node.triangleCount * parentArea;
    if (bestCost >= parentCost) return;
#elif BVH_SPLIT_METHOD == 2
    int axis;
    float splitPos;
    float splitCost = FindBestSplitPlane(node, axis, splitPos);

    float nosplitCost = CalculateNodeCost(node);
    if (splitCost >= nosplitCost) return;
#elif BVH_SPLIT_METHOD == 3
    int axis;
    float splitPos;
    float splitCost = FindBestSplitPlane(node, axis, splitPos);

    float nosplitCost = CalculateNodeCost(node);
    if (splitCost >= nosplitCost) return;
#else
    return;
#endif

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
