#version 460 core

in vec3 originalRayDir;

out vec4 FragColor;

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct Triangle {
    vec4 a;
    vec4 b;
    vec4 c;
};

struct HitRecord {
    bool hit;
    float t;
    vec3 pos;
    vec3 normal;
};

uniform uint triangleCount;
layout (std430, binding = 0) buffer TriangleBuffer {
    Triangle triangles[];
};

HitRecord intersectTriangle(Ray ray, Triangle triangle) {
    HitRecord record;
    record.hit = false;

    vec3 edge1 = triangle.b.xyz - triangle.a.xyz;
    vec3 edge2 = triangle.c.xyz - triangle.a.xyz;

    vec3 normal = normalize(cross(edge1, edge2));
//    if (dot(normal, ray.dir) > 0) return record;

    vec3 ray_cross_e2 = cross(ray.dir, edge2);
    float det = dot(edge1, ray_cross_e2);

    if (abs(det) < 0) return record;

    float inv_det = 1.0 / det;
    vec3 s = ray.origin - triangle.a.xyz;
    float u = inv_det * dot(s, ray_cross_e2);

    if (u < -0 || u - 1 > 0) return record;

    vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(ray.dir, s_cross_e1);

    if (v < 0 || u + v - 1 > 0) return record;

    float t = inv_det * dot(edge2, s_cross_e1);

    if (t > 0) {
        record.hit = true;
        record.t = t;
        record.pos = ray.origin + ray.dir * t;
        record.normal = normal;
    }
    return record;
}

HitRecord intersectScene(Ray ray) {
    HitRecord closestRecord;
    closestRecord.hit = false;
    float closestT = 1.0 / 0.0;

    for (uint i = 0; i < triangleCount; i++) {
        HitRecord record = intersectTriangle(ray, triangles[i]);
        if (record.hit && record.t < closestT) {
            closestRecord = record;
            closestT = record.t;
        }
    }

    return closestRecord;
}

void main() {
    FragColor = vec4(1.0);

    Ray ray = {vec3(-1.5, 1.1, 5.0), normalize(originalRayDir)};

    HitRecord record = intersectScene(ray);
    if (record.hit) {
        FragColor = vec4(record.normal, 1.0);
    }
}
